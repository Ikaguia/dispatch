#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

namespace JSONish {
	struct Token {
		enum Type { IDENT, STRING, NUMBER, SYMBOL, END } type;

		Token(Type type, std::string text, int line, int col) : type{type}, text{text}, line{line}, col{col} {};

		std::string text = "";
		int line = 1;
		int col = 1;
	};

	struct Node {
		enum Type { STRING, NUMBER, BOOL, ARRAY, OBJECT, NIL } type = NIL;

		Node(const std::string& s) : type{STRING}, sval{s} {};
		Node(int i) : type{NUMBER}, nval{(double)i} {};
		Node(double d) : type{NUMBER}, nval{d} {};
		Node(bool b) : type{BOOL}, bval{b} {};
		Node(Type type=NIL) : type{type} {};

		std::string sval;
		double nval = 0;
		bool bval = false;

		std::vector<Node> arr;
		std::map<std::string, Node> obj;

		std::string toString(int indent = 0) const;
	};

	std::vector<Token> tokenize(const std::string& src);

	class Parser {
	public:
		Parser(const std::string& src);
		Parser(const std::vector<Token>& tokens);

		Node parseValue();
		Node parseObject();
		Node parseArray();

	private:
		const std::vector<Token> t;
		size_t i = 0;

		const Token& peek() const;
		const Token& next();
		bool match(const std::string& s);

		[[noreturn]] void error(const std::string& msg) const;
	};
}