#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "reader.h"
#include "fluxmap.h"
#include "sql.h"
#include "dataspec.h"
#include "decoders.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "image.h"
#include "fmt/format.h"

static DataSpecFlag source(
    { "--source", "-s" },
    "source for data",
    ":t=0-79:s=0-1:d=0");

static StringFlag destination(
    { "--write-flux", "-f" },
    "write the raw magnetic flux to this file",
    "");

static SettableFlag justRead(
    { "--just-read", "-R" },
    "just read the disk but do no further processing");

static IntFlag revolutions(
    { "--revolutions" },
    "read this many revolutions of the disk",
    1);

static SettableFlag dumpRecords(
	{ "--dump-records" },
	"Dump the parsed records.");

static IntFlag retries(
	{ "--retries" },
	"How many times to retry each track in the event of a read failure.",
	5);

static sqlite3* indb;
static sqlite3* outdb;

void setReaderDefaultSource(const std::string& source)
{
    ::source.set(source);
}

void setReaderRevolutions(int revolutions)
{
    ::revolutions.value = ::revolutions.defaultValue = revolutions;
}

std::unique_ptr<Fluxmap> ReaderTrack::read()
{
    std::cout << fmt::format("{0:>3}.{1}: ", track, side) << std::flush;
    std::unique_ptr<Fluxmap> fluxmap = reallyRead();
    std::cout << fmt::format(
        "{0} ms in {1} bytes", int(fluxmap->duration()/1e6), fluxmap->bytes()) << std::endl;

    if (outdb)
        sqlWriteFlux(outdb, track, side, *fluxmap);

    return fluxmap;
}
    
std::unique_ptr<Fluxmap> CapturedReaderTrack::reallyRead()
{
    usbSetDrive(drive);
    usbSeek(track);
    return usbRead(side, revolutions);
}

void CapturedReaderTrack::recalibrate()
{
    usbRecalibrate();
}

std::unique_ptr<Fluxmap> FileReaderTrack::reallyRead()
{
    if (!indb)
	{
        indb = sqlOpen(source.value.filename, SQLITE_OPEN_READONLY);
		atexit([]() { sqlClose(indb); });
	}
    return sqlReadFlux(indb, track, side);
}

void FileReaderTrack::recalibrate()
{}

std::vector<std::unique_ptr<ReaderTrack>> readTracks()
{
    const DataSpec& dataSpec = source.value;

    std::cout << "Reading from: " << dataSpec << std::endl;

	if (!destination.value.empty())
	{
		outdb = sqlOpen(destination, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
		std::cout << "Writing a copy of the flux to " << destination.value << std::endl;
		sqlPrepareFlux(outdb);
		sqlStmt(outdb, "BEGIN;");
		atexit([]()
			{
				sqlStmt(outdb, "COMMIT;");
				sqlClose(outdb);
			}
		);
	}

    std::vector<std::unique_ptr<ReaderTrack>> tracks;
    for (const auto& location : dataSpec.locations)
    {
        std::unique_ptr<ReaderTrack> t(
            dataSpec.filename.empty()
                ? (ReaderTrack*)new CapturedReaderTrack()
                : (ReaderTrack*)new FileReaderTrack());
        t->drive = location.drive;
        t->track = location.track;
        t->side = location.side;
        tracks.push_back(std::move(t));
    }

	if (justRead)
	{
		for (auto& track : tracks)
			track->read();
		std::cout << "--just-read specified, terminating without further processing" << std::endl;
		exit(0);
	}
				
    return tracks;
}

void readDiskCommand(
    const BitmapDecoder& bitmapDecoder, const RecordParser& recordParser,
    const std::string& outputFilename)
{
	bool failures = false;
	SectorSet allSectors;
    for (auto& track : readTracks())
    {
		std::map<int, std::unique_ptr<Sector>> readSectors;
		for (int retry = ::retries; retry >= 0; retry--)
		{
			std::unique_ptr<Fluxmap> fluxmap = track->read();

			nanoseconds_t clockPeriod = bitmapDecoder.guessClock(*fluxmap);
			std::cout << fmt::format("       {:.2f} us clock; ", (double)clockPeriod/1000.0) << std::flush;

			auto bitmap = fluxmap->decodeToBits(clockPeriod);
			std::cout << fmt::format("{} bytes encoded; ", bitmap.size()/8) << std::flush;

			auto records = bitmapDecoder.decodeBitsToRecords(bitmap);
			std::cout << records.size() << " records." << std::endl;

			auto sectors = recordParser.parseRecordsToSectors(records);
			std::cout << "       " << sectors.size() << " sectors; ";

			for (auto& sector : sectors)
			{
                auto& replacing = readSectors[sector->sector];
                if (!replacing || (sector->status == Sector::OK))
                    replacing = std::move(sector);
			}

			bool hasBadSectors = false;
			for (const auto& i : readSectors)
			{
                const auto& sector = i.second;
				if (sector->status != Sector::OK)
				{
					std::cout << std::endl
							  << "       Failed to read sector " << sector->sector
                              << " (" << Sector::statusToString((Sector::Status)sector->status) << "); ";
					hasBadSectors = true;
				}
			}

			if (hasBadSectors)
				failures = false;

			if (dumpRecords && (!hasBadSectors || (retry == 0)))
			{
				std::cout << "\nRaw records follow:\n\n";
				for (auto& record : records)
				{
					std::cout << fmt::format("I+{:.3f}ms", (double)(record->position*clockPeriod)/1e6)
					          << std::endl;
					hexdump(std::cout, record->data);
					std::cout << std::endl;
				}
			}

			if (!hasBadSectors)
				break;

			std::cout << std::endl
                      << "       ";
            if (retry == 0)
                std::cout << "giving up" << std::endl
                          << "       ";
            else
            {
				std::cout << retry << " retries remaining" << std::endl;
                track->recalibrate();
            }
		}

        int size = 0;
		bool printedTrack = false;
        for (auto& i : readSectors)
        {
			auto& sector = i.second;
			if (sector)
			{
				if (!printedTrack)
				{
					std::cout << "logical track " << sector->track << "; ";
					printedTrack = true;
				}

				size += sector->data.size();
				allSectors[{sector->track, sector->side, sector->sector}] = std::move(sector);
			}
        }
        std::cout << size << " bytes decoded." << std::endl;
    }

	Geometry geometry = guessGeometry(allSectors);
    writeSectorsToFile(allSectors, geometry, outputFilename);
	if (failures)
		std::cerr << "Warning: some sectors could not be decoded." << std::endl;
}