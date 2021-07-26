#include <Main.h>

CRpfFile::CRpfFile(const char* filePath)
{
	this->file.open(filePath, std::ios_base::binary | std::ios_base::in);
	if (this->file.fail())
	{
		std::cerr << "Unable to open file for reading";
		return;
	}

	this->filePath = filePath;
	std::filesystem::path p(filePath);
	this->fileName = p.filename().string();

	this->file.seekg(0, std::ios_base::end);
	this->fileSize = this->file.tellg();
	this->file.seekg(0, std::ios_base::beg);

	this->ReadHeader();
}

void CRpfFile::ReadHeader(void)
{
	if (!this->file.is_open())
		return;

	this->file.read((char*)&this->header.version, 1);
	this->file.read((char*)&this->header.magic, 3);
	this->file.read((char*)&this->header.entryCount, 4);
	this->file.read((char*)&this->header.namesLength, 4);

	if (this->header.version >= '2')
		this->file.read((char*)&this->header.encrypted, 4);

	std::vector<std::byte> entriesData((int)this->header.entryCount * 16);
	std::vector<std::byte> namesData((int)this->header.namesLength);

	this->file.read((char*)entriesData.data(), (int)this->header.entryCount * 16);
	this->file.read((char*)namesData.data(), (int)this->header.namesLength);

	switch (this->header.encrypted)
	{
	case Encryption::NONE:
		std::cout << "NONE encryption";
		break;
	case Encryption::OPEN:
		std::cout << "OPEN encryption";
		break;
	case Encryption::AES:
	{
		std::cout << "AES encryption";
		this->isAESEncrypted = true;
		break;
	}
	case Encryption::NG:
	{
		//std::cout << "NG encryption" << std::endl;
		entriesData = CGTACrypto::DecryptNG(entriesData, this->fileName.c_str(), this->fileSize);
		namesData = CGTACrypto::DecryptNG(namesData, this->fileName.c_str(), this->fileSize);
		this->isNGEncrypted = true;

		break;
	}
	default:
		std::cout << "default encryption";
		break;
	}

	auto entryReader = ByteReader(entriesData);
	auto namesReader = ByteReader(namesData);
	
	for (uint32_t i = 0; i < this->header.entryCount; i++)
	{
		//entriesrdr.Position += 4;
		uint32_t y = entryReader.ReadInt<uint32_t>();
		uint32_t x = entryReader.ReadInt<uint32_t>();

		entryReader.byteRead -= 8;

		CRpfEntry *e;

		if (x == 0x7fffff00) //directory entry
		{
			e = new CRpfDirectoryEntry();
			this->totalFolderCount++;
		}
		else if ((x & 0x80000000) == 0) //binary file entry
		{
			e = new CRpfBinaryFileEntry();

			this->totalBinaryFileCount++;
			this->totalFileCount++;
		}
		else //assume resource file entry
		{
			e = new CRpfResourceFileEntry();

			this->totalResourceCount++;
			this->totalFileCount++;
		}

		e->file = this;
		e->H1 = y;
		e->H2 = x;

		e->Read(entryReader);

		namesReader.byteRead = e->nameOffset;
		auto tempString = namesReader.ReadString();
		e->name = tempString;
		std::transform(tempString.begin(), tempString.end(), tempString.begin(), ::tolower);
		e->nameLower = tempString;
		//e->NameLower = e->Name.ToLowerInvariant();

		//if(std::is_base_of<CRpfResourceFileEntry, typeof(e)>::value)
		//if ((e instanceof CRpfResourceFileEntry))// && string.IsNullOrEmpty(e.Name))
		//{
		//	auto rfe = e as RpfResourceFileEntry;
		//	rfe.IsEncrypted = rfe.NameLower.EndsWith(".ysc");//any other way to know..?
		//}

		this->allEntries.push_back(e);
	}

	this->root = (CRpfDirectoryEntry*)this->allEntries[0];
	this->root->path = this->fileName + this->fileRelativePath;// + "\\" + Root.Name;
	auto stack = std::stack<CRpfDirectoryEntry*>();
	stack.push(this->root);

	while (stack.size() > 0)
	{
		auto item = stack.top();
		stack.pop();

		int starti = (int)item->entriesIndex;
		int endi = (int)(item->entriesIndex + item->entriesCount);

		for (int i = starti; i < endi; i++)
		{
			CRpfEntry *e = this->allEntries[i];
			e->parent = item;

			if (e->entryType == CRpfEntry::Type::DIRECTORY_ENTRY)
			{
				CRpfDirectoryEntry *rde = (CRpfDirectoryEntry*)e;
				rde->path = item->path + "\\" + rde->nameLower;
				item->directories.push_back(rde);
				stack.push(rde);
			}
			else if (e->entryType == CRpfEntry::Type::FILE_ENTRY)
			{
				CRpfFileEntry *rfe = (CRpfFileEntry*)e;
				rfe->path = item->path + "\\" + rfe->nameLower;
				item->files.push_back(rfe);
			}
		}
	}
}

