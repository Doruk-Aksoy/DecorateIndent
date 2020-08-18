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
					if(*(s.end() - 1) != ':')
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
				return 1;
			if (sid == SYMBOL_COLON && line.find("\"") != string::npos)
				return 1;
			if (sid == SYMBOL_DIV && (line.at(index + 1) == '/' || (!std::isdigit(line.at(index - 1)) && !std::isdigit(line.at(index + 1)))))
				return 1;
			// + symbol and left and right of it isn't number or starts with +
			if (sid == SYMBOL_PLUS && !std::isdigit(line.at(index - 1)) && !std::isdigit(line.at(index + 1)))
				return 1;
			if (sid == SYMBOL_MINUS && !std::isdigit(line.at(index - 1)))
				return 1;
			return 0;
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
						if (i != SYMBOL_COMMA && line.at(temp - 1) != ' ') {
							line.insert(line.begin() + temp, ' ');
							++inc;
						}
						if (line.at(temp + 1 + inc) != ' ')
							line.insert(line.begin() + temp + 1 + inc, ' ');
						temp = line.find(dk_punctuation[i], temp + 2 + inc);
						inc = 0;
					}
				}
			}
		}

		// process the line before formatting it, split into pieces for cleaner format
		void process(string& line) {
			// clean output buffer
			output.clear();
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
					if(line.at(temp + 1 + added) != ' ')
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
				if (!word_states_found && containsString(line, dk_States) != string::npos)
					word_states_found = true;
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
				if(temp)
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
					if(line.at(0) != '}')
					lines.push_back("}");
				}
				else { // case of }}}}
					lines.push_back("}");
					line.erase(0, 1);
					// safety in case } } } appears
					trim(line);
				}
			}
			if(!noAdd)
				lines.push_back(line);
			for (auto l : lines)
				formatLine(l);
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
				if(freePass > 0)
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
				if (closing_bracket_count == bracket_limit) {
					linesRead << "\n\n";
					closing_bracket_count = 0;
					bracket_limit = 0;
					word_states_found = false;
				}
				else
					linesRead << '\n';
				output.push_back(linesRead.str());
				//std::cout << linesRead.str();
				cleanStream(linesRead);
			}
		}

		bool isOutputReady() const {
			return giveOutput;
		}

		const vector<string>& getOutput() {
			return output;
		}
};

int Formatter::level_of_depth = 0;