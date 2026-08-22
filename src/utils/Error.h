#pragma once
#include <stdexcept>
#include <string>

class SemanticError : public std::runtime_error {
public:
    explicit SemanticError(const std::string& msg) : std::runtime_error(msg) {}
};

class IndexError : public std::runtime_error {
public:
    explicit IndexError(const std::string& msg) : std::runtime_error(msg) {}
};

class StorageError : public std::runtime_error {
public:
    explicit StorageError(const std::string& msg) : std::runtime_error(msg) {}
};

class TypeError : public std::runtime_error {
public:
    explicit TypeError(const std::string& msg) : std::runtime_error(msg) {}
};
