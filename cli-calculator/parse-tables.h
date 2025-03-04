#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <vector>

std::unordered_set<std::string> strings {
	"Unary", "Div", "Mul", "Sub", "Grouping", "Add", "Expression", "Statement"
};

enum TokenType {
	t_RP, t_DIVIDE, t_TIMES, t_LP, t_MINUS, t_NUMBER, t_PLUS, t_EOF
};

std::vector<std::pair<std::string_view, size_t>> reduce_info {
	{ *strings.find("Add"), 3 }, { *strings.find("Div"), 3 },
	{ *strings.find("Unary"), 2 }, { *strings.find("Sub"), 3 },
	{ *strings.find("Expression"), 1 }, { *strings.find("Grouping"), 3 },
	{ *strings.find("Statement"), 1 }, { *strings.find("Mul"), 3 }
};

struct PairHash {
	size_t operator()(const std::pair<size_t, std::string_view>& pair) const {
		return std::hash<size_t>{}(pair.first) ^ std::hash<std::string_view>{}(pair.second);
	}

	size_t operator()(const std::pair<size_t, TokenType>& pair) const {
		return std::hash<size_t>{}(pair.first) ^ pair.second;
	}
};

enum ActionType {
	SHIFT,
	REDUCE,
	ACCEPT
};

struct Action {
	ActionType type;
	size_t value;
};

