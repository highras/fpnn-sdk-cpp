#ifndef FPNN_Endian_h
#define FPNN_Endian_h

#ifdef __linux__
#include <endian.h>
#else

#include <stdint.h>

namespace fpnn
{
	class Endian
	{
	public:
		static uint16_t htobe16(uint16_t host_16bits);
		static uint16_t htole16(uint16_t host_16bits);
		static uint16_t be16toh(uint16_t big_endian_16bits);
		static uint16_t le16toh(uint16_t little_endian_16bits);

		static uint32_t htobe32(uint32_t host_32bits);
		static uint32_t htole32(uint32_t host_32bits);
		static uint32_t be32toh(uint32_t big_endian_32bits);
		static uint32_t le32toh(uint32_t little_endian_32bits);

		static uint64_t htobe64(uint64_t host_64bits);
		static uint64_t htole64(uint64_t host_64bits);
		static uint64_t be64toh(uint64_t big_endian_64bits);
		static uint64_t le64toh(uint64_t little_endian_64bits);
	};
}

inline uint16_t htobe16(uint16_t host_16bits) { return fpnn::Endian::htobe16(host_16bits); }
inline uint16_t htole16(uint16_t host_16bits) { return fpnn::Endian::htole16(host_16bits); }
inline uint16_t be16toh(uint16_t big_endian_16bits) { return fpnn::Endian::be16toh(big_endian_16bits); }
inline uint16_t le16toh(uint16_t little_endian_16bits) { return fpnn::Endian::le16toh(little_endian_16bits); }

inline uint32_t htobe32(uint32_t host_32bits) { return fpnn::Endian::htobe32(host_32bits); }
inline uint32_t htole32(uint32_t host_32bits) { return fpnn::Endian::htole32(host_32bits); }
inline uint32_t be32toh(uint32_t big_endian_32bits) { return fpnn::Endian::be32toh(big_endian_32bits); }
inline uint32_t le32toh(uint32_t little_endian_32bits) { return fpnn::Endian::le32toh(little_endian_32bits); }

inline uint64_t htobe64(uint64_t host_64bits) { return fpnn::Endian::htobe64(host_64bits); }
inline uint64_t htole64(uint64_t host_64bits) { return fpnn::Endian::htole64(host_64bits); }
inline uint64_t be64toh(uint64_t big_endian_64bits) { return fpnn::Endian::be64toh(big_endian_64bits); }
inline uint64_t le64toh(uint64_t little_endian_64bits) { return fpnn::Endian::le64toh(little_endian_64bits); }

#endif

#endif
