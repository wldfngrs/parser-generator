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
	enum Action {
		SHIFT,
		REDUCE,
		ACCEPT,
		ERROR,
	};

	enum ParseGenLvl {
		TERMINALS,
		PRODUCTIONS,
		CANONICAL_SET,
	};

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

	struct Item {
		size_t position;
		std::vector<std::string> production;
		std::string lookahead;

		Item(size_t pos, std::vector<std::string> prod, std::string la)
			: position{ pos }, production{ prod }, lookahead{ la } {}

		bool operator==(const Item& other) const {
			return position == other.position &&
				production == other.production &&
				lookahead == other.lookahead;
		}
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

	void print_debug_info(ParseGenLvl pg_lvl ) {
		switch (pg_lvl) {
		case TERMINALS:
			// TODO: pretty print terminals in 8-column table.
			std::cout << "Terminals\n=========\n";
			for (auto& terminal : terminals) {
				std::cout << terminal << "\n";
			}
			break;
		case PRODUCTIONS:
			std::cout << "\nProductions\n===========\n";
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
		case CANONICAL_SET:
			std::cout << "\nCanonical Set\n=============\n";
			for (auto& item : canonicalCollection) {
				std::cout << "[" << item.position << ", " << item.production[0] << " ->";
				for (auto i = 1; i < item.production.size(); i++) {
					std::cout << " " << item.production[i];
				}
				std::cout << ", " << item.lookahead << "]\n";
			}
		}
	}

	void closure_function(std::unordered_set<Item, ItemHash>& canonicalCollection) {
		for (auto& item : canonicalCollection) {
			std::string C = item.production[item.position];
			
			// generate [first] for item
			std::unordered_set<std::string> first{item.lookahead};
			for (auto i = item.position + 1; i < item.production.size(); i++) {
				std::string symbol = item.production[i];
				if (terminals.count(symbol) == 1) {
					// if rhs symbol is a terminal
					first.insert(symbol);
				} else if (productions.count(symbol) == 1) {
					// if rhs symbol is a non-terminal
					// check if symbol is in "first" cache
					if (firstCache.count(symbol) == 1) {
						// yes
						first.insert(firstCache[symbol]);
					}
					else {
						// no
						std::vector<std::vector<std::string>> rhs = productions[symbol];

						for (auto& r : rhs) {
							if (terminals.count(r[0]) == 1) {
								first.insert(r[0]);
								firstCache[r[0]] = r[0];
							}
						}
					}
				}
			}

			// Rest of closure algorithm
			std::vector<std::vector<std::string>> rhs = productions[C];

			for (auto& prod : rhs) {
				std::vector<std::string> cItemProd{ C };
				for (auto& p : prod) {
					cItemProd.push_back(p);
				}

				for (auto& b : first) {
					canonicalCollection.emplace(1, cItemProd, b);
				}
			}
		}
	}

	void goto_function() {
	
	}

	// 'string' as key, 'vector of vectors of strings' as value
	// Example key-value pairing:
	// "List" : {{"List", "Pair"}, {"Pair"}}
	// "Pair" : {{"t_lp", "Pair", "t_rp"}, {"t_lp", "t_rp"}}
	std::unordered_map<std::string, std::vector<std::vector<std::string>>> productions;
	std::unordered_set<std::string> terminals;
	std::unordered_map<std::string, std::string> firstCache;

	std::vector<ActionTableInput> actionTable;
	std::vector<GotoTableInput> gotoTable;

	std::string txt;
	bool debug = true;
	std::unordered_set<Item, ItemHash> canonicalCollection;

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
			if (txt[i] == '\n' || i == txt.length() - 1) {
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
							if (value != "") rhs.push_back(value);
							start = k + 1;
						}
						else if (k == line.size() - 1) {
							std::string value = std::string(line, start, k - start + 1);
							if (value != "") rhs.push_back(value);
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
		auto goal = productions.begin();
		for (auto& el : (*goal).second) {			
			std::vector<std::string> beginItemProd;
			beginItemProd.push_back((*goal).first);
			for (auto& e : el) {
				beginItemProd.push_back(e);
			}
			canonicalCollection.emplace(1, beginItemProd, "t_eof");
		}

		if (debug) print_debug_info(CANONICAL_SET);

		closure_function(canonicalCollection);

		if (debug) print_debug_info(CANONICAL_SET);
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