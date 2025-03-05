#include <iostream>
#include <vector>
#include <string>
#include <stack>

#include "parse-tables.h"

static bool is_whitespace(char c) {
	return (c == ' ' || c == '\r' || c == '\t');
}

static bool is_digit(char c) {
	return c >= '0' && c <= '9';
}

struct Token {
	TokenType type;
	std::string value;
};

static bool scan(std::string& input, std::vector<Token>& output) {
	bool scan_error = false;
	auto start = 0;
	
	for (auto i = 0; i < input.size(); i++) {
		if (input[i] == '\n' ||
			is_whitespace(input[i]))
		{
			start = i + 1;
			continue;
		}

		switch (input[i]) {
		case '(': {
			Token token{ TokenType::t_LP, std::string(input, start, 1)};
			output.push_back(token);
			start = i + 1;
			break;
		}
		case ')': {
			Token token{ TokenType::t_RP, std::string(input, start, 1) };
			output.push_back(token);
			start = i + 1;
			break;
		}
		case '+': {
			Token token{ TokenType::t_PLUS, std::string(input, start, 1) };
			output.push_back(token);
			start = i + 1;
			break;
		}
		case '-': {
			Token token{ TokenType::t_MINUS, std::string(input, start, 1) };
			output.push_back(token);
			start = i + 1;
			break;
		}
		case '*': {
			Token token{ TokenType::t_TIMES, std::string(input, start, 1) };
			output.push_back(token);
			start = i + 1;
			break;
		}
		case '/': {
			Token token{ TokenType::t_DIVIDE, std::string(input, start, 1) };
			output.push_back(token);
			start = i + 1;
			break;
		}
		default:
			if (is_digit(input[i])) {
				while (is_digit(input[i])) {
					++i;
				}
				Token token{ TokenType::t_NUMBER, std::string(input, start, i - start) };
				i--;
				output.push_back(token);
			}
			else {
				std::cout << "Unexpected token '" << input[i] << "'\n";
				scan_error = true;
			}
			start = i + 1;
		}
	}
	if (scan_error) {
		return false;
	}
	Token token{ TokenType::t_EOF, "$" };
	output.push_back(token);
	return true;
}

static bool parse(std::vector<Token> tokens, float& output) {
	std::stack<size_t> states;
	std::stack<float> values;
	
	states.push(0);
	auto state = static_cast<size_t>(0);
	
	for (auto i = 0; i < tokens.size();) {
		state = states.top();
		if (actionTable.find(std::make_pair(state, tokens[i].type)) == actionTable.end()) {
			return false;
		}
	
		Action& action = actionTable[std::make_pair(state, tokens[i].type)];
	
		if (action.type == ActionType::REDUCE) {
			auto index = actionTable[std::make_pair(state, tokens[i].type)].value;
			auto pop_count = reduce_info[index].second;
			while (pop_count--) {
				states.pop();
			}
			state = states.top();
			auto reduce_symbol = reduce_info[index].first;
			auto next_state = gotoTable[std::make_pair(state, reduce_symbol)];
			states.push(next_state);

			if (reduce_symbol == "Add") {
				auto rhs = values.top();
				values.pop();
				auto lhs = values.top();
				values.pop();
				values.push(lhs + rhs);
			}
			else if (reduce_symbol == "Sub") {
				auto rhs = values.top();
				values.pop();
				auto lhs = values.top();
				values.pop();
				values.push(lhs - rhs);
			}
			else if (reduce_symbol == "Mul") {
				auto rhs = values.top();
				values.pop();
				auto lhs = values.top();
				values.pop();
				values.push(lhs * rhs);
			}
			else if (reduce_symbol == "Div") {
				auto rhs = values.top();
				values.pop();
				auto lhs = values.top();
				values.pop();
				values.push(lhs / rhs);
			}
			else if (reduce_symbol == "Unary") {
				auto val = values.top();
				values.pop();
				values.push(-val);
			}
		}
		else if (action.type == ActionType::SHIFT) {
			auto next_state = actionTable[std::make_pair(state, tokens[i].type)].value;
			states.push(next_state);
			// Only SHIFT actions proceed to the next symbol
			if (tokens[i].type == t_NUMBER) {
				values.push(std::stoi(tokens[i].value));
			}
			++i;
		}
		else if (action.type == ActionType::ACCEPT) {
			output = values.top();
			return true;
		}
	}

	return false;
}

int main() {
	std::string input;
	std::vector<Token> tokens;
	float output = 0;
	std::cout << "Math Expressions Evaluator ('q' or CTRL-C to exit)\n";
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

		if (parse(tokens, output)) {
			std::cout << output << "\n";
		}
		else {
			std::cout << "Invalid Input String\n";
		}
		tokens.clear();
	}
}
