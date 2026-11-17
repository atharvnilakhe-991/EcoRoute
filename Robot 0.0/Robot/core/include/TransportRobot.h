#ifndef TRANSPORT_ROBOT_H
#define TRANSPORT_ROBOT_H

#include "Robot.h"

class TransportRobot : public Robot {
private:
    int heavyPayloadLimit;

public:
    TransportRobot(const std::string& id, const std::string& name, Position initialPos);

    bool moveNextStep() override;
    bool pickItem(int productId, int quantity) override;
    bool deliverItem() override;
    double getBatteryDrainPerMove() const override;
    std::string getRobotTypeName() const override;
};

#endif // TRANSPORT_ROBOT_H
