#pragma once


#include <string>
#include <vector>
#include <fstream>

constexpr auto cckeyfile = "cc_keys.txt";
constexpr auto uckeyfile = "uc_keys.txt";

// depth keywords -- must be upper case
constexpr auto dk_Actor = "ACTOR";
constexpr auto dk_States = "STATES";

// state termination keywords
enum {
	ST_GOTO,
	ST_LOOP,
	ST_STOP,
	ST_WAIT,
	ST_FAIL
};

#define MAX_STATE_TERMINATION_KEYS ST_FAIL + 1
constexpr auto STATE_TERMINATOR_LENGTH = 4;
const std::string dk_stateTerminate[MAX_STATE_TERMINATION_KEYS] = {
	"GOTO",
	"LOOP",
	"STOP",
	"WAIT",
	"FAIL"
};

std::vector<std::string> ccase_keys;

enum {
	SYMBOL_COMMA,
	SYMBOL_COLON,
	SYMBOL_PLUS,
	SYMBOL_MINUS,
	SYMBOL_DIV,
	SYMBOL_MUL,
	SYMBOL_EQUALS,
	SYMBOL_ASSIGN
};

#define MAX_PUNCTUATION_FOR_SPACING (SYMBOL_ASSIGN + 1)
const char* dk_punctuation[MAX_PUNCTUATION_FOR_SPACING] = {
	",",
	":",
	"+",
	"-",
	"/",
	"*",
	"==",
	"="
};

// these are the keys that will be used for forcing case
void loadKeys() {
	// first camel case
	std::fstream in(cckeyfile, std::ios::in);

	std::string temp;
	while (std::getline(in, temp)) {
		ccase_keys.push_back(temp);
	}
}