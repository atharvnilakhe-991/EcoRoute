#ifndef HYBRID_ROBOT_H
#define HYBRID_ROBOT_H

#include "Robot.h"

class HybridRobot : public Robot {
public:
    HybridRobot(const std::string& id, const std::string& name, Position initialPos);

    bool moveNextStep() override;
    bool pickItem(int productId, int quantity) override;
    bool deliverItem() override;
    double getBatteryDrainPerMove() const override;
    std::string getRobotTypeName() const override;
};

#endif // HYBRID_ROBOT_H
