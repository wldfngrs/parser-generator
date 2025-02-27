#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <variant>
#include <unordered_set>
#include <unordered_map>

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
		std::vector<std::string_view> production;
		std::string_view lookahead;

		Item(size_t pos, std::vector<std::string_view> prod, std::string_view la)
			: position{ pos }, production{ prod }, lookahead{ la } {}

		bool operator==(const Item& other) const {
			return position == other.position &&
				production == other.production &&
				lookahead == other.lookahead;
		}
	};

	struct Terminal {
		std::string_view str;
		int precedence;
		std::string associativity; // l (left-associative), r (right-associative), n (non-associative)

		Terminal(std::string_view t_term, size_t prec, std::string assoc) :
			str(t_term), precedence(prec), associativity(assoc) {}

		bool operator==(const Terminal& other) const {
			return str == other.str;
		}
	};

	//TODO: Combine all of the hashes into a single hash struct for compile time polymorphism/function overloading

	struct MapHash {
		// PairHash: std::pair<size_t, std::string_view>
		size_t operator()(const std::pair<size_t, std::string_view>& pair) const {
			return std::hash<size_t>{}(pair.first) ^ std::hash<std::string_view>{}(pair.second);
		}

		// PairHash: std::pair<std::string_view, size_t>
		size_t operator()(const std::pair<std::string_view, size_t>& pair) const {
			return std::hash<std::string_view>{}(pair.first) ^ std::hash<size_t>{}(pair.second);
		}

		// TerminalHash
		size_t operator()(const Terminal& terminal) const {
			return std::hash<std::string_view>{}(terminal.str);
		}

		// ItemHash
		size_t operator()(const Item& item) const {
			std::string out;
			for (auto p : item.production) {
				out += p;
			}

			out += item.lookahead;
			return std::hash<std::string>{}(out) ^ std::hash<size_t>{}(item.position);
		}

		// CanonicalCollectionHash
		size_t operator()(const std::unordered_set<Item, MapHash>& c) const {
			size_t out = 1;
			for (auto& item : c) {
				out ^= MapHash()(item);
			}

			return out;
		}
	};

	enum ActionType {
		SHIFT,
		REDUCE,
		ACCEPT,
	};

	struct Action {
		ActionType type;
		size_t value;
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
				if (col == terminals.size()) std::cout << "\n";
				else if (!(col % 8)) std::cout << "\n";
				else if (col < terminals.size()) std::cout << ", ";
			}
			break;
		}
		case NON_TERMINALS: {
			// pretty print non-terminals in 8-column table.
			std::cout << "\nExtracted Non-Terminals\n=======================\n";
			auto col = 0;
			for (auto& non_terminal : non_terminals) {
				std::cout << non_terminal;
				++col;
				if (col == non_terminals.size()) std::cout << "\n";
				else if (!(col % 8)) std::cout << "\n";
				else if (col < non_terminals.size()) std::cout << ", ";
			}
			break;
		}
		case PRODUCTIONS:
			std::cout << "\nExtracted Productions\n=====================\n";
			for (auto& production : productions) {
				std::cout << production.first << " > ";

				std::vector<std::string_view> values = production.second;
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
					std::cout << "SHIFT " << entry.second.value << "\n";
					break;
				case REDUCE:
					std::cout << "REDUCE " << reduce_info[entry.second.value].first << "\n";
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

	void closure_function(std::unordered_set<Item, MapHash>& canonicalSet_i) {
		for (auto& item : canonicalSet_i) {
			if (item.position >= item.production.size()) continue;
			std::string_view C = item.production[item.position];

			// generate [first] for item
			std::unordered_set<std::string_view> item_firsts;

			for (auto i = item.position + 1; i < item.production.size(); i++) {
				std::string_view symbol = item.production[i];
				if (terminals.count(Terminal(symbol, 0, "n"))) {
					item_firsts.insert(symbol);
					break;
				}
				else if (productions.count(symbol)){
					std::unordered_set<std::string_view> initial_terminals = firsts[symbol];
					for (auto& initial_term : initial_terminals) {
						item_firsts.insert(initial_term);
					}

					if (!item_firsts.empty()) {
						break;
					}
				}
			}

			if (item_firsts.empty()) {
				item_firsts.insert(item.lookahead);
			}

			// Rest of closure algorithm
			std::vector<std::string_view> rhs = productions[C];

			for (auto& prod : rhs) {
				std::vector<std::string_view> cItemProd{ C };

				auto start = 0;
				auto i = 0;
				for (; i < prod.size(); i++) {
					if (is_whitespace(prod[i])) {
						cItemProd.push_back(*strings.find(std::string(prod, start, i - start)));
						start = i + 1;
					}
				}
				cItemProd.push_back(*strings.find(std::string(prod, start, i - start)));

				for (auto& b : item_firsts) {
					canonicalSet_i.emplace(1, cItemProd, b);
				}
			}
		}
	}

	void goto_function(const std::unordered_set<Item, MapHash>& canonicalSet_i, std::string_view symbol,
		std::unordered_set<Item, MapHash>& moved) {
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

	void set_last_terminal(Terminal& last_terminal, const std::vector<std::string_view>& item_production) {
		for (auto i = static_cast<int>(item_production.size() - 1); i >= 0; i--) {
			if (terminals.count(Terminal(item_production[i], 0, "n"))) {
				last_terminal.str = terminals.find(Terminal(item_production[i], 0, "n"))->str;
				last_terminal.precedence = terminals.find(Terminal(item_production[i], 0, "n"))->precedence;
				last_terminal.associativity = terminals.find(Terminal(item_production[i], 0, "n"))->associativity;
				break;
			}
		}
	}

	std::unordered_set<std::string> strings;

	// 'string' as key, 'vector of strings' as value
	// Example key-value pairing:
	// "List" : {{"List Pair"}, {"Pair"}}
	// "Pair" : {{"t_lp Pair t_rp"}, {"t_lp t_rp"}}
	std::unordered_map<std::string_view, std::vector<std::string_view>> productions;

	// The terminals that appear first in every possible production
	std::unordered_map<std::string_view, std::unordered_set<std::string_view>> firsts;

	// first: symbol to reduce to, second: number of symbols to pop off the stack
	std::vector<std::pair<std::string_view, size_t>> reduce_info;

	std::unordered_set<Terminal, MapHash> terminals;
	std::unordered_set<std::string_view> non_terminals;

	std::string grammar_txt;
	std::string_view goal_production_lookahead_symbol;
	std::string_view goal_lhs_symbol;

	struct CanonicalCollectionValue {
		size_t state;
		bool marked;
	};

	std::unordered_map<std::unordered_set<Item, MapHash>, CanonicalCollectionValue, MapHash> canonicalCollection;

	std::unordered_map<std::pair<size_t, std::string_view>, Action, MapHash> actionTable;
	std::unordered_map<std::pair<size_t, std::string_view>, size_t, MapHash> gotoTable;

public:
	bool debug = true;
	bool error_in_get_terminals_and_productions = false;
	bool file_access_error = false;

	ParserGen(std::string path_to_grammar) {
		std::stringstream ss;
		std::ifstream file(path_to_grammar, std::ios::in);

		if (!file.is_open()) {
			file_access_error = true;
			return;
		}

		ss << file.rdbuf();
		grammar_txt = ss.str();

		file.close();
	}

	void get_terminals_and_productions() {
		// TODO: don't attempt parsing productions if error is found while parsing terminals.
		auto start_txt = 0;
		std::string line;
		size_t l_no = 1; // solely for error-reporting
		bool parsing_terminals = true;
		bool parsing_productions = false;
		std::unordered_map<std::string, std::string> right_deriv;

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
							strings.insert(term_info[0]);
							terminals.emplace(*strings.find(term_info[0]), 0, "n");
						}
						else if (term_info.size() == 2) {
							strings.insert(term_info[0]);
							if (term_info[1] == "n" || term_info[1] == "l" || term_info[1] == "r") {
								terminals.emplace(*strings.find(term_info[0]), 0, term_info[1]);
							}
							terminals.emplace(*strings.find(term_info[0]), std::stoi(term_info[1]), "n");
						}
						else if (term_info.size() == 3) {
							if (term_info[2] != "n" && term_info[2] != "l" && term_info[2] != "r") {
								error_in_get_terminals_and_productions = true;
								std::cout << "Error: Terminal associativity field can only be one of: 'r' (right-associative), 'l' (left-associative), 'n' (non-associative)\n"
									<< "[Line " << l_no << "]: " << line << "\n\n";
								continue;
							}

							strings.insert(term_info[0]);
							terminals.emplace(*strings.find(term_info[0]), std::stoi(term_info[1]), term_info[2]);
						}
						else {
							error_in_get_terminals_and_productions = true;
							std::cout << "Error: Terminal symbol declaration information should not contain more than three fields (or two whitespace characters)! \n"
								<< "[Line " << l_no << "]: " << line << "\n\n";
							continue;
						}

						if (l_no == 1) goal_production_lookahead_symbol = *strings.find(term_info[0]);
					}
					else if (line == "") {
						parsing_terminals = false;
						if (!error_in_get_terminals_and_productions) parsing_productions = true;
						//parsing_productions = true;
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
					strings.insert(lhs);
					if (productions.size() == 0) goal_lhs_symbol = *strings.find(lhs);
					non_terminals.emplace(*strings.find(lhs));

					start = j;  j += 3;
					std::string delim = std::string(line, start, j - start);

					if (delim != " > ") {
						error_in_get_terminals_and_productions = true;
						std::cout << "Error: Expected ' > ' delimiter between LHS and RHS of production rule\n"
							<< "[Line " << l_no++ << "]: " << line << "\n\n";
						continue;
					}

					auto symbol_start = start = j;
					// ensure that only a single whitespace between consecutive symbols
					auto whitespace_count = 0;
					bool found_first = false;
					for (; j < line.size(); j++) {
						if (is_whitespace(line[j])) {
							// check if lhs symbol after whitespace is a terminal
							if (!found_first && terminals.count(Terminal(std::string(line, symbol_start, j - symbol_start), 0, "n"))) {
								found_first = true;
								firsts[*strings.find(lhs)].insert(*strings.find(std::string(line, symbol_start, j - symbol_start)));
							}
							whitespace_count++;
							if (whitespace_count > 1) {
								error_in_get_terminals_and_productions = true;
								std::cout << "Error: Grammar production rhs should not contain more than one whitespace between grammar symbols.\n"
									<< "[Line " << l_no << "]: " << line << "\n\n";
								break;
							}
							symbol_start = j + 1;
						}
						else {
							whitespace_count = 0;
						}
					}

					// first check if rightmost lhs symbol is a terminal, if found first is still false
					if (!found_first && terminals.count(Terminal(std::string(line, symbol_start, j - symbol_start), 0, "n"))) {
						found_first = true;
						firsts[*strings.find(lhs)].insert(*strings.find(std::string(line, symbol_start, j - symbol_start)));
					}

					// Test if lhs or rhs can come up empty.
					std::string rhs(line, start, line.size() - start);
					if (whitespace_count == 0) {
						if (right_deriv.count(rhs)) {
							error_in_get_terminals_and_productions = true;
							std::cout << "Error: Grammar production has a potential REDUCE-REDUCE conflict.\n"
								<< "[Line " << l_no << "]: " << line << " AND "
								<< right_deriv[rhs] << " > " << rhs << "\n\n";
							continue;
						}

						if (rhs == "") {
							error_in_get_terminals_and_productions = true;
							std::cout << "Error: Grammar production rhs is empty. Ill-defined grammar.\n"
								<< "[Line " << l_no << "]: " << line << "\n\n";
							continue;
						}
						strings.insert(rhs);
						strings.insert(lhs);

						right_deriv[*strings.find(rhs)] = *strings.find(lhs);
						productions[*strings.find(lhs)].push_back(*strings.find(rhs));
					}

					if (is_whitespace(rhs[0])) {
						error_in_get_terminals_and_productions = true;
						std::cout << "Error: Grammar production rhs is empty (cannont start with whitespace). Ill-defined grammar.\n"
							<< "[Line " << l_no << "]: " << line << "\n\n";
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
		std::unordered_set<Item, MapHash> canonicalSet_0;
		auto& goal = productions[goal_lhs_symbol];
		for (auto& production : goal) {
			std::vector<std::string_view> beginItemProd;
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
						std::unordered_set<Item, MapHash> new_set;
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
		// Key [first: symbol to be reduced to, second: number of symbols to be popped off the stack at the reduction] Value [the index of the key in 'reduce_info' vector] 
		std::unordered_map<std::pair<std::string_view, size_t>, size_t, MapHash> reduce_info_map;

		for (auto& canonicalSet_i : canonicalCollection) {
			for (auto& item : canonicalSet_i.first) {
				// Checking the condition for the SHIFT action first, ensures that in the possible case 
				// of a base SHIFT-REDUCE conflict the parser-generator favors the SHIFT action.
				if (item.position < item.production.size() &&
					terminals.count(Terminal(item.production[item.position], 0, "n")))
				{
					std::unordered_set<Item, MapHash> next_set;
					std::string_view terminal = item.production[item.position];
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
							std::string_view lhs = item.production[0];
							// The reduce_info_map is to avoid duplicates in the reduce_info vector. It is defined within this build_tables() 
							// function to limit it's scope.

							// If the to-be-added reduce info does not exist in the reduce_info_map, add this new reduce info to the reduce_info
							// vector, add it as key to the reduce_info_map (with value being the index of the vector the key was added to), then 
							// generate the REDUCE action.
							// Else, find the already existing reduce info, and generate the REDUCE action using it. 

							if (!reduce_info_map.count(std::make_pair(lhs, item.production.size() - 1))) {
								reduce_info.emplace_back(lhs, item.production.size() - 1);
								reduce_info_map[std::make_pair(lhs, item.production.size() - 1)] = reduce_info.size() - 1;
								Action action{ REDUCE, reduce_info.size() - 1 };
								actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
							}
							else {
								Action action{ REDUCE, reduce_info_map.find(std::make_pair(lhs, item.production.size() - 1))->second };
								actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
							}
						}
					}
					else {
						// first handle SHIFT-REDUCE conflicts that require precedence and associativity rules for resolution
						Terminal last_terminal("", 0, "n"); // set a dummy terminal
						set_last_terminal(last_terminal, item.production);

						if (last_terminal.str == "") {
							// production rhs has no terminals (production that derives a production)
							// auto grammar_rule_no = non_terminals[item.production[item.position - 1]];
							// add to actionTable that action[i, item.lookahead] ==> reduce (by) grammar_rule
							std::string_view lhs = item.production[0];
							if (!reduce_info_map.count(std::make_pair(lhs, item.production.size() - 1))) {
								reduce_info.emplace_back(lhs, item.production.size() - 1);
								reduce_info_map[std::make_pair(lhs, item.production.size() - 1)] = reduce_info.size() - 1;
								Action action{ REDUCE, reduce_info.size() - 1 };
								actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
							}
							else {
								Action action{ REDUCE, reduce_info_map.find(std::make_pair(lhs, item.production.size() - 1))->second };
								actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
							}
						}
						else {
							// using the now defined last_terminal variable, resolve SHIFT-REDUCE conflict
							// thanks to defined precedence and associativity rules
							if (last_terminal.precedence > terminals.find(Terminal(item.lookahead, 0, "n"))->precedence) {
								std::string_view lhs = item.production[0];

								if (!reduce_info_map.count(std::make_pair(lhs, item.production.size() - 1))) {
									reduce_info.emplace_back(lhs, item.production.size() - 1);
									reduce_info_map[std::make_pair(lhs, item.production.size() - 1)] = reduce_info.size() - 1;
									Action action{ REDUCE, reduce_info.size() - 1 };
									actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
								}
								else {
									Action action{ REDUCE, reduce_info_map.find(std::make_pair(lhs, item.production.size() - 1))->second };
									actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
								}
							}
							else if ((last_terminal.precedence == terminals.find(Terminal(item.lookahead, 0, "n"))->precedence) &&
								(last_terminal.associativity == "l" || last_terminal.associativity == "n"))
							{
								std::string_view lhs = item.production[0];

								if (!reduce_info_map.count(std::make_pair(lhs, item.production.size() - 1))) {
									reduce_info.emplace_back(lhs, item.production.size() - 1);
									reduce_info_map[std::make_pair(lhs, item.production.size() - 1)] = reduce_info.size() - 1;
									Action action{ REDUCE, reduce_info.size() - 1 };
									actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
								}
								else {
									Action action{ REDUCE, reduce_info_map.find(std::make_pair(lhs, item.production.size() - 1))->second };
									actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
								}
							}
							else {
								std::unordered_set<Item, MapHash> next_set;
								goto_function(canonicalSet_i.first, item.lookahead, next_set);
								if (canonicalCollection.count(next_set)) {
									auto next_state = canonicalCollection[next_set].state;
									// add to actionTable that action[i, item.production[item.position]] ==> shift (to) next_state
									Action action{ SHIFT, next_state };
									actionTable.emplace(std::make_pair(canonicalSet_i.second.state, item.lookahead), action);
								}
							}
						}
					}
				}
			}

			for (auto& non_term : non_terminals) {
				std::unordered_set<Item, MapHash> next_set;
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

	void build_output_file() {
		std::ofstream file("output.h", std::ios::out);
		file << "#include <unordered_map>\n"
			"#include <unordered_set>\n"
			"#include <string>"
			"#include <vector>\n\n"
// start define strings cache
			"std::unordered_set<std::string> strings {\n\t";
		auto col = 0;
		auto total = non_terminals.size();
		for (auto& non_term : non_terminals) {
			file << "\"" << non_term << "\"";
			++col;
			if (col == total) file << "\n};\n\n";
			else if (!(col % 8)) file << ",\n\t";
			else if (col < total) file << ", ";
		}
// end define strings cache

// start define terminals enum
		col = 0;
		total = terminals.size();
		file << "enum Token {\n\t";
		for (auto& term : terminals) {
			file << term.str;
			++col;
			if (col == total) file << "\n};\n\n";
			if (!(col % 8)) file << ",\n\t";
			else if (col < total) file << ", ";
		}
// end define terminals enum

// start define reduce_info vector
		col = 0;
		total = reduce_info.size();
		file << "std::vector<std::pair<std::string_view, size_t>> reduce_info {\n\t";
		for (auto& info : reduce_info) {
			file << "{ *strings.find(\"" << info.first << "\"), " << info.second << " }";
			++col;
			if (col == total) file << "\n};\n\n";
			else if (!(col % 2)) file << ",\n\t";
			else if (col < total) file << ", ";
		}
// end define reduce_info vector

// start define PairHash for hashing the key to the actionTable unordered_map, and gotoTable unordered_map
		file << "struct PairHash {\n"
				"\tsize_t operator()(const std::pair<size_t, std::string_view>& pair) const {\n"
				"\t\treturn std::hash<size_t>{}(pair.first) ^ std::hash<std::string_view>{}(pair.second);\n"
				"\t}\n\n"
				"\tsize_t operator()(const std::pair<size_t, Token>& pair) const {\n"
				"\t\treturn std::hash<size_t>{}(pair.first) ^ pair.second;\n"
				"\t}\n"
				"};\n\n";
// end define PairHash

// start define ActionType enum, and Action struct
		file << "enum ActionType {\n"
				"\tSHIFT,\n"
				"\tREDUCE,\n"
				"\tACCEPT\n"
				"};\n\n"
				"struct Action {\n"
				"\tActionType type;\n"
				"\tsize_t value;\n"
				"};\n\n"
// end define ActionType enum, and Action struct

// start actionTable definition
				"std::unordered_map<std::pair<size_t, Token>, Action, PairHash> actionTable {\n\t";
		col = 0;
		total = actionTable.size();
		for (auto& entry : actionTable) {
			file << "{{ " << entry.first.first << ", " << entry.first.second << " }, {";
			switch (entry.second.type) {
			case SHIFT:
				file << "SHIFT, ";
				break;
			case REDUCE:
				file << "REDUCE, ";
				break;
			case ACCEPT:
				file << "ACCEPT, ";
				break;
			}
			file << entry.second.value << " }";
			++col;
			if (col == total) file << "}\n};\n\n";
			else if (!(col % 2)) file << "},\n\t";
			else if (col < total) file << "}, ";
		}
// end actionTable definition

// start gotoTable definition
		file << "std::unordered_map<std::pair<size_t, std::string_view>, size_t, PairHash> gotoTable {\n\t";
		col = 0;
		total = gotoTable.size();
		for (auto& entry : gotoTable) {
			file << "{{ " << entry.first.first << ", *strings.find(\"" << entry.first.second << "\") }, {" << entry.second << "}";
			++col;
			if (col == total) file << "}\n};";
			else if (!(col % 2)) file << "},\n\t";
			else if (col < total) file << "}, ";
		}
// end gotoTabe definition
	}
};

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cout << "usage: ./parsegen <path/to/grammar>\n";
		exit(1);
	}

	ParserGen parserGen(argv[1]);

	if (parserGen.file_access_error) {
		std::cout << "Unable to open " << argv[1] << " file\n";
		return -1;
	}

	parserGen.get_terminals_and_productions();

	if (parserGen.error_in_get_terminals_and_productions) {
		std::cout << "Fatal error in grammar definition. Parser-Generator terminated early.\n";
		return -1;
	}

	parserGen.build_cc();
	parserGen.build_tables();
	//parserGen.build_output_file();
}