std::unordered_map<std::pair<size_t, TokenType>, Action, PairHash> actionTable {
	{{ 28, t_RP }, {REDUCE, 7 }}, {{ 28, t_PLUS }, {REDUCE, 7 }},
	{{ 34, t_NUMBER }, {SHIFT, 3 }}, {{ 34, t_MINUS }, {SHIFT, 2 }},
	{{ 26, t_NUMBER }, {SHIFT, 17 }}, {{ 26, t_LP }, {SHIFT, 13 }},
	{{ 25, t_LP }, {SHIFT, 13 }}, {{ 28, t_TIMES }, {REDUCE, 7 }},
	{{ 25, t_MINUS }, {SHIFT, 11 }}, {{ 24, t_DIVIDE }, {REDUCE, 5 }},
	{{ 24, t_MINUS }, {REDUCE, 5 }}, {{ 24, t_EOF }, {REDUCE, 5 }},
	{{ 24, t_PLUS }, {REDUCE, 5 }}, {{ 21, t_RP }, {REDUCE, 2 }},
	{{ 21, t_DIVIDE }, {REDUCE, 2 }}, {{ 36, t_RP }, {REDUCE, 1 }},
	{{ 36, t_PLUS }, {REDUCE, 1 }}, {{ 36, t_DIVIDE }, {REDUCE, 1 }},
	{{ 23, t_NUMBER }, {SHIFT, 17 }}, {{ 23, t_MINUS }, {SHIFT, 11 }},
	{{ 23, t_LP }, {SHIFT, 13 }}, {{ 18, t_PLUS }, {REDUCE, 4 }},
	{{ 18, t_MINUS }, {REDUCE, 4 }}, {{ 14, t_MINUS }, {REDUCE, 4 }},
	{{ 14, t_RP }, {REDUCE, 4 }}, {{ 14, t_PLUS }, {REDUCE, 4 }},
	{{ 16, t_PLUS }, {REDUCE, 4 }}, {{ 34, t_LP }, {SHIFT, 8 }},
	{{ 15, t_PLUS }, {REDUCE, 4 }}, {{ 15, t_RP }, {REDUCE, 4 }},
	{{ 19, t_PLUS }, {REDUCE, 4 }}, {{ 22, t_NUMBER }, {SHIFT, 17 }},
	{{ 8, t_LP }, {SHIFT, 13 }}, {{ 10, t_EOF }, {REDUCE, 4 }},
	{{ 31, t_MINUS }, {SHIFT, 2 }}, {{ 39, t_EOF }, {REDUCE, 3 }},
	{{ 16, t_DIVIDE }, {REDUCE, 4 }}, {{ 39, t_MINUS }, {REDUCE, 3 }},
	{{ 16, t_MINUS }, {REDUCE, 4 }}, {{ 39, t_TIMES }, {SHIFT, 31 }},
	{{ 41, t_PLUS }, {REDUCE, 1 }}, {{ 20, t_DIVIDE }, {REDUCE, 4 }},
	{{ 39, t_DIVIDE }, {SHIFT, 32 }}, {{ 39, t_PLUS }, {REDUCE, 3 }},
	{{ 33, t_NUMBER }, {SHIFT, 3 }}, {{ 26, t_MINUS }, {SHIFT, 11 }},
	{{ 17, t_TIMES }, {REDUCE, 4 }}, {{ 9, t_DIVIDE }, {REDUCE, 4 }},
	{{ 12, t_MINUS }, {SHIFT, 26 }}, {{ 7, t_EOF }, {REDUCE, 4 }},
	{{ 12, t_RP }, {SHIFT, 24 }}, {{ 40, t_DIVIDE }, {REDUCE, 7 }},
	{{ 32, t_MINUS }, {SHIFT, 2 }}, {{ 12, t_PLUS }, {SHIFT, 23 }},
	{{ 29, t_RP }, {REDUCE, 0 }}, {{ 31, t_NUMBER }, {SHIFT, 3 }},
	{{ 24, t_TIMES }, {REDUCE, 5 }}, {{ 29, t_MINUS }, {REDUCE, 0 }},
	{{ 35, t_RP }, {SHIFT, 38 }}, {{ 35, t_PLUS }, {SHIFT, 23 }},
	{{ 30, t_EOF }, {REDUCE, 2 }}, {{ 7, t_TIMES }, {REDUCE, 4 }},
	{{ 29, t_DIVIDE }, {SHIFT, 22 }}, {{ 35, t_MINUS }, {SHIFT, 26 }},
	{{ 29, t_PLUS }, {REDUCE, 0 }}, {{ 40, t_EOF }, {REDUCE, 7 }},
	{{ 11, t_LP }, {SHIFT, 13 }}, {{ 20, t_MINUS }, {REDUCE, 4 }},
	{{ 35, t_TIMES }, {SHIFT, 25 }}, {{ 40, t_MINUS }, {REDUCE, 7 }},
	{{ 18, t_DIVIDE }, {REDUCE, 4 }}, {{ 33, t_MINUS }, {SHIFT, 2 }},
	{{ 41, t_DIVIDE }, {REDUCE, 1 }}, {{ 18, t_TIMES }, {REDUCE, 4 }},
	{{ 7, t_MINUS }, {REDUCE, 4 }}, {{ 18, t_RP }, {REDUCE, 4 }},
	{{ 1, t_PLUS }, {SHIFT, 34 }}, {{ 30, t_DIVIDE }, {REDUCE, 2 }},
	{{ 33, t_LP }, {SHIFT, 8 }}, {{ 40, t_PLUS }, {REDUCE, 7 }},
	{{ 31, t_LP }, {SHIFT, 8 }}, {{ 5, t_TIMES }, {REDUCE, 4 }},
	{{ 3, t_DIVIDE }, {REDUCE, 4 }}, {{ 9, t_PLUS }, {REDUCE, 4 }},
	{{ 32, t_LP }, {SHIFT, 8 }}, {{ 30, t_PLUS }, {REDUCE, 2 }},
	{{ 9, t_EOF }, {REDUCE, 4 }}, {{ 32, t_NUMBER }, {SHIFT, 3 }},
	{{ 38, t_PLUS }, {REDUCE, 5 }}, {{ 3, t_EOF }, {REDUCE, 4 }},
	{{ 11, t_NUMBER }, {SHIFT, 17 }}, {{ 16, t_RP }, {REDUCE, 4 }},
	{{ 7, t_PLUS }, {REDUCE, 4 }}, {{ 29, t_TIMES }, {SHIFT, 25 }},
	{{ 30, t_MINUS }, {REDUCE, 2 }}, {{ 21, t_PLUS }, {REDUCE, 2 }},
	{{ 38, t_RP }, {REDUCE, 5 }}, {{ 28, t_DIVIDE }, {REDUCE, 7 }},
	{{ 6, t_TIMES }, {REDUCE, 4 }}, {{ 40, t_TIMES }, {REDUCE, 7 }},
	{{ 41, t_EOF }, {REDUCE, 1 }}, {{ 21, t_MINUS }, {REDUCE, 2 }},
	{{ 38, t_DIVIDE }, {REDUCE, 5 }}, {{ 19, t_MINUS }, {REDUCE, 4 }},
	{{ 38, t_TIMES }, {REDUCE, 5 }}, {{ 19, t_DIVIDE }, {REDUCE, 4 }},
	{{ 38, t_MINUS }, {REDUCE, 5 }}, {{ 6, t_DIVIDE }, {REDUCE, 4 }},
	{{ 41, t_TIMES }, {REDUCE, 1 }}, {{ 21, t_TIMES }, {REDUCE, 2 }},
	{{ 6, t_MINUS }, {REDUCE, 4 }}, {{ 19, t_RP }, {REDUCE, 4 }},
	{{ 6, t_PLUS }, {REDUCE, 4 }}, {{ 0, t_NUMBER }, {SHIFT, 3 }},
	{{ 6, t_EOF }, {REDUCE, 4 }}, {{ 27, t_PLUS }, {REDUCE, 3 }},
	{{ 10, t_PLUS }, {REDUCE, 4 }}, {{ 12, t_TIMES }, {SHIFT, 25 }},
	{{ 9, t_MINUS }, {REDUCE, 4 }}, {{ 10, t_DIVIDE }, {REDUCE, 4 }},
	{{ 22, t_LP }, {SHIFT, 13 }}, {{ 15, t_MINUS }, {REDUCE, 4 }},
	{{ 10, t_TIMES }, {REDUCE, 4 }}, {{ 4, t_EOF }, {REDUCE, 4 }},
	{{ 14, t_TIMES }, {REDUCE, 4 }}, {{ 11, t_MINUS }, {SHIFT, 11 }},
	{{ 4, t_DIVIDE }, {REDUCE, 4 }}, {{ 19, t_TIMES }, {REDUCE, 4 }},
	{{ 4, t_MINUS }, {REDUCE, 4 }}, {{ 2, t_NUMBER }, {SHIFT, 3 }},
	{{ 17, t_RP }, {REDUCE, 4 }}, {{ 4, t_PLUS }, {REDUCE, 4 }},
	{{ 0, t_MINUS }, {SHIFT, 2 }}, {{ 4, t_TIMES }, {REDUCE, 4 }},
	{{ 5, t_PLUS }, {REDUCE, 4 }}, {{ 12, t_DIVIDE }, {SHIFT, 22 }},
	{{ 3, t_MINUS }, {REDUCE, 4 }}, {{ 15, t_TIMES }, {REDUCE, 4 }},
	{{ 8, t_MINUS }, {SHIFT, 11 }}, {{ 5, t_DIVIDE }, {REDUCE, 4 }},
	{{ 37, t_EOF }, {REDUCE, 0 }}, {{ 5, t_EOF }, {REDUCE, 4 }},
	{{ 1, t_MINUS }, {SHIFT, 33 }}, {{ 20, t_TIMES }, {REDUCE, 4 }},
	{{ 16, t_TIMES }, {REDUCE, 4 }}, {{ 5, t_MINUS }, {REDUCE, 4 }},
	{{ 3, t_PLUS }, {REDUCE, 4 }}, {{ 1, t_DIVIDE }, {SHIFT, 32 }},
	{{ 3, t_TIMES }, {REDUCE, 4 }}, {{ 25, t_NUMBER }, {SHIFT, 17 }},
	{{ 2, t_LP }, {SHIFT, 8 }}, {{ 15, t_DIVIDE }, {REDUCE, 4 }},
	{{ 9, t_TIMES }, {REDUCE, 4 }}, {{ 2, t_MINUS }, {SHIFT, 2 }},
	{{ 41, t_MINUS }, {REDUCE, 1 }}, {{ 37, t_PLUS }, {REDUCE, 0 }},
	{{ 37, t_DIVIDE }, {SHIFT, 32 }}, {{ 37, t_MINUS }, {REDUCE, 0 }},
	{{ 7, t_DIVIDE }, {REDUCE, 4 }}, {{ 10, t_MINUS }, {REDUCE, 4 }},
	{{ 1, t_TIMES }, {SHIFT, 31 }}, {{ 36, t_TIMES }, {REDUCE, 1 }},
	{{ 17, t_MINUS }, {REDUCE, 4 }}, {{ 1, t_EOF }, {ACCEPT, 0 }},
	{{ 0, t_LP }, {SHIFT, 8 }}, {{ 20, t_PLUS }, {REDUCE, 4 }},
	{{ 13, t_LP }, {SHIFT, 13 }}, {{ 14, t_DIVIDE }, {REDUCE, 4 }},
	{{ 13, t_MINUS }, {SHIFT, 11 }}, {{ 20, t_RP }, {REDUCE, 4 }},
	{{ 13, t_NUMBER }, {SHIFT, 17 }}, {{ 17, t_PLUS }, {REDUCE, 4 }},
	{{ 36, t_MINUS }, {REDUCE, 1 }}, {{ 17, t_DIVIDE }, {REDUCE, 4 }},
	{{ 8, t_NUMBER }, {SHIFT, 17 }}, {{ 27, t_RP }, {REDUCE, 3 }},
	{{ 27, t_DIVIDE }, {SHIFT, 22 }}, {{ 30, t_TIMES }, {REDUCE, 2 }},
	{{ 27, t_MINUS }, {REDUCE, 3 }}, {{ 28, t_MINUS }, {REDUCE, 7 }},
	{{ 27, t_TIMES }, {SHIFT, 25 }}, {{ 35, t_DIVIDE }, {SHIFT, 22 }},
	{{ 37, t_TIMES }, {SHIFT, 31 }}, {{ 22, t_MINUS }, {SHIFT, 11 }}
};

