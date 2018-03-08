#include "Endian.h"

#ifndef __linux__

using namespace fpnn;

static bool isLittleEndian()
{
	const int32_t i = 0x12345678;
	const char *c = (const char *)&i;

	return (c[0] == 0x78);
}

/*
static bool isBigEndian()
{
	const int32_t i = 0x12345678;
	const char *c = (const char *)&i;

	return (c[0] == 0x12);
}
*/

static uint16_t exchange2(uint16_t value)
{
	unsigned char *c = (unsigned char *)&value;

	c[0] ^= c[1];
	c[1] ^= c[0];
	c[0] ^= c[1];

	return value;
}

static uint32_t exchange4(uint32_t value)
{
	unsigned char *c = (unsigned char *)&value;
	unsigned char a = c[0], b = c[1];

	c[0] = c[3];
	c[1] = c[2];
	c[2] = b;
	c[3] = a;

	return value;
}

static uint64_t exchange8(uint64_t value)
{
	unsigned char *c = (unsigned char *)&value;
	unsigned char a = c[0], b = c[1], x = c[2], y = c[3];

	c[0] = c[7];
	c[1] = c[6];
	c[2] = c[5];
	c[3] = c[4];
	c[4] = y;
	c[5] = x;
	c[6] = b;
	c[7] = a;

	return value;
}

static bool gc_hostIsLittleEndian = isLittleEndian();

uint16_t Endian::htobe16(uint16_t host_16bits)
{
	if (gc_hostIsLittleEndian)
		return exchange2(host_16bits);
	else
		return host_16bits;
}
uint16_t Endian::htole16(uint16_t host_16bits)
{
	if (gc_hostIsLittleEndian)
		return host_16bits;
	else
		return exchange2(host_16bits);
}
uint16_t Endian::be16toh(uint16_t big_endian_16bits)
{
	if (gc_hostIsLittleEndian)
		return exchange2(big_endian_16bits);
	else
		return big_endian_16bits;
}
uint16_t Endian::le16toh(uint16_t little_endian_16bits)
{
	if (gc_hostIsLittleEndian)
		return little_endian_16bits;
	else
		return exchange2(little_endian_16bits);
}


uint32_t Endian::htobe32(uint32_t host_32bits)
{
	if (gc_hostIsLittleEndian)
		return exchange4(host_32bits);
	else
		return host_32bits;
}
uint32_t Endian::htole32(uint32_t host_32bits)
{
	if (gc_hostIsLittleEndian)
		return host_32bits;
	else
		return exchange4(host_32bits);
}
uint32_t Endian::be32toh(uint32_t big_endian_32bits)
{
	if (gc_hostIsLittleEndian)
		return exchange4(big_endian_32bits);
	else
		return big_endian_32bits;
}
uint32_t Endian::le32toh(uint32_t little_endian_32bits)
{
	if (gc_hostIsLittleEndian)
		return little_endian_32bits;
	else
		return exchange4(little_endian_32bits);
}


uint64_t Endian::htobe64(uint64_t host_64bits)
{
	if (gc_hostIsLittleEndian)
		return exchange8(host_64bits);
	else
		return host_64bits;
}
uint64_t Endian::htole64(uint64_t host_64bits)
{
	if (gc_hostIsLittleEndian)
		return host_64bits;
	else
		return exchange8(host_64bits);
}
uint64_t Endian::be64toh(uint64_t big_endian_64bits)
{
	if (gc_hostIsLittleEndian)
		return exchange8(big_endian_64bits);
	else
		return big_endian_64bits;
}
uint64_t Endian::le64toh(uint64_t little_endian_64bits)
{
	if (gc_hostIsLittleEndian)
		return little_endian_64bits;
	else
		return exchange8(little_endian_64bits);
}

#endif
