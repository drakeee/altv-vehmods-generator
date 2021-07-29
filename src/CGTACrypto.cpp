#include <Main.h>

CGTACrypto::CGTACrypto()
{
    
}

void CGTACrypto::ReadNgKeys(std::vector<std::byte> &buffer, NG_KEY_VECTOR &output)
{
    auto rd = ByteReader(buffer);

    for (int i = 0; i < 101; i++)
    {
        auto t = rd.ReadBytes(272);
        output[i].assign(t.data(), t.data() + 272);
    }
}

void CGTACrypto::ReadNgTables(std::vector<std::byte> & buffer, NG_TABLES_VECTOR &output)
{
    auto rd = ByteReader(buffer);

    // 17 rounds...
    for (int i = 0; i < 17; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            for (int k = 0; k < 256; k++)
            {
                output[i][j][k] = rd.ReadInt<uint32_t>();
            }
        }
    }
}

void CGTACrypto::ReadNgLuts(std::vector<std::byte> &buffer, NG_ENCRYPT_LUTS_VECTOR &output)
{
    auto rd = ByteReader(buffer);

    for (int i = 0; i < 17; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            for (int k = 0; k < 256; k++)
            {
                auto t = rd.ReadBytes(256).data();
                std::move(t, t + 256, output[i][j].LUT0[k]);
            }

            for (int k = 0; k < 256; k++)
            {
                auto t = rd.ReadBytes(256).data();
                std::move(t, t + 256, output[i][j].LUT1[k]);
            }

            auto t = rd.ReadBytes(65536).data();
            std::move(t, t + 256, output[i][j].Indices);
        }
    }
}

void CGTACrypto::GetNGKey(const char* name, uint32_t length, std::vector<std::byte> &output)
{
    uint32_t hash = CUtil::CalculateHash(name);
    uint32_t keyidx = (hash + length + (101 - 40)) % 0x65;

    output.assign(GTA5Keys->NG_KEY[keyidx].data(), GTA5Keys->NG_KEY[keyidx].data() + 272);
}

void CGTACrypto::DecryptAES(std::vector<std::byte> &data, uint32_t length)
{
    aes_decrypt_ecb(AES_CYPHER_256, (uint8_t*)data.data(), data.size(), (uint8_t*)GTA5Keys->AES_KEY.data());
}

std::vector<std::byte> CGTACrypto::DecryptNG(std::vector<std::byte>& data, const char* name, uint32_t length)
{
    std::vector<std::byte> key(272);
    GetNGKey(name, length, key);
    return DecryptNG(data, key);
}

std::vector<std::byte> CGTACrypto::DecryptNG(std::vector<std::byte>& data, std::vector<std::byte>& key)
{
    std::vector<std::byte> decryptedData(data.size());

    //uint32_t* keyuints = new uint32_t[key.size() / sizeof uint32_t];
    //CUtil::BlockCopy(key.data(), 0, keyuints, 0, key.size());

    for (int blockIndex = 0; blockIndex < (data.size() / 16); blockIndex++)
    {
        std::vector<std::byte> encryptedBlock(16); // = new std::byte[16];
        CUtil::BlockCopy(data.data(), 16 * blockIndex, encryptedBlock.data(), 0, 16);

        //auto decryptedBlock = DecryptNGBlock(encryptedBlock, keyuints);
        auto decryptedBlock = DecryptNGBlock(encryptedBlock, (uint32_t*)key.data());
        CUtil::BlockCopy(decryptedBlock.data(), 0, decryptedData.data(), 16 * blockIndex, 16);
    }

    if (data.size() % 16 != 0)
    {
        auto left = data.size() % 16;
        CUtil::BlockCopy(data.data(), data.size() - left, decryptedData.data(), data.size() - left, left);
    }

    //delete keyuints;

    return std::vector<std::byte>(decryptedData.data(), decryptedData.data() + data.size());
}

std::vector<std::byte> CGTACrypto::DecryptNGBlock(std::vector<std::byte> &data, uint32_t* key)
{
    auto buffer = data;

    // prepare key...
    uint32_t subKeys[17][4];
    for (int i = 0; i < 17; i++)
    {
        subKeys[i][0] = key[4 * i + 0];
        subKeys[i][1] = key[4 * i + 1];
        subKeys[i][2] = key[4 * i + 2];
        subKeys[i][3] = key[4 * i + 3];
    }

    buffer = DecryptNGRoundA(buffer.data(), subKeys[0], GTA5Keys->NG_DECRYPT_TABLES[0]);
    buffer = DecryptNGRoundA(buffer.data(), subKeys[1], GTA5Keys->NG_DECRYPT_TABLES[1]);
    for (int k = 2; k <= 15; k++)
        buffer = DecryptNGRoundB(buffer.data(), subKeys[k], GTA5Keys->NG_DECRYPT_TABLES[k]);
    buffer = DecryptNGRoundA(buffer.data(), subKeys[16], GTA5Keys->NG_DECRYPT_TABLES[16]);

    return buffer;
}

