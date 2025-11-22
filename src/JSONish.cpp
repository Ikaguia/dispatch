#include <sstream>
#include <cctype>
#include <format>

#include <JSONish.hpp>
#include <Utils.hpp>

//  ERROR UTILITY
void JSONish::Parser::error(const std::string& msg) const {
	const auto& tk = t[i < t.size() ? i : t.size() - 1];
	throw std::runtime_error(std::format("JSON-ish parse error at line {}, col {}: {} (token: '{}')", tk.line, tk.col, msg, tk.text));
}

//  TOKENIZER
std::vector<JSONish::Token> JSONish::tokenize(const std::string& src) {
	std::vector<JSONish::Token> out;
	size_t i = 0;
	int line = 1, col = 1;

	auto push = [&](JSONish::Token::Type t, std::string s) { out.emplace_back(t, s, line, col); };

	while (i < src.size()) {
		char c = src[i];

		// whitespace
		if (c == ' ' || c == '\t' || c == '\r') {
			i++; col++;
			continue;
		}
		if (c == '\n') {
			i++; line++; col = 1;
			continue;
		}

		// comments
		if (c == '/' && i + 1 < src.size() && src[i+1] == '/') {
			// // comment
			i += 2; col += 2;
			while (i < src.size() && src[i] != '\n') { i++; col++; }
			continue;
		}
		if (c == '#') {
			// # comment
			i++; col++;
			while (i < src.size() && src[i] != '\n') { i++; col++; }
			continue;
		}

		// symbols
		if (c == '{' || c == '}' || c == '[' || c == ']' ||
			c == ':' || c == ',') {
			push(JSONish::Token::SYMBOL, std::string(1,c));
			i++; col++;
			continue;
		}

		// string
		if (c == '"') {
			size_t j = i + 1;
			std::string s;
			while (j < src.size() && src[j] != '"') {
				if (src[j] == '\\' && j + 1 < src.size()) {
					j++;
					s += src[j];
				} else {
					s += src[j];
				}
				j++;
			}
			if (j >= src.size()) throw std::runtime_error("Unterminated string literal");

			push(JSONish::Token::STRING, s);
			col += (j - i + 1);
			i = j + 1;
			continue;
		}

		// number
		if (isdigit(c) || c == '-' || c == '+') {
			size_t j = i;
			while (j < src.size() && (isdigit(src[j]) || src[j] == '.' || src[j] == '-' || src[j] == '+')) j++;
			push(JSONish::Token::NUMBER, src.substr(i, j - i));
			col += (j - i);
			i = j;
			continue;
		}

		// identifier
		if (isalpha(c)) {
			size_t j = i;
			while (j < src.size() && (isalnum(src[j]) || src[j]=='_' || src[j]=='-')) j++;
			push(JSONish::Token::IDENT, src.substr(i, j - i));
			col += (j - i);
			i = j;
			continue;
		}

		// unknown
		std::ostringstream ss;
		ss << "Unexpected character '" << c << "'";
		throw std::runtime_error(ss.str());
	}

	push(JSONish::Token::END, "");

	return out;
}

//  PARSER CORE

JSONish::Parser::Parser(const std::vector<JSONish::Token>& tokens) : t(tokens) {}
JSONish::Parser::Parser(const std::string& src) : t(JSONish::tokenize(src)) {}

const JSONish::Token& JSONish::Parser::peek() const {
	if (i >= t.size()) return t.back();
	return t[i];
}

const JSONish::Token& JSONish::Parser::next() {
	if (i >= t.size()) return t.back();
	return t[i++];
}

bool JSONish::Parser::match(const std::string& s) {
	if (peek().text == s) { i++; return true; }
	return false;
}

//  VALUE
JSONish::Node JSONish::Parser::parseValue() {
	const auto& tk = peek();

	if (tk.type == JSONish::Token::STRING) {
		next();
		return {tk.text};
	}

	if (tk.type == JSONish::Token::NUMBER) {
		next();
		return {atof(tk.text.c_str())};
	}

	if (tk.type == JSONish::Token::IDENT) {
		if (tk.text == "true" || tk.text == "false") {
			next();
			return {tk.text == "true"};
		}
		error("Unexpected identifier");
	}

	if (tk.text == "{") return parseObject();
	if (tk.text == "[") return parseArray();

	error("Unexpected token in value");
}

//  OBJECT

JSONish::Node JSONish::Parser::parseObject() {
	JSONish::Node n{JSONish::Node::OBJECT};
	next(); // {

	while (!match("}")) {
		const auto& key = peek();
		if (key.type != JSONish::Token::IDENT && key.type != JSONish::Token::STRING) error("Expected object key");

		std::string k = next().text;

		if (!match(":")) error("Expected ':' after key");

		n.obj[k] = parseValue();

		// optional comma
		match(",");
	}

	return n;
}

//  ARRAY
JSONish::Node JSONish::Parser::parseArray() {
	JSONish::Node n{JSONish::Node::ARRAY};
	next(); // [

	while (!match("]")) {
		n.arr.push_back(parseValue());
		match(",");
	}

	return n;
}

//  PRETTY PRINTER
static std::string indentStr(int n) { return std::string(n, ' '); }

std::string JSONish::Node::toString(int indent) const {
	std::ostringstream out;

	switch (type) {
		case STRING:
			out << '"' << sval << '"';
			break;

		case NUMBER:
			out << nval;
			break;

		case BOOL:
			out << (bval ? "true" : "false");
			break;

		case ARRAY: {
			out << "[\n";
			for (size_t i = 0; i < arr.size(); i++) {
				out << indentStr(indent+2) << arr[i].toString(indent+2);
				if (i + 1 < arr.size()) out << ",";
				out << "\n";
			}
			out << indentStr(indent) << "]";
			break;
		}

		case OBJECT: {
			out << "{\n";
			size_t nCount = 0;
			for (auto& [k, v] : obj) {
				out << indentStr(indent+2) << k << ": " << v.toString(indent+2);
				if (++nCount < obj.size()) out << ",";
				out << "\n";
			}
			out << indentStr(indent) << "}";
			break;
		}

		case NIL:
			out << "null";
			break;
	}

	return out.str();
}
