#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <array>
#include <unordered_set>
#include <unordered_map>

// enum Action {
//		SHIFT,
//		REDUCE,
//		ACCEPT,
//		ERROR,
//	};
// 
// struct Action {
//		ActionType type;
//		size_t state;
// };

// std::stack<size_t> states {0};
// std::stack<TokenType> tokens;.
// std::stack<Statement> symbols;

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
	enum ParseGenLvl {
		TERMINALS,
		NON_TERMINALS,
		PRODUCTIONS,
		CANONICAL_SET,
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

			out += item.lookahead;
			return std::hash<std::string>{}(out) ^ std::hash<size_t>{}(item.position);
		}
	};

	struct CanonicalCollectionHash {
		size_t operator()(const std::unordered_set<Item, ItemHash>& c) const {
			size_t out = 1;
			for (auto& item : c) {
				out ^= ItemHash()(item);
			}

			return out;
		}
	};

	struct PairHash {
		size_t operator()(const std::pair<size_t, std::string>& pair) const {
			return std::hash<size_t>{}(pair.first) ^ std::hash<std::string>{}(pair.second);
		}
	};

	void print_debug_info(ParseGenLvl pg_lvl) {
		switch (pg_lvl) {
		case TERMINALS: {
			// pretty print terminals in 8-column table.
			std::cout << "\nExtracted Terminals\n===================\n";
			auto col = 0;
			for (auto& terminal : terminals) {
				std::cout << terminal;
				++col;
				if (!(col % 8)) std::cout << "\n";
				else if (col < terminals.size()) std::cout << ", ";
			}
			std::cout << "\n";
			break;
		}
		case NON_TERMINALS: {
			// pretty print non-terminals in 8-column table.
			std::cout << "\nExtracted Non-Terminals\n=======================\n";
			auto col = 0;
			for (auto& non_terminal : non_terminals) {
				std::cout << non_terminal;
				++col;
				if (!(col % 8)) std::cout << "\n";
				else if (col < non_terminals.size()) std::cout << ", ";
			}
			std::cout << "\n";
			break;
		}
		case PRODUCTIONS:
			std::cout << "\nExtracted Productions\n=====================\n";
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
			std::cout << "\nGenerated Canonical Set\n=======================\n";
			for (auto& canonicalSet_i : canonicalCollection) {
				std::cout << "C_" << canonicalSet_i.second.state << ":\n";
				for (auto& item : canonicalSet_i.first) {
					std::cout << "[" << item.position << ", " << item.production[0] << " ->";
					for (auto i = 1; i < item.production.size(); i++) {
						std::cout << " " << item.production[i];
					}
					std::cout << ", " << item.lookahead << "]\n";
				}
				std::cout << "\n";
			}
		}
	}

	void closure_function(std::unordered_set<Item, ItemHash>& canonicalSet_i) {
		for (auto& item : canonicalSet_i) {
			if (item.position >= item.production.size()) continue;
			std::string C = item.production[item.position];

			// generate [first] for item
			std::unordered_set<std::string> first;
			if ((item.position + 1) < item.production.size()) {
				std::string symbol = item.production[item.position + 1];
				if (terminals.count(symbol) == 1) {
					// if rhs symbol is a terminal
					first.emplace(symbol);
				}
				else if (productions.count(symbol) == 1) {
					// if rhs symbol is a non-terminal
					// check if symbol is in "first" cache
					if (first_cache.count(symbol) == 1) {
						// yes
						first.emplace(first_cache[symbol]);
					}
					else {
						// no
						std::vector<std::vector<std::string>> rhs = productions[symbol];

						for (auto& r : rhs) {
							if (terminals.count(r[0]) == 1) {
								first.emplace(r[0]);
								first_cache[r[0]] = r[0];
							}
						}
					}
				}
			}

			if (first.empty()) {
				first.emplace(item.lookahead);
			}

			// Rest of closure algorithm
			std::vector<std::vector<std::string>> rhs = productions[C];

			for (auto& prod : rhs) {
				std::vector<std::string> cItemProd{ C };
				for (auto& p : prod) {
					cItemProd.push_back(p);
				}

				for (auto& b : first) {
					canonicalSet_i.emplace(1, cItemProd, b);
				}
			}
		}
	}

	void goto_function(const std::unordered_set<Item, ItemHash>& canonicalSet_i, std::string symbol,
						std::unordered_set<Item, ItemHash>& moved) {
		for (auto& item : canonicalSet_i) {
			if (item.position < item.production.size() &&
				item.production[item.position] == symbol)
			{
				auto new_item_position = item.position + 1;
				moved.emplace(new_item_position, item.production, item.lookahead);
			}
		}

		closure_function(moved);
	}

	// 'string' as key, 'vector of vectors of strings' as value
	// Example key-value pairing:
	// "List" : {{"List", "Pair"}, {"Pair"}}
	// "Pair" : {{"t_lp", "Pair", "t_rp"}, {"t_lp", "t_rp"}}
	std::unordered_map<std::string, std::vector<std::vector<std::string>>> productions;
	//std::unordered_map<std::vector<std::vector<std::string>>, std::string> rightmost_deriv;

	std::unordered_set<std::string> terminals;
	std::unordered_set<std::string> non_terminals;
	std::unordered_map<std::string, std::string> first_cache;

	std::string txt;
	bool error = false;
	std::string goal_production_lookahead_symbol;
	std::string goal_lhs_symbol;
	// Key: Set of items
	// Value: marked/unmarked-ness of set, index of set]
	struct CanonicalCollectionValue {
		size_t state;
		bool marked;
	};
	
	std::unordered_map<std::unordered_set<Item, ItemHash>, CanonicalCollectionValue, CanonicalCollectionHash> canonicalCollection;
	
	// std::unordered_map<std::pair<size_t, TokenType>, Action, PairHash> actionTable;
	// std::unordered_map<std::pair<size_t, std::string>, Action, PairHash> actionTable;
	std::unordered_map<std::pair<size_t, std::string>, size_t, PairHash> gotoTable;

