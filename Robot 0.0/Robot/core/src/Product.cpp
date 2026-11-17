#include "../include/Product.h"
#include <sstream>

Product::Product() : id(0), name(""), category(""), quantity(0), price(0.0), location(Position(0, 0)) {}

Product::Product(int id, const std::string& name, const std::string& category, int quantity, double price, Position location)
    : id(id), name(name), category(category), quantity(quantity), price(price), location(location) {}

int Product::getId() const { return id; }
std::string Product::getName() const { return name; }
std::string Product::getCategory() const { return category; }
int Product::getQuantity() const { return quantity; }
double Product::getPrice() const { return price; }
Position Product::getLocation() const { return location; }

void Product::setLocation(const Position& pos) {
    location = pos;
}

void Product::setPrice(double newPrice) {
    if (newPrice >= 0.0) {
        price = newPrice;
    }
}

bool Product::updateQuantity(int delta) {
    if (quantity + delta < 0) {
        return false;
    }
    quantity += delta;
    return true;
}

std::string Product::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id << ","
       << "\"name\":\"" << name << "\","
       << "\"category\":\"" << category << "\","
       << "\"quantity\":" << quantity << ","
       << "\"price\":" << price << ","
       << "\"location\":{\"x\":" << location.x << ",\"y\":" << location.y << "}"
       << "}";
    return ss.str();
}
