#ifndef POSITION_H
#define POSITION_H

#include <iostream>
#include <cmath>
#include <string>

/**
 * @brief Represents a 2D coordinate position within the warehouse grid.
 */
class Position {
public:
    int x;
    int y;

    Position() : x(0), y(0) {}
    Position(int xCoord, int yCoord) : x(xCoord), y(yCoord) {}

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Position& other) const {
        return !(*this == other);
    }

    bool operator<(const Position& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }

    int manhattanDistance(const Position& other) const {
        return std::abs(x - other.x) + std::abs(y - other.y);
    }

    double euclideanDistance(const Position& other) const {
        return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
    }

    std::string toString() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }

    std::string toJSON() const {
        return "{\"x\":" + std::to_string(x) + ",\"y\":" + std::to_string(y) + "}";
    }

    friend std::ostream& operator<<(std::ostream& os, const Position& pos) {
        os << pos.toString();
        return os;
    }
};

namespace std {
    template <>
    struct hash<Position> {
        size_t operator()(const Position& p) const {
            return (hash<int>()(p.x) ^ (hash<int>()(p.y) << 1));
        }
    };
}

#endif // POSITION_H
