#include "robot.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

namespace {

void stripCR(std::string& s) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
}

bool parseDouble(const std::string& s, double& out) {
    if (s.empty()) return false;
    std::size_t pos = 0;
    try {
        out = std::stod(s, &pos);
    } catch (const std::exception&) {
        return false;
    }
    return pos == s.size();
}

}  // namespace

// ---------------------------------------------------------------- 枚举

std::string toString(BatteryStatus s) {
    switch (s) {
        case BatteryStatus::Full:    return "Full";
        case BatteryStatus::Medium:  return "Medium";
        case BatteryStatus::Low:     return "Low";
        case BatteryStatus::Empty:   return "Empty";
        case BatteryStatus::Invalid: return "Invalid";
    }
    return "Unknown";
}

std::ostream& operator<<(std::ostream& os, BatteryStatus s) {
    return os << toString(s);
}

// ---------------------------------------------------------------- 构造

Robot::Robot(double battery, double temperature)
    : battery_(battery), temperature_(temperature) {}

// ---------------------------------------------------------------- 读

double Robot::battery() const     { return battery_; }
double Robot::temperature() const { return temperature_; }
std::size_t Robot::historySize() const { return history_.size(); }

bool Robot::isValid() const {
    return battery_ >= 0.0 && battery_ <= 100.0;
}

bool Robot::isTemperatureValid() const {
    return temperature_ >= -20.0 && temperature_ <= 80.0;
}

BatteryStatus Robot::status() const {
    if (!isValid())       return BatteryStatus::Invalid;
    if (battery_ >= 75.0) return BatteryStatus::Full;
    if (battery_ >= 50.0) return BatteryStatus::Medium;
    if (battery_ >= 25.0) return BatteryStatus::Low;
    return BatteryStatus::Empty;
}

bool Robot::isOperational() const {
    if (!isValid() || !isTemperatureValid())          return false;
    if (battery_ < 25.0)                              return false;
    if (temperature_ < -10.0 || temperature_ > 60.0)  return false;
    return true;
}

double Robot::averageBattery() const {
    if (history_.empty()) return 0.0;
    double sum = 0.0;
    for (const double& v : history_) {
        sum += v;
    }
    return sum / history_.size();
}

double Robot::minBattery() const {
    if (history_.empty()) return 0.0;
    return *std::min_element(history_.begin(), history_.end());
}

double Robot::maxBattery() const {
    if (history_.empty()) return 0.0;
    return *std::max_element(history_.begin(), history_.end());
}

// ---------------------------------------------------------------- 写

bool Robot::setBattery(double b) {
    if (b < 0.0 || b > 100.0) return false;
    battery_ = b;
    return true;
}

bool Robot::setTemperature(double t) {
    if (t < -20.0 || t > 80.0) return false;
    temperature_ = t;
    return true;
}

bool Robot::recordBattery(double v) {
    if (v < 0.0 || v > 100.0) return false;
    history_.push_back(v);
    battery_ = v;
    return true;
}

// ---------------------------------------------------------------- 文件

bool Robot::saveToFile(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return false;

    out << "# robot_state v1\n";
    out << temperature_ << "\n";

    for (std::size_t i = 0; i < history_.size(); ++i) {
        if (i != 0) out << ",";
        out << history_[i];
    }
    out << "\n";

    return static_cast<bool>(out);
}

bool Robot::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    std::string line;

    // 第 1 行：版本
    if (!std::getline(in, line)) return false;
    stripCR(line);
    if (line.rfind("# robot_state v1", 0) != 0) return false;

    // 第 2 行：温度
    if (!std::getline(in, line)) return false;
    stripCR(line);
    double t = 0.0;
    if (!parseDouble(line, t)) return false;
    if (t < -20.0 || t > 80.0) return false;

    // 第 3 行：历史（允许为空行）
    if (!std::getline(in, line)) return false;
    stripCR(line);

    std::vector<double> loaded;
    if (!line.empty()) {
        std::istringstream iss(line);
        std::string item;
        while (std::getline(iss, item, ',')) {
            double v = 0.0;
            if (!parseDouble(item, v)) return false;
            if (v < 0.0 || v > 100.0) return false;
            loaded.push_back(v);
        }
    }

    // 全部校验通过，才真正改动对象
    temperature_ = t;
    history_ = std::move(loaded);
    if (!history_.empty()) {
        battery_ = history_.back();
    }
    return true;
}

// ---------------------------------------------------------------- 输出

void Robot::print() const {
    std::cout << "Battery: " << battery_ << "%"
              << "  Temp: " << temperature_ << "C"
              << "  Status: " << status()
              << "  [" << (isOperational() ? "operational" : "NOT operational") << "]\n";

    if (history_.empty()) {
        std::cout << "  History: (empty)\n";
    } else {
        std::cout << "  History: " << history_.size() << " records"
                  << "  avg=" << averageBattery()
                  << "  min=" << minBattery()
                  << "  max=" << maxBattery() << "\n";
    }
}
