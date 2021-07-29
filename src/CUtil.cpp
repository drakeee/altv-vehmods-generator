#include <Main.h>

uint32_t CUtil::CalculateHash(const char* text)
{
    uint32_t result = 0;
    for (int index = 0; index < strlen(text); index++)
    {
        auto temp = 1025 * ((int)GTA5Keys->HASH_LUT[text[index]] + result);
        result = (temp >> 6) ^ temp;
    }

    return 32769 * ((9 * result >> 11) ^ (9 * result));
}

void CUtil::PrintByteArray(std::byte* array, size_t length)
{
	if (array)
	{
		for (int i = 0; i < length; i++)
		{
			if (i > 0) printf(":");
			printf("%02X", array[i]);
		}
		printf("\n");
	}
}

#define CHUNK (128)
void CUtil::DecompressBytes(std::vector<std::byte>& data, std::vector<std::byte>& dst)
{
	z_stream strm;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = Z_NULL;
	strm.avail_in = 0;

	int initRet = inflateInit2(&strm, -MAX_WBITS);
	assert(initRet == Z_OK);

	unsigned char buffer[CHUNK];

	do
	{
		strm.avail_in = data.size();
		strm.next_in = (unsigned char*)data.data();

		do
		{
			std::memset(&buffer, 0, CHUNK);

			strm.avail_out = CHUNK;
			strm.next_out = &buffer[0];

			initRet = inflate(&strm, Z_NO_FLUSH);

			dst.insert(dst.end(), (std::byte*)&buffer, (std::byte*)&buffer + CHUNK);
		} while (strm.avail_out == 0);
	} while (initRet != Z_STREAM_END);

	if (initRet < 0)
	{
		printf("Error %d in zlib uncompress\n", initRet);
	}

	dst.resize(strm.total_out);

	(void)inflateReset(&strm);
}