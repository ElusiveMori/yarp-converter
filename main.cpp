#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost;

const string ahk_header = R"ahk(
#NoEnv
; #Warn
SendMode Input
SetWorkingDir %A_ScriptDir%
#IfWinActive ahk_class Warcraft III

Msg(str)
{
	Send, {Enter}
	Send, %str%
	Send, {Enter}
	Sleep, 25
}

^q::
	Msg("_cq")
)ahk";

using match_iterator = regex_iterator<vector<char>::iterator>;

struct command {
	string type;
	string args;
};

void process_file(filesystem::path& path) {
	filesystem::ifstream in(path, ios::in | ios::binary | ios::ate);
	filesystem::ofstream out(path.replace_extension(".ahk"));
	ifstream::pos_type size = in.tellg();
	in.seekg(0, ios::beg);

	vector<char> bytes(size);
	in.read(bytes.data(), size);
	in.close();

	out.write(ahk_header.c_str(), ahk_header.size());
	// matches the contents of #[[ ]]# brackets within save files
	regex line_pattern(R"regex(#\[\[([-\w\|\.:]+)\]\]#)regex");
	regex extractor_pattern(R"regex(([\w]+)::([-\w\|\.]+))regex");
	regex pipe_replacer(R"regex(\|)regex");

	vector<command> commands;
	string x = "0";
	string y = "0";

	for (auto it = match_iterator(bytes.begin(), bytes.end(), line_pattern); it != match_iterator(); ++it) {
		string entry = (*it)[1];
		smatch info;

		if (regex_match(entry, info, extractor_pattern)) {
			string type = info[1];
			string args = regex_replace(info[2].str(), pipe_replacer, " ");

			if (type == "ox") { x = args; continue; };
			if (type == "oy") { y = args; continue; };

			commands.push_back(command{ type, args });
		}
	}

	out << "\n\tMsg(\"_so " + x + " " + y + "\")";

	for (auto& cmd : commands) {
		out << "\n\tMsg(\"";

		if (cmd.type == "tile") {
			out << "_at ";
		} else if (cmd.type == "deform") {
			out << "_ad ";
		} else if (cmd.type == "unit") {
			out << "_au ";
		}

		out << cmd.args << "\")";
	}

	out << "\n\tMsg(\"_sp\")\nReturn\n";
	out.close();
}

int main() {
	for (auto& entry : filesystem::directory_iterator(filesystem::current_path())) {
		if (filesystem::is_regular_file(entry)) {
			auto path = entry.path();
			if (path.extension() == ".txt") {
				process_file(path);
			}
		}
	}
}