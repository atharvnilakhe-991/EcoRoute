#ifndef CHARGING_STATION_H
#define CHARGING_STATION_H

#include "Station.h"
#include "Robot.h"

class ChargingStation : public Station {
private:
    double chargeRate; // percentage per tick

public:
    ChargingStation(const std::string& id, Position pos, double rate = 20.0)
        : Station(id, pos), chargeRate(rate) {}

    double getChargeRate() const { return chargeRate; }

    void recharge(Robot& robot) {
        robot.charge(chargeRate);
    }

    std::string getStationType() const override {
        return "CHARGING_STATION";
    }
};

#endif // CHARGING_STATION_H
