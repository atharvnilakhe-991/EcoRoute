#include "../include/Warehouse.h"
#include <iostream>
#include <sstream>
#include <limits>
#include <algorithm>

std::string cellTypeToString(CellType type) {
    switch (type) {
        case CellType::EMPTY: return "EMPTY";
        case CellType::OBSTACLE: return "OBSTACLE";
        case CellType::SHELF: return "SHELF";
        case CellType::CHARGING_STATION: return "CHARGING_STATION";
        case CellType::PACKING_STATION: return "PACKING_STATION";
        default: return "UNKNOWN";
    }
}

Warehouse::Warehouse(int width, int height)
    : width(width), height(height),
      grid(height, std::vector<CellType>(width, CellType::EMPTY)),
      weightGrid(height, std::vector<double>(width, 1.0)) {}

int Warehouse::getWidth() const { return width; }
int Warehouse::getHeight() const { return height; }

bool Warehouse::isValidPosition(const Position& pos) const {
    return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height;
}

bool Warehouse::isWalkable(const Position& pos) const {
    if (!isValidPosition(pos)) return false;
    if (isDynamicObstacle(pos)) return false;
    CellType type = grid[pos.y][pos.x];
    // Obstacles and Shelves are impassable (robots stop adjacent to shelves to pick)
    // Stations can be stepped onto to perform charging / delivering
    return (type == CellType::EMPTY || type == CellType::CHARGING_STATION || type == CellType::PACKING_STATION);
}

double Warehouse::getCellCost(const Position& pos) const {
    if (!isValidPosition(pos)) return std::numeric_limits<double>::infinity();
    return weightGrid[pos.y][pos.x];
}

double Warehouse::getMinCellCost() const {
    return 1.0;
}

CellType Warehouse::getCellType(const Position& pos) const {
    if (!isValidPosition(pos)) return CellType::OBSTACLE;
    if (isDynamicObstacle(pos)) return CellType::OBSTACLE;
    return grid[pos.y][pos.x];
}

void Warehouse::setCellType(const Position& pos, CellType type) {
    if (isValidPosition(pos)) {
        grid[pos.y][pos.x] = type;
    }
}

void Warehouse::setCellCost(const Position& pos, double cost) {
    if (isValidPosition(pos)) {
        weightGrid[pos.y][pos.x] = cost;
    }
}

void Warehouse::setObstacle(const Position& pos, bool isObstacle) {
    if (isValidPosition(pos)) {
        grid[pos.y][pos.x] = isObstacle ? CellType::OBSTACLE : CellType::EMPTY;
    }
}

bool Warehouse::addDynamicObstacle(const Position& pos) {
    if (!isValidPosition(pos)) return false;
    // Cannot place dynamic obstacle on static obstacles, shelves, or stations
    CellType t = grid[pos.y][pos.x];
    if (t != CellType::EMPTY) return false;
    if (isDynamicObstacle(pos)) return true;
    dynamicObstacles.push_back(pos);
    return true;
}

bool Warehouse::removeDynamicObstacle(const Position& pos) {
    auto it = std::find(dynamicObstacles.begin(), dynamicObstacles.end(), pos);
    if (it != dynamicObstacles.end()) {
        dynamicObstacles.erase(it);
        return true;
    }
    return false;
}

bool Warehouse::isDynamicObstacle(const Position& pos) const {
    return std::find(dynamicObstacles.begin(), dynamicObstacles.end(), pos) != dynamicObstacles.end();
}

const std::vector<Position>& Warehouse::getDynamicObstacles() const {
    return dynamicObstacles;
}

void Warehouse::clearDynamicObstacles() {
    dynamicObstacles.clear();
}

void Warehouse::addShelf(const Shelf& shelf) {
    shelves.push_back(shelf);
    setCellType(shelf.getPosition(), CellType::SHELF);
}

void Warehouse::addChargingStation(const ChargingStation& station) {
    chargingStations.push_back(station);
    setCellType(station.getPosition(), CellType::CHARGING_STATION);
}

void Warehouse::addPackingStation(const PackingStation& station) {
    packingStations.push_back(station);
    setCellType(station.getPosition(), CellType::PACKING_STATION);
}

const std::vector<Shelf>& Warehouse::getShelves() const {
    return shelves;
}

const std::vector<ChargingStation>& Warehouse::getChargingStations() const {
    return chargingStations;
}

std::vector<ChargingStation>& Warehouse::getChargingStations() {
    return chargingStations;
}

ChargingStation* Warehouse::getChargingStationAt(const Position& pos) {
    for (auto& cs : chargingStations) {
        if (cs.getPosition() == pos) {
            return &cs;
        }
    }
    return nullptr;
}

const ChargingStation* Warehouse::getChargingStationAt(const Position& pos) const {
    for (const auto& cs : chargingStations) {
        if (cs.getPosition() == pos) {
            return &cs;
        }
    }
    return nullptr;
}

const std::vector<PackingStation>& Warehouse::getPackingStations() const {
    return packingStations;
}

