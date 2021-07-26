#pragma once

#include <Main.h>

class CRpfEntry;
class CRpfBinaryFileEntry;
class CRpfDirectoryEntry;

class CRpfFile
{
public:
	enum Encryption
	{
		NONE = 0, //some modded RPF's may use this
		OPEN = 0x4E45504F, //1313165391 "OPEN", ie. "no encryption"
		AES = 0x0FFFFFF9, //268435449
		NG = 0x0FEFFFFF, //267386879
	};

#pragma pack(push, 1)
	struct RpfHeader
	{
		unsigned char version;
		char magic[3];
		int entryCount;
		int namesLength;
		int encrypted;
	};
#pragma pack(pop)

	RpfHeader header;
    CRpfFile* parent;

	std::ifstream file;
	std::string filePath;
    std::string fileRelativePath = "";
	std::string fileName;
	uint32_t fileSize;

    std::vector<CRpfEntry*> allEntries;
    CRpfDirectoryEntry* root;
    std::string path;

    uint32_t totalFileCount = 0;
    uint32_t totalFolderCount = 0;
    uint32_t totalResourceCount = 0;
    uint32_t totalBinaryFileCount = 0;

    bool isAESEncrypted = false;
    bool isNGEncrypted = false;

	CRpfFile(const char* filePath);

    std::vector<std::byte> ExtractFileBinary(CRpfBinaryFileEntry* entry);
    void ExtractFileBinary(CRpfBinaryFileEntry* entry, std::string path);
    std::vector<std::byte> DecompressBytes(std::vector<std::byte> bytes);

    void SearchFiles(std::string name, std::vector<CRpfEntry*>& filesOutput);
    void SearchFilesEndsWith(std::string endsWith, std::vector<CRpfEntry*>& filesOutput);

    std::vector<CRpfEntry*> SearchFiles(std::string name);
    std::vector<CRpfEntry*> SearchFilesEndsWith(std::string endsWith);

    inline CRpfFile *GetTopParent()
    {
        CRpfFile *pfile = this;
        while (pfile->parent != nullptr)
        {
            pfile = pfile->parent;
        }
        return pfile;
    }

    std::string GetPhysicalFilePath()
    {
        return GetTopParent()->filePath;
    }

    //{
    //    try
    //    {
    //        using (DeflateStream ds = new DeflateStream(new MemoryStream(bytes), CompressionMode.Decompress))
    //        {
    //            MemoryStream outstr = new MemoryStream();
    //            ds.CopyTo(outstr);
    //            byte[] deflated = outstr.GetBuffer();
    //            byte[] outbuf = new byte[outstr.Length]; //need to copy to the right size buffer for output.
    //            Array.Copy(deflated, outbuf, outbuf.Length);

    //            if (outbuf.Length <= bytes.Length)
    //            {
    //                LastError = "Warning: Decompressed data was smaller than compressed data...";
    //                //return null; //could still be OK for tiny things!
    //            }

    //            return outbuf;
    //        }
    //    }
    //    catch (Exception ex)
    //    {
    //        LastError = "Could not decompress.";// ex.ToString();
    //        LastException = ex;
    //        return null;
    //    }
    //}

private:
	void ReadHeader(void);
};

class CRpfFileEntry;
class CRpfDirectoryEntry;

class CRpfEntry
{
public:
    enum Type
    {
        NONE = 0,
        DIRECTORY_ENTRY,
        FILE_ENTRY,
        RESOURCE_FILE_ENTRY
    };

    Type entryType = Type::NONE;

    CRpfFile *file = nullptr;
    CRpfDirectoryEntry *parent = nullptr;

    uint32_t nameHash;
    uint32_t shortNameHash;

    uint32_t nameOffset;
    std::string name;
    std::string nameLower;
    std::string path;

    uint32_t H1;
    uint32_t H2;

    virtual void Read(ByteReader &reader) {};
    //virtual void Write(ByteReader writer);

