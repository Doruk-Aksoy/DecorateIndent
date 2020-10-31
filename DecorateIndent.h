#pragma once

#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <vector>

#include "KeyList.h"
#include "Helper.h"

//#define VERBOSE

using std::string;
using std::fstream;
using std::stringstream;
using std::string_view;
using std::vector;

/*
future improvements:

- add meaning to each line added (wrap in class, avoids rechecking state stuff)
- make parsing more efficient?
*/

class Formatter {
private:

	static int level_of_depth;

	bool foundState;
	bool foundStateTerminator;
	bool skipTab;
	bool giveOutput;
	bool appendSpace;
	bool includeTabs;
	bool word_states_found;

	vector<string> output;

	int freePass;
	int closing_bracket_count;
	int bracket_limit;

	bool isComment;

	stringstream linesRead;

public:

	Formatter() :
		foundState(false),
		foundStateTerminator(false),
		skipTab(false),
		giveOutput(false),
		appendSpace(false),
		includeTabs(true),
		word_states_found(false),

		freePass(0),
		closing_bracket_count(0),
		bracket_limit(0)
	{
		level_of_depth = 0;
	}

	~Formatter() {
		level_of_depth = 0;
	}

	void updateTabs(string& tabs) {
		tabs = "";
		for (int i = 0; i < level_of_depth - skipTab; ++i)
			tabs += '\t';
	}

	void applyChecks(string& s) {
		// detect curly bracket closure
		if (s.at(0) == '{') {
			giveOutput = true;
			appendSpace = true;
			++bracket_limit;
		}
		if (s.at(0) == '}') {
			--level_of_depth;
			++closing_bracket_count;
		}
		// detect state name
		if (s.length() >= STATE_TERMINATOR_LENGTH) {
			if (!foundState && *(s.end() - 1) == ':') {
				//std::cout << "State found: " << s << "\n";
				++level_of_depth;
				foundState = true;
				skipTab = true;
			}
			else {
				if (!foundState && foundStateTerminator) {
					++level_of_depth;
					foundStateTerminator = false;
				}

				// if the rest aren't more states
				if (*(s.end() - 1) != ':')
					skipTab = false;
				if (isStateTerminator(s)) {
					--level_of_depth;
					// if previously had found one
					if (!foundState && foundStateTerminator)
						--level_of_depth;
					foundState = false;
					foundStateTerminator = true;
				}
			}
		}
		//std::cout << "<" << level_of_depth << ">\n";
	}

	bool isSymbolException(const string& line, size_t index, int sid) const {
		// no symbols at beginning for enforcing spacing
		if (!index)
			return true;
		if (sid == SYMBOL_COLON && line.find("\"") != string::npos)
			return true;
		if (sid == SYMBOL_DIV && (line.at(index + 1) == '/' || (!std::isdigit(line.at(index - 1)) && !std::isdigit(line.at(index + 1)))))
			return true;
		// + symbol and left and right of it isn't number or starts with +
		if (sid == SYMBOL_PLUS && !std::isdigit(line.at(index - 1)) && !std::isdigit(line.at(index + 1)))
			return true;
		if (sid == SYMBOL_MINUS && !std::isdigit(line.at(index - 1)))
			return true;
		return false;
	}

	bool isSymbolOK(const string& line, size_t index, int sid) const {
		if (sid == SYMBOL_ASSIGN || sid == SYMBOL_GREATER || sid == SYMBOL_LESS) {
			if (line.at(index - 1) == '<' || line.at(index - 1) == '>')
				return false;
		}
		return sid != SYMBOL_COMMA && line.at(index - 1) != ' ';
	}

	// enforces a whitespace before and after a punctuation is used
	void enforceSpacing(string& line) {
		size_t temp = 0, inc = 0;
		//std::cout << "line: <" << line << ">\n";
		for (int i = 0; i < MAX_PUNCTUATION_FOR_SPACING; ++i) {
			temp = line.find(dk_punctuation[i], 0);
			if (temp < line.length() - 1) {
				//std::cout << "try: " << dk_punctuation[i] << " " << temp << '\n';
				while (temp != string::npos) {
					// don't count in the comment symbol
					if (isSymbolException(line, temp, i))
						break;
					if (isSymbolOK(line, temp, i)) {
						++inc;
						line.insert(line.begin() + temp, ' ');
					}
					// skip one further for equals symbol, contains extra char (==)
					temp += inc + 1 + (i == SYMBOL_EQUALS || i == SYMBOL_GE || i == SYMBOL_LE);
					if (temp < line.length() && line.at(temp) != ' ') {
						// little exception check for SYMBOL_ASSIGN, GREATER and LESS, as they can be part of SYMBOL_EQUALS (==) (>=) and (<=) respectively
						if (i == SYMBOL_ASSIGN || i == SYMBOL_GREATER || i == SYMBOL_LESS) {
							if (line.at(temp) != '=')
								line.insert(line.begin() + temp, ' ');
						}
						else
							line.insert(line.begin() + temp, ' ');
					}
					temp = line.find(dk_punctuation[i], temp + 2 + inc);
					inc = 0;
				}
			}
		}
	}

	// check for abnormalities within this line, like same line comments etc. and move them up
	void checkLine(vector<string>& lines, string& line) {
		size_t n;
		if ((n = line.find(comment_char)) != string::npos) {
			// enforce a space after the comment begins
			if (line.at(n + 2) != ' ')
				line.insert(n + 2, " ");
			string comment = line.substr(n);
			lines.push_back(comment);
			line = line.substr(0, n - 1);

			// comment lines get a free pass
			freePass = 1;
			isComment = true;
		}
		else
			isComment = false;
		lines.push_back(line);
	}


