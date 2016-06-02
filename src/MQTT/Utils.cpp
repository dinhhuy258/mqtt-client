#include "Utils.h"
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>

void WriteShort(uint8_t **pptr, uint16_t data)
{
	**pptr = (char)(data / 256);
	++(*pptr);
	**pptr = (char)(data % 256);
	++(*pptr);
}

void WriteChar(uint8_t **pptr, uint8_t data)
{
	**pptr = data;
	++(*pptr);
}

void WriteUTF(uint8_t** pptr, std::string string)
{
	uint16_t len = static_cast<uint16_t>(string.size());
	WriteShort(pptr, len);
	memcpy(*pptr, string.c_str(), len);
	*pptr += len;
}
