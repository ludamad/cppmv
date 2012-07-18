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
	bool operator=(const Path& o) const {
		return dotdots == o.dotdots && components == o.components;
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
	//from abs to rel
	Path relpath_to(const Path& p) const {
		std::string filename = p.components.back();
		int nparts = components.size() - 1;
		int oparts = p.components.size() - 1;
		Path relpath;
		int i;
		for (i = 0; i < nparts && i < oparts; i++) {
			if (p.components[i] != components[i])
				break;
		}
		relpath.dotdots = nparts - i;
		for (; i < oparts; i++) {
			relpath.components.push_back(p.components[i]);
		}
		relpath.components.push_back(filename);
		return relpath;
	}
	//from rel to abs
	Path absolute_path(const Path& base) const {
		int bparts = base.components.size() - 1 - dotdots;
		Path relpath;
		for (int i = 0; i < bparts; i++) {
			relpath.components.push_back(base.components[i]);
		}
		for (int i = 0; i < components.size(); i++) {
			relpath.components.push_back(components[i]);
		}
		return relpath;
	}
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

	std::string inccont, line;
	const char* content;
	int lineno = 0;

	while (!f.eof()) {
		std::getline(f, line);
		file.lines.push_back(line);
		if (starts_with(line.c_str(), "#include \"", &content)) {
			inccont = copy_until(content, '"');
			inc.includepath = Path(inccont.c_str());
			inc.lineno = lineno;
			file.includes.push_back(inc);
		}
		lineno++;
	}
	f.close();
}

static void update_refs_for_self_move(FileBuffer& file, const char* src,
		const char* dst) {
	Path p1(src), p2(dst);
	for (int i = 0; i < file.includes.size(); i++) {
		Path& incpath = file.includes[i].includepath;
		Path abspath = incpath.absolute_path(p1);
		incpath = p2.relpath_to(abspath);
	}
}
static void update_refs_for_ref_move(FileBuffer& file, const char* thisfile,
		const char** argv, int argc) {
	Path p(thisfile);
	std::vector<std::pair<Path, Path> > pathpairs;
	for (int i = 0; i < argc; i += 2) {
		Path p1(argv[i]), p2(argv[i + 1]);
		pathpairs.push_back(
				std::pair<Path, Path>(p1.absolute_path(p),
						p2.absolute_path(p)));
	}
	for (int i = 0; i < file.includes.size(); i++) {
		Path& incpath = file.includes[i].includepath;
		Path abspath = incpath.absolute_path(p);
		for (int j = 0; j < pathpairs.size(); j++) {
		}
	}
}

static void sort_includes(FileBuffer& file) {
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
		"Usage: <file old> <file new> <ref old> <ref new> ... <ref old> <ref new>\n";
int main(int argc, const char** argv) {
	if (argc < 3 || (argc % 2) != 1) {
		printf(USAGE);
		return 0;
	}

	FileBuffer file;
	const char* src = argv[1], *dst = argv[2];

	consume_file(file, src);

	update_refs_for_self_move(file, src, dst);
	update_refs_for_ref_move(file, dst, argv + 5, argc - 5);

	sort_includes(file);

	update_file(file, dst);

	return 0;
}
