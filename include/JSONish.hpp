#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <Utils.hpp>

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

		template <typename T>
		T as() const;

		template <typename T>
		T get(const std::string& key, T def) const {
			if (type != OBJECT) throw std::invalid_argument("Node is not an object, tried reading optional key '" + key + "'");
			auto it = obj.find(key);
			if (it == obj.end()) return def;
			return it->second.as<T>();
		}
		template <typename T>
		T get(int idx, T def) const {
			if (type != ARRAY) throw std::invalid_argument("Node is not an array, tried reading optional index " + std::to_string(idx));
			if (idx < 0 || idx >= (int)arr.size()) return def;
			return arr[idx].as<T>();
		}

		template<typename T>
		T get(const std::string& key) const {
			if (type != OBJECT) throw std::runtime_error("Node is not an object, tried reading required key '" + key + "'");

			auto it = obj.find(key);
			if (it == obj.end()) {
				Utils::println("Missing required key: '{}'", key);
				throw std::runtime_error("Missing required key: '" + key + "'");
			}

			return it->second.as<T>();
		}
		template<typename T>
		T get(int idx) const {
			if (type != ARRAY) throw std::runtime_error("Node is not an array, tried reading required index " + std::to_string(idx));

			if (idx < 0 || idx >= (int)arr.size()) throw std::runtime_error("Missing required array element at index " + std::to_string(idx));

			return arr[idx].as<T>();
		}

		bool has(const std::string key) const {
			if (type != OBJECT) throw std::runtime_error("Node is not an object, tried checking key '" + key + "'");
			return obj.count(key);
		}

		const Node& operator[](const char* key) const { return obj.at(key); }
		const Node& operator[](const std::string& key) const { return obj.at(key); }
		const Node& operator[](int idx) const { return arr[idx]; }
		operator bool() const { return type != NIL; }
	};

	template <>
	inline int Node::as<int>() const {
		if (type != Node::NUMBER) throw std::invalid_argument("Node is not a number");
		return static_cast<int>(nval);
	}

	template <>
	inline float Node::as<float>() const {
		if (type != Node::NUMBER) throw std::invalid_argument("Node is not a number");
		return static_cast<float>(nval);
	}

	template <>
	inline double Node::as<double>() const {
		if (type != Node::NUMBER) throw std::invalid_argument("Node is not a number");
		return nval;
	}

	template <>
	inline bool Node::as<bool>() const {
		if (type != Node::BOOL) throw std::invalid_argument("Node is not a boolean");
		return bval;
	}

	template <>
	inline std::string Node::as<std::string>() const {
		if (type != Node::STRING) throw std::invalid_argument("Node is not a string");
		return sval;
	}



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