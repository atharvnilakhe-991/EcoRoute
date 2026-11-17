#ifndef PACKING_STATION_H
#define PACKING_STATION_H

#include "Station.h"

class PackingStation : public Station {
private:
    int ordersProcessed;

public:
    PackingStation(const std::string& id, Position pos)
        : Station(id, pos), ordersProcessed(0) {}

    int getOrdersProcessed() const { return ordersProcessed; }

    void recordCompletedOrder() {
        ordersProcessed++;
    }

    std::string getStationType() const override {
        return "PACKING_STATION";
    }
};

#endif // PACKING_STATION_H