std::unordered_map<std::pair<size_t, std::string_view>, size_t, PairHash> gotoTable {
	{{ 34, *strings.find("Expression") }, {37}}, {{ 34, *strings.find("Add") }, {10}},
	{{ 34, *strings.find("Grouping") }, {5}}, {{ 34, *strings.find("Sub") }, {6}},
	{{ 34, *strings.find("Mul") }, {9}}, {{ 34, *strings.find("Unary") }, {7}},
	{{ 26, *strings.find("Expression") }, {27}}, {{ 26, *strings.find("Add") }, {15}},
	{{ 26, *strings.find("Grouping") }, {14}}, {{ 26, *strings.find("Sub") }, {20}},
	{{ 26, *strings.find("Mul") }, {18}}, {{ 26, *strings.find("Div") }, {19}},
	{{ 26, *strings.find("Unary") }, {16}}, {{ 25, *strings.find("Mul") }, {18}},
	{{ 25, *strings.find("Sub") }, {20}}, {{ 25, *strings.find("Div") }, {19}},
	{{ 23, *strings.find("Expression") }, {29}}, {{ 23, *strings.find("Add") }, {15}},
	{{ 25, *strings.find("Expression") }, {28}}, {{ 23, *strings.find("Grouping") }, {14}},
	{{ 23, *strings.find("Sub") }, {20}}, {{ 25, *strings.find("Add") }, {15}},
	{{ 23, *strings.find("Mul") }, {18}}, {{ 23, *strings.find("Div") }, {19}},
	{{ 22, *strings.find("Expression") }, {36}}, {{ 22, *strings.find("Add") }, {15}},
	{{ 22, *strings.find("Grouping") }, {14}}, {{ 25, *strings.find("Unary") }, {16}},
	{{ 22, *strings.find("Sub") }, {20}}, {{ 11, *strings.find("Add") }, {15}},
	{{ 11, *strings.find("Div") }, {19}}, {{ 2, *strings.find("Sub") }, {6}},
	{{ 2, *strings.find("Div") }, {4}}, {{ 0, *strings.find("Add") }, {10}},
	{{ 13, *strings.find("Grouping") }, {14}}, {{ 32, *strings.find("Expression") }, {41}},
	{{ 33, *strings.find("Add") }, {10}}, {{ 32, *strings.find("Grouping") }, {5}},
	{{ 11, *strings.find("Unary") }, {16}}, {{ 32, *strings.find("Mul") }, {9}},
	{{ 0, *strings.find("Sub") }, {6}}, {{ 13, *strings.find("Add") }, {15}},
	{{ 33, *strings.find("Unary") }, {7}}, {{ 33, *strings.find("Div") }, {4}},
	{{ 13, *strings.find("Mul") }, {18}}, {{ 32, *strings.find("Div") }, {4}},
	{{ 34, *strings.find("Div") }, {4}}, {{ 2, *strings.find("Mul") }, {9}},
	{{ 32, *strings.find("Unary") }, {7}}, {{ 33, *strings.find("Mul") }, {9}},
	{{ 8, *strings.find("Div") }, {19}}, {{ 25, *strings.find("Grouping") }, {14}},
	{{ 31, *strings.find("Expression") }, {40}}, {{ 33, *strings.find("Sub") }, {6}},
	{{ 22, *strings.find("Unary") }, {16}}, {{ 33, *strings.find("Grouping") }, {5}},
	{{ 23, *strings.find("Unary") }, {16}}, {{ 32, *strings.find("Sub") }, {6}},
	{{ 31, *strings.find("Unary") }, {7}}, {{ 31, *strings.find("Mul") }, {9}},
	{{ 31, *strings.find("Add") }, {10}}, {{ 8, *strings.find("Unary") }, {16}},
	{{ 33, *strings.find("Expression") }, {39}}, {{ 31, *strings.find("Grouping") }, {5}},
	{{ 31, *strings.find("Div") }, {4}}, {{ 31, *strings.find("Sub") }, {6}},
	{{ 32, *strings.find("Add") }, {10}}, {{ 11, *strings.find("Expression") }, {21}},
	{{ 8, *strings.find("Sub") }, {20}}, {{ 8, *strings.find("Grouping") }, {14}},
	{{ 2, *strings.find("Expression") }, {30}}, {{ 8, *strings.find("Add") }, {15}},
	{{ 8, *strings.find("Expression") }, {12}}, {{ 11, *strings.find("Mul") }, {18}},
	{{ 2, *strings.find("Unary") }, {7}}, {{ 22, *strings.find("Div") }, {19}},
	{{ 2, *strings.find("Grouping") }, {5}}, {{ 13, *strings.find("Div") }, {19}},
	{{ 8, *strings.find("Mul") }, {18}}, {{ 2, *strings.find("Add") }, {10}},
	{{ 0, *strings.find("Div") }, {4}}, {{ 0, *strings.find("Unary") }, {7}},
	{{ 11, *strings.find("Grouping") }, {14}}, {{ 13, *strings.find("Expression") }, {35}},
	{{ 0, *strings.find("Mul") }, {9}}, {{ 11, *strings.find("Sub") }, {20}},
	{{ 0, *strings.find("Grouping") }, {5}}, {{ 13, *strings.find("Sub") }, {20}},
	{{ 0, *strings.find("Expression") }, {1}}, {{ 13, *strings.find("Unary") }, {16}},
	{{ 22, *strings.find("Mul") }, {18}}
};