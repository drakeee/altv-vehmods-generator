#pragma once

#include <Main.h>

class CPsoElementIndexInfo;
class CPsoStructureEntryInfo;
class CPsoEnumEntryInfo;
class CPsoEnumEntryInfo;

enum DataType : unsigned char
{
    Boolean = 0x00,
    LONG_01h = 0x01,
    ByteDef = 0x02,
    SHORT_03h = 0x03,
    SHORT_04h = 0x04,
    INT_05h = 0x05,
    Integer = 0x06,
    Float = 0x07,
    Float2 = 0x08,
    TYPE_09h = 0x09,
    Float4 = 0x0a,
    String = 0x0b,
    Structure = 0x0c,
    Array = 0x0d,
    Enum = 0x0e,
    Flags = 0x0f,
    Map = 0x10,
    TYPE_14h = 0x14,
    Float3 = 0x15,
    SHORT_1Eh = 0x1e,
    LONG_20h = 0x20
};

enum Section : uint32_t
{
    Data = 0x5053494E,
    DataMapping = 0x504D4150,
    Definition = 0x50534348,
};

class CPsoData
{
public:
    virtual void Read(ByteReader& reader) = 0;
    virtual void Write(ByteReader& write) = 0;
};

class CPsoDefinitionSection : public CPsoData
{
public:
    int ident = 0x50534348;
    uint32_t count;

    std::vector<CPsoElementIndexInfo*> entriesIdx;
    std::vector<CPsoData*> entries;

    void Read(ByteReader& reader) override;
    void Write(ByteReader& writer) override;
};

class CPsoElementIndexInfo : public CPsoData
{
public:
    int32_t nameHash;
    int32_t offset;

    void Read(ByteReader& reader) override;
    void Write(ByteReader& writer) override;
};

//class CPsoElementInfo
//{
//public:
//    virtual void Read(DataReader reader);
//    virtual void Write(DataWriter writer);
//}

class CPsoStructureInfo : public CPsoData
{
public:
    std::byte type = (std::byte)0;
    short entriesCount;
    std::byte unk;
    int structureLength;
    uint32_t unk_Ch = 0x00000000;
    std::vector<CPsoStructureEntryInfo*> entries;

    void Read(ByteReader& reader) override;
    void Write(ByteReader& writer) override;

    /*override void Write(DataWriter writer)
    {
        Type = 0;
        EntriesCount = (short)Entries.Count;

        uint typeAndEntriesCount = (uint)(Type << 24) | (uint)(Unk << 16) | (ushort)EntriesCount;
        writer.Write(typeAndEntriesCount);
        writer.Write(StructureLength);
        writer.Write(Unk_Ch);

        foreach(auto entry in Entries)
        {
            entry.Write(writer);
        }
    }*/
};

class CPsoStructureEntryInfo : public CPsoData
{
public:
    int entryNameHash;
    DataType type;
    std::byte unk_5h;
    short dataOffset;
    int referenceKey; // when array -> entry index with type

    CPsoStructureEntryInfo() {}
    CPsoStructureEntryInfo(int nameHash, DataType type, std::byte unk5, short dataOffset, int referenceKey)
    {
        this->entryNameHash = nameHash;
        this->type = type;
        this->unk_5h = unk5;
        this->dataOffset = dataOffset;
        this->referenceKey = referenceKey;
    }

    void Read(ByteReader& reader) override;
    void Write(ByteReader& writer) override;
    /*void Write(DataWriter writer)
    {
        writer.Write(EntryNameHash);
        writer.Write((byte)Type);
        writer.Write(Unk_5h);
        writer.Write(DataOffset);
        writer.Write(ReferenceKey);
    }*/
};

class CPsoEnumInfo : public CPsoData
{
public:
    std::byte type = (std::byte)1;
    int entriesCount;
    std::vector<CPsoEnumEntryInfo*> entries;

    void Read(ByteReader& reader) override;
    void Write(ByteReader& reader) override;

    //override void Write(DataWriter writer)
    //{
    //    // update...
    //    Type = 1;
    //    EntriesCount = Entries.Count;

    //    uint typeAndEntriesCount = (uint)(Type << 24) | (uint)EntriesCount;
    //    writer.Write(typeAndEntriesCount);

    //    foreach(auto entry in Entries)
    //    {
    //        entry.Write(writer);
    //    }
    //}
};

class CPsoEnumEntryInfo : public CPsoData
{
public:
    int32_t entryNameHash;
    int32_t entryKey;

    CPsoEnumEntryInfo()
    { }

    CPsoEnumEntryInfo(int nameHash, int key)
    {
        this->entryNameHash = nameHash;
        this->entryKey = key;
    }

    void Read(ByteReader& reader) override;
    void Write(ByteReader& writer) override;

    /*void Write(DataWriter writer)
    {
        writer.Write(EntryNameHash);
        writer.Write(EntryKey);
    }*/
};

class CPsoDataSection : public CPsoData
{
public:
    uint32_t ident = 0x5053494E;
    int32_t length;
    std::vector<std::byte> data;

    void Read(ByteReader& reader) override;
    void Write(ByteReader& reader) override;

    /*void Write(DataWriter writer)
    {
        Length = Data.Length;

        writer.Write(Data);
        writer.Position -= Length;
        writer.Write(Ident);
        writer.Write((uint)(Length));
        writer.Position += Length - 8;
    }*/

    /*override string ToString()
    {
        return Ident.ToString() + ": " + Length.ToString();
    }*/
};

class CPsoDataMappingEntry : public CPsoData
{
public:
    int nameHash;
    int offset;
    int unknown_8h = 0x00000000;
    int length;

    void Read(ByteReader& reader) override;
    void Write(ByteReader& writer) override;
};

class CPsoDataMappingSection : public CPsoData
{
public:
    int ident = 0x504D4150;
    int length;
    int rootIndex;
    short entriesCount;
    short unknown_Eh = 0x7070;
    std::vector<CPsoDataMappingEntry*> entries;

    void Read(ByteReader& reader) override;
    void Write(ByteReader& writer) override;
};

class CPso
{
public:
	ByteReader &reader;
    CPsoDataSection *dataSection;
    CPsoDataMappingSection *dataMappingSection;
    CPsoDefinitionSection *definitionSection;

	CPso::CPso(ByteReader& reader) :
		reader(reader)
	{

	}

    void Load()
    {
        this->reader.byteRead = 0;

        //auto reader = new DataReader(stream, Endianess.BigEndian);
        while (this->reader.byteRead < this->reader.length)
        {
            auto identInt = reader.ReadInt<uint32_t>();
            auto ident = (Section)identInt;
            auto length = reader.ReadInt<int32_t>();

            reader.byteRead -= 8;

            auto sectionData = reader.ReadBytes(length);
            auto sectionReader = ByteReader(sectionData);

            switch (ident)
            {
            case Section::Data:
            {
                this->dataSection = new CPsoDataSection();
                this->dataSection->Read(sectionReader);
                break;
            }
            case Section::DataMapping:
            {
                this->dataMappingSection = new CPsoDataMappingSection();
                this->dataMappingSection->Read(sectionReader);
                break;
            }
            case Section::Definition:
            {
                this->definitionSection = new CPsoDefinitionSection();
                this->definitionSection->Read(sectionReader);
                break;
            }
            default:
                // ignore
                break;
            }
        }
    }

	inline bool IsPSO(void)
	{
		uint32_t identInt = reader.ReadInt<uint32_t>();
		this->reader.byteRead = 0;

		printf("Shit: %d\n", identInt);

		return ((identInt) == 1313428304); //"PSIN"
	}
};