#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <array>
#include <unordered_set>
#include <unordered_map>

// std::map<std::pair<std::string, Action>, size_t> actionTable;
// std::map<std::pair<size_t, std::string>, size_t> gotoTable;

// Interpret 'next' differently. 
// SHIFT action interprets 'next' variable as holding the
// next state to move to
// REDUCE action interprets 'next' as holding the grammar rule
// the reduction will be following.

static bool isAlpha(char c) {
	return (c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z');
}

class ParserGen {
	std::string txt;

	enum Action {
		SHIFT,
		REDUCE,
		ACCEPT,
		ERROR,
	};

	enum ParseGenLvl {
		TERMINALS,
		PRODUCTIONS,
	};

	bool debug = true;

	struct ActionTableInput {
		// First string: Terminal,
		// Second string: Action,
		std::array<std::string, 2> handle;
		size_t next;
	};

	struct GotoTableInput {
		std::string nonTerminal;
		size_t next;
	};

	//struct Terminal {
	//	std::string literal;
	//	size_t line;
	//
	//	Terminal(std::string literal, size_t line)
	//		: literal{ literal }, line{ line } {}
	//
	//	bool operator==(const Terminal& other) const {
	//		return literal == other.literal;
	//	}
	//};

	// 'string' as key, 'vector of vectors of strings' as value
	// Example key-value pairing:
	// "List" : {{"List", "Pair"}, {"Pair"}}
	// "Pair" : {{"t_lp", "Pair", "t_rp"}, {"t_lp", "t_rp"}}
	std::unordered_map<std::string, std::vector<std::vector<std::string>>> productions;
	std::unordered_set<std::string> terminals;

	std::vector<ActionTableInput> actionTable;
	std::vector<GotoTableInput> gotoTable;

	struct Item {
		size_t position;
		std::vector<std::string> production;
		std::string lookahead;
	};

	struct ItemHash {
		size_t operator()(const Item& item) const {
			std::string out;
			for (auto p : item.production) {
				out += p;
			}

			return std::hash<std::string>{}(out) ^ std::hash<size_t>{}(item.position);
		}
	};

	std::unordered_set<Item, ItemHash> canonicalCollection;

	void print_debug_info(ParseGenLvl pg_lvl ) {
		switch (pg_lvl) {
		case TERMINALS:
			// TODO: pretty print terminals in 8-column table.
			std::cout << "Terminals\n" << "=========\n";
			for (auto& terminal : terminals) {
				std::cout << terminal << "\n";
			}
			break;
		case PRODUCTIONS:
			std::cout << "\nProductions\n" << "===========\n";
			for (auto& production : productions) {
				std::cout << production.first << " > ";

				std::vector<std::vector<std::string>> values = production.second;
				for (auto& value : values) {
					std::cout << "{ ";
					for (auto& v : value) {
						std::cout << v << " ";
					}
					std::cout << "} ";
				}

				std::cout << "\n";
			}
			break;
		}
	}

	std::unordered_set<Item> closure_function(std::vector<Item> canonicalCollection) {
		
	}

	void goto_function() {
	
	}

public:
	ParserGen(std::string pathToGrammar) {
		std::stringstream ss;
		std::ifstream file(pathToGrammar, std::ios_base::in);

		ss << file.rdbuf();
		txt = ss.str();
	}

	void get_terminals_and_productions() {
		auto start_txt = 0;
		std::string line;
		size_t l_no = 1; // solely for error-reporting
		for (size_t i = 0; i < txt.length(); i++) {
			if (txt[i] == '\n') {
				line = std::string(txt, start_txt, i - start_txt + 1);
				start_txt = i + 1;

				// if terminal
				if (line[0] == 't' && line[1] == '_') {
					// TODO: ensure that the line contains no spaces.
					terminals.emplace(std::string(line, 0, line.size() - 1));
				}
				else if (isAlpha(line[0])) {
					// if non-terminal. That is, production
					auto start = 0;
					auto j = 0;

					while ((line[j] != ' ' || line[j] != '\t') &&
						isAlpha(line[j])) {
						j++;
					}

					// lhs of production has been scanned
					std::string lhs = std::string(line, start, j++);

					// scan until the first letter, representing the rhs of a production.
					while (!isAlpha(line[j])) {
						j++;
					}

					start = j;
					std::vector<std::string> rhs;
					for (auto k = j; k < line.size(); k++) {

						if (line[k] == ' ' ||
							line[k] == '\t' ||
							line[k] == '\n')
						{
							std::string value = std::string(line, start, k - start);
							rhs.push_back(value);
							start = k + 1;
						}
					}

					productions[lhs].push_back(rhs);
				}
				else {
					// error in grammar_txt
				}
				l_no++;
			}
		}

		if (debug) {
			print_debug_info(TERMINALS);
			print_debug_info(PRODUCTIONS);
		}
	}

	void generate_cc() {
		Item begin{ 1, {"Goal", "List"}, "t_eof" };
	}
};

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cout << "usage: ./parsegen <path/to/grammar>\n";
		exit(1);
	}

	ParserGen parserGen(argv[1]);
	parserGen.get_terminals_and_productions();
	parserGen.generate_cc();
	//parserGen.generateCC();
}