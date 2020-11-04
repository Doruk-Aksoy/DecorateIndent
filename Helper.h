#pragma once


#include <string>
#include <sstream>

#include "KeyList.h"

std::string workingPath;
const std::string exeName("DecorateIndent.exe");
const size_t exeNameLength = exeName.length();

void setworkingPath(const char* fpath) {
	size_t targetLen = strlen(fpath) - exeNameLength;
	workingPath = fpath;
	while (workingPath.length() > targetLen)
		workingPath.pop_back();
}

void cleanStream(std::stringstream& s) {
	s.str("");
	s.clear();
}

std::string toUpperString(const std::string& s) {
	std::string res(s);
	std::transform(res.begin(), res.end(), res.begin(),
		[](unsigned char c) -> unsigned char { return std::toupper(c); });
	return res;
}

std::string getFnamefromArg(const char* fname) {
	std::string f(fname);
	// skip that as well
	return f.substr(f.find_last_of("\\") + 1);
}

// from stack overflow
bool findStringIC(const std::string& strHaystack, const std::string& strNeedle) {
	auto it = std::search(strHaystack.begin(), strHaystack.end(), strNeedle.begin(), strNeedle.end(),
		[](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
	return (it != strHaystack.end());
}

bool beginsWith(const std::string& s, const std::string& comp) {
	if (s.length() < 2)
		return false;
	// uppercase them
	std::string sw(toUpperString(s).substr(0, comp.length()));
	std::string sc(comp);
	return sw == sc;
}

// trim from left -- from so
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
		}));
}

// trim from right -- from so
static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both sides -- from so
static inline void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

bool isStateTerminator(const std::string& s) {
	for (int i = 0; i < MAX_STATE_TERMINATION_KEYS; ++i)
		if (beginsWith(s, dk_stateTerminate[i]) && *(s.end() - 1) != ':')
			return true;
	return false;
}

size_t containsString(const std::string& s, const std::string& comp) {
	std::string sw(toUpperString(s));
	return sw.find(comp);
}

size_t containsStateTerminator(const std::string& s) {
	size_t res = std::string::npos;
	std::string sw(toUpperString(s));
	for (int i = 0; i < MAX_STATE_TERMINATION_KEYS && res == std::string::npos; ++i) {
		res = (sw.find(dk_stateTerminate[i]));
		// check if this is a false positive by trying to find "_" left of it or "
		if (res != std::string::npos && res != 0 && (sw.at(res - 1) == '_' || sw.at(res - 1) == '"'))
			res = std::string::npos;
	}
	return res;
}