    inline CRpfEntry* GetTopParent()
    {
        CRpfEntry* pfile = this;
        while (pfile->parent != nullptr)
        {
            pfile = (CRpfEntry*)pfile->parent;
        }
        return pfile;
    }

    std::string GetPhysicalFilePath()
    {
        return GetTopParent()->path;
    }


    std::string ToString()
    {
        return this->path;
    }
};

class CRpfDirectoryEntry : public CRpfEntry
{
public:
    CRpfDirectoryEntry()
    {
        this->entryType = CRpfEntry::Type::DIRECTORY_ENTRY;
    }

    uint32_t entriesIndex;
    uint32_t entriesCount;

    std::vector<CRpfDirectoryEntry*> directories;
    std::vector<CRpfFileEntry*> files;

    void Read(ByteReader &reader) override
    {
        this->nameOffset = reader.ReadInt<uint32_t>();
        uint32_t ident = reader.ReadInt<uint32_t>();

        if (ident != 0x7FFFFF00u)
        {
            throw ("Error in RPF7 directory entry.");
            return;
        }

        this->entriesIndex = reader.ReadInt<uint32_t>();
        this->entriesCount = reader.ReadInt<uint32_t>();
    }

    /*void Write(ByteReader writer) override
    {
        writer.Write(NameOffset);
        writer.Write(0x7FFFFF00u);
        writer.Write(EntriesIndex);
        writer.Write(EntriesCount);
    }*/

    std::string ToString()
    {
        return "Directory: " + this->path;
    }
};

class CRpfFileEntry : public CRpfEntry
{
public:
    CRpfFileEntry()
    {
        this->entryType = CRpfEntry::Type::FILE_ENTRY;
    }

    uint32_t fileOffset;
    uint32_t fileSize;
    bool isEncrypted;

    virtual long GetFileSize() { return 0; };
    virtual void SetFileSize(uint32_t s) {};
};

class CRpfBinaryFileEntry : public CRpfFileEntry
{
public:
    uint32_t fileUncompressedSize;
    uint32_t encryptionType;

    void Read(ByteReader &reader) override
    {
        uint64_t buf = reader.ReadInt<uint64_t>();

        this->nameOffset = (uint32_t)buf & 0xFFFF;
        this->fileSize = (uint32_t)(buf >> 16) & 0xFFFFFF;
        this->fileOffset = (uint32_t)(buf >> 40) & 0xFFFFFF;

        this->fileUncompressedSize = reader.ReadInt<uint32_t>();

        this->encryptionType = reader.ReadInt<uint32_t>();

        switch (this->encryptionType)
        {
        case 0: this->isEncrypted = false; break;
        case 1: this->isEncrypted = true; break;
        default:
            throw ("Error in RPF7 file entry.");
            break;
        }

    }

    void ExtractFile(std::string path)
    {
        this->file->ExtractFileBinary(this, path);
    }

    long GetFileSize() override
    {
        return (this->fileSize == 0) ? this->fileUncompressedSize : this->fileSize;
    }

    void SetFileSize(uint32_t s) override
    {
        //FileUncompressedSize = s;
        this->fileSize = s;
    }
};

class CRpfResourceFileEntry : public CRpfFileEntry
{
public:
    CRpfResourceFileEntry()
    {
        this->entryType = CRpfEntry::Type::RESOURCE_FILE_ENTRY;
    }
    //CRpfResourcePageFlags SystemFlags{ get; set; }
    //CRpfResourcePageFlags GraphicsFlags{ get; set; }


