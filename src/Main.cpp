#include "main.h"

namespace fs = std::filesystem;

int main()
{
	std::string gtaPath = "C:\\StemDl\\steamapps\\common\\Grand Theft Auto V\\";
	std::string dlcList = "C:\\StemDl\\steamapps\\common\\Grand Theft Auto V\\update\\update.rpf";
	std::string testFile = gtaPath + "common.rpf";

	fs::create_directory("./output_files");

	CRpfFile *test = new CRpfFile(dlcList.c_str());
	for each (auto &a in test->SearchFiles("dlclist.xml"))
	{
		printf("%s - %d - Path: %s\n", a->nameLower.c_str(), a->entryType, a->path.c_str());
		std::string outputName(("./output_files/" + a->nameLower));
		((CRpfBinaryFileEntry*)a)->ExtractFile(outputName);
	}

	
	for (const auto& fileEntry : fs::recursive_directory_iterator(gtaPath))
	{
		if (CUtil::EndsWith(fileEntry.path().string(), ".rpf"))
		{
			CRpfFile* file = new CRpfFile(fileEntry.path().string().c_str());
			//printf("File: %s - EntryCount: %d - Path: %s\n", file->fileName.c_str(), file->allEntries.size(), file->filePath.c_str());
			
			for each (auto & rpfEntry in file->SearchFiles("carcols.meta"))
			{
				printf("Found file: %s - Path: %s\n", rpfEntry->nameLower.c_str(), rpfEntry->path.c_str());
			}

			delete file;
		}
	}

	/*for each (auto &entry in test->allEntries)
	{
		if (entry->nameLower == "carcols.meta")
		{
			printf("File: %s\n", entry->nameLower.c_str());
		}
	}*/
	/*for each (auto &entry in test->allEntries)
	{
		if (entry->nameLower == "branchbend_maxshadersetting.xml")
		{
			printf("File binary: %s - %s\n", entry->nameLower.c_str(), entry->GetPhysicalFilePath().c_str());
			CUtil::PrintByteArray(test->ExtractFileBinary((CRpfBinaryFileEntry*)entry));
			break;
		}
	}*/

	std::system("pause");

	return 0;
}
