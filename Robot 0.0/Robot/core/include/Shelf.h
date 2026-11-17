#ifndef SHELF_H
#define SHELF_H

#include <string>
#include <vector>
#include "Position.h"

class Shelf {
private:
    std::string id;
    Position position;
    std::vector<int> productIds;

public:
    Shelf();
    Shelf(const std::string& id, Position pos);

    std::string getId() const;
    Position getPosition() const;
    const std::vector<int>& getProductIds() const;

    void addProduct(int productId);
    bool hasProduct(int productId) const;
    std::string toJSON() const;
};

#endif // SHELF_H
