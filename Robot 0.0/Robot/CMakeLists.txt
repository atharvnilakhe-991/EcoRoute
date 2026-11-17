cmake_minimum_required(VERSION 3.15)
project(RobotWarehouseSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(core/include)

set(CORE_SOURCES
    core/src/Product.cpp
    core/src/Inventory.cpp
    core/src/Task.cpp
    core/src/Robot.cpp
    core/src/PickerRobot.cpp
    core/src/TransportRobot.cpp
    core/src/HybridRobot.cpp
    core/src/Shelf.cpp
    core/src/Station.cpp
    core/src/Order.cpp
    core/src/Warehouse.cpp
    core/src/PathFinder.cpp
    core/src/TaskManager.cpp
    core/src/WarehouseSystem.cpp
    core/src/ReservationTable.cpp
    core/src/MultiRobotCoordinator.cpp
    core/src/WarehouseAnalytics.cpp
    core/src/AlgorithmBenchmark.cpp
    core/src/AlgorithmLab.cpp
    core/src/EcoRoute.cpp
    core/src/FoodSaver.cpp
    core/src/JsonParser.cpp
    core/src/HttpHandler.cpp
)

# Core CLI simulation executable
add_executable(WarehouseCore
    core/main.cpp
    ${CORE_SOURCES}
)

# HTTP REST API & Web Server executable
add_executable(WarehouseServer
    server/server.cpp
    ${CORE_SOURCES}
)

# Automated Regression Test Suite executable
add_executable(WarehouseTests
    tests/test_regression.cpp
    ${CORE_SOURCES}
)

if (WIN32)
    target_link_libraries(WarehouseServer PRIVATE ws2_32)
endif()

if (MSVC)
    target_compile_options(WarehouseCore PRIVATE /W4 /EHsc)
    target_compile_options(WarehouseServer PRIVATE /W4 /EHsc)
    target_compile_options(WarehouseTests PRIVATE /W4 /EHsc)
else()
    target_compile_options(WarehouseCore PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(WarehouseServer PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(WarehouseTests PRIVATE -Wall -Wextra -pedantic)
endif()
