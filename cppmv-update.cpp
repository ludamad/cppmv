//============================================================================
// Usage: <src> <dst> <updated src 1> <updated dst 1> ... etc
//============================================================================

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>

using namespace std;

static bool starts_with(const char* line, const char* prefix,
		const char** content = NULL) {
	int length = strlen(prefix);
	bool hasprefix = strncmp(line, prefix, length) == 0;
	if (hasprefix) {
		if (content) {
			*content = line + length;
		}
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

static void trim_space_at_end(std::string& str) {
	int i = str.length();
	while (i > 0 && isspace(str[i - 1])) {
		i--;
	}
	str.resize(i);
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
		starts_with(p, "./", &p); //skip ./ at start
		while (starts_with(p, "../", &p)) {
			dotdots++;
		}
		while (*p) {
			//Will copy to end for last string
			components.push_back(copy_until(p, '/', &p));
			if (components.back().empty()) {
				components.pop_back();
			}
		}
		trim_space_at_end(components.back());
	}
	bool operator==(const Path& o) const {
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
	std::string restofline;
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

	if (!f) {
		printf("Source file '%s' does not exist!\n", fname);
		exit(-1);
	}

	std::string inccont, line;
	const char* content;
	int lineno = 0;

	while (!f.eof()) {
		std::getline(f, line);
		if (f.eof() && line.empty()) {
			break; // Ensure we do not duplicate newline
		}
		file.lines.push_back(line);
		if (starts_with(line.c_str(), "#include \"", &content)) {
			inccont = copy_until(content, '"', &content);
			inc.restofline = content;
			inc.includepath = Path(inccont.c_str());
			inc.lineno = lineno;
			file.includes.push_back(inc);
		}
		lineno++;
	}
	f.close();
}

static void trimspaces(std::string& arg, int& s, int& e) {
	for (s = 0; s < arg.size() && isspace(arg[s]); s++) {
	}
	for (e = arg.size(); e > s && isspace(arg[e - 1]); e--) {
	}
}

static void update_cmakerefs_for_ref_move(FileBuffer& file,
		const char* thisfile, const char** argv, int argc) {
	Path p(thisfile);
	std::vector<std::pair<Path, Path> > pathpairs;
	for (int i = 0; i < argc; i += 2) {
		Path p1(argv[i]), p2(argv[i + 1]);
		pathpairs.push_back(std::pair<Path, Path>(p1, p2));
	}
	for (int i = 0; i < file.lines.size(); i++) {
		std::string& line = file.lines[i];

		int s, e;
		trimspaces(line, s, e);
		if (s == e) {
			continue;
		}
		Path path(line.c_str() + s);
		for (int j = 0; j < pathpairs.size(); j++) {
//			std::cout << path.to_string() << " vs "
//					<< pathpairs[j].first.to_string() << std::endl;
			if (path == pathpairs[j].first) {
				Path opath = pathpairs[j].second;
				line.replace(line.begin() + s, line.begin() + e,
						opath.to_string());
				break;
			}
		}
	}
}
static void update_refs_for_self_move(FileBuffer& file, const char* src,
		const char* dst) {
	Path p1(src), p2(dst);
	for (int i = 0; i < file.includes.size(); i++) {
		Path& incpath = file.includes[i].includepath;
		std::cout << "-----------------" << std::endl;
		std::cout << "SRC: " << src << std::endl;
		std::cout << "DST: " << p2.to_string() << std::endl;
		std::cout << "OLDINC: " << incpath.to_string() << std::endl;
		Path abspath = incpath.absolute_path(p1);
		incpath = p2.relpath_to(abspath);

		std::cout << "NEWINC: " << incpath.to_string() << std::endl;
		std::cout << "ABS: " << abspath.to_string() << std::endl;
	}
}
static void update_refs_for_ref_move(FileBuffer& file, const char* thisfile,
		const char** argv, int argc) {
	Path p(thisfile);
	std::vector<std::pair<Path, Path> > pathpairs;
	for (int i = 0; i < argc; i += 2) {
		Path p1(argv[i]), p2(argv[i + 1]);
		pathpairs.push_back(std::pair<Path, Path>(p1, p2));
	}
	for (int i = 0; i < file.includes.size(); i++) {
		Path& incpath = file.includes[i].includepath;
		Path abspath = incpath.absolute_path(p);
		for (int j = 0; j < pathpairs.size(); j++) {
//			std::cout << abspath.to_string() << " VS "
//					<< pathpairs[j].first.to_string() << std::endl;
			if (abspath == pathpairs[j].first) {
				incpath = p.relpath_to(pathpairs[j].second);
				break;
			}
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
	if (!f) {
		printf("Error creating destination file %s!\n", fname);
		exit(-1);
	}

	for (int i = 0; i < file.lines.size(); i++) {
		std::string line = file.lines[i];
		for (int z = 0; z < file.includes.size(); z++) {
			if (i == file.includes[z].lineno) {
				line = "#include \"" + file.includes[z].includepath.to_string()
						+ "\"" + file.includes[z].restofline;
				break;
			}
		}
		f.write(line.c_str(), line.size());
		f.write("\n", 1);
	}
	f.close();
}

const char* USAGE =
		"Usage: [--sort --cmake] <file old> <file new> <ref old> <ref new> ... <ref old> <ref new>\n";
int main(int argc, const char** argv) {
	bool sort = false, cmake = false, self_write_only = false;
//	for (int i = 0; i < argc; i++) {
//		printf("Arg %d: %s\n", i, argv[i]);
//	}

	int options;
	for (options = 1; options < argc; options++) {
		const char* arg = argv[options];
		if (!starts_with(arg, "--")) {
			break;
		}
		if (strcmp(arg, "--help") == 0) {
			printf(USAGE);
			return 0;
		} else if (strcmp(arg, "--sort") == 0) {
			sort = true;
		} else if (strcmp(arg, "--cmake") == 0) {
			cmake = true;
		} else if (strcmp(arg, "--self-write-only") == 0) {
			self_write_only = true;
		} else {
			printf("Invalid option '%s', aborting \n", arg);
			exit(1);
		}
	}
	const char** restargv = argv + options;
	int restargc = argc - options;

	if (restargc < 2 || (restargc % 2) != 0) {
		printf(USAGE);
		return 0;
	}

	const char* src = restargv[0], *dst = restargv[1];

	FileBuffer file;
	consume_file(file, src);

	if (cmake) {
		update_cmakerefs_for_ref_move(file, dst, restargv + 2, restargc - 2);
	} else {
		update_refs_for_self_move(file, src, dst);
		update_refs_for_ref_move(file, dst, restargv + 2, restargc - 2);

		if (sort) {
			sort_includes(file);
		}
	}

	update_file(file, self_write_only ? src : dst);

	return 0;
}
