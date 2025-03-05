#include <iostream>
#include <vector>
#include <stack>

#include "parse-tables.h"

static bool is_whitespace(char c) {
	return (c == ' ' || c == '\r' || c == '\t');
}

static bool scan(std::string& input, std::vector<TokenType>& output) {
	bool scan_error = false;
	for (auto& c : input) {
		if (c == '\n' ||
			is_whitespace(c)) {
			continue;
		}

		switch (c) {
		case '(':
			output.push_back(TokenType::t_LP);
			break;
		case ')':
			output.push_back(TokenType::t_RP);
			break;
		default:
			std::cout << "Unexpected token '" << c << "'\n";
			scan_error = true;
		}
	}
	if (scan_error) {
		return false;
	}
	output.push_back(TokenType::t_EOF);
	return true;
}

static bool parse(std::vector<TokenType> tokens) {
	std::stack<size_t> states;
	states.push(0);
	auto state = static_cast<size_t>(0);

	for (auto i = 0; i < tokens.size();) {
		state = states.top();
		if (actionTable.find(std::make_pair(state, tokens[i])) == actionTable.end()) {
			return false;
		}
		
		Action& action = actionTable[std::make_pair(state, tokens[i])];

		if (action.type == ActionType::REDUCE) {
			auto index = actionTable[std::make_pair(state, tokens[i])].value;
			auto pop_count = reduce_info[index].second;
			while (pop_count--) {
				states.pop();
			}
			state = states.top();
			auto reduce_symbol = reduce_info[index].first;
			auto next_state = gotoTable[std::make_pair(state, reduce_symbol)];
			states.push(next_state);
		}
		else if (action.type == ActionType::SHIFT) {
			auto next_state = actionTable[std::make_pair(state, tokens[i])].value;
			states.push(next_state);
			// Only SHIFT actions proceed to the next symbol
			++i;
		}
		else if (action.type == ActionType::ACCEPT) {
			return true;
		}
	}
	return false;
}

int main() {
	std::string input;
	std::vector<TokenType> tokens;
	std::cout << "Parentheses Grammar Interpreter (enter 'q' or CTRL-C to exit)\n";
	while (true) {
		std::cout << "> ";
		if (!std::getline(std::cin, input) || input == "q") {
			std::cout << (input == "q" ? "quit...\n" : "\nquit...\n");
			return 0;
		} 
		else if (input == "") {
			continue;
		}
		
		if (!scan(input, tokens)) {
			tokens.clear();
			continue;
		}
		
		if (parse(tokens)) {
			std::cout << "Valid Input String\n";
		}
		else {
			std::cout << "Invalid Input String\n";
		}
		tokens.clear();
	}
}
