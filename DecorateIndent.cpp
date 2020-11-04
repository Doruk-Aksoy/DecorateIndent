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
	for (int i = 1; i < argc; ++i) {
		Formatter F;
		string fname(getFnamefromArg(argv[i]));
		ss << workingPath << INDENTED_FOLDER << "\\" << fname;
		fstream in(argv[i], std::ios::in);
		fstream out(ss.str(), std::ios::out);
		cleanStream(ss);
		F.format(fname, in, out);
		out.close();
	}
	system("PAUSE");
	return 0;
}