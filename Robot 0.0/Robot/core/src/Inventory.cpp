#include "../include/Inventory.h"
#include "../include/Exceptions.h"
#include <iostream>
#include <iomanip>
#include <sstream>

Inventory::Inventory() {}

bool Inventory::addProduct(const Product& product) {
    if (products.find(product.getId()) != products.end()) {
        // If product already exists, simply add quantity
        products[product.getId()].updateQuantity(product.getQuantity());
        return true;
    }
    products[product.getId()] = product;
    return true;
}

bool Inventory::removeProduct(int productId) {
    auto it = products.find(productId);
    if (it != products.end()) {
        products.erase(it);
        return true;
    }
    return false;
}

Product* Inventory::getProduct(int productId) {
    auto it = products.find(productId);
    if (it != products.end()) {
        return &(it->second);
    }
    return nullptr;
}

const Product* Inventory::getProduct(int productId) const {
    auto it = products.find(productId);
    if (it != products.end()) {
        return &(it->second);
    }
    return nullptr;
}

int Inventory::getOnHandStock(int productId) const {
    const Product* p = getProduct(productId);
    return p ? p->getQuantity() : 0;
}

int Inventory::getReservedStock(int productId) const {
    auto it = reservedStock.find(productId);
    return (it != reservedStock.end()) ? it->second : 0;
}

int Inventory::getAvailableStock(int productId) const {
    return std::max(0, getOnHandStock(productId) - getReservedStock(productId));
}

bool Inventory::hasStock(int productId, int quantity) const {
    if (quantity <= 0) return true;
    return getAvailableStock(productId) >= quantity;
}

bool Inventory::hasStockForOrder(int orderId, int productId, int quantity) const {
    auto it = orderReservations.find(orderId);
    if (it != orderReservations.end()) {
        if (it->second.productId == productId) {
            // Already has reservation for this order
            return true;
        }
    }
    return hasStock(productId, quantity);
}

bool Inventory::reserveStock(int orderId, int productId, int quantity) {
    if (quantity <= 0) return false;
    auto it = orderReservations.find(orderId);
    if (it != orderReservations.end()) {
        // Idempotent: already reserved for this order with matching product and quantity
        if (it->second.productId == productId && it->second.reservedQty == quantity) {
            return true;
        }
        return false;
    }

    if (getAvailableStock(productId) < quantity) {
        return false;
    }

    reservedStock[productId] += quantity;
    orderReservations[orderId] = OrderReservation{productId, quantity, 0};
    return true;
}

bool Inventory::releaseReservation(int orderId) {
    auto it = orderReservations.find(orderId);
    if (it == orderReservations.end()) {
        return false; // Idempotent: already released or no reservation
    }

    int unpicked = it->second.reservedQty - it->second.pickedQty;
    if (unpicked > 0) {
        reservedStock[it->second.productId] = std::max(0, reservedStock[it->second.productId] - unpicked);
    }
    orderReservations.erase(it);
    return true;
}

bool Inventory::recordPhysicalPickup(int orderId, int quantity) {
    auto it = orderReservations.find(orderId);
    if (it == orderReservations.end()) {
        return false;
    }

    int toPick = std::min(quantity, it->second.reservedQty - it->second.pickedQty);
    if (toPick <= 0) {
        // Idempotent: already picked, do not deduct again
        return true;
    }

    // Physically deduct on-hand stock exactly once
    deductStock(it->second.productId, toPick);
    reservedStock[it->second.productId] = std::max(0, reservedStock[it->second.productId] - toPick);
    it->second.pickedQty += toPick;
    return true;
}

int Inventory::getOrderReservedQuantity(int orderId) const {
    auto it = orderReservations.find(orderId);
    if (it == orderReservations.end()) return 0;
    return it->second.reservedQty - it->second.pickedQty;
}

int Inventory::getOrderPickedQuantity(int orderId) const {
    auto it = orderReservations.find(orderId);
    if (it == orderReservations.end()) return 0;
    return it->second.pickedQty;
}

void Inventory::clearReservations() {
    reservedStock.clear();
    orderReservations.clear();
}