std::vector<std::byte> CGTACrypto::DecryptNGRoundA(std::byte *data, uint32_t key[4], std::vector<std::vector<uint32_t>>& table)
{
    auto x1 =
        table[0][(unsigned char)data[0]] ^
        table[1][(unsigned char)data[1]] ^
        table[2][(unsigned char)data[2]] ^
        table[3][(unsigned char)data[3]] ^
        key[0];
    auto x2 =
        table[4][(unsigned char)data[4]] ^
        table[5][(unsigned char)data[5]] ^
        table[6][(unsigned char)data[6]] ^
        table[7][(unsigned char)data[7]] ^
        key[1];
    auto x3 =
        table[8][(unsigned char)data[8]] ^
        table[9][(unsigned char)data[9]] ^
        table[10][(unsigned char)data[10]] ^
        table[11][(unsigned char)data[11]] ^
        key[2];
    auto x4 =
        table[12][(unsigned char)data[12]] ^
        table[13][(unsigned char)data[13]] ^
        table[14][(unsigned char)data[14]] ^
        table[15][(unsigned char)data[15]] ^
        key[3];

    std::byte result[16];

    CUtil::BlockCopy(CUtil::ToByte(x1).data(), 0, result, 0, 4);
    CUtil::BlockCopy(CUtil::ToByte(x2).data(), 0, result, 4, 4);
    CUtil::BlockCopy(CUtil::ToByte(x3).data(), 0, result, 8, 4);
    CUtil::BlockCopy(CUtil::ToByte(x4).data(), 0, result, 12, 4);

    return std::vector<std::byte>(result, result + 16);
}

std::vector<std::byte> CGTACrypto::DecryptNGRoundB(std::byte *data, uint32_t key[4], std::vector<std::vector<std::uint32_t>>& table)
{
    auto x1 =
        table[0][(uint32_t)data[0]] ^
        table[7][(uint32_t)data[7]] ^
        table[10][(uint32_t)data[10]] ^
        table[13][(uint32_t)data[13]] ^
        key[0];
    auto x2 =
        table[1][(uint32_t)data[1]] ^
        table[4][(uint32_t)data[4]] ^
        table[11][(uint32_t)data[11]] ^
        table[14][(uint32_t)data[14]] ^
        key[1];
    auto x3 =
        table[2][(uint32_t)data[2]] ^
        table[5][(uint32_t)data[5]] ^
        table[8][(uint32_t)data[8]] ^
        table[15][(uint32_t)data[15]] ^
        key[2];
    auto x4 =
        table[3][(uint32_t)data[3]] ^
        table[6][(uint32_t)data[6]] ^
        table[9][(uint32_t)data[9]] ^
        table[12][(uint32_t)data[12]] ^
        key[3];

    std::byte result[16];
    result[0] = (std::byte)((x1 >> 0) & 0xFF);
    result[1] = (std::byte)((x1 >> 8) & 0xFF);
    result[2] = (std::byte)((x1 >> 16) & 0xFF);
    result[3] = (std::byte)((x1 >> 24) & 0xFF);
    result[4] = (std::byte)((x2 >> 0) & 0xFF);
    result[5] = (std::byte)((x2 >> 8) & 0xFF);
    result[6] = (std::byte)((x2 >> 16) & 0xFF);
    result[7] = (std::byte)((x2 >> 24) & 0xFF);
    result[8] = (std::byte)((x3 >> 0) & 0xFF);
    result[9] = (std::byte)((x3 >> 8) & 0xFF);
    result[10] = (std::byte)((x3 >> 16) & 0xFF);
    result[11] = (std::byte)((x3 >> 24) & 0xFF);
    result[12] = (std::byte)((x4 >> 0) & 0xFF);
    result[13] = (std::byte)((x4 >> 8) & 0xFF);
    result[14] = (std::byte)((x4 >> 16) & 0xFF);
    result[15] = (std::byte)((x4 >> 24) & 0xFF);
    return std::vector<std::byte>(result, result + 16);
}