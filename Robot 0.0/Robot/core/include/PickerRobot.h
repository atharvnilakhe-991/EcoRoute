#ifndef PICKER_ROBOT_H
#define PICKER_ROBOT_H

#include "Robot.h"

class PickerRobot : public Robot {
private:
    double pickSpeedFactor;

public:
    PickerRobot(const std::string& id, const std::string& name, Position initialPos);

    bool moveNextStep() override;
    bool pickItem(int productId, int quantity) override;
    bool deliverItem() override;
    double getBatteryDrainPerMove() const override;
    std::string getRobotTypeName() const override;
};

#endif // PICKER_ROBOT_H
