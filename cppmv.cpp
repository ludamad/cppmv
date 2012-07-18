//============================================================================
// Usage: <file to update> <file1> <file1 new location> ... <filen> <filen new location>
//============================================================================

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>

using namespace std;

static bool starts_with(const char* line, const char* prefix,
		const char** content) {
	int length = strlen(prefix);
	bool hasprefix = strncmp(line, prefix, length) == 0;
	if (hasprefix) {
		*content = line + length;
		return true;
	}
	return false;
}

static std::string copy_until(const char* line, char c,
		const char** loc = NULL) {
	std::string ret;
	int i;
	for (i = 0; line[i] && line[i] != c; i++) {
		ret += line[i];
	}
	if (loc) {
		*loc = line + i + (line[i] ? 1 : 0);
	}
	return ret;
}

struct Path {
	int dotdots;
	std::vector<std::string> components;

	Path() :
			dotdots(0) {
	}
	Path(const char* path) :
			dotdots(0) {
		const char* p = path;
		while (starts_with(p, "../", &p)) {
			dotdots++;
		}
		while (*p) {
			//Will copy to end for last string
			components.push_back(copy_until(p, '/', &p));
		}
	}
	bool operator<(const Path& o) const {
		if (dotdots != o.dotdots) {
			return dotdots > o.dotdots;
		}
		if (components == o.components) {
			return false;
		}

		int i = 0;
		while (components[i] == o.components[i]) {
			i++;
		}
		//file name component
		bool last = (i == components.size() - 1);
		bool olast = (i == o.components.size() - 1);
		if (last != olast) {
			return components.size() > o.components.size();
		} else {
			return components[i] < o.components[i];
		}
		return true;
	}
	std::string to_string() const {
		std::string ret;
		for (int i = 0; i < dotdots; i++) {
			ret.append("../", 3);
		}
		for (int i = 0; i < components.size(); i++) {
			ret.append(components[i]);
			if (i != components.size() - 1) {
				ret.append("/");
			}
		}
		return ret;
	}
//	void move_to()
};

struct Include {
	Path includepath;
	int lineno;
	bool operator<(const Include& o) const {
		return includepath < o.includepath;
	}
};

struct FileBuffer {
	std::vector<std::string> lines;
	std::vector<Include> includes;
};

static void consume_file(FileBuffer& file, const char* fname) {
	Include inc;
	std::fstream f(fname, std::ios_base::in);

	std::string inccont;
	const char* content;
	int lineno = 0;
	char buff[255];

	while (!f.eof()) {
		f.getline(buff, 255);
		file.lines.push_back(buff);
		if (starts_with(buff, "#include \"", &content)) {
			inccont = copy_until(content, '"');
			inc.includepath = Path(inccont.c_str());
			inc.lineno = lineno;
			file.includes.push_back(inc);
		}
		lineno++;
	}
	f.close();
}

static void update_includes(FileBuffer& file) {
	std::vector<int> linenos;
	for (int i = 0; i < file.includes.size(); i++) {
		linenos.push_back(file.includes[i].lineno);
	}
	std::sort(file.includes.begin(), file.includes.end());

	for (int i = 0; i < file.includes.size(); i++) {
		file.includes[i].lineno = linenos[i];
	}
}

static void update_file(FileBuffer& file, const char* fname) {
	std::fstream f(fname, std::ios_base::out);

	for (int i = 0; i < file.lines.size(); i++) {
		std::string line = file.lines[i];
		for (int z = 0; z < file.includes.size(); z++) {
			if (i == file.includes[z].lineno) {
				line = "#include \"" + file.includes[z].includepath.to_string()
						+ "\"";
				break;
			}
		}
		f.write(line.c_str(), line.size());
		f.write("\n", 1);
	}
	f.close();
}

const char* USAGE =
		"Usage: --refs <file to update> <file1 old> <file1 new> ... <filen old> <filen new>\n"
				"Usage: --mv <file old> <file new>\n";

int main(int argc, const char** argv) {
	bool include_update = false;
	bool header_update = false;

	if (argc >= 5 && strcmp(argv[1], "--refs") == 0) {
		include_update = true;
	}
	if (argc >= 4 && strcmp(argv[1], "--mv") == 0) {
		header_update = true;
	}

	if (!include_update && !header_update) {
		printf(USAGE);
		return 0;
	}
	const char* fname1 = argv[2];
	const char* fname2 = argv[3];
	FileBuffer file;
	consume_file(file, fname1);
	update_includes(file);
	update_file(file, fname2);

	return 0;
}