public:
	bool debug = true;

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
					// ensure that the token declarations do not contain whitespace.
					auto j = 0;
					while (j++ < line.size()) {
						if (line[j] == ' ' ||
							line[j] == '\t' ||
							line[j] == '\r') {
							std::cout << "Error at [" << l_no << "]: 'token' declarations shouldn't include spaces!\n";
							error = true;
							break;
						}
					}

					// if space is found, don't parse tokens?
					
					if (l_no == 1) goal_production_lookahead_symbol = std::string(line, 0, line.size() - 1);
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
					if (productions.size() == 0) goal_lhs_symbol = lhs;

					non_terminals.emplace(lhs);

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
				else if (line == "\n") {
					l_no++;
				}
				else {
					// error in grammar_txt
					std::cout << "Error at line [" << l_no << "]: Wrongly specified grammar. Review the guide to properly specifying grammar for the Parser-Generator.\n";
					error = true;
				}
			}
		}

		if (debug) {
			print_debug_info(TERMINALS);
			print_debug_info(NON_TERMINALS);
			print_debug_info(PRODUCTIONS);
		}
	}

	void build_cc() {
		if (error) {
			std::cout << "\n\nFatal error in grammar definition. Parser-Generator terminated early.\n";
			return;
		}

		std::unordered_set<Item, ItemHash> canonicalSet_0;
		auto goal = productions[goal_lhs_symbol];
		for (auto& production : goal) {
			std::vector<std::string> beginItemProd;
			beginItemProd.push_back(goal_lhs_symbol);
			for (auto& symbol : production) {
				beginItemProd.push_back(symbol);
			}
			canonicalSet_0.emplace(1, beginItemProd, goal_production_lookahead_symbol);
		}
		
		auto number_of_unmarked_sets = 0;
		size_t set_index = 0;
		closure_function(canonicalSet_0);
		CanonicalCollectionValue ccv{ set_index, false };
		canonicalCollection.emplace(canonicalSet_0, ccv);
		
		// count "unmarked" and keep looping until "unmarked" is equal
		// to zero. That is, all available sets in canonicalCollection have
		// been processed. This is to ensure that sets inserted after the
		// current iterator position get visited in subsequent passes.

		++number_of_unmarked_sets;
		while (number_of_unmarked_sets > 0) {
			for (auto& canonicalSet_i : canonicalCollection) {
				if (canonicalSet_i.second.marked == false) {
					canonicalSet_i.second.marked = true;
					for (auto& item : canonicalSet_i.first) {
						std::unordered_set<Item, ItemHash> new_set;
						if (item.position >= item.production.size()) continue;
						goto_function(canonicalSet_i.first, item.production[item.position], new_set);
						
						if (canonicalCollection.count(new_set) == 0) {
							++set_index;
							ccv = { set_index, false };
							canonicalCollection.emplace(new_set, ccv);
							++number_of_unmarked_sets;
						}
					}
				}
				else {
					--number_of_unmarked_sets;
				}
			}
		}

		if (debug) print_debug_info(CANONICAL_SET);
	}
	
	// TODO: the algorithm for build_cc() seems to suggest that the tables *could* be built
	// within the routine. I'm not sure, and don't have the time or energy to prove this to 
	// be workable. Instead, here's an implementation that seperates the build_cc() routine 
	// from the build_table() routine, first. For my sanity, I just need to feel like I'm 
	// making progress here.
	void build_tables() {
		for (auto& canonicalSet_i : canonicalCollection) {
			for (auto& item : canonicalSet_i.first) {
				;
			}
	
			for (auto& non_term : non_terminals) {
				std::unordered_set<Item, ItemHash> next_set;
				goto_function(canonicalSet_i.first, non_term, next_set);
				if (canonicalCollection.count(next_set)) {
					auto next_state = canonicalCollection[next_set].state;
					// add to goto_table that goto[i, non_term] ==> next_set_index
					gotoTable.emplace(std::make_pair(canonicalSet_i.second.state, non_term), next_state);
				}
			}
		}
	}
};

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cout << "usage: ./parsegen <path/to/grammar>\n";
		exit(1);
	}

	ParserGen parserGen(argv[1]);
	// parserGen.debug = false;
	parserGen.get_terminals_and_productions();
	parserGen.build_cc();
	parserGen.build_tables();
}