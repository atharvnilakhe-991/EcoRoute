#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include <vector>
#include <string>
#include <algorithm>
#include "Position.h"
#include "Shelf.h"
#include "ChargingStation.h"
#include "PackingStation.h"

enum class CellType {
    EMPTY = 0,
    OBSTACLE = 1,
    SHELF = 2,
    CHARGING_STATION = 3,
    PACKING_STATION = 4
};

std::string cellTypeToString(CellType type);

class Warehouse {
private:
    int width;
    int height;
    std::vector<std::vector<CellType>> grid;
    std::vector<std::vector<double>> weightGrid;
    std::vector<Shelf> shelves;
    std::vector<ChargingStation> chargingStations;
    std::vector<PackingStation> packingStations;
    std::vector<Position> dynamicObstacles;

public:
    Warehouse(int width = 15, int height = 15);

    int getWidth() const;
    int getHeight() const;

    bool isValidPosition(const Position& pos) const;
    bool isWalkable(const Position& pos) const;
    double getCellCost(const Position& pos) const;
    double getMinCellCost() const;
    CellType getCellType(const Position& pos) const;

    void setCellType(const Position& pos, CellType type);
    void setCellCost(const Position& pos, double cost);
    void setObstacle(const Position& pos, bool isObstacle);

    // Dynamic Obstacles
    bool addDynamicObstacle(const Position& pos);
    bool removeDynamicObstacle(const Position& pos);
    bool isDynamicObstacle(const Position& pos) const;
    const std::vector<Position>& getDynamicObstacles() const;
    void clearDynamicObstacles();

    void addShelf(const Shelf& shelf);
    void addChargingStation(const ChargingStation& station);
    void addPackingStation(const PackingStation& station);

    const std::vector<Shelf>& getShelves() const;
    const std::vector<ChargingStation>& getChargingStations() const;
    std::vector<ChargingStation>& getChargingStations();
    const std::vector<PackingStation>& getPackingStations() const;
    std::vector<PackingStation>& getPackingStations();

    Shelf* findShelfForProduct(int productId);
    Position getNearestChargingStation(const Position& fromPos) const;
    Position getNearestPackingStation(const Position& fromPos) const;
    ChargingStation* getChargingStationAt(const Position& pos);
    const ChargingStation* getChargingStationAt(const Position& pos) const;
    PackingStation* getPackingStationAt(const Position& pos);
    const PackingStation* getPackingStationAt(const Position& pos) const;
    void recordOrderDelivered(const Position& stationPos);

    std::vector<Position> getWalkableNeighbors(const Position& pos) const;

    std::string toJSON() const;
    void printWarehouseGrid() const;
};

#endif // WAREHOUSE_H
