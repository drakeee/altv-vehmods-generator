#pragma once

#include <Main.h>

class GTA5NGLUT;
typedef std::vector<std::byte> AES_KEY_VECTOR;
typedef std::vector<std::vector<std::byte>> NG_KEY_VECTOR; //101, 272
typedef std::vector<std::vector<std::vector<uint32_t>>> NG_TABLES_VECTOR; //17, 16, 256
typedef std::vector<std::vector<GTA5NGLUT>> NG_ENCRYPT_LUTS_VECTOR; //17, 16
typedef std::vector<std::byte> HASH_LUT_VECTOR; //256

struct ByteReader
{

public:
	std::byte* data;
	uint32_t byteRead = 0;

public:
	ByteReader(std::byte* data) :
		data(data) { }

	ByteReader(std::vector<std::byte> &data) :
		data(std::move(data.data())) { }

	std::unique_ptr<std::byte[]> ReadBytes(uint32_t size)
	{
		auto buffer = std::make_unique<std::byte[]>(size);
		std::copy(this->data + this->byteRead, this->data + (this->byteRead + size), buffer.get());

		this->byteRead += size;

		return buffer;
	}

	template<typename T>
	inline T ReadInt()
	{
		T tempValue;
		std::memcpy(&tempValue, ReadBytes(sizeof(T)).get(), sizeof(T));

		return tempValue;
	}

	inline std::string ReadString()
	{
		std::vector<unsigned char> bytes;
		std::byte temp;

		while ((unsigned char)(temp = ReadBytes(1)[0]) != 0)
			bytes.push_back((unsigned char)temp);

		return std::string(bytes.begin(), bytes.end());
	}

	/*inline uint16_t ReadUInt16()
	{
		uint32_t tempValue;
		std::memcpy(&tempValue, ReadBytes(4).get(), sizeof uint32_t);

		return tempValue;
	}

	inline uint32_t ReadUInt32()
	{
		uint32_t tempValue;
		std::memcpy(&tempValue, ReadBytes(4).get(), sizeof uint32_t);

		return tempValue;
	}

	inline uint64_t ReadUInt64()
	{
		uint64_t tempValue;
		std::memcpy(&tempValue, ReadBytes(8).get(), sizeof uint64_t);

		return tempValue;
	}*/
};

class CUtil
{
public:
	static uint32_t CalculateHash(const char* text);
	static void PrintByteArray(std::byte* array, size_t length);

	template<typename S, typename D>
	inline static void BlockCopy(S *src, int srcOffset, D *dst, int dstOffset, int count)
	{
		//std::copy(src + srcOffset, src + srcOffset + count, dst + dstOffset);
		std::memcpy(dst + dstOffset, src + srcOffset, count);
	}

	template<typename T>
	static void PrintByteArray(std::vector<T> array)
	{
		PrintByteArray((std::byte*)array.data(), array.size());
	}

	template<typename T>
	static void PrintByteArray(T& array)
	{
		PrintByteArray(array, sizeof(array));
	}

	template<typename T>
	inline static size_t ReadFile(const char* filePath, std::vector<T> &buffer)
	{
		std::ifstream infile(filePath, std::ios_base::binary | std::ios_base::in);

		if (infile.fail())
			return 0;

		infile.seekg(0, std::ios::end);
		size_t length = infile.tellg();
		infile.seekg(0, std::ios::beg);

		buffer.resize(length);
		infile.read((char*)buffer.data(), length);

		infile.close();

		return length;
	}

	template<typename T>
	inline static std::vector<T> ReadFile(const char* filePath)
	{
		std::vector<T> buffer;
		ReadFile<T>(filePath, buffer);

		return buffer;
	}

	template <typename T>
	static std::vector<std::byte> ToByte(T input)
	{
		std::byte* bytePointer = reinterpret_cast<std::byte*>(&input);
		return std::vector<std::byte>(bytePointer, bytePointer + sizeof(T));
	}

	template<typename T>
	static std::vector<T> BytesToInt(std::vector<std::byte>& src, int offset)
	{
		std::vector<T> values(src.size() / 4);
		for (int i = 0; i < src.size() / 4; i++)
		{
			T value = (T)(((T)src[offset] & 0xFF)
				| (((T)src[offset + 1] & 0xFF) << 8)
				| (((T)src[offset + 2] & 0xFF) << 16)
				| (((T)src[offset + 3] & 0xFF) << 24));

			printf("value: %u\n", (uint32_t)value);

			values[i] = value;

			offset += 4;
		}

		return values;
	}

	static bool EndsWith(const std::string& mainStr, const std::string& toMatch)
	{
		if (mainStr.size() >= toMatch.size() &&
			mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0)
			return true;
		else
			return false;
	}

	/*template<typename T>
	inline static size_t ReadFile(const char* filePath, T** buffer)
	{
		std::ifstream infile(filePath, std::ios_base::binary | std::ios_base::in);

		if (infile.fail())
			return 0;

		infile.seekg(0, std::ios::end);
		size_t length = infile.tellg();
		infile.seekg(0, std::ios::beg);

		*buffer = new T[length];
		infile.read(reinterpret_cast<char*>(*buffer), length);
		infile.close();

		return length;
	}

	template<typename T>
	inline static std::unique_ptr<T> ReadFile(const char* filePath)
	{
		T* buffer;
		ReadFile<T>(filePath, &buffer);

		return std::unique_ptr<T>(buffer);
	}

	template<typename T>
	inline static bool ReadFile(const char* filePath, T &buffer)
	{
		std::ifstream infile(filePath, std::ios_base::binary | std::ios_base::in);

		if (infile.fail())
			return false;

		printf("Size: %d\n", sizeof(buffer));

		infile.read(reinterpret_cast<char*>(buffer), sizeof(buffer));
		infile.close();

		return true;
	}*/
};