	void formatLine(string& line) {
		std::string tabs;
		trim(line);
		enforceSpacing(line);
		if (!line.length())
			return;
		//std::cout << "Formatting:\n<" << line << ">\n";
		// uppercase first char
		line.at(0) = std::toupper(line.at(0));
		if (!freePass) {
			// handle tabs at beginning, we check here because we don't want Actor or States lines to be considered
			if (level_of_depth) {
				// put tabs before the string
				updateTabs(tabs);
			}

			if (beginsWith(line, dk_Actor)) {
				// this means this is a new actor definition, reset level of depth to 1
				// also push this in immediately
				linesRead << line;
				includeTabs = false;
				giveOutput = false;
				foundStateTerminator = false;
				level_of_depth = 1;
			}

			if (level_of_depth < 2 && beginsWith(line, dk_States)) {
				linesRead << tabs << line;

				includeTabs = false;
				giveOutput = false;
				++level_of_depth;
			}
			applyChecks(line);
		}
		//std::cout << '\n' << level_of_depth << '\n';
		if (giveOutput || freePass) {
			if (freePass > 0)
				--freePass;

			if (appendSpace) {
				linesRead << ' ';
				appendSpace = false;
			}
			// add in the tabs
			if (includeTabs) {
				updateTabs(tabs);
				linesRead << tabs;
			}
			else
				includeTabs = true;

			linesRead << line;

			//std::cout << closing_bracket_count << " " << bracket_limit << '\n';
			if (!isComment && closing_bracket_count == bracket_limit) {
				linesRead << "\n\n";
				closing_bracket_count = 0;
				bracket_limit = 0;
				word_states_found = false;
			}
			else
				linesRead << '\n';
			output.push_back(linesRead.str());
			isComment = false;
			//std::cout << linesRead.str();
			cleanStream(linesRead);
		}
	}

	// process the line before formatting it, split into pieces for cleaner format
	void process(string& line) {
		// make sure the string is well formed
		vector<string> lines;
		// do trimming of whitespaces
		trim(line);
		// handle proper formatting expected, { on new line, everything else seperated etc.
		// two loops below hunt cases like Actor asd { asdasdf }
		size_t temp, added = 0;
		bool noAdd = false;
		while (line.length() > 1 && (temp = line.find_first_of('{')) != string::npos) {
			//std::cout << "in loop\n";
			if (!noAdd && line.length() > temp + 1 && line.at(temp + 1) == '}') {
				//std::cout << "free pass\n";
				++freePass;
				giveOutput = true;
				// insert white spaces
				if (line.at(temp - 1) != ' ') {
					line.insert(line.begin() + temp, ' ');
					added = 1;
				}
				// +2 because of the above insert
				if (line.at(temp + 1 + added) != ' ')
					line.insert(line.begin() + temp + 1 + added, ' ');
				if (line.length() > temp + added + 3) {
					// means way more in here that must be checked
					lines.push_back(line.substr(0, temp + added + 3));
					line = line.substr(temp + added + 3);
					continue;
				}
				break;
			}
			noAdd = true;
			lines.push_back(line.substr(0, temp));
			lines.push_back("{");
			line = line.substr(temp + 1);

			// check if word 'states' occured
			if (!word_states_found && containsString(line, dk_States) != string::npos) {
				word_states_found = true;
				
				// when it finds states label here manually add it as we don't want to discard it, the part below will catch the whole word but not this
				lines.push_back("States");
			}
		}
		// check if word 'states' occured
		if (!word_states_found && containsString(line, dk_States) != string::npos)
			word_states_found = true;
		// check for state names
		while (word_states_found && line.length() > 1 && (temp = line.find_first_of(':')) != string::npos) {
			noAdd = true;
			lines.push_back(line.substr(0, temp + 1));
			line = line.substr(temp + 1);
		}
		// check for state terminators
		// assumes only 1 state terminator
		if ((temp = containsStateTerminator(line)) != string::npos) {
			noAdd = true;
			// no empty substr
			if (temp)
				lines.push_back(line.substr(0, temp));
			// get until all or }
			if ((added = line.find_first_of('}')) != string::npos) {
				lines.push_back(line.substr(temp, added - temp));
				line = line.substr(added);
			}
			else {
				line = line.substr(temp);
				lines.push_back(line);
			}
		}

		// check if } is also found
		// also don't care if we got a freePass
		while (!freePass && line.length() && (temp = line.find_first_of('}')) != string::npos) {
			noAdd = true;
			if (line.at(0) != '}') {
				lines.push_back(line.substr(0, temp));
				line = line.substr(temp);
				// add in the one we cut off
				if (line.at(0) != '}')
					lines.push_back("}");
			}
			else { // case of }}}}
				lines.push_back("}");
				line.erase(0, 1);
				// safety in case } } } appears
				trim(line);
			}
		}
		if (!noAdd) {
			checkLine(lines, line);
		}
		for (auto l : lines)
			formatLine(l);
	}

	void format(const string& fname, fstream& in, fstream& out) {
		std::cout << "Starting " << fname << "\n";
		string temp_in;
		while (std::getline(in, temp_in)) {
			// ignore bad lines
			if (!temp_in.length())
				continue;
#ifdef VERBOSE
			std::cout << "processing " << temp_in << '\n';
#endif
			process(temp_in);
			if (isOutputReady()) {
				vector<string> tv = getOutput();
				for (auto o : tv)
					out << o;
				clearOutput();
			}
		}
		out.close();
		std::cout << fname << " finished.\n";
	}

	bool isOutputReady() const {
		return giveOutput;
	}

	const vector<string>& getOutput() {
		return output;
	}

	void clearOutput() {
		output.clear();
	}
};

int Formatter::level_of_depth = 0;