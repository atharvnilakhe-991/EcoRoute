#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>
#include <string>

class WarehouseException : public std::runtime_error {
public:
    explicit WarehouseException(const std::string& message) : std::runtime_error(message) {}
};

class InsufficientStockException : public WarehouseException {
public:
    explicit InsufficientStockException(const std::string& message) : WarehouseException("Insufficient Stock: " + message) {}
};

class RobotUnreachableException : public WarehouseException {
public:
    explicit RobotUnreachableException(const std::string& message) : WarehouseException("Robot Unreachable: " + message) {}
};

class PathNotFoundException : public WarehouseException {
public:
    explicit PathNotFoundException(const std::string& message) : WarehouseException("Path Not Found: " + message) {}
};

class InvalidOperationException : public WarehouseException {
public:
    explicit InvalidOperationException(const std::string& message) : WarehouseException("Invalid Operation: " + message) {}
};

#endif // EXCEPTIONS_H
