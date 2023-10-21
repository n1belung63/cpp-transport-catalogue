#include "json.h"

#include <cmath>
#include <sstream>
#include <iostream>
#include <string>

using namespace std;

namespace json {

namespace {
  
using namespace std::literals;

Node LoadNode(istream& input);
Node LoadString(std::istream& input);

std::string LoadLiteral(std::istream& input) {
    std::string s;
    while (std::isalpha(input.peek())) {
        s.push_back(static_cast<char>(input.get()));
    }
    return s;
}

Node LoadArray(istream& input) {
    Array result;

    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }

        result.push_back(LoadNode(input));
    }

    if (!input) {
        throw ParsingError("Array parsing error"s);
    }

    return Node(move(result));
}

Node LoadNumber(std::istream& input) {
    std::string parsed_num;

    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    if (input.peek() == '0') {
        read_char();
    } else {
        read_digits();
    }

    bool is_int = true;
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            try {
                return Node(std::stoi(parsed_num));
            } catch (...) {
            }
        }
        return Node(std::stod(parsed_num)); 
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadNull(istream& input) {
    string line = LoadLiteral(input);
    if (line == "null"s) {
        return Node(nullptr);
    }
    else {
        throw ParsingError("Null parsing error");
    } 
}

Node LoadString(istream& input) {
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string str;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    str.push_back('\n');
                    break;
                case 't':
                    str.push_back('\t');
                    break;
                case 'r':
                    str.push_back('\r');
                    break;
                case '"':
                    str.push_back('"');
                    break;
                case '\\':
                    str.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line"s);
        } else {
            str.push_back(ch);
        }
        ++it;
    }
    return Node(str);
}

Node LoadBool(istream& input) {
    string line = LoadLiteral(input);

    if (line == "true"s) {
        return Node(true);
    }
    else if (line == "false"s) {
        return Node(false);
    }
    else {
        throw ParsingError("Bool parsing error");
    } 
}

Node LoadDict(istream& input) {
    Dict result;

    char c;
    for (;input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }

    if (!input) {
        throw ParsingError("Dictionary parsing error"s);
    }

    return Node(move(result));
}

Node LoadNode(istream& input) {
    char c;
    if (!(input >> c)) {
        throw ParsingError("Unexpected EOF"s);
    }

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else if (c == '"') {
        return LoadString(input);
    } else {
        input.putback(c);
        return LoadNumber(input);
    }
}

std::string EscapeString(const std::string & inp) {
    std::string str;
    str.append("\""s);
    for (char c : inp) {
        switch (c) {
            case '\\':
                str.append("\\\\"s);
                break;
            case '\"':
                str.append("\\\""s);
                break;
            case '\'':
                str.append("\\\'"s);
                break;
            case '\n':
                str.append("\\n"s);
                break;
            case '\r':
                str.append("\\r"s);
                break;
            case '\t':
                str.append("\t"s);
                break;        
            default:
                str.push_back(c);
        }
    }
    str.append("\""s);

    return str;
}

void PrintNode(const Node& node, std::ostream& out);

void PrintValue(std::nullptr_t, std::ostream& out) {
    out << "null"sv;
}
void PrintValue(const Array& value, std::ostream& out) {
    bool is_first = true;
    out << "["sv;
    for (const auto& el : value) {
        if (is_first) {
            is_first = false;    
        }
        else {
            out << ","sv;   
        }     
        PrintNode(el, out);       
    }   
    out << "]"sv;
}
void PrintValue(const Dict& value, std::ostream& out) {
    bool is_first = true;
    out << "{"sv;
    for (const auto& [key, val] : value) {
        if (is_first) {
            is_first = false;    
        }
        else {
            out << ","sv;   
        }
        out << EscapeString(key) << ": "sv;
        PrintNode(val, out);
    }   
    out << "}"sv;
}
void PrintValue(bool value, std::ostream& out) {
    if (value) {
        out << "true"sv;
    }
    else {
        out << "false"sv;
    }   
}
void PrintValue(double value, std::ostream& out) {
    out << value;
}
void PrintValue(int value, std::ostream& out) {
    out << value;
}
void PrintValue(const std::string& str, std::ostream& out) {
    out << EscapeString(str);
}

void PrintNode(const Node& node, std::ostream& out) {
    std::visit([&out](const auto& value){ PrintValue(value, out); }, node.GetValue());
}

}  // namespace

bool Node::IsInt() const {
    return std::holds_alternative<int>(*this);
}
int Node::AsInt() const {
    using namespace std::literals;
    if (!IsInt()) {
        throw std::logic_error("Not an int"s);   
    }
    return std::get<int>(*this);
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(*this);
}

bool Node::IsDouble() const {
    return IsPureDouble() || IsInt();
}
double Node::AsDouble() const {
    using namespace std::literals;
    if (!IsDouble()) {
        throw std::logic_error("Not a double"s);
    }
    return IsPureDouble() ? std::get<double>(*this) : AsInt();
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(*this);
}
bool Node::AsBool() const {
    using namespace std::literals;
    if (!IsBool()) {
        throw std::logic_error("Not a bool");     
    }
    return std::get<bool>(*this);     
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(*this);
}
const Array& Node::AsArray() const {
    using namespace std::literals;
    if (!IsArray()) {
        throw std::logic_error("Not an array"s);      
    }
    return std::get<Array>(*this);
}
Array& Node::AsArray() {
    using namespace std::literals;
    if (!IsArray()) {
        throw std::logic_error("Not an array"s);
    }

    return std::get<Array>(*this);
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(*this);
}
const std::string& Node::AsString() const {
    using namespace std::literals;
    if (!IsString()) {
        throw std::logic_error("Not a string"s);
    }
    return std::get<std::string>(*this); 
}

bool Node::IsMap() const {
    return std::holds_alternative<Dict>(*this);
}
const Dict& Node::AsMap() const {
    using namespace std::literals;
    if (!IsMap()) {
        throw std::logic_error("Not a dictionary");
    }
    return std::get<Dict>(*this); 
}
Dict& Node::AsMap() {
    using namespace std::literals;
    if (!IsMap()) {
        throw std::logic_error("Not a dictionary");
    }
    return std::get<Dict>(*this); 
}

const Node::Value& Node::GetValue() const { return *this; }

Node::Value& Node::GetValue() { return *this; }

bool Node::operator==(const Node& rhs) const {
    return GetValue() == rhs.GetValue();
}

Document::Document(Node root) : root_(std::move(root)) { }

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

}  // namespace json