#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>
#include "Position.h"

class Product {
private:
    int id;
    std::string name;
    std::string category;
    int quantity;
    double price;
    Position location;

public:
    Product();
    Product(int id, const std::string& name, const std::string& category, int quantity, double price, Position location);

    int getId() const;
    std::string getName() const;
    std::string getCategory() const;
    int getQuantity() const;
    double getPrice() const;
    Position getLocation() const;

    void setLocation(const Position& pos);
    void setPrice(double newPrice);
    bool updateQuantity(int delta); // delta can be negative (decrease) or positive (restock)

    std::string toJSON() const;
};

#endif // PRODUCT_H
