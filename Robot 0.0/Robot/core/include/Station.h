#ifndef STATION_H
#define STATION_H

#include <string>
#include "Position.h"

class Station {
protected:
    std::string id;
    Position position;
    bool occupied;
    std::string occupyingRobotId;

public:
    Station(const std::string& id, Position pos);
    virtual ~Station() = default;

    std::string getId() const;
    Position getPosition() const;
    bool isOccupied() const;
    std::string getOccupyingRobotId() const;
    void setOccupied(bool occ, const std::string& robotId = "");

    virtual std::string getStationType() const = 0;
    virtual std::string toJSON() const;
};

#endif // STATION_H
