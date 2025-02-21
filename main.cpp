#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <array>
#include <unordered_set>
#include <unordered_map>

// std::stack<size_t> states {0};
// std::stack<TokenType> tokens;.
// std::stack<Statement> symbols;

// Interpret 'next' differently. 
// SHIFT action interprets 'next' variable as holding the
// next state to move to
// REDUCE action interprets 'next' as holding the grammar rule
// the reduction will be following.

static bool is_alpha(char c) {
	return (c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z');
}

static bool is_whitespace(char c) {
	return (c == ' ' || c == '\r' || c == '\t');
}

class ParserGen {
	enum ParseGenLvl {
		TERMINALS,
		NON_TERMINALS,
		PRODUCTIONS,
		CANONICAL_SET,
		ACTION_TABLE,
		GOTO_TABLE
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

	struct Terminal {
		std::string str;
		std::string associativity; // l (left-associative), r (right-associative), n (non-associative)
		int precedence;

		Terminal(std::string t_term, size_t prec, std::string assoc) :
			str(t_term), precedence(prec), associativity(assoc) {}

		bool operator==(const Terminal& other) const {
			return str == other.str;
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

	struct TerminalHash {
		size_t operator()(const Terminal& terminal) const {
			return std::hash<std::string>{}(terminal.str);
		}
	};

	enum ActionType {
		SHIFT,
		REDUCE,
		ACCEPT,
		ERROR,
	};

	struct Action {
		ActionType type;
		size_t state;
	};

	void print_debug_info(ParseGenLvl pg_lvl) {
		switch (pg_lvl) {
		case TERMINALS: {
			// pretty print terminals in 8-column table.
			std::cout << "Extracted Terminals\n===================\n";
			auto col = 0;
			for (auto& terminal : terminals) {
				std::cout << "[" << terminal.str << ", " << terminal.precedence << ", " << terminal.associativity << "]";
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

				std::vector<std::string> values = production.second;
				for (auto& value : values) {
					std::cout << "{ " << value << " } ";
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
			break;
		case ACTION_TABLE:
			std::cout << "\nAction Table\n============\n";
			for (auto& entry : actionTable) {
				std::cout << "[" << entry.first.first << ", " << entry.first.second << "] > ";
				switch (entry.second.type) {
				case SHIFT:
					std::cout << "SHIFT " << entry.second.state << "\n";
					break;
				case REDUCE:
					std::cout << "REDUCE " << entry.second.state << "\n";
					break;
				case ACCEPT:
					std::cout << "ACCEPT\n";
					break;
				}
			}
			break;
		case GOTO_TABLE:
			std::cout << "\nGoto Table\n==========\n";
			for (auto& entry : gotoTable) {
				std::cout << "[" << entry.first.first << ", " << entry.first.second << "] > " << entry.second << "\n";
			}
			break;
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
				if (terminals.count(Terminal(symbol, 0, "n")) == 1) {
					// if rhs symbol is a terminal
					first.insert(symbol);
				}
				else if (productions.count(symbol) == 1) {
					// if rhs symbol is a non-terminal
					// check if symbol is in "first" cache
					if (first_cache.count(symbol) == 1) {
						// yes
						first.insert(first_cache[symbol]);
					}
					else {
						// no
						std::vector<std::string> rhs = productions[symbol];

						for (auto& r : rhs) {
							for (auto i = 0; i < r.size(); i++) {
								if (r[i] == ' ') {
									std::string temp = std::string(r, 0, i);
									if (terminals.count(Terminal(temp, 0, "n")) == 1) {
										first.emplace(temp);
										first_cache[temp] = temp;
										break;
									}
								}
							}
						}
					}
				}
			}

			if (first.empty()) {
				first.insert(item.lookahead);
			}

			// Rest of closure algorithm
			std::vector<std::string> rhs = productions[C];

			for (auto& prod : rhs) {
				std::vector<std::string> cItemProd{ C };

				auto start = 0;
				auto i = 0;
				for (; i < prod.size(); i++) {
					if (prod[i] == ' ') {
						cItemProd.push_back(std::string(prod, start, i - start));
						start = i + 1;
					}
				}
				cItemProd.push_back(std::string(prod, start, i - start));

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

	// 'string' as key, 'vector of strings' as value
	// Example key-value pairing:
	// "List" : {{"List Pair"}, {"Pair"}}
	// "Pair" : {{"t_lp Pair t_rp"}, {"t_lp t_rp"}}
	std::unordered_map<std::string, std::vector<std::string>> productions;
	//std::unordered_map<std::vector<std::vector<std::string>>, std::string> rightmost_deriv;

	std::unordered_set<Terminal, TerminalHash> terminals;
	std::unordered_set<std::string> non_terminals;
	std::unordered_map<std::string, std::string> first_cache;

	std::string grammar_txt;
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
	std::unordered_map<std::pair<size_t, std::string>, Action, PairHash> actionTable;
	std::unordered_map<std::pair<size_t, std::string>, size_t, PairHash> gotoTable;

public:
	bool debug = true;
	bool error_in_get_terminals_and_productions = false;

	ParserGen(std::string pathToGrammar) {
		std::stringstream ss;
		std::ifstream file(pathToGrammar, std::ios_base::in);

		ss << file.rdbuf();
		grammar_txt = ss.str();
	}

	void get_terminals_and_productions() {
		// TODO: stop parsing if error count exceeds a certain amount.
		// TODO: don't attempt parsing productions if error is found while parsing terminals.
		auto start_txt = 0;
		std::string line;
		size_t l_no = 1; // solely for error-reporting
		bool parsing_terminals = true;
		bool parsing_productions = false;

		for (size_t i = 0; i < grammar_txt.length(); i++) {
			if (grammar_txt[i] == '\n' || i == grammar_txt.length() - 1) {
				if (i == grammar_txt.length() - 1) line = std::string(grammar_txt, start_txt, i - start_txt + 1);
				else line = std::string(grammar_txt, start_txt, i - start_txt);
				start_txt = i + 1;

				if (parsing_terminals) {
					if (line[0] == 't' && line[1] == '_') {
						std::vector<std::string> term_info;

						auto j = 2;
						auto start = 0;

						if (j >= line.size()) {
							error_in_get_terminals_and_productions = true;
							std::cout << "Error: Incomplete terminal symbol declaration.\n"
										<< "[Line " << l_no << "]: " << line << "\n\n";
							continue;
						}

						while (j++ < line.size()) {
							if (is_whitespace(line[j])) {
								std::string temp = std::string(line, start, line.size() - start);
								if (temp != "") term_info.emplace_back(line, start, line.size() - start);
								start = j + 1;
							}

							if (term_info.size() > 2) break;
						}

						std::string temp = std::string(line, start, line.size() - start);
						if (temp != "") term_info.emplace_back(line, start, line.size() - start);
						
						if (term_info.size() == 1) {
							terminals.emplace(term_info[0], 0, "n");
						}
						else if (term_info.size() == 2) {
							if (term_info[1] == "n" || term_info[1] == "l" || term_info[1] == "r") {
								terminals.emplace(term_info[0], 0, term_info[1]);
							}
							terminals.emplace(term_info[0], std::stoi(term_info[1]), "n");
						}
						else if (term_info.size() == 3) {
							terminals.emplace(term_info[0], std::stoi(term_info[1]), term_info[2]);
						}
						else {
							error_in_get_terminals_and_productions = true;
							std::cout << "Error: Terminal symbol declaration information should not contain more than three fields (or two whitespace characters)! \n"
										<< "[Line " << l_no << "]: " << line << "\n\n";
							continue;
						}

						if (l_no == 1) goal_production_lookahead_symbol = term_info[0];
					}
					else if (line == "") {
						parsing_terminals = false;
						parsing_productions = true;
					}
					else {
						error_in_get_terminals_and_productions = true;
						std::cout << "Error: Terminal symbols must be declared with a 't_' prefix.\n"
							<< "[Line " << l_no++ << "]: " << line << "\n\n";
					}
				}
				else if (parsing_productions) {
					if (line[0] == 't' && line[1] == '_') {
						error_in_get_terminals_and_productions = true;
						std::cout << "Error: Terminal symbols ('t_' prefix) cannot be on the LHS of a production rule\n"
							<< "[Line " << l_no++ << "]: " << line << "\n\n";
						continue;
					}
					else if (line == "") {
						l_no++;
						continue;
					}

					size_t start = 0;
					size_t j = 0;

					while (!is_whitespace(line[j])) { j++; }

					// valid lhs of production has been scanned
					std::string lhs = std::string(line, start, j);
					if (productions.size() == 0) goal_lhs_symbol = lhs;
					non_terminals.emplace(lhs);

					start = j;  j += 3;
					std::string delim = std::string(line, start, j - start);

					if (delim != " > ") {
						error_in_get_terminals_and_productions = true;
						std::cout << "Error: Expected ' > ' delimiter between LHS and RHS of production rule\n"
							<< "[Line " << l_no++ << "]: " << line << "\n\n";
						continue;
					}

					start = j;
					// ensure that only a single whitespace between consecutive symbols
					auto whitespace_count = 0;
					for (; j < line.size(); j++) {
						if (is_whitespace(line[j])) {
							whitespace_count++;
							if (whitespace_count > 1) {
								error_in_get_terminals_and_productions = true;
								std::cout << "Error: Grammar production rhs should not contain more than one whitespace between grammar symbols.\n"
									<< "[Line " << l_no << "]: " << line << "\n\n";
								break;
							}
						}
						else {
							whitespace_count = 0;
						}
					}
					
					if (whitespace_count == 0) {
						std::string rhs(line, start, line.size() - start);
						productions[lhs].push_back(rhs);
					}
				}
				l_no++;
			}
		}

		if (debug && !error_in_get_terminals_and_productions) {
			print_debug_info(TERMINALS);
			print_debug_info(NON_TERMINALS);
			print_debug_info(PRODUCTIONS);
		}
	}

	void build_cc() {
		std::unordered_set<Item, ItemHash> canonicalSet_0;
		auto& goal = productions[goal_lhs_symbol];
		for (auto& production : goal) {
			std::vector<std::string> beginItemProd;
			beginItemProd.push_back(goal_lhs_symbol);
			beginItemProd.push_back(production);
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
	// within the routine (i.e while building the canonicalCollection itself). However, 
	// I'm not sure, and don't have the time or energy to prove this to be a workable solution.
	// Instead, here's an implementation that seperates the build_cc() routine from the build_table()
	// routine. For my sanity. I just need to feel like I'm making progress here.
	void build_tables() {
		for (auto& canonicalSet_i : canonicalCollection) {
			for (auto& item : canonicalSet_i.first) {
				// Checking the condition for the SHIFT action first, ensures that in the possible case 
				// of a base SHIFT-REDUCE conflict the parser-generator favors the SHIFT action.
				if (item.position < item.production.size() &&
					terminals.count(Terminal(item.production[item.position], 0, "n")))
				{
					// for SHIFT-REDUCE conflicts that require precedence and associativity rules for resolution
					std::unordered_set<Item, ItemHash> next_set;
					std::string terminal = item.production[item.position];
					goto_function(canonicalSet_i.first, terminal, next_set);
					if (canonicalCollection.count(next_set)) {
						auto next_state = canonicalCollection[next_set].state;
						// add to actionTable that action[i, item.production[item.position]] ==> shift (to) next_state
						Action action{ SHIFT, next_state };
						actionTable.emplace(std::make_pair(canonicalSet_i.second.state, terminal), action);
					}
				}
				else if (item.position >= item.production.size()) {
					if (item.lookahead == goal_production_lookahead_symbol) {
						if (item.production[0] == goal_lhs_symbol) {
							// recall; for an accept action, the second 'state' member is irrelevant.
							// I'd just use a 0 for literally no particular reason
							Action action{ ACCEPT, 0 };
							actionTable.emplace(std::make_pair(canonicalSet_i.second.state, goal_production_lookahead_symbol), action);
						}
						else {
							// add to actionTable that action[i, item.lookahead] ==> reduce (by) grammar_rule
							auto grammar_rule_no = 0;
							Action action{ REDUCE, grammar_rule_no };
							actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
						}
					}
					else {
						// auto grammar_rule_no = non_terminals[item.production[item.position - 1]];
						// add to actionTable that action[i, item.lookahead] ==> reduce (by) grammar_rule
						auto grammar_rule_no = 0;
						Action action{ REDUCE, grammar_rule_no };
						actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
					}
				}
			}
	
			for (auto& non_term : non_terminals) {
				std::unordered_set<Item, ItemHash> next_set;
				goto_function(canonicalSet_i.first, non_term, next_set);
				if (canonicalCollection.count(next_set)) {
					auto next_state = canonicalCollection[next_set].state;
					// add to gotoTable that goto[i, non_term] ==> next_set_index
					gotoTable.emplace(std::make_pair(canonicalSet_i.second.state, non_term), next_state);
				}
			}
		}

		if (debug) {
			print_debug_info(ACTION_TABLE);
			print_debug_info(GOTO_TABLE);
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

	if (parserGen.error_in_get_terminals_and_productions) {
		std::cout << "Fatal error in grammar definition. Parser-Generator terminated early.\n";
		return -1;
	}

	parserGen.build_cc();
	parserGen.build_tables();
}