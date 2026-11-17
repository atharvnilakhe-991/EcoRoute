#ifndef INVENTORY_H
#define INVENTORY_H

#include <map>
#include <vector>
#include "Product.h"

class Inventory {
private:
    std::map<int, Product> products;

    // Track reserved quantities per product
    std::map<int, int> reservedStock;

    // Track reservations per order: orderId -> OrderReservation
    struct OrderReservation {
        int productId{0};
        int reservedQty{0};
        int pickedQty{0};
    };
    std::map<int, OrderReservation> orderReservations;

public:
    Inventory();

    bool addProduct(const Product& product);
    bool removeProduct(int productId);
    Product* getProduct(int productId);
    const Product* getProduct(int productId) const;
    
    // Physical on-hand stock vs reserved and available
    int getOnHandStock(int productId) const;
    int getReservedStock(int productId) const;
    int getAvailableStock(int productId) const;

    bool hasStock(int productId, int quantity) const;
    bool hasStockForOrder(int orderId, int productId, int quantity) const;
    bool deductStock(int productId, int quantity);
    bool restock(int productId, int quantity);

    // Reservation & Physical Pickup Lifecycle
    bool reserveStock(int orderId, int productId, int quantity);
    bool releaseReservation(int orderId);
    bool recordPhysicalPickup(int orderId, int quantity);

    int getOrderReservedQuantity(int orderId) const;
    int getOrderPickedQuantity(int orderId) const;
    void clearReservations();
    void clear();

    struct OrderReservationInfo {
        int orderId{0};
        int productId{0};
        int reservedQty{0};
        int pickedQty{0};
    };
    void restoreOrderReservation(int orderId, int productId, int reservedQty, int pickedQty);
    std::vector<OrderReservationInfo> getOrderReservationList() const;
    std::string orderReservationsToJSON() const;

    std::vector<Product> getAllProducts() const;
    std::string toJSON() const;
    void printInventory() const;
};

#endif // INVENTORY_H