std::vector<PackingStation>& Warehouse::getPackingStations() {
    return packingStations;
}

PackingStation* Warehouse::getPackingStationAt(const Position& pos) {
    for (auto& ps : packingStations) {
        if (ps.getPosition() == pos) {
            return &ps;
        }
    }
    return nullptr;
}

const PackingStation* Warehouse::getPackingStationAt(const Position& pos) const {
    for (const auto& ps : packingStations) {
        if (ps.getPosition() == pos) {
            return &ps;
        }
    }
    return nullptr;
}

void Warehouse::recordOrderDelivered(const Position& stationPos) {
    for (auto& ps : packingStations) {
        if (ps.getPosition() == stationPos) {
            ps.recordCompletedOrder();
            ps.setOccupied(true);
            return;
        }
    }
    if (!packingStations.empty()) {
        packingStations[0].recordCompletedOrder();
        packingStations[0].setOccupied(true);
    }
}

Shelf* Warehouse::findShelfForProduct(int productId) {
    for (auto& shelf : shelves) {
        if (shelf.hasProduct(productId)) {
            return &shelf;
        }
    }
    return nullptr;
}

Position Warehouse::getNearestChargingStation(const Position& fromPos) const {
    if (chargingStations.empty()) return Position(0, 0);
    Position best = chargingStations[0].getPosition();
    int minDistance = fromPos.manhattanDistance(best);
    for (const auto& cs : chargingStations) {
        int d = fromPos.manhattanDistance(cs.getPosition());
        if (d < minDistance) {
            minDistance = d;
            best = cs.getPosition();
        }
    }
    return best;
}

Position Warehouse::getNearestPackingStation(const Position& fromPos) const {
    if (packingStations.empty()) return Position(width - 1, height - 1);
    Position best = packingStations[0].getPosition();
    int minDistance = fromPos.manhattanDistance(best);
    for (const auto& ps : packingStations) {
        int d = fromPos.manhattanDistance(ps.getPosition());
        if (d < minDistance) {
            minDistance = d;
            best = ps.getPosition();
        }
    }
    return best;
}

std::vector<Position> Warehouse::getWalkableNeighbors(const Position& pos) const {
    std::vector<Position> neighbors;
    static const int dx[] = {0, 1, 0, -1};
    static const int dy[] = {-1, 0, 1, 0};

    for (int i = 0; i < 4; ++i) {
        Position next(pos.x + dx[i], pos.y + dy[i]);
        if (isWalkable(next)) {
            neighbors.push_back(next);
        }
    }
    return neighbors;
}

std::string Warehouse::toJSON() const {
    std::ostringstream ss;
    ss << "{"
       << "\"width\":" << width << ","
       << "\"height\":" << height << ","
       << "\"grid\":[";
    for (int y = 0; y < height; ++y) {
        if (y > 0) ss << ",";
        ss << "[";
        for (int x = 0; x < width; ++x) {
            if (x > 0) ss << ",";
            ss << static_cast<int>(grid[y][x]);
        }
        ss << "]";
    }
    ss << "],\"shelves\":[";
    for (size_t i = 0; i < shelves.size(); ++i) {
        if (i > 0) ss << ",";
        ss << shelves[i].toJSON();
    }
    ss << "],\"chargingStations\":[";
    for (size_t i = 0; i < chargingStations.size(); ++i) {
        if (i > 0) ss << ",";
        ss << chargingStations[i].toJSON();
    }
    ss << "],\"packingStations\":[";
    for (size_t i = 0; i < packingStations.size(); ++i) {
        if (i > 0) ss << ",";
        ss << packingStations[i].toJSON();
    }
    ss << "],\"dynamicObstacles\":[";
    for (size_t i = 0; i < dynamicObstacles.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{\"x\":" << dynamicObstacles[i].x << ",\"y\":" << dynamicObstacles[i].y << "}";
    }
    ss << "]}";
    return ss.str();
}

void Warehouse::printWarehouseGrid() const {
    std::cout << "\n=== WAREHOUSE MAP (" << width << "x" << height << ") ===\n";
    std::cout << "  ";
    for (int x = 0; x < width; ++x) std::cout << (x % 10) << " ";
    std::cout << "\n";

    for (int y = 0; y < height; ++y) {
        std::cout << (y % 10) << " ";
        for (int x = 0; x < width; ++x) {
            Position p(x, y);
            if (isDynamicObstacle(p)) {
                std::cout << "D ";
            } else {
                switch (grid[y][x]) {
                    case CellType::EMPTY: std::cout << ". "; break;
                    case CellType::OBSTACLE: std::cout << "X "; break;
                    case CellType::SHELF: std::cout << "S "; break;
                    case CellType::CHARGING_STATION: std::cout << "C "; break;
                    case CellType::PACKING_STATION: std::cout << "P "; break;
                }
            }
        }
        std::cout << "\n";
    }
    std::cout << "===============================\n\n";
}