void Inventory::clear() {
    products.clear();
    reservedStock.clear();
    orderReservations.clear();
}

void Inventory::restoreOrderReservation(int orderId, int productId, int reservedQty, int pickedQty) {
    // If order already has a reservation, remove old contribution first
    auto it = orderReservations.find(orderId);
    if (it != orderReservations.end()) {
        int oldUnpicked = std::max(0, it->second.reservedQty - it->second.pickedQty);
        reservedStock[it->second.productId] = std::max(0, reservedStock[it->second.productId] - oldUnpicked);
    }
    orderReservations[orderId] = OrderReservation{productId, reservedQty, pickedQty};
    int unpicked = std::max(0, reservedQty - pickedQty);
    reservedStock[productId] += unpicked;
}

std::vector<Inventory::OrderReservationInfo> Inventory::getOrderReservationList() const {
    std::vector<OrderReservationInfo> list;
    list.reserve(orderReservations.size());
    for (const auto& pair : orderReservations) {
        list.push_back({pair.first, pair.second.productId, pair.second.reservedQty, pair.second.pickedQty});
    }
    return list;
}

std::string Inventory::orderReservationsToJSON() const {
    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (const auto& pair : orderReservations) {
        if (!first) ss << ",";
        first = false;
        ss << "{"
           << "\"orderId\":" << pair.first << ","
           << "\"productId\":" << pair.second.productId << ","
           << "\"reservedQty\":" << pair.second.reservedQty << ","
           << "\"pickedQty\":" << pair.second.pickedQty
           << "}";
    }
    ss << "]";
    return ss.str();
}

bool Inventory::deductStock(int productId, int quantity) {
    Product* p = getProduct(productId);
    if (!p) {
        throw InsufficientStockException("Product ID " + std::to_string(productId) + " not found.");
    }
    if (p->getQuantity() < quantity) {
        throw InsufficientStockException("Not enough stock for product: " + p->getName() + 
            " (Requested: " + std::to_string(quantity) + ", Available: " + std::to_string(p->getQuantity()) + ")");
    }
    return p->updateQuantity(-quantity);
}

bool Inventory::restock(int productId, int quantity) {
    Product* p = getProduct(productId);
    if (!p) {
        return false;
    }
    return p->updateQuantity(quantity);
}

std::vector<Product> Inventory::getAllProducts() const {
    std::vector<Product> list;
    list.reserve(products.size());
    for (const auto& pair : products) {
        list.push_back(pair.second);
    }
    return list;
}

std::string Inventory::toJSON() const {
    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (const auto& pair : products) {
        if (!first) ss << ",";
        int pid = pair.first;
        const Product& p = pair.second;
        ss << "{"
           << "\"id\":" << p.getId() << ","
           << "\"name\":\"" << p.getName() << "\","
           << "\"category\":\"" << p.getCategory() << "\","
           << "\"quantity\":" << p.getQuantity() << ","
           << "\"reserved\":" << getReservedStock(pid) << ","
           << "\"available\":" << getAvailableStock(pid) << ","
           << "\"price\":" << p.getPrice() << ","
           << "\"location\":{\"x\":" << p.getLocation().x << ",\"y\":" << p.getLocation().y << "}"
           << "}";
        first = false;
    }
    ss << "]";
    return ss.str();
}

void Inventory::printInventory() const {
    std::cout << "\n========== CURRENT INVENTORY ==========\n";
    std::cout << std::left << std::setw(6) << "ID"
              << std::setw(20) << "Name"
              << std::setw(15) << "Category"
              << std::setw(8) << "Qty"
              << std::setw(10) << "Price"
              << "Location" << "\n";
    std::cout << "------------------------------------------------------------\n";
    for (const auto& pair : products) {
        const Product& p = pair.second;
        std::cout << std::left << std::setw(6) << p.getId()
                  << std::setw(20) << p.getName()
                  << std::setw(15) << p.getCategory()
                  << std::setw(8) << p.getQuantity()
                  << "$" << std::fixed << std::setprecision(2) << std::setw(9) << p.getPrice()
                  << p.getLocation() << "\n";
    }
    std::cout << "=======================================\n\n";
}
