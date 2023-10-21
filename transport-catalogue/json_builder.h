#pragma once

#include "json.h"

#include <deque>
#include <optional>

namespace json {

class BaseContext;
class DictValueContext;
class DictItemContext;
class ArrayContext;

class Builder {
public:
    Builder() { }
    Node Build();
    BaseContext Value(Node::Value value);

    DictItemContext StartDict();
    DictValueContext Key(std::string value);
    BaseContext EndDict();

    ArrayContext StartArray();
    BaseContext EndArray();

protected:
    Node root_ = nullptr;
    std::deque<Node*> nodes_stack_;
    std::optional<std::string> dict_key_ = std::nullopt;
};

class BaseContext {
public:
    BaseContext(Builder& builder) : builder_(builder) { }

    DictItemContext StartDict();
    ArrayContext StartArray();
    BaseContext EndDict();
    BaseContext EndArray();

    Node Build();
    DictValueContext Key(std::string value);
    BaseContext Value(Node::Value value);

private:
    Builder& builder_;  
};

class DictItemContext : BaseContext {
public:
    DictItemContext(BaseContext base) : BaseContext(base) { }

    DictValueContext Key(std::string value);
    BaseContext EndDict();

    Node Build() = delete;
    BaseContext EndArray() = delete;
    BaseContext Value(Node::Value value) = delete;
    DictItemContext StartDict() = delete;
    ArrayContext StartArray() = delete;
};

class DictValueContext : BaseContext {
public:
    DictValueContext(BaseContext base) : BaseContext(base) { }

    DictItemContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayContext StartArray();

    Node Build() = delete;
    DictValueContext Key(std::string value) = delete;
    BaseContext EndArray() = delete;
    BaseContext EndDict() = delete;
};

class ArrayContext : BaseContext {
public:
    ArrayContext(BaseContext base) : BaseContext(base) { }

    ArrayContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayContext StartArray();
    BaseContext EndArray();

    Node Build() = delete;
    DictValueContext Key(std::string value) = delete;
    BaseContext EndDict() = delete;
};

}

