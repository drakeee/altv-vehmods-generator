#pragma once

#include <Main.h>

class GTA5NGLUT;
class CGTACrypto
{
public:
	CGTACrypto();
	
	static void ReadNgKeys(std::vector<std::byte> &buffer, NG_KEY_VECTOR& output);
	static void ReadNgTables(std::vector<std::byte> &buffer, NG_TABLES_VECTOR &output);
	static void ReadNgLuts(std::vector<std::byte> &buffer, NG_ENCRYPT_LUTS_VECTOR &output);

	static void GetNGKey(const char* name, uint32_t length, std::vector<std::byte> &output);
    static std::vector<std::byte> DecryptNG(std::vector<std::byte>& data, const char* name, uint32_t length);
    static std::vector<std::byte> DecryptNG(std::vector<std::byte>& data, std::vector<std::byte>& key);
    static std::vector<std::byte> DecryptNGBlock(std::vector<std::byte> &data, uint32_t *key);
    static std::vector<std::byte> DecryptNGRoundA(std::byte *data, uint32_t key[4], std::vector<std::vector<uint32_t>>& table);
	static std::vector<std::byte> DecryptNGRoundB(std::byte *data, uint32_t key[4], std::vector<std::vector<uint32_t>>& table);
};