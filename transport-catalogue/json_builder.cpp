#include "json_builder.h"

namespace json {
using namespace std::literals;

Node Builder::Build() {
    if (nodes_stack_.size() > 1 || root_ == nullptr) {
        throw std::logic_error("Impossible to build"s);
    }
    return root_;
}

BaseContext Builder::Value(Node::Value value) {
    if (!(nodes_stack_.empty() || nodes_stack_.back()->IsArray() || (nodes_stack_.back()->IsMap() && dict_key_.has_value()))) {
        throw std::logic_error("Impossible to add value"s);
    }

    if (nodes_stack_.empty()) {
        root_.GetValue() = value;
        nodes_stack_.push_back(&root_);
    }

    else if (nodes_stack_.back()->IsMap() && dict_key_.has_value()) {
        nodes_stack_.back()->AsMap()[dict_key_.value()].GetValue() = value;
        dict_key_.reset();
    }

    else if(nodes_stack_.back()->IsArray()) {
        Node& last_array_node_ref = (nodes_stack_.back()->AsArray()).emplace_back(Node());
        last_array_node_ref.GetValue() = value;
    }

    return BaseContext(*this);
}

DictItemContext Builder::StartDict() {
    if (!(nodes_stack_.empty() || nodes_stack_.back()->IsArray() || (nodes_stack_.back()->IsMap() && dict_key_.has_value()))) {
        throw std::logic_error("Impossible start dict from here"s);
    }

    if (nodes_stack_.empty()) {
        root_ = Dict();
        nodes_stack_.push_back(&root_);     
    }

    else if (nodes_stack_.back()->IsMap() && dict_key_.has_value()) {
        Node& last_dict_value_ref = nodes_stack_.back()->AsMap()[dict_key_.value()];
        last_dict_value_ref = Dict();
        nodes_stack_.push_back(&last_dict_value_ref);
        dict_key_.reset();
    }

    else if(nodes_stack_.back()->IsArray()) {
        Node& last_array_node_ref = (nodes_stack_.back()->AsArray()).emplace_back(Dict());
        nodes_stack_.push_back(&last_array_node_ref);
    }

    return DictItemContext(*this);
}

DictValueContext Builder::Key(std::string value) {
    if (dict_key_.has_value()) {
        throw std::logic_error("Impossible to put key after key"s);
    }
    dict_key_ = std::move(value);
    nodes_stack_.back()->AsMap().emplace(dict_key_.value(), nullptr);

    return DictValueContext(*this);
}

BaseContext Builder::EndDict() {
    if (!(nodes_stack_.back()->IsMap())) {
        throw std::logic_error("It's not a dict"s);
    }
    nodes_stack_.pop_back();
    return BaseContext(*this);
}

ArrayContext Builder::StartArray() {
    if (!(nodes_stack_.empty() || nodes_stack_.back()->IsArray() || (nodes_stack_.back()->IsMap() && dict_key_.has_value()))) {
        throw std::logic_error("Impossible start array from here"s);
    }

    if (nodes_stack_.empty()) {
        root_ = Array();
        nodes_stack_.push_back(&root_);     
    }

    else if (nodes_stack_.back()->IsMap() && dict_key_.has_value()) {
        Node& last_dict_value_ref = nodes_stack_.back()->AsMap()[dict_key_.value()];
        last_dict_value_ref = Array();
        nodes_stack_.push_back(&last_dict_value_ref);
        dict_key_.reset();
    }
    
    else if(nodes_stack_.back()->IsArray()) {
        Node& last_array_node_ref = (nodes_stack_.back()->AsArray()).emplace_back(Array());
        nodes_stack_.push_back(&last_array_node_ref);
    }
 
    return ArrayContext(*this);
}

BaseContext Builder::EndArray() {
    if (!(nodes_stack_.back()->IsArray())) {
        throw std::logic_error("It's not an array"s);
    }
    nodes_stack_.pop_back();
    return BaseContext(*this);
}


DictItemContext BaseContext::StartDict() {
    return builder_.StartDict();
}

ArrayContext BaseContext::StartArray() {
    return builder_.StartArray();
}

BaseContext BaseContext::EndDict() {
    return builder_.EndDict();
}

BaseContext BaseContext::EndArray() {
    return builder_.EndArray();
}

Node BaseContext::Build() {
    return builder_.Build();
}

DictValueContext BaseContext::Key(std::string key) {
    return builder_.Key(key);
}

BaseContext BaseContext::Value(Node::Value value) {
    return builder_.Value(std::move(value));
}


DictValueContext DictItemContext::Key(std::string value) {
    return BaseContext::Key(std::move(value));
}

BaseContext DictItemContext::EndDict() {
    return BaseContext::EndDict();
}


DictItemContext DictValueContext::Value(Node::Value value) {
    return BaseContext::Value(std::move(value));
}

DictItemContext DictValueContext::StartDict() {
    return BaseContext::StartDict();
}

ArrayContext DictValueContext::StartArray() {
    return BaseContext::StartArray();
}


ArrayContext ArrayContext::Value(Node::Value value) {
    return BaseContext::Value(std::move(value));
}

DictItemContext ArrayContext::StartDict() {
    return BaseContext::StartDict();
}

ArrayContext ArrayContext::StartArray() {
    return BaseContext::StartArray();
}

BaseContext ArrayContext::EndArray() {
    return BaseContext::EndArray();
}

}
