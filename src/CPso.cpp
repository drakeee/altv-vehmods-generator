#include <Main.h>

void CPsoDefinitionSection::Read(ByteReader& reader)
{
    this->ident = reader.ReadInt<int32_t>();
    auto length = reader.ReadInt<int32_t>();
    this->count = reader.ReadInt<int32_t>();

    this->entriesIdx.resize(this->count);
    for (int i = 0; i < this->count; i++)
    {
        auto entry = new CPsoElementIndexInfo();
        entry->Read(reader);
        this->entriesIdx.push_back(entry);
    }

    this->entries.resize(this->count);
    for (int i = 0; i < this->count; i++)
    {
        reader.byteRead = this->entriesIdx[i]->offset;
        auto type = reader.ReadBytes(1)[0];

        reader.byteRead = this->entriesIdx[i]->offset;
        if (type == (std::byte)0)
        {
            auto entry = new CPsoStructureInfo();
            entry->Read(reader);
            this->entries.push_back(entry);
        }
        else if (type == (std::byte)1)
        {
            auto entry = new CPsoEnumInfo();
            entry->Read(reader);
            this->entries.push_back(entry);
        }
        else
            throw new std::exception("unknown type!");
    }
}

void CPsoDefinitionSection::Write(ByteReader& reader)
{

}

void CPsoElementIndexInfo::Read(ByteReader& reader)
{
    this->nameHash = reader.ReadInt<int32_t>();
    this->offset = reader.ReadInt<int32_t>();
}

void CPsoElementIndexInfo::Write(ByteReader& writer)
{

}

void CPsoStructureInfo::Read(ByteReader& reader)
{
    uint32_t x = reader.ReadInt<uint32_t>();
    this->type = (std::byte)((x & 0xFF000000) >> 24);
    this->entriesCount = (short)(x & 0xFFFF);
    this->unk = (std::byte)((x & 0x00FF0000) >> 16);
    this->structureLength = reader.ReadInt<int32_t>();
    this->unk_Ch = reader.ReadInt<uint32_t>();

    this->entries.resize(this->entriesCount);
    for (int i = 0; i < this->entriesCount; i++)
    {
        auto entry = new CPsoStructureEntryInfo();
        entry->Read(reader);
        this->entries.push_back(entry);
    }
}

void CPsoStructureInfo::Write(ByteReader& writer)
{

}

void CPsoStructureEntryInfo::Read(ByteReader& reader)
{
    this->entryNameHash = reader.ReadInt<int32_t>();
    this->type = (DataType)reader.ReadBytes(1)[0];
    this->unk_5h = reader.ReadBytes(1)[0];
    this->dataOffset = reader.ReadInt<int16_t>();
    this->referenceKey = reader.ReadInt<int32_t>();
}

void CPsoStructureEntryInfo::Write(ByteReader& writer)
{

}

void CPsoEnumInfo::Read(ByteReader& reader)
{
    uint32_t x = reader.ReadInt<uint32_t>();
    this->type = (std::byte)((x & 0xFF000000) >> 24);
    this->entriesCount = (int32_t)(x & 0x00FFFFFF);

    this->entries.resize(this->entriesCount);
    for (int i = 0; i < this->entriesCount; i++)
    {
        auto entry = new CPsoEnumEntryInfo();
        entry->Read(reader);
        this->entries.push_back(entry);
    }
}

void CPsoEnumInfo::Write(ByteReader& reader)
{

}

void CPsoEnumEntryInfo::Read(ByteReader& reader)
{
    this->entryNameHash = reader.ReadInt<int32_t>();
    this->entryKey = reader.ReadInt<int32_t>();
}

void CPsoEnumEntryInfo::Write(ByteReader& reader)
{

}

void CPsoDataSection::Read(ByteReader& reader)
{
    this->ident = reader.ReadInt<uint32_t>();
    this->length = reader.ReadInt<int32_t>();
    reader.byteRead -= 8;

    this->data.resize(this->length);
    this->data = reader.ReadBytes(this->length);
}

void CPsoDataSection::Write(ByteReader& reader)
{

}

void CPsoDataMappingEntry::Read(ByteReader& reader)
{
    this->nameHash = reader.ReadInt<int32_t>();
    this->offset = reader.ReadInt<int32_t>();
    this->unknown_8h = reader.ReadInt<int32_t>();
    this->length = reader.ReadInt<int32_t>();
}

void CPsoDataMappingEntry::Write(ByteReader& reader)
{

}

void CPsoDataMappingSection::Read(ByteReader& reader)
{
    this->ident = reader.ReadInt<int32_t>();
    this->length = reader.ReadInt<int32_t>();
    this->rootIndex = reader.ReadInt<int32_t>();
    this->entriesCount = reader.ReadInt<int16_t>();
    this->unknown_Eh = reader.ReadInt<int16_t>();
    this->entries.resize(this->entriesCount);

    for (int i = 0; i < this->entriesCount; i++)
    {
        auto entry = new CPsoDataMappingEntry();
        entry->Read(reader);
        this->entries.push_back(entry);
    }
}

void CPsoDataMappingSection::Write(ByteReader& write)
{

}