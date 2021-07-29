#include <Main.h>

CRpfFile::CRpfFile(const char* filePath)
{
	this->file = new std::ifstream(filePath, std::ios_base::binary | std::ios_base::in);
	if (this->file->fail())
	{
		std::cerr << "Unable to open file for reading";
		return;
	}

	std::filesystem::path p(filePath);
	this->filePath = p.parent_path().string();
	this->fileName = p.filename().string();
	this->fileFullPath = this->filePath + kPathSeparator + this->fileName;

	this->file->seekg(0, std::ios_base::end);
	this->fileSize = this->file->tellg();
	this->file->seekg(0, std::ios_base::beg);

	this->ReadHeader();
}

CRpfFile::CRpfFile(CRpfFile* parent, CRpfBinaryFileEntry* entry)
{
	this->parent = parent;
	this->filePath = entry->path;
	this->fileName = entry->nameLower;
	this->fileFullPath = entry->path;
	this->file = this->parent->file;
	this->fileOffset = entry->fileOffsetPos;
	this->file->seekg(this->fileOffset, std::ios_base::beg);
	this->fileSize = entry->GetFileSize();

	/*printf("BufferPos: %u\n", entry->fileOffsetPos);
	printf("Size1: %u\n", entry->fileSize);
	printf("Size2: %u\n", entry->fileUncompressedSize);
	printf("Size3: %u\n", entry->GetFileSize());
	printf("Size4: %u\n", this->fileSize);*/

	this->ReadHeader();
}

CRpfFile::~CRpfFile()
{
	for each (auto entry in this->allEntries)
	{
		delete entry;
	}

	for each (auto child in this->children)
	{
		delete child;
	}
}

void CRpfFile::ReadHeader(void)
{
	if (!this->file->is_open())
		return;

	//For some reason it is impossible to read this file from "update\update.rpf\dlc_patch\patchday1ng\x64\patch\data\lang\chinesesimp.rpf"
	//so we might just skip it or not read it at all
	//TODO: Fix it (?)

	if (this->fileName == "chinesesimp.rpf")
	{
		return;
	}

	this->file->read((char*)&this->header.version, 1);
	this->file->read((char*)&this->header.magic, 3);
	std::reverse(this->header.magic, this->header.magic + 3);
	this->file->read((char*)&this->header.entryCount, 4);
	this->file->read((char*)&this->header.namesLength, 4);

	/*printf("Version: %c\n", this->header.version);
	printf("Magic: %s\n", this->header.magic);
	printf("EntryCount: %d\n", this->header.entryCount);
	printf("NamesLength: %d\n", this->header.namesLength);
	printf("Path: %s\n", this->fileFullPath.c_str());*/

	if (this->header.version >= '2')
		this->file->read((char*)&this->header.encrypted, 4);

	std::vector<std::byte> entriesData((uint32_t)this->header.entryCount * 16);
	std::vector<std::byte> namesData((uint32_t)this->header.namesLength);

	this->file->read((char*)entriesData.data(), (uint32_t)this->header.entryCount * 16);
	this->file->read((char*)namesData.data(), (uint32_t)this->header.namesLength);

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
		//std::cout << "AES encryption" << std::endl;

		CGTACrypto::DecryptAES(entriesData, this->header.entryCount * 16);
		CGTACrypto::DecryptAES(namesData, this->header.namesLength);
		this->isAESEncrypted = true;

		break;
	}
	case Encryption::NG:
	{
		//std::cout << "NG encryption" << std::endl;

		//printf("Before decrypt: %s\n", this->fileName.c_str());
		//CUtil::PrintByteArray(namesData);

		entriesData = CGTACrypto::DecryptNG(entriesData, this->fileName.c_str(), this->fileSize);
		namesData = CGTACrypto::DecryptNG(namesData, this->fileName.c_str(), this->fileSize);
		this->isNGEncrypted = true;

		//printf("After decrypt: ");
		//CUtil::PrintByteArray(namesData);

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
			//printf("DIRECTORY!\n");

			this->totalFolderCount++;
		}
		else if ((x & 0x80000000) == 0) //binary file entry
		{
			e = new CRpfBinaryFileEntry();
			//printf("BINARY!\n");

			this->totalBinaryFileCount++;
			this->totalFileCount++;
		}
		else //assume resource file entry
		{
			e = new CRpfResourceFileEntry();
			//printf("RESOURCE!\n");

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
	this->root->path = this->fileFullPath;// + "\\" + Root.Name;
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
			//printf("Size: %d - %d - %d - %d\n", this->allEntries.size(), i, item->entriesIndex, item->entriesCount);
			CRpfEntry *e = this->allEntries.at(i);

			if (e == nullptr)
				break;

			e->parent = item;

			if (e->entryType == CRpfEntry::Type::DIRECTORY_ENTRY)
			{
				CRpfDirectoryEntry *rde = (CRpfDirectoryEntry*)e;
				//rde->path = item->path + "\\" + rde->nameLower;
				item->directories.push_back(rde);
				stack.push(rde);
			}
			else if (e->entryType == CRpfEntry::Type::FILE_ENTRY)
			{
				CRpfFileEntry *rfe = (CRpfFileEntry*)e;
				//rfe->path = item->path + "\\" + rfe->nameLower;
				item->files.push_back(rfe);
			}

			e->path = item->path + "\\" + e->nameLower;
		}
	}

	for each (auto & entry in this->allEntries)
	{
		if (CUtil::EndsWith(entry->nameLower, ".rpf") && entry->entryType == CRpfEntry::Type::FILE_ENTRY)
		{
			CRpfBinaryFileEntry* fileEntry = (CRpfBinaryFileEntry*)entry;
			CRpfFile* file = new CRpfFile(this, fileEntry);

			this->children.push_back(file);
		}
	}
}

std::vector<std::byte> CRpfFile::ExtractFileBinary(CRpfBinaryFileEntry* entry)
{
	this->file->seekg(entry->fileOffsetPos, std::ios_base::beg);

	long l = entry->GetFileSize();

	if (l > 0)
	{
		uint32_t offset = 0;// 0x10;
		uint32_t totlen = (uint32_t)l - offset;

		std::vector<std::byte> tbytes(totlen);
		this->file->read((char*)tbytes.data(), totlen);

		if (entry->isEncrypted)
		{
			if (this->isAESEncrypted)
				CGTACrypto::DecryptAES(tbytes, entry->fileUncompressedSize);
			else
				tbytes = CGTACrypto::DecryptNG(tbytes, entry->name.c_str(), entry->fileUncompressedSize);
		}

		if (entry->fileSize > 0)
		{
			printf("FILE IS COMPRESSED!\n");
			return this->DecompressBytes(tbytes);
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

std::vector<std::byte> CRpfFile::DecompressBytes(std::vector<std::byte> bytes)
{
	std::vector<std::byte> def;
	CUtil::DecompressBytes(bytes, def);

	return def;
}

void CRpfFile::SearchFiles(std::string name, std::vector<CRpfEntry*>& filesOutput)
{
	for each (auto & child in this->children)
	{
		child->SearchFiles(name, filesOutput);
	}

	for each (auto & entry in this->allEntries)
	{
		if (entry->nameLower.find(name) != std::string::npos)
			filesOutput.push_back(entry);
	}
}

void CRpfFile::SearchFilesEndsWith(std::string endsWith, std::vector<CRpfEntry*>& filesOutput)
{
	for each (auto & child in this->children)
	{
		child->SearchFiles(endsWith, filesOutput);
	}

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