#include "main.h"

namespace fs = std::filesystem;

std::string temp;
void BEGIN_INFO(const char* x1) { temp.assign(x1); printf("\x1B[31m>\033[0m %s...\033[0m\n", temp.c_str()); }
void END_INFO() { printf("\x1B[31m>\033[0m %s... \x1B[32mDone\033[0m\n", temp.c_str()); }

int main()
{
	std::string gtaPath = "C:\\StemDl\\steamapps\\common\\Grand Theft Auto V\\";
	std::string dlcList = gtaPath + "update\\update.rpf";

	std::string outputFolder("./output_files/");
	fs::create_directories(outputFolder);
	//fs::create_directories(outputFolder + "update");
	fs::create_directories(outputFolder + "dlcpacks");
	fs::create_directories(outputFolder + "dlc_patch");

	BEGIN_INFO("Search for dlclist.xml in RPF archives");
	CRpfFile *test = new CRpfFile(dlcList.c_str());
	for each (auto &a in test->SearchFiles("dlclist.xml"))
	{
		printf("Found file: %s - Path: %s\n", a->nameLower.c_str(), a->path.c_str());
		((CRpfBinaryFileEntry*)a)->ExtractFile(outputFolder + a->nameLower);
	}
	END_INFO();

	BEGIN_INFO("Search for carcols.meta files in RPF archives");
	for (const auto& fileEntry : fs::recursive_directory_iterator(gtaPath))
	{
		if (CUtil::EndsWith(fileEntry.path().string(), ".rpf"))
		{
			CRpfFile* file = new CRpfFile(fileEntry.path().string().c_str());
			
			for each (auto & rpfEntry in file->SearchFiles("carcols.meta"))
			{
				std::smatch match;
				std::regex search(R"regex((dlc_patch|dlcpacks)\\(\w+))regex");
				std::regex_search(rpfEntry->path, match, search);

				if (match.size() >= 3)
				{
					std::string carcolPath(outputFolder + match[1].str() + kPathSeparator + match[2].str() + kPathSeparator);
					fs::create_directories(carcolPath);
					((CRpfBinaryFileEntry*)rpfEntry)->ExtractFile(carcolPath + rpfEntry->nameLower);

					printf("Found file: %s - Path: %s\n", rpfEntry->nameLower.c_str(), rpfEntry->path.c_str());
				}
			}

			delete file;
		}
	}
	END_INFO();

	//std::system("pause");

	return 1;
}
