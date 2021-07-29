// vehmods-generator.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <istream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <assert.h>
#include <stack>

#ifdef _WIN32
const std::string kPathSeparator("\\");
#else
const std::string kPathSeparator("/");
#endif

#include <zlib.h>
#include <rijndael.h>
#include <regex>

#include <CUtil.h>
#include <CGTACrypto.h>
#include <CGTAKeys.h>
#include <CPso.h>
#include <CRpfFile.h>

// TODO: Reference additional headers your program requires here.