    static int GetSizeFromFlags(uint32_t flags)
    {
        //dexfx simplified version
        auto s0 = ((flags >> 27) & 0x1) << 0;   // 1 bit  - 27        (*1)
        auto s1 = ((flags >> 26) & 0x1) << 1;   // 1 bit  - 26        (*2)
        auto s2 = ((flags >> 25) & 0x1) << 2;   // 1 bit  - 25        (*4)
        auto s3 = ((flags >> 24) & 0x1) << 3;   // 1 bit  - 24        (*8)
        auto s4 = ((flags >> 17) & 0x7F) << 4;   // 7 bits - 17 - 23   (*16)   (max 127 * 16)
        auto s5 = ((flags >> 11) & 0x3F) << 5;   // 6 bits - 11 - 16   (*32)   (max 63  * 32)
        auto s6 = ((flags >> 7) & 0xF) << 6;   // 4 bits - 7  - 10   (*64)   (max 15  * 64)
        auto s7 = ((flags >> 5) & 0x3) << 7;   // 2 bits - 5  - 6    (*128)  (max 3   * 128)
        auto s8 = ((flags >> 4) & 0x1) << 8;   // 1 bit  - 4         (*256)
        auto ss = ((flags >> 0) & 0xF);         // 4 bits - 0  - 3
        auto baseSize = 0x200 << (int)ss;
        auto size = baseSize * (s0 + s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8);
        return (int)size;
    }
    static uint32_t GetFlagsFromSize(int size, uint32_t version)
    {
        //WIP - may make crashes :(
        //type: see SystemSize and GraphicsSize below

        //aim for s4: blocksize (0 remainder for 0x2000 block) 
        int origsize = size;
        int remainder = size & 0x1FF;
        int blocksize = 0x200;
        if (remainder != 0)
        {
            size = (size - remainder) + blocksize; //round up to the minimum blocksize
        }

        uint32_t blockcount = (uint32_t)size >> 9; //how many blocks of the minimum size (0x200)
        uint32_t ss = 0;

        while (blockcount > 1024)
        {
            ss++;
            blockcount = blockcount >> 1;
        }
        if (ss > 0)
        {
            size = origsize;
            blocksize = blocksize << (int)ss; //adjust the block size to reduce the block count.
            remainder = size & blocksize;
            if (remainder != 0)
            {
                size = (size - remainder) + blocksize; //readjust size with round-up
            }
        }

        auto s0 = (blockcount >> 0) & 0x1;  //*1         X
        auto s1 = (blockcount >> 1) & 0x1;  //*2          X
        auto s2 = (blockcount >> 2) & 0x1;  //*4           X
        auto s3 = (blockcount >> 3) & 0x1;  //*8            X
        auto s4 = (blockcount >> 4) & 0x7F; //*16  7 bits    XXXXXXX
        auto s5 = (blockcount >> 5) & 0x3F; //*32  6 bits           XXXXXX
        auto s6 = (blockcount >> 6) & 0xF;  //*64  4 bits                 XXXX
        auto s7 = (blockcount >> 7) & 0x3;  //*128 2 bits                     XX
        auto s8 = (blockcount >> 8) & 0x1;  //*256                              X

        if (ss > 4)
        {
        }
        if (s4 > 0x7F)
        {
        } //too big...
     //needs more work to include higher bits..


        uint32_t f = 0;
        f |= (version & 0xF) << 28;
        f |= (s0 & 0x1) << 27;
        f |= (s1 & 0x1) << 26;
        f |= (s2 & 0x1) << 25;
        f |= (s3 & 0x1) << 24;
        f |= (s4 & 0x7F) << 17;
        f |= (ss & 0xF);

        return f;
    }
    static uint32_t GetFlagsFromBlocks(uint32_t blockCount, uint32_t blockSize, uint32_t version)
    {
        uint32_t s0 = 0;
        uint32_t s1 = 0;
        uint32_t s2 = 0;
        uint32_t s3 = 0;
        uint32_t s4 = 0;
        uint32_t s5 = 0;
        uint32_t s6 = 0;
        uint32_t s7 = 0;
        uint32_t s8 = 0;
        uint32_t ss = 0;

        uint32_t bst = blockSize;
        if (blockCount > 0)
        {
            while (bst > 0x200) //ss is number of bits to shift 0x200 to get blocksize...
            {
                ss++;
                bst = bst >> 1;
            }
        }

        s0 = (blockCount >> 0) & 0x1;  //*1         X
        s1 = (blockCount >> 1) & 0x1;  //*2          X
        s2 = (blockCount >> 2) & 0x1;  //*4           X
        s3 = (blockCount >> 3) & 0x1;  //*8            X
        s4 = (blockCount >> 4) & 0x7F; //*16  7 bits    XXXXXXX

        if (ss > 0xF)
        {
        } //too big...
        if (s4 > 0x7F)
        {
        } //too big...

        uint32_t f = 0;
        f |= (version & 0xF) << 28;
        f |= (s0 & 0x1) << 27;
        f |= (s1 & 0x1) << 26;
        f |= (s2 & 0x1) << 25;
        f |= (s3 & 0x1) << 24;
        f |= (s4 & 0x7F) << 17;
        f |= (s5 & 0x3F) << 11;
        f |= (s6 & 0xF) << 7;
        f |= (s7 & 0x3) << 5;
        f |= (s8 & 0x1) << 4;
        f |= (ss & 0xF);

        return f;
    }

