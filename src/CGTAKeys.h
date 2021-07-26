#pragma once

#include <Main.h>

class GTA5NGLUT
{
public:
    std::byte LUT0[256][256] = { std::byte(0) };
    std::byte LUT1[256][256] = { std::byte(0) };
    std::byte Indices[65536] = { std::byte(0) };

    GTA5NGLUT() { }

    inline std::byte LookUp(uint32_t value)
    {
        uint32_t h16 = (value & 0xFFFF0000) >> 16;
        uint32_t l8 = (value & 0x0000FF00) >> 8;
        uint32_t l0 = (value & 0x000000FF) >> 0;
        return LUT0[(uint32_t)LUT1[(uint32_t)Indices[h16]][l8]][l0];
    }
};

class CGTAKeys
{
public:
    AES_KEY_VECTOR AES_KEY; //32
    NG_KEY_VECTOR NG_KEY; //101, 272
    NG_TABLES_VECTOR NG_DECRYPT_TABLES; //17, 16, 256
    NG_TABLES_VECTOR NG_ENCRYPT_TABLES; //17, 16, 256
    NG_ENCRYPT_LUTS_VECTOR NG_ENCRYPT_LUTS; //17, 16
    HASH_LUT_VECTOR HASH_LUT; //256

    static CGTAKeys& getInstance()
    {
        static CGTAKeys instance;
        return instance;
    }
private:
    CGTAKeys()
    {
        this->AES_KEY = AES_KEY_VECTOR(32);
        this->NG_KEY = NG_KEY_VECTOR(101, std::vector<std::byte>(272)); //101, 272
        this->NG_DECRYPT_TABLES = NG_TABLES_VECTOR(17, std::vector<std::vector<uint32_t>>(16, std::vector<uint32_t>(256))); //17, 16, 256
        this->NG_ENCRYPT_TABLES = NG_TABLES_VECTOR(17, std::vector<std::vector<uint32_t>>(16, std::vector<uint32_t>(256))); //17, 16, 256
        this->NG_ENCRYPT_LUTS = NG_ENCRYPT_LUTS_VECTOR(17, std::vector<GTA5NGLUT>(16)); //17, 16
        this->HASH_LUT = HASH_LUT_VECTOR(256);

        CUtil::ReadFile<std::byte>("./keys/gtav_aes_key.dat", this->AES_KEY);
        CGTACrypto::ReadNgKeys(CUtil::ReadFile<std::byte>("./keys/gtav_ng_key.dat"), this->NG_KEY);
        CGTACrypto::ReadNgTables(CUtil::ReadFile<std::byte>("./keys/gtav_ng_decrypt_tables.dat"), this->NG_DECRYPT_TABLES);
        CGTACrypto::ReadNgTables(CUtil::ReadFile<std::byte>("./keys/gtav_ng_encrypt_tables.dat"), this->NG_ENCRYPT_TABLES);
        CGTACrypto::ReadNgLuts(CUtil::ReadFile<std::byte>("./keys/gtav_ng_encrypt_luts.dat"), this->NG_ENCRYPT_LUTS);
        CUtil::ReadFile<std::byte>("./keys/gtav_hash_lut.dat", this->HASH_LUT);
    }
};

extern CGTAKeys* GTA5Keys;