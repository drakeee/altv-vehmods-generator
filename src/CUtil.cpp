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