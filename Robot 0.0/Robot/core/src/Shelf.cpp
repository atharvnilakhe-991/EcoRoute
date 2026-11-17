#include "../include/Shelf.h"
#include <algorithm>
#include <sstream>

Shelf::Shelf() : id(""), position(Position(0, 0)) {}

Shelf::Shelf(const std::string& id, Position pos) : id(id), position(pos) {}

std::string Shelf::getId() const { return id; }
Position Shelf::getPosition() const { return position; }
const std::vector<int>& Shelf::getProductIds() const { return productIds; }

void Shelf::addProduct(int productId) {
    if (!hasProduct(productId)) {
        productIds.push_back(productId);
    }
}

bool Shelf::hasProduct(int productId) const {
    return std::find(productIds.begin(), productIds.end(), productId) != productIds.end();
}

std::string Shelf::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":\"" << id << "\","
       << "\"position\":{\"x\":" << position.x << ",\"y\":" << position.y << "},"
       << "\"productIds\":[";
    for (size_t i = 0; i < productIds.size(); ++i) {
        if (i > 0) ss << ",";
        ss << productIds[i];
    }
    ss << "]}";
    return ss.str();
}
