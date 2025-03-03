#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string_view>
#include <map>
#include <vector>
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
		int production_precedence;

		Item(size_t pos, std::vector<std::string_view> prod, std::string_view la, int prod_prec)
			: position{ pos }, production{ prod }, lookahead{ la }, production_precedence{ prod_prec } {}

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

		Terminal(std::string_view t_term, int prec, std::string assoc) :
			str(t_term), precedence(prec), associativity(assoc) {}

		bool operator==(const Terminal& other) const {
			return str == other.str;
		}
	};

	//TODO: Combine all of the hashes into a single hash struct for compile time polymorphism/function overloading

	struct CustomHash {
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
		size_t operator()(const std::unordered_set<Item, CustomHash>& c) const {
			size_t out = 1;
			for (auto& item : c) {
				out ^= CustomHash()(item);
			}
			
			return out;
		}
	};

	std::vector<size_t> hashes;

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
				else if (!(col % 3)) std::cout << "\n";
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
				else if (!(col % 3)) std::cout << "\n";
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

	void closure_function(std::unordered_set<Item, CustomHash>& canonicalSet_i) {
		auto recorded_size = static_cast<size_t>(0);
		std::unordered_set<Item, CustomHash> processed_items;
		while (canonicalSet_i.size() > recorded_size) {
			recorded_size = canonicalSet_i.size();
			for (auto& item : canonicalSet_i) {
				// generate [first] for item
				if (processed_items.count(item)) continue;
				else processed_items.insert(item);
				
				if (item.position >= item.production.size()) continue;
				std::string_view C = item.production[item.position];

				std::unordered_set<std::string_view> item_firsts;

				for (auto i = item.position + 1; i < item.production.size(); i++) {
					std::string_view symbol = item.production[i];
					if (terminals.count(Terminal(symbol, 0, "n"))) {
						item_firsts.insert(symbol);
						break;
					}
					else if (productions.count(symbol)) {
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

				std::vector<std::string_view> rhs = productions[C];

				for (auto& prod : rhs) {
					std::vector<std::string_view> cItemProd{ C };

					auto start = 0;
					auto i = 0;
					std::string symbol;
					for (; i < prod.size(); i++) {
						if (is_whitespace(prod[i])) {
							symbol = std::string(prod, start, i - start);
							if (symbol != "") {
								cItemProd.push_back(*strings.find(symbol));
							}
							start = i + 1;
						}
					}
					
					symbol = std::string(prod, start, i - start);
					if (symbol != "") cItemProd.push_back(*strings.find(symbol));

					for (auto& b : item_firsts) {
						canonicalSet_i.emplace(1, cItemProd, b, production_precedence[prod]);
					}
				}
			}
		}
	}

	void goto_function(const std::unordered_set<Item, CustomHash>& canonicalSet_i, std::string_view symbol,
		std::unordered_set<Item, CustomHash>& moved) {
		for (auto& item : canonicalSet_i) {
			if (item.position < item.production.size() &&
				item.production[item.position] == symbol)
			{
				auto new_item_position = item.position + 1;
				moved.emplace(new_item_position, item.production, item.lookahead, item.production_precedence);
			}

			if (!reduce_info_map.count(std::make_pair(item.production[0], item.production.size() - 1))) {
				reduce_info.emplace_back(item.production[0], item.production.size() - 1);
				reduce_info_map[std::make_pair(item.production[0], item.production.size() - 1)] = reduce_info.size() - 1;
			}
		}

		closure_function(moved);
	}

	void set_last_terminal(int& last_terminal_precedence, std::string& last_terminal_associativity, const std::vector<std::string_view>& item_production) {
		for (auto i = static_cast<int>(item_production.size() - 1); i >= 0; i--) {
			if (terminals.count(Terminal(item_production[i], 0, "n"))) {
				last_terminal_precedence = terminals.find(Terminal(item_production[i], 0, "n"))->precedence;
				last_terminal_associativity = terminals.find(Terminal(item_production[i], 0, "n"))->associativity;
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

	// All production rhs have 0 precedence as default. If explicitly set, however, the precedence is stored as stated.
	std::unordered_map<std::string_view, int> production_precedence;

	// The terminals that appear first in every possible production
	std::unordered_map<std::string_view, std::unordered_set<std::string_view>> firsts;

	// first: symbol to reduce to, second: number of symbols to pop off the stack
	std::vector<std::pair<std::string_view, size_t>> reduce_info;
	
	// first: symbol to reduce_info information, second: index into reduce_info array containing this information
	std::unordered_map<std::pair<std::string_view, size_t>, size_t, CustomHash> reduce_info_map;

	std::unordered_set<Terminal, CustomHash> terminals;
	std::unordered_set<std::string_view> non_terminals;

	std::string grammar_txt;

	std::string_view goal_production_lookahead_symbol;
	std::string_view goal_lhs_symbol;

	struct CanonicalCollectionValue {
		size_t state;
		bool marked;
	};

	std::unordered_map<std::unordered_set<Item, CustomHash>, CanonicalCollectionValue, CustomHash> canonicalCollection;

	std::unordered_map<std::pair<size_t, std::string_view>, Action, CustomHash> actionTable;
	std::unordered_map<std::pair<size_t, std::string_view>, size_t, CustomHash> gotoTable;

public:
	bool debug = true;
	bool file_access_error = false;
	
	// Defaults to output.h in the parser-generator directory if a path is not provided by the user
	std::string output_file_path{ "output.h" };

	ParserGen(int cmd_args_count, char** cmd_args) {
		std::stringstream ss;
		std::ifstream file(cmd_args[1], std::ios::in);

		if (cmd_args_count == 3) output_file_path = cmd_args[2];

		if (!file.is_open()) {
			file_access_error = true;
			return;
		}

		ss << file.rdbuf();
		grammar_txt = ss.str();

		file.close();
	}

	bool get_terminals_and_productions() {
		auto start_txt = static_cast<size_t>(0);
		std::string line;
		size_t l_no = 1; // solely for error-reporting
		bool parsing_terminals = true;
		bool parsing_productions = false;
		bool error_in_get_terminals_and_productions = false;
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

						if (j == line.size()) {
							error_in_get_terminals_and_productions = true;
							std::cout << "\nError: Incomplete terminal symbol declaration.\n"
								<< "[Line " << l_no << "]: " << line << "\n";
							continue;
						}

						for (; j < line.size(); j++) {
							if (is_whitespace(line[j])) {
								std::string temp = std::string(line, start, j - start);
								if (temp != "") term_info.emplace_back(line, start, j - start);
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

							int prec = 0;
							std::string associativity = "n";
							try {
								prec = std::stoi(term_info[1]);
							}
							catch (std::invalid_argument const& ex) {
								if (term_info[1] == "n" || term_info[1] == "l" || term_info[1] == "r") {
									//terminals.emplace(*strings.find(term_info[0]), 0, term_info[1]);
									associativity = term_info[1];
								} else {
									error_in_get_terminals_and_productions = true;
									std::cout << (is_alpha(term_info[1][0]) ? 
												// is alphabet
												"\nCaughtException: Invalid non-integer argument to precedence field: '" : 
												// not-alphabet
												"Error: Terminal associativity field can only be one of : 'r' (right - associative), 'l' (left - associative), 'n' (non - associative)\n")
											<< term_info[1] << "'\n"
											<< "[Line " << l_no << "]: " << line << "\n";
									continue;
								}
							}
							catch (std::out_of_range const& ex) {
								error_in_get_terminals_and_productions = true;
								std::cout << "\nCaughtException: Argument to precedence field exceeds integer range: '" << term_info[1] << "'\n"
									<< "[Line " << l_no << "]: " << line << "\n";
								continue;
							}

							terminals.emplace(*strings.find(term_info[0]), prec, associativity);
						}
						else if (term_info.size() == 3) {
							if (term_info[2] != "n" && term_info[2] != "l" && term_info[2] != "r") {
								error_in_get_terminals_and_productions = true;
								std::cout << "\nError: Terminal associativity field can only be one of: 'r' (right-associative), 'l' (left-associative), 'n' (non-associative)\n"
									<< "[Line " << l_no << "]: " << line << "\n";
							}

							strings.insert(term_info[0]);
							
							int prec = 0;
							try {
								prec = std::stoi(term_info[1]);
							}
							catch (std::invalid_argument const& ex) {
								error_in_get_terminals_and_productions = true;
								std::cout << "\nCaughtException: Invalid non-integer argument to precedence field: '" << term_info[1] << "'\n"
									<< "[Line " << l_no << "]: " << line << "\n";
								continue;
							}
							catch (std::out_of_range const& ex) {
								error_in_get_terminals_and_productions = true;
								std::cout << "\nCaughtException: Argument to precedence field exceeds integer range: '" << term_info[1] << "'\n"
									<< "[Line " << l_no << "]: " << line << "\n";
								continue;
							}
							
							terminals.emplace(*strings.find(term_info[0]), prec, term_info[2]);
						}
						else {
							error_in_get_terminals_and_productions = true;
							std::cout << "\nError: Terminal symbol declaration information should not contain more than three fields\n"
								<< "[Line " << l_no << "]: " << line << "\n";
							continue;
						}

						if (l_no == 1) goal_production_lookahead_symbol = *strings.find(term_info[0]);
					}
					else if (line == "") {
						parsing_terminals = false;
						if (!error_in_get_terminals_and_productions) parsing_productions = true;
					}
					else {
						error_in_get_terminals_and_productions = true;
						std::cout << "\nError: Terminal symbols must be declared with a 't_' prefix.\n"
							<< "[Line " << l_no++ << "]: " << line << "\n";
					}
				}
				else if (parsing_productions) {
					if (line[0] == 't' && line[1] == '_') {
						error_in_get_terminals_and_productions = true;
						std::cout << "\nError: Terminal symbols ('t_' prefix) cannot be on the LHS of a production rule\n"
							<< "[Line " << l_no++ << "]: " << line << "\n";
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
						std::cout << "\nError: Expected ' > ' delimiter between LHS and RHS of production rule\n"
							<< "[Line " << l_no++ << "]: " << line << "\n";
						continue;
					}

					auto symbol_start = start = j;
					bool found_first = false;
					// Check for leftmost terminal in rhs of production
					for (; j < line.size(); j++) {
						if (is_whitespace(line[j])) {
							if (!found_first && terminals.count(Terminal(std::string(line, symbol_start, j - symbol_start), 0, "n"))) {
								found_first = true;
								firsts[*strings.find(lhs)].insert(*strings.find(std::string(line, symbol_start, j - symbol_start)));
							}
							symbol_start = j + 1;
						}

						if (found_first) break;
					}

					// if 'found_first' is still false, check if rightmost rhs symbol is a terminal
					if (!found_first && terminals.count(Terminal(std::string(line, symbol_start, j - symbol_start), 0, "n"))) {
						found_first = true;
						firsts[*strings.find(lhs)].insert(*strings.find(std::string(line, symbol_start, j - symbol_start)));
					}

					// Test if lhs or rhs can come up empty.
					std::string rhs(line, start, line.size() - start);

					if (rhs == "") {
						error_in_get_terminals_and_productions = true;
						std::cout << "\nError: Grammar production rhs is empty. Ill-defined grammar.\n"
							<< "[Line " << l_no << "]: " << line << "\n";
						continue;
					}
					
					if (right_deriv.count(rhs)) {
						error_in_get_terminals_and_productions = true;
						std::cout << "\nError: Grammar production has a potential REDUCE-REDUCE conflict.\n"
							<< "[Line " << l_no << "]: " << line << " AND "
							<< right_deriv[rhs] << " > " << rhs << "\n";
						continue;
					}

					// check for explicitly defined precedence for rule/production
					int prec = 0;
					for (auto i = static_cast<int>(rhs.size()); i > 0; i--) {
						if (is_whitespace(rhs[i])) {
							std::string symbol = std::string(rhs, i + 1, rhs.size() - i);
							if (symbol == "") continue;

							try {
								prec = std::stoi(symbol);
							}
							catch (std::invalid_argument const& ex) {
								// final symbol is not a precedence value
								break;
							}
							catch (std::out_of_range const& ex) {
								// final symbol is a precedence value but is beyond the integer range
								error_in_get_terminals_and_productions = true;
								std::cout << "\nCaughtException: Argument to precedence field exceeds integer range: '" << symbol << "'\n"
									<< "[Line " << l_no << "]: " << line << "\n";
								break;
							}

							// final is a precedence value
							rhs = std::string(rhs, 0, i);
							break;
						}
					}

					if (error_in_get_terminals_and_productions) continue;

					strings.insert(rhs);
					strings.insert(lhs);

					right_deriv[*strings.find(rhs)] = *strings.find(lhs);
					productions[*strings.find(lhs)].push_back(*strings.find(rhs));
					production_precedence[*strings.find(rhs)] = prec;
				}
				l_no++;
			}
		}

		if (debug && !error_in_get_terminals_and_productions) {
			print_debug_info(TERMINALS);
			print_debug_info(NON_TERMINALS);
			print_debug_info(PRODUCTIONS);
		}

		return error_in_get_terminals_and_productions ? false : true;
	}

	bool check_symbols_in_productions() {
		bool found_invalid_symbol = false;
		std::string symbol;

		for (auto& production : productions) {
			std::vector<std::string_view> values = production.second;
			for (auto& value : values) {
				auto start = 0; auto i = 0;
				for (auto i = 0; i < value.size(); i++) {
					if (is_whitespace(value[i])) {
						symbol = std::string(value, start, i - start);
						if (symbol != "") {
							if (terminals.find(Terminal(symbol, 0, "n")) == terminals.end() &&
								non_terminals.find(symbol) == non_terminals.end())
							{
								found_invalid_symbol = true;
								std::cout << "\nError: Unexpected symbol '" << symbol << "'\n"
									<< "Fix production rule: '" << production.first << " > " << value << "'\n";
							}
						}
						start = i + 1;
					}
				}

				symbol = std::string(value, start, value.size() - start);
				if (symbol != "") {
					if (terminals.find(Terminal(symbol, 0, "n")) == terminals.end() &&
						non_terminals.find(symbol) == non_terminals.end())
					{
						found_invalid_symbol = true;
						std::cout << "\nError: Unexpected symbol '" << symbol << "'\n"
							<< "Fix production rule: '" << production.first << " > " << value << "'\n";
					}
				}
			}
		}

		return found_invalid_symbol ? false : true;
	}

	void build_cc() {
		std::unordered_set<Item, CustomHash> canonicalSet_0;
		auto& goal = productions[goal_lhs_symbol];
		for (auto& prod : goal) {
			std::vector<std::string_view> beginItemProd;
			std::string symbol;
			auto start = 0;
			auto i = 0;
			beginItemProd.push_back(goal_lhs_symbol);
			for (; i < prod.size(); i++) {
				if (is_whitespace(prod[i])) {
					symbol = std::string(prod, start, i - start);
					if (symbol != "") {
						beginItemProd.push_back(*strings.find(symbol));
					}
					start = i + 1;
				}
			}
			
			symbol = std::string(prod, start, i - start);
			if (symbol != "") beginItemProd.push_back(*strings.find(symbol));
			
			canonicalSet_0.emplace(1, beginItemProd, goal_production_lookahead_symbol, production_precedence[prod]);
		}

		auto number_of_unmarked_sets = 0;
		size_t set_index = 0;
		closure_function(canonicalSet_0);
		CanonicalCollectionValue ccv{ set_index, false };
		canonicalCollection[canonicalSet_0] = ccv;

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
						std::unordered_set<Item, CustomHash> new_set;
						if (item.position >= item.production.size()) continue;
						goto_function(canonicalSet_i.first, item.production[item.position], new_set);

						if (!canonicalCollection.count(new_set)) {
							++set_index;
							ccv = { set_index, false };
							canonicalCollection[new_set] = ccv;
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

	void build_tables() {
		for (auto& canonicalSet_i : canonicalCollection) {
			auto current_state = canonicalSet_i.second.state;

			if (current_state == 28) {
				std::cout << "Hit\n";
			}
			
			for (auto& item : canonicalSet_i.first) {
				if (item.position < item.production.size() &&
					terminals.count(Terminal(item.production[item.position], 0, "n"))) 
				{
					std::string_view terminal = item.production[item.position];
					// check for SHIFT-REDUCE conflict
					if (!actionTable.count(std::make_pair(current_state, terminal))) {
						std::unordered_set<Item, CustomHash> next_set;
						goto_function(canonicalSet_i.first, terminal, next_set);

						if (canonicalCollection.count(next_set)) {
							auto next_state = canonicalCollection[next_set].state;
							Action action{ SHIFT, next_state };
							actionTable[std::make_pair(current_state, terminal)] = action;
						}
					}
					else if (actionTable.find(std::make_pair(current_state, terminal))->second.type == REDUCE) {
						// SHIFT-REDUCE Conflict
						int last_terminal_precedence = 0;
						std::string last_terminal_associativity = "n";
						std::string_view item_production_lhs = item.production[0];

						set_last_terminal(last_terminal_precedence, last_terminal_associativity, item.production);

						if (item.production_precedence > terminals.find(Terminal(terminal, 0, "n"))->precedence ||
							last_terminal_precedence == 0)
						{
							// do nothing
						}
						else if (last_terminal_precedence > terminals.find(Terminal(terminal, 0, "n"))->precedence) {
							// do nothing
						}
						else if (last_terminal_precedence == terminals.find(Terminal(terminal, 0, "n"))->precedence &&
							last_terminal_associativity == "l")
						{
							// do nothing
						}
						else {
							std::unordered_set<Item, CustomHash> next_set;
							goto_function(canonicalSet_i.first, terminal, next_set);

							if (canonicalCollection.count(next_set)) {
								auto next_state = canonicalCollection[next_set].state;
								Action action{ SHIFT, next_state };
								actionTable[std::make_pair(current_state, terminal)] = action;
							}
						}
					}
				}
				else if (item.position == item.production.size()) {
					if (item.production[0] == goal_lhs_symbol &&
						item.lookahead == goal_production_lookahead_symbol) 
					{
						Action action{ ACCEPT, 0 };
						actionTable[std::make_pair(current_state, goal_production_lookahead_symbol)] = action;
					}
					else {
						std::string_view item_production_lhs = item.production[0];
						// Check for SHIFT-REDUCE conflict
						int last_terminal_precedence = 0;
						std::string last_terminal_associativity = "n";
						set_last_terminal(last_terminal_precedence, last_terminal_associativity, item.production);

						if (item.production_precedence > terminals.find(Terminal(item.lookahead, 0, "n"))->precedence ||
							last_terminal_precedence == 0)
						{
							Action action{ REDUCE, reduce_info_map.find(std::make_pair(item_production_lhs, item.production.size() - 1))->second };
							actionTable[std::make_pair(current_state, item.lookahead)] = action;
						}
						else if (last_terminal_precedence > terminals.find(Terminal(item.lookahead, 0, "n"))->precedence) {
							Action action{ REDUCE, reduce_info_map.find(std::make_pair(item_production_lhs, item.production.size() - 1))->second };
							actionTable[std::make_pair(current_state, item.lookahead)] = action;
						}
						else if (last_terminal_precedence == terminals.find(Terminal(item.lookahead, 0, "n"))->precedence &&
							last_terminal_associativity == "l")
						{
							Action action{ REDUCE, reduce_info_map.find(std::make_pair(item_production_lhs, item.production.size() - 1))->second };
							actionTable[std::make_pair(current_state, item.lookahead)] = action;
						}
						else {
							std::unordered_set<Item, CustomHash> next_set;
							goto_function(canonicalSet_i.first, item.lookahead, next_set);

							if (canonicalCollection.count(next_set)) {
								auto next_state = canonicalCollection[next_set].state;
								Action action{ SHIFT, next_state };
								actionTable[std::make_pair(current_state, item.lookahead)] = action;
							}
						}
					}
				}
			}

			for (auto& non_term : non_terminals) {
				std::unordered_set<Item, CustomHash> next_set;
				goto_function(canonicalSet_i.first, non_term, next_set);
				if (canonicalCollection.count(next_set)) {
					auto next_state = canonicalCollection[next_set].state;
					gotoTable[std::make_pair(current_state, non_term)] = next_state;
				}
			}
		}

		if (debug) {
			print_debug_info(ACTION_TABLE);
			print_debug_info(GOTO_TABLE);
		}
	}

	void build_output_file() {
		std::ofstream file(output_file_path, std::ios::out);
		file << "#pragma once\n\n"
			"#include <unordered_map>\n"
			"#include <unordered_set>\n"
			"#include <string>\n"
			"#include <string_view>\n"
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
		file << "enum TokenType {\n\t";
		for (auto& term : terminals) {
			file << term.str;
			++col;
			if (col == total) file << "\n};\n\n";
			else if (!(col % 8)) file << ",\n\t";
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
			"\tsize_t operator()(const std::pair<size_t, TokenType>& pair) const {\n"
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
			"std::unordered_map<std::pair<size_t, TokenType>, Action, PairHash> actionTable {\n\t";
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

void print_help() {
	std::cout << "";
}

int main(int argc, char** argv) {
	if (argc < 2 || argc > 3) {
		std::cout << "usage: ./parsegen <path/to/grammar> [OPTIONAL] <path/to/output/file>\n       './parsegen -h' or './parsegen -H' for help information\n";
		exit(1);
	}

	if (argv[1] == "-h" || argv[1] == "-H") {
		print_help();
	}

	ParserGen parserGen(argc, argv);
	parserGen.debug = false;

	if (parserGen.file_access_error) {
		std::cout << "Unable to open " << argv[1] << " file\n";
		return -1;
	}

	if (!parserGen.get_terminals_and_productions()) {
		std::cout << "\nFatal error in grammar definition. Parser-Generator terminated early.\n";
		return -1;
	}

	if (!parserGen.check_symbols_in_productions()) {
		std::cout << "\nFatal Error in grammar definition. Parser-Generator terminated early.\n";
		return -1;
	}

	parserGen.build_cc();
	parserGen.build_tables();
	parserGen.build_output_file();

	std::cout << "\nParse tables have been generated and written to '" << parserGen.output_file_path << "' successfully!\n";
}