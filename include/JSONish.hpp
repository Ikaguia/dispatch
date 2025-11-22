#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

struct JToken {
	enum Type { IDENT, STRING, NUMBER, SYMBOL, END } type;

	JToken(Type type, std::string text, int line, int col) : type{type}, text{text}, line{line}, col{col} {};

	std::string text = "";
	int line = 1;
	int col = 1;
};

struct JNode {
	enum Type { STRING, NUMBER, BOOL, ARRAY, OBJECT, NIL } type = NIL;

	JNode(const std::string& s) : type{STRING}, sval{s} {};
	JNode(int i) : type{NUMBER}, nval{(double)i} {};
	JNode(double d) : type{NUMBER}, nval{d} {};
	JNode(bool b) : type{BOOL}, bval{b} {};
	JNode(Type type=NIL) : type{type} {};

	std::string sval;
	double nval = 0;
	bool bval = false;

	std::vector<JNode> arr;
	std::map<std::string, JNode> obj;

	std::string toString(int indent = 0) const;
};

std::vector<JToken> jsonishTokenize(const std::string& src);

class JParser {
public:
	JParser(const std::string& src);
	JParser(const std::vector<JToken>& tokens);

	JNode parseValue();
	JNode parseObject();
	JNode parseArray();

private:
	const std::vector<JToken> t;
	size_t i = 0;

	const JToken& peek() const;
	const JToken& next();
	bool match(const std::string& s);

	[[noreturn]] void error(const std::string& msg) const;
};