std::vector<std::byte> CRpfFile::ExtractFileBinary(CRpfBinaryFileEntry* entry)
{
	this->file.seekg(0 + ((long)entry->fileOffset * 512), std::ios_base::beg);

	long l = entry->GetFileSize();

	if (l > 0)
	{
		uint32_t offset = 0;// 0x10;
		uint32_t totlen = (uint32_t)l - offset;

		std::vector<std::byte> tbytes(totlen);
		this->file.read((char*)tbytes.data(), totlen);

		if (entry->isEncrypted)
		{
			if (this->isAESEncrypted)
			{
				//decr = CGTACrypto::DecryptAES(tbytes);
			}
			else
				tbytes = CGTACrypto::DecryptNG(tbytes, entry->name.c_str(), entry->fileUncompressedSize);
		}

		if (entry->fileSize > 0)
		{
			printf("FILE IS COMPRESSED!\n");
			return DecompressBytes(tbytes);
		}

		return tbytes;
	}

	return std::vector<std::byte>();
}

void CRpfFile::ExtractFileBinary(CRpfBinaryFileEntry* entry, std::string path)
{
	std::ofstream f;
	std::string outputName(path);

	f.open(outputName.c_str(), std::ofstream::binary | std::ofstream::out);
	if (f.fail())
	{
		printf("File failed to open! File: %s\n", outputName.c_str());
		return;
	}

	auto uncompressed = this->ExtractFileBinary((CRpfBinaryFileEntry*)entry);
	f.write((char*)uncompressed.data(), uncompressed.size());
	f.close();
}

#define CHUNK (128)
void decompressBytes(std::vector<std::byte> &data, std::vector<std::byte> &dst)
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

std::vector<std::byte> CRpfFile::DecompressBytes(std::vector<std::byte> bytes)
{
	Bytef* dest = new Bytef[bytes.size() * 10];
	uLongf destLen;

	std::vector<std::byte> def;
	decompressBytes(bytes, def);

	return def;
	
	/*try
	{
		using (DeflateStream ds = new DeflateStream(new MemoryStream(bytes), CompressionMode.Decompress))
		{
			MemoryStream outstr = new MemoryStream();
			ds.CopyTo(outstr);
			byte[] deflated = outstr.GetBuffer();
			byte[] outbuf = new byte[outstr.Length]; //need to copy to the right size buffer for output.
			Array.Copy(deflated, outbuf, outbuf.Length);

			if (outbuf.Length <= bytes.Length)
			{
				LastError = "Warning: Decompressed data was smaller than compressed data...";
				//return null; //could still be OK for tiny things!
			}

			return outbuf;
		}
	}
	catch (Exception ex)
	{
		LastError = "Could not decompress.";// ex.ToString();
		LastException = ex;
		return null;
	}*/
}

void CRpfFile::SearchFiles(std::string name, std::vector<CRpfEntry*>& filesOutput)
{
	for each (auto & entry in this->allEntries)
	{
		if (entry->nameLower == name)
			filesOutput.push_back(entry);
	}
}

void CRpfFile::SearchFilesEndsWith(std::string endsWith, std::vector<CRpfEntry*>& filesOutput)
{
	for each (auto & entry in this->allEntries)
	{
		if (CUtil::EndsWith(entry->nameLower, endsWith))
			filesOutput.push_back(entry);
	}
}

std::vector<CRpfEntry*> CRpfFile::SearchFiles(std::string name)
{
	std::vector<CRpfEntry*> temp;
	SearchFiles(name, temp);

	return temp;
}

std::vector<CRpfEntry*> CRpfFile::SearchFilesEndsWith(std::string endsWith)
{
	std::vector<CRpfEntry*> temp;
	SearchFilesEndsWith(endsWith, temp);

	return temp;
}