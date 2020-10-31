#include "DecorateIndent.h"

#include <iostream>

constexpr auto INDENTED_FOLDER = "Indents";

int main(int argc, const char* argv[]) {
	if (argc < 2)
		return -1;
	// get working path
	setworkingPath(argv[0]);
	// make new directory if it doesnt exist
	// check if directory exists, if not, create it
	namespace fs = std::filesystem;
	if (!fs::exists(INDENTED_FOLDER))
		fs::create_directory(INDENTED_FOLDER);
	stringstream ss;
	ss << workingPath << INDENTED_FOLDER << "\\";
	for (int i = 1; i < argc; ++i) {
		Formatter F;
		string fname(getFnamefromArg(argv[i]));
		ss << fname;
		fstream in(argv[i], std::ios::in);
		fstream out(ss.str(), std::ios::out);
		cleanStream(ss);

		std::cout << "Starting " << fname << "\n";
		string temp_in;
		while (std::getline(in, temp_in)) {
			
			// ignore bad lines
			if (!temp_in.length())
				continue;
			//std::cout << "processing " << temp_in << '\n';
			F.process(temp_in);
			if (F.isOutputReady()) {
				vector<string> tv = F.getOutput();
				for(auto o : tv)
					out << o;
			}
		}
		out.close();
		std::cout << fname << " finished.\n";
	}
	system("PAUSE");
	return 0;
}