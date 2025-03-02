#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <vector>

std::unordered_set<std::string> strings {
	"Statement", "Expression", "Add", "Grouping", "Sub", "Mul", "Unary", "Div"
};

enum TokenType {
	t_RP, t_EOF, t_MINUS, t_PLUS, t_TIMES, t_NUMBER, t_DIVIDE, t_LP
};

std::vector<std::pair<std::string_view, size_t>> reduce_info {
	{ *strings.find("Sub"), 3 }, { *strings.find("Statement"), 1 },
	{ *strings.find("Expression"), 1 }, { *strings.find("Mul"), 3 },
	{ *strings.find("Add"), 3 }, { *strings.find("Grouping"), 3 },
	{ *strings.find("Div"), 3 }, { *strings.find("Unary"), 2 }
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
	{{ 8, t_EOF }, {REDUCE, 2 }}, {{ 40, t_RP }, {REDUCE, 3 }},
	{{ 17, t_DIVIDE }, {REDUCE, 2 }}, {{ 0, t_MINUS }, {SHIFT, 10 }},
	{{ 40, t_PLUS }, {REDUCE, 3 }}, {{ 38, t_TIMES }, {REDUCE, 6 }},
	{{ 40, t_MINUS }, {REDUCE, 3 }}, {{ 22, t_MINUS }, {REDUCE, 2 }},
	{{ 7, t_DIVIDE }, {REDUCE, 2 }}, {{ 38, t_MINUS }, {REDUCE, 6 }},
	{{ 40, t_TIMES }, {REDUCE, 3 }}, {{ 7, t_TIMES }, {REDUCE, 2 }},
	{{ 9, t_MINUS }, {SHIFT, 24 }}, {{ 3, t_EOF }, {REDUCE, 2 }},
	{{ 40, t_DIVIDE }, {REDUCE, 3 }}, {{ 7, t_EOF }, {REDUCE, 2 }},
	{{ 7, t_PLUS }, {REDUCE, 2 }}, {{ 22, t_DIVIDE }, {REDUCE, 2 }},
	{{ 7, t_MINUS }, {REDUCE, 2 }}, {{ 0, t_NUMBER }, {SHIFT, 3 }},
	{{ 20, t_MINUS }, {REDUCE, 2 }}, {{ 5, t_DIVIDE }, {REDUCE, 2 }},
	{{ 0, t_LP }, {SHIFT, 9 }}, {{ 37, t_MINUS }, {REDUCE, 4 }},
	{{ 28, t_DIVIDE }, {REDUCE, 4 }}, {{ 13, t_MINUS }, {SHIFT, 10 }},
	{{ 37, t_PLUS }, {REDUCE, 4 }}, {{ 3, t_TIMES }, {REDUCE, 2 }},
	{{ 5, t_EOF }, {REDUCE, 2 }}, {{ 37, t_RP }, {REDUCE, 4 }},
	{{ 28, t_PLUS }, {REDUCE, 4 }}, {{ 37, t_DIVIDE }, {SHIFT, 31 }},
	{{ 37, t_TIMES }, {SHIFT, 34 }}, {{ 3, t_PLUS }, {REDUCE, 2 }},
	{{ 9, t_NUMBER }, {SHIFT, 15 }}, {{ 16, t_MINUS }, {SHIFT, 33 }},
	{{ 1, t_DIVIDE }, {SHIFT, 11 }}, {{ 41, t_MINUS }, {REDUCE, 5 }},
	{{ 39, t_TIMES }, {REDUCE, 0 }}, {{ 1, t_PLUS }, {SHIFT, 12 }},
	{{ 1, t_EOF }, {ACCEPT, 0 }}, {{ 41, t_PLUS }, {REDUCE, 5 }},
	{{ 16, t_DIVIDE }, {SHIFT, 31 }}, {{ 15, t_TIMES }, {REDUCE, 2 }},
	{{ 1, t_MINUS }, {SHIFT, 13 }}, {{ 39, t_PLUS }, {REDUCE, 0 }},
	{{ 15, t_MINUS }, {REDUCE, 2 }}, {{ 1, t_TIMES }, {SHIFT, 14 }},
	{{ 22, t_TIMES }, {REDUCE, 2 }}, {{ 21, t_NUMBER }, {SHIFT, 15 }},
	{{ 2, t_TIMES }, {REDUCE, 2 }}, {{ 36, t_PLUS }, {REDUCE, 7 }},
	{{ 22, t_RP }, {REDUCE, 2 }}, {{ 4, t_EOF }, {REDUCE, 2 }},
	{{ 36, t_RP }, {REDUCE, 7 }}, {{ 22, t_PLUS }, {REDUCE, 2 }},
	{{ 3, t_MINUS }, {REDUCE, 2 }}, {{ 28, t_TIMES }, {SHIFT, 14 }},
	{{ 3, t_DIVIDE }, {REDUCE, 2 }}, {{ 9, t_LP }, {SHIFT, 21 }},
	{{ 2, t_EOF }, {REDUCE, 2 }}, {{ 29, t_TIMES }, {REDUCE, 0 }},
	{{ 19, t_MINUS }, {REDUCE, 2 }}, {{ 10, t_NUMBER }, {SHIFT, 3 }},
	{{ 2, t_DIVIDE }, {REDUCE, 2 }}, {{ 2, t_PLUS }, {REDUCE, 2 }},
	{{ 2, t_MINUS }, {REDUCE, 2 }}, {{ 10, t_LP }, {SHIFT, 9 }},
	{{ 27, t_DIVIDE }, {REDUCE, 6 }}, {{ 4, t_TIMES }, {REDUCE, 2 }},
	{{ 10, t_MINUS }, {SHIFT, 10 }}, {{ 6, t_EOF }, {REDUCE, 2 }},
	{{ 6, t_TIMES }, {REDUCE, 2 }}, {{ 23, t_DIVIDE }, {REDUCE, 2 }},
	{{ 8, t_TIMES }, {REDUCE, 2 }}, {{ 6, t_MINUS }, {REDUCE, 2 }},
	{{ 6, t_PLUS }, {REDUCE, 2 }}, {{ 14, t_NUMBER }, {SHIFT, 3 }},
	{{ 6, t_DIVIDE }, {REDUCE, 2 }}, {{ 14, t_LP }, {SHIFT, 9 }},
	{{ 14, t_MINUS }, {SHIFT, 10 }}, {{ 4, t_PLUS }, {REDUCE, 2 }},
	{{ 27, t_TIMES }, {REDUCE, 6 }}, {{ 21, t_MINUS }, {SHIFT, 24 }},
	{{ 4, t_DIVIDE }, {REDUCE, 2 }}, {{ 4, t_MINUS }, {REDUCE, 2 }},
	{{ 35, t_PLUS }, {SHIFT, 30 }}, {{ 11, t_MINUS }, {SHIFT, 10 }},
	{{ 5, t_TIMES }, {REDUCE, 2 }}, {{ 35, t_TIMES }, {SHIFT, 34 }},
	{{ 5, t_PLUS }, {REDUCE, 2 }}, {{ 20, t_DIVIDE }, {REDUCE, 2 }},
	{{ 5, t_MINUS }, {REDUCE, 2 }}, {{ 8, t_PLUS }, {REDUCE, 2 }},
	{{ 25, t_MINUS }, {REDUCE, 7 }}, {{ 23, t_TIMES }, {REDUCE, 2 }},
	{{ 8, t_DIVIDE }, {REDUCE, 2 }}, {{ 25, t_DIVIDE }, {REDUCE, 7 }},
	{{ 8, t_MINUS }, {REDUCE, 2 }}, {{ 18, t_MINUS }, {REDUCE, 2 }},
	{{ 11, t_NUMBER }, {SHIFT, 3 }}, {{ 11, t_LP }, {SHIFT, 9 }},
	{{ 12, t_NUMBER }, {SHIFT, 3 }}, {{ 29, t_DIVIDE }, {SHIFT, 11 }},
	{{ 12, t_MINUS }, {SHIFT, 10 }}, {{ 12, t_LP }, {SHIFT, 9 }},
	{{ 26, t_TIMES }, {REDUCE, 3 }}, {{ 13, t_NUMBER }, {SHIFT, 3 }},
	{{ 13, t_LP }, {SHIFT, 9 }}, {{ 41, t_TIMES }, {REDUCE, 5 }},
	{{ 39, t_MINUS }, {REDUCE, 0 }}, {{ 15, t_PLUS }, {REDUCE, 2 }},
	{{ 15, t_RP }, {REDUCE, 2 }}, {{ 16, t_TIMES }, {SHIFT, 34 }},
	{{ 15, t_DIVIDE }, {REDUCE, 2 }}, {{ 41, t_DIVIDE }, {REDUCE, 5 }},
	{{ 16, t_PLUS }, {SHIFT, 30 }}, {{ 16, t_RP }, {SHIFT, 32 }},
	{{ 31, t_LP }, {SHIFT, 21 }}, {{ 17, t_RP }, {REDUCE, 2 }},
	{{ 17, t_MINUS }, {REDUCE, 2 }}, {{ 17, t_PLUS }, {REDUCE, 2 }},
	{{ 31, t_MINUS }, {SHIFT, 24 }}, {{ 17, t_TIMES }, {REDUCE, 2 }},
	{{ 18, t_DIVIDE }, {REDUCE, 2 }}, {{ 18, t_RP }, {REDUCE, 2 }},
	{{ 18, t_TIMES }, {REDUCE, 2 }}, {{ 18, t_PLUS }, {REDUCE, 2 }},
	{{ 20, t_RP }, {REDUCE, 2 }}, {{ 20, t_PLUS }, {REDUCE, 2 }},
	{{ 20, t_TIMES }, {REDUCE, 2 }}, {{ 19, t_RP }, {REDUCE, 2 }},
	{{ 19, t_DIVIDE }, {REDUCE, 2 }}, {{ 19, t_PLUS }, {REDUCE, 2 }},
	{{ 29, t_MINUS }, {REDUCE, 0 }}, {{ 19, t_TIMES }, {REDUCE, 2 }},
	{{ 21, t_LP }, {SHIFT, 21 }}, {{ 23, t_RP }, {REDUCE, 2 }},
	{{ 25, t_TIMES }, {REDUCE, 7 }}, {{ 23, t_MINUS }, {REDUCE, 2 }},
	{{ 23, t_PLUS }, {REDUCE, 2 }}, {{ 24, t_LP }, {SHIFT, 21 }},
	{{ 24, t_NUMBER }, {SHIFT, 15 }}, {{ 24, t_MINUS }, {SHIFT, 24 }},
	{{ 25, t_EOF }, {REDUCE, 7 }}, {{ 25, t_PLUS }, {REDUCE, 7 }},
	{{ 26, t_PLUS }, {REDUCE, 3 }}, {{ 26, t_DIVIDE }, {REDUCE, 3 }},
	{{ 26, t_EOF }, {REDUCE, 3 }}, {{ 26, t_MINUS }, {REDUCE, 3 }},
	{{ 39, t_RP }, {REDUCE, 0 }}, {{ 39, t_DIVIDE }, {REDUCE, 0 }},
	{{ 27, t_PLUS }, {REDUCE, 6 }}, {{ 27, t_MINUS }, {REDUCE, 6 }},
	{{ 27, t_EOF }, {REDUCE, 6 }}, {{ 28, t_EOF }, {REDUCE, 4 }},
	{{ 28, t_MINUS }, {REDUCE, 4 }}, {{ 29, t_PLUS }, {REDUCE, 0 }},
	{{ 29, t_EOF }, {REDUCE, 0 }}, {{ 30, t_NUMBER }, {SHIFT, 15 }},
	{{ 30, t_MINUS }, {SHIFT, 24 }}, {{ 30, t_LP }, {SHIFT, 21 }},
	{{ 31, t_NUMBER }, {SHIFT, 15 }}, {{ 32, t_PLUS }, {REDUCE, 5 }},
	{{ 32, t_TIMES }, {REDUCE, 5 }}, {{ 32, t_EOF }, {REDUCE, 5 }},
	{{ 32, t_DIVIDE }, {REDUCE, 5 }}, {{ 32, t_MINUS }, {REDUCE, 5 }},
	{{ 33, t_LP }, {SHIFT, 21 }}, {{ 33, t_NUMBER }, {SHIFT, 15 }},
	{{ 33, t_MINUS }, {SHIFT, 24 }}, {{ 34, t_NUMBER }, {SHIFT, 15 }},
	{{ 34, t_LP }, {SHIFT, 21 }}, {{ 34, t_MINUS }, {SHIFT, 24 }},
	{{ 35, t_DIVIDE }, {SHIFT, 31 }}, {{ 35, t_RP }, {SHIFT, 41 }},
	{{ 35, t_MINUS }, {SHIFT, 33 }}, {{ 36, t_DIVIDE }, {REDUCE, 7 }},
	{{ 36, t_MINUS }, {REDUCE, 7 }}, {{ 36, t_TIMES }, {REDUCE, 7 }},
	{{ 38, t_DIVIDE }, {REDUCE, 6 }}, {{ 38, t_RP }, {REDUCE, 6 }},
	{{ 38, t_PLUS }, {REDUCE, 6 }}, {{ 41, t_RP }, {REDUCE, 5 }}
};

