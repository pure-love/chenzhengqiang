#ifndef _CZQ_CRC_H_
#define _CZQ_CRC_H_

#define BSWAP16C(x) (((x) << 8 & 0xff00)  | ((x) >> 8 & 0x00ff))
#define BSWAP32C(x) (BSWAP16C(x) << 16 | BSWAP16C((x) >> 16))
#define BSWAP64C(x) (BSWAP32C(x) << 32 | BSWAP32C((x) >> 32))

unsigned int  calc_crc32 (unsigned char *data, unsigned int datalen);
unsigned int Zwg_ntohl(unsigned int s);
#endif
