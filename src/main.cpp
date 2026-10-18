#include "robot.hpp"

#include <iostream>
#include <string>

int main() {
    const std::string path = "data/robot_state.txt";

    Robot r;

    if (r.loadFromFile(path)) {
        std::cout << "loaded from " << path << "\n";
    } else {
        std::cout << "no valid data, using defaults\n";
    }
    r.print();

    // 只在没有历史时才记录，避免每次运行都追加
    if (r.historySize() == 0) {
        // 这三个值是写死的常量，确定合法，显式忽略返回值
        (void)r.recordBattery(80.0);
        (void)r.recordBattery(55.5);
        (void)r.recordBattery(12.0);
        std::cout << "recorded 3 samples\n";
        r.print();
    }

    if (!r.saveToFile(path)) {
        std::cout << "save failed\n";
    } else {
        std::cout << "saved to " << path << "\n";
    }
}