    static int GetVersionFromFlags(uint32_t sysFlags, uint32_t gfxFlags)
    {
        auto sv = (sysFlags >> 28) & 0xF;
        auto gv = (gfxFlags >> 28) & 0xF;
        return (int)((sv << 4) + gv);
    }

    void Read(ByteReader &reader) override
    {
        this->nameOffset = reader.ReadInt<uint16_t>();

        auto buf1 = reader.ReadBytes(3);
        this->fileSize = (uint32_t)buf1[0] + (uint32_t)(buf1[1] << 8) + (uint32_t)(buf1[2] << 16);

        auto buf2 = reader.ReadBytes(3);
        this->fileOffset = ((uint32_t)buf2[0] + (uint32_t)(buf2[1] << 8) + (uint32_t)(buf2[2] << 16)) & 0x7FFFFF;

        reader.ReadInt<uint32_t>();
        reader.ReadInt<uint32_t>();

        // there are sometimes resources with length=0xffffff which actually
        // means length>=0xffffff
        if (this->fileSize == 0xFFFFFF)
        {
            //BinaryReader cfr = File.CurrentFileReader;
            //long opos = cfr.BaseStream.Position;
            //cfr.BaseStream.Position = File.StartPos + ((long)FileOffset * 512); //need to use the base offset!!
            //auto buf = cfr.ReadBytes(16);
            //FileSize = ((uint32_t)buf[7] << 0) | ((uint32_t)buf[14] << 8) | ((uint32_t)buf[5] << 16) | ((uint32_t)buf[2] << 24);
            //cfr.BaseStream.Position = opos;
        }

    }
    
    //void Write(DataWriter writer)
    //{
    //    writer.Write((ushort)NameOffset);

    //    auto fs = FileSize;
    //    if (fs > 0xFFFFFF) fs = 0xFFFFFF;//will also need to make sure the RSC header is updated...

    //    auto buf1 = new byte[]{
    //        (byte)((fs >> 0) & 0xFF),
    //        (byte)((fs >> 8) & 0xFF),
    //        (byte)((fs >> 16) & 0xFF)
    //    };
    //    writer.Write(buf1);

    //    auto buf2 = new byte[]{
    //        (byte)((FileOffset >> 0) & 0xFF),
    //        (byte)((FileOffset >> 8) & 0xFF),
    //        (byte)(((FileOffset >> 16) & 0xFF) | 0x80)
    //    };
    //    writer.Write(buf2);

    //    writer.Write(SystemFlags);
    //    writer.Write(GraphicsFlags);
    //}

    /*override string ToString()
    {
        return "Resource file: " + Path;
    }*/

    /*long GetFileSize() override
    {
        return (FileSize == 0) ? (long)(SystemSize + GraphicsSize) : FileSize;
    }
    override void SetFileSize(uint32_t s)
    {
        FileSize = s;
    }*/
};