std::unordered_map<std::pair<size_t, std::string_view>, size_t, PairHash> gotoTable {
	{{ 0, *strings.find("Expression") }, {1}}, {{ 0, *strings.find("Grouping") }, {6}},
	{{ 0, *strings.find("Add") }, {4}}, {{ 21, *strings.find("Mul") }, {22}},
	{{ 0, *strings.find("Sub") }, {7}}, {{ 0, *strings.find("Mul") }, {2}},
	{{ 21, *strings.find("Add") }, {20}}, {{ 9, *strings.find("Div") }, {19}},
	{{ 24, *strings.find("Unary") }, {23}}, {{ 9, *strings.find("Mul") }, {22}},
	{{ 0, *strings.find("Div") }, {5}}, {{ 24, *strings.find("Div") }, {19}},
	{{ 0, *strings.find("Unary") }, {8}}, {{ 14, *strings.find("Grouping") }, {6}},
	{{ 9, *strings.find("Expression") }, {16}}, {{ 21, *strings.find("Div") }, {19}},
	{{ 13, *strings.find("Unary") }, {8}}, {{ 9, *strings.find("Add") }, {20}},
	{{ 14, *strings.find("Expression") }, {26}}, {{ 9, *strings.find("Grouping") }, {17}},
	{{ 12, *strings.find("Unary") }, {8}}, {{ 9, *strings.find("Sub") }, {18}},
	{{ 13, *strings.find("Add") }, {4}}, {{ 24, *strings.find("Mul") }, {22}},
	{{ 12, *strings.find("Sub") }, {7}}, {{ 9, *strings.find("Unary") }, {23}},
	{{ 13, *strings.find("Grouping") }, {6}}, {{ 10, *strings.find("Expression") }, {25}},
	{{ 11, *strings.find("Sub") }, {7}}, {{ 14, *strings.find("Unary") }, {8}},
	{{ 10, *strings.find("Add") }, {4}}, {{ 13, *strings.find("Expression") }, {29}},
	{{ 10, *strings.find("Grouping") }, {6}}, {{ 11, *strings.find("Add") }, {4}},
	{{ 10, *strings.find("Sub") }, {7}}, {{ 10, *strings.find("Mul") }, {2}},
	{{ 14, *strings.find("Add") }, {4}}, {{ 10, *strings.find("Unary") }, {8}},
	{{ 10, *strings.find("Div") }, {5}}, {{ 11, *strings.find("Unary") }, {8}},
	{{ 14, *strings.find("Sub") }, {7}}, {{ 14, *strings.find("Mul") }, {2}},
	{{ 14, *strings.find("Div") }, {5}}, {{ 12, *strings.find("Grouping") }, {6}},
	{{ 11, *strings.find("Expression") }, {27}}, {{ 12, *strings.find("Expression") }, {28}},
	{{ 11, *strings.find("Grouping") }, {6}}, {{ 11, *strings.find("Mul") }, {2}},
	{{ 11, *strings.find("Div") }, {5}}, {{ 13, *strings.find("Sub") }, {7}},
	{{ 12, *strings.find("Add") }, {4}}, {{ 24, *strings.find("Sub") }, {18}},
	{{ 12, *strings.find("Mul") }, {2}}, {{ 12, *strings.find("Div") }, {5}},
	{{ 13, *strings.find("Mul") }, {2}}, {{ 24, *strings.find("Add") }, {20}},
	{{ 21, *strings.find("Unary") }, {23}}, {{ 13, *strings.find("Div") }, {5}},
	{{ 21, *strings.find("Expression") }, {35}}, {{ 21, *strings.find("Grouping") }, {17}},
	{{ 21, *strings.find("Sub") }, {18}}, {{ 24, *strings.find("Expression") }, {36}},
	{{ 24, *strings.find("Grouping") }, {17}}, {{ 30, *strings.find("Expression") }, {37}},
	{{ 31, *strings.find("Sub") }, {18}}, {{ 30, *strings.find("Add") }, {20}},
	{{ 30, *strings.find("Grouping") }, {17}}, {{ 31, *strings.find("Add") }, {20}},
	{{ 30, *strings.find("Sub") }, {18}}, {{ 30, *strings.find("Mul") }, {22}},
	{{ 30, *strings.find("Unary") }, {23}}, {{ 30, *strings.find("Div") }, {19}},
	{{ 31, *strings.find("Expression") }, {38}}, {{ 31, *strings.find("Grouping") }, {17}},
	{{ 31, *strings.find("Mul") }, {22}}, {{ 31, *strings.find("Unary") }, {23}},
	{{ 31, *strings.find("Div") }, {19}}, {{ 33, *strings.find("Expression") }, {39}},
	{{ 33, *strings.find("Add") }, {20}}, {{ 33, *strings.find("Grouping") }, {17}},
	{{ 33, *strings.find("Sub") }, {18}}, {{ 33, *strings.find("Mul") }, {22}},
	{{ 33, *strings.find("Unary") }, {23}}, {{ 33, *strings.find("Div") }, {19}},
	{{ 34, *strings.find("Expression") }, {40}}, {{ 34, *strings.find("Add") }, {20}},
	{{ 34, *strings.find("Grouping") }, {17}}, {{ 34, *strings.find("Sub") }, {18}},
	{{ 34, *strings.find("Mul") }, {22}}, {{ 34, *strings.find("Unary") }, {23}},
	{{ 34, *strings.find("Div") }, {19}}
};