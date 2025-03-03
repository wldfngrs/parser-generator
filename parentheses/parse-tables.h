#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <vector>

std::unordered_set<std::string> strings {
	"Goal", "Pair", "List"
};

enum TokenType {
	t_RP, t_EOF, t_LP
};

std::vector<std::pair<std::string_view, size_t>> reduce_info {
	{ *strings.find("Goal"), 1 }, { *strings.find("Pair"), 3 },
	{ *strings.find("List"), 2 }, { *strings.find("List"), 1 },
	{ *strings.find("Pair"), 2 }
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
	{{ 6, t_EOF }, {REDUCE, 4 }}, {{ 0, t_LP }, {SHIFT, 2 }},
	{{ 8, t_RP }, {SHIFT, 10 }}, {{ 6, t_LP }, {REDUCE, 4 }},
	{{ 11, t_LP }, {REDUCE, 1 }}, {{ 5, t_RP }, {SHIFT, 11 }},
	{{ 11, t_EOF }, {REDUCE, 1 }}, {{ 4, t_EOF }, {REDUCE, 2 }},
	{{ 10, t_RP }, {REDUCE, 1 }}, {{ 4, t_LP }, {REDUCE, 2 }},
	{{ 7, t_LP }, {SHIFT, 7 }}, {{ 9, t_RP }, {REDUCE, 4 }},
	{{ 3, t_EOF }, {REDUCE, 3 }}, {{ 3, t_LP }, {REDUCE, 3 }},
	{{ 1, t_EOF }, {ACCEPT, 0 }}, {{ 1, t_LP }, {SHIFT, 2 }},
	{{ 2, t_RP }, {SHIFT, 6 }}, {{ 2, t_LP }, {SHIFT, 7 }},
	{{ 7, t_RP }, {SHIFT, 9 }}
};

std::unordered_map<std::pair<size_t, std::string_view>, size_t, PairHash> gotoTable {
	{{ 0, *strings.find("List") }, {1}}, {{ 0, *strings.find("Pair") }, {3}},
	{{ 1, *strings.find("Pair") }, {4}}, {{ 2, *strings.find("Pair") }, {5}},
	{{ 7, *strings.find("Pair") }, {8}}
};