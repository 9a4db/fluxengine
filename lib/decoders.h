#ifndef DECODERS_H
#define DECODERS_H

/* IBM format (i.e. ordinary PC floppies). */

#define IBM_IAM        0xFC   /* start-of-track record */
#define IBM_IAM_LEN    4
#define IBM_IDAM       0xFE  /* sector header */
#define IBM_IDAM_LEN   10
#define IBM_DAM1       0xF8  /* sector data (type 1) */
#define IBM_DAM2       0xFB  /* sector data (type 2) */
#define IBM_DAM_LEN    6 /* plus user data */
/* Length of a DAM record is determined by the previous sector header. */

struct IbmIdam
{
    uint8_t marker[3];
    uint8_t id;
    uint8_t cylinder;
    uint8_t side;
    uint8_t sector;
    uint8_t sectorSize;
    uint8_t crc[2];
};

class Sector;
class Fluxmap;

extern std::vector<std::vector<uint8_t>> decodeBitsToRecordsMfm(const std::vector<bool>& bitmap);

extern std::vector<std::unique_ptr<Sector>> parseRecordsToSectorsIbm(const std::vector<std::vector<uint8_t>>& records);

#endif
