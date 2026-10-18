#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

enum class BatteryStatus { Full, Medium, Low, Empty, Invalid };

std::string toString(BatteryStatus s);
std::ostream& operator<<(std::ostream& os, BatteryStatus s);

class Robot {
public:
    explicit Robot(double battery = 100.0, double temperature = 20.0);

    // ---- 读 ----
    double battery() const;
    double temperature() const;
    std::size_t historySize() const;

    bool isValid() const;
    bool isTemperatureValid() const;
    BatteryStatus status() const;
    bool isOperational() const;

    double averageBattery() const;   // 历史为空时返回 0
    double minBattery() const;       // 历史为空时返回 0
    double maxBattery() const;       // 历史为空时返回 0

    // ---- 写 ----
    [[nodiscard]] bool setBattery(double b);
    [[nodiscard]] bool setTemperature(double t);
    [[nodiscard]] bool recordBattery(double v);

    // ---- 文件 ----
    [[nodiscard]] bool saveToFile(const std::string& path) const;
    [[nodiscard]] bool loadFromFile(const std::string& path);

    void print() const;

private:
    double battery_;
    double temperature_;
    std::vector<double> history_;
};
