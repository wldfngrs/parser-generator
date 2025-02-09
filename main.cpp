#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <array>

// std::map<std::pair<std::string, Action>, size_t> actionTable;
// std::map<std::pair<size_t, std::string>, size_t> gotoTable;

// Interpret 'next' differently. 
// SHIFT action interprets 'next' variable as holding the
// next state to move to
// REDUCE action interprets 'next' as holding the grammar rule
// the reduction will be following.

class ParserGen {
	enum Action {
		SHIFT,
		REDUCE,
		ACCEPT,
		ERROR,
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

	std::vector<std::vector<std::string>> grammar;

	std::vector<ActionTableInput> actionTable;
	std::vector<GotoTableInput> gotoTable;

	struct Item {
		std::vector<std::string> production;
		std::string lookahead;
	};

	//enum TokenType {
	//	TOKEN_TERMINAL,
	//	TOKEN_NONTERMINAL,
	//};
	//
	//struct Token {
	//	TokenType type;
	//	std::string literal;
	//};

	std::vector<Item> canonicalCollection;

	//std::vector<Token> parseTxt(std::string grammar) {
	//	
	//}

public:
	ParserGen(std::string pathToGrammar) {
		std::stringstream ss;
		std::ifstream file(pathToGrammar, std::ios_base::in);

		ss << file.rdbuf();
		std::string input = ss.str();

		// For each line, collect until you see a space
		// When space is found, push to symbol, until line is exhausted.
		// When line is exhausted, push to grammar.

		std::vector<std::string> symbol;
		int symbolIndex = 0;
		int start = 0;

		for (int i = 0; i < input.size(); i++) {
			if (input[i] == ' ') {
				symbol.push_back(std::string(input, start, i - start));
				start = i + 1;
			}

			if (input[i] == '\n') {
				symbol.push_back(std::string(input, start, i - start));
				grammar.push_back(symbol);
				start = i + 1;
			}
		}

		for (auto g : grammar) {
			for (int i = 0; i < g.size(); i++) {
				std::cout << g[i] << std::endl;
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
}