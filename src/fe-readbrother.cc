#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "brother.h"
#include "sector.h"
#include "sectorset.h"
#include "image.h"
#include "record.h"
#include <fmt/format.h>
#include <fstream>

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "brother.img");

static SettableFlag dumpRecords(
	{ "--dump-records" },
	"Dump the parsed records.");

static IntFlag retries(
	{ "--retries" },
	"How many times to retry each track in the event of a read failure.",
	5);

#define SECTOR_COUNT 12
#define TRACK_COUNT 78

int main(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-81:s=0");
    Flag::parseFlags(argc, argv);

	bool failures = false;
	SectorSet allSectors;
    for (auto& track : readTracks())
    {
		std::map<int, std::unique_ptr<Sector>> readSectors;
		for (int retry = ::retries; retry >= 0; retry--)
		{
			std::unique_ptr<Fluxmap> fluxmap = track->read();

			nanoseconds_t clockPeriod = fluxmap->guessClock();
			std::cout << fmt::format("       {:.1f} us clock; ", (double)clockPeriod/1000.0) << std::flush;

			auto bitmap = fluxmap->decodeToBits(clockPeriod);
			std::cout << fmt::format("{} bytes encoded; ", bitmap.size()/8) << std::flush;

			auto records = decodeBitsToRecordsBrother(bitmap);
			std::cout << records.size() << " records." << std::endl;

			auto sectors = parseRecordsToSectorsBrother(records);
			std::cout << "       " << sectors.size() << " sectors; ";

			for (auto& sector : sectors)
			{
				if ((sector->sector < SECTOR_COUNT) && (sector->track < TRACK_COUNT))
				{
					auto& replacing = readSectors[sector->sector];
					if (sector->status == Sector::OK)
						replacing = std::move(sector);
					else
					{
						if (!replacing || (replacing->status == Sector::OK))
							replacing = std::move(sector);
					}
				}
			}

			bool hasBadSectors = false;
			for (int i=0; i<SECTOR_COUNT; i++)
			{
				auto& sector = readSectors[i];
                Sector::Status status = sector ? (Sector::Status)sector->status : Sector::MISSING;
				if (status != Sector::OK)
				{
					std::cout << std::endl
							  << "       Failed to read sector " << i
                              << " (" << Sector::statusToString(status) << "); ";
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
					  << "       " << retry << " retries remaining" << std::endl;
		}

        int size = 0;
		bool printedTrack = false;
		for (int sectorId = 0; sectorId < SECTOR_COUNT; sectorId++)
		{
			auto& sector = readSectors[sectorId];
			if (sector)
			{
				if (!printedTrack)
				{
					std::cout << "logical track " << sector->track << "; ";
					printedTrack = true;
				}

				size += sector->data.size();
				allSectors[{sector->track, 0, sector->sector}] = std::move(sector);
			}
        }
        std::cout << size << " bytes decoded." << std::endl;

    }

	Geometry geometry = guessGeometry(allSectors);
    writeSectorsToFile(allSectors, geometry, outputFilename);
	if (failures)
		std::cerr << "Warning: some sectors could not be decoded." << std::endl;
    return 0;
}

