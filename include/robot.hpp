#pragma once   // 防止头文件被重复包含

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

// 电量档位枚举：用 enum class 强类型，避免隐式转换
enum class BatteryStatus { Full, Medium, Low, Empty, Invalid };

// 【问】为什么不直接 cout << status()，非要套一层 toString？
// 【答】enum class 刻意不允许隐式转换成 int 或字符串，标准库也没有它的 operator<<，
//       所以 cout << status() 直接编译失败（no match for operator<<）。
//       于是分成两层：toString 负责"枚举 → 字符串"，operator<< 负责"让 cout 认识这个枚举"。
//       toString 还能给日志、文件、ROS2 消息共用；operator<< 只服务流输出。
std::string toString(BatteryStatus s);                          // 枚举转字符串
// 【问】toString 为什么不写成 Robot 的成员函数？
// 【答】它只依赖枚举值，不碰 Robot 的任何成员。写成成员的话，想打印一个从别处
//       拿到的枚举，就得先造一个 Robot——没有道理。自由函数谁都能用。
std::ostream& operator<<(std::ostream& os, BatteryStatus s);  // 重载<<让枚举可直接cout打印

// 【问】这里为什么用 class 而不是 struct？两者到底差在哪？
// 【答】唯一的语言级区别是"默认访问权限"：
//         struct：成员默认 public；  class：成员默认 private。
//       除此之外完全等价——class 里能写 public:，struct 里也能写 private:。
//       习惯用法：struct 表示"一捆公开的数据"（坐标、配置项、消息体）；
//                 class 表示"有封装、有不变式要维持的对象"（Robot 就是，电量必须在 0~100）。
//       这个类有私有成员和校验逻辑，用 class 更贴切。
class Robot {
public:
    // explicit：禁止隐式转换，必须显式调用构造
    // 默认参数：电量100、温度20
    explicit Robot(double battery = 100.0, double temperature = 20.0);

    // ---- 读接口（const：不修改对象）----
    // 【问】const 到底限的是什么？
    // 【答】限的是 *this。const 成员函数里 this 的类型是 const Robot*，
    //       所以写 battery_ = 1; 会直接编译错误；但写文件、打印屏幕、调用别的
    //       const 成员函数都不受影响。const 不是"什么都不许做"，是"不许改自己"。
    // 【问】为什么这些 getter 非要加 const？
    // 【答】外部经常拿到 const Robot&（只读传参、ROS2 回调参数）。
    //       const 对象只能调用 const 成员函数，不加 const 等于"这个对象在任何只读场景下都用不了"。
    double battery() const;          // 当前电量
    double temperature() const;       // 当前温度
    std::size_t historySize() const;  // 历史记录条数

    bool isValid() const;             // 电量是否合法(0~100)
    bool isTemperatureValid() const;  // 温度是否合法(-20~80)
    BatteryStatus status() const;    // 电量分档
    bool isOperational() const;      // 是否可运行

    double averageBattery() const;   // 历史平均电量（空历史返回0）
    double minBattery() const;       // 历史最小电量（空历史返回0）
    double maxBattery() const;       // 历史最大电量（空历史返回0）

    // ---- 写接口（带校验，失败返回false）----
    // 【问】为什么 setter 要返回 bool，而不是 void？
    // 【答】失败是预期内的事（用户输入、文件内容、消息都可能是脏的）。
    //       返回 void 的话，调用者无从知道"到底设成功没有"，失败会静默发生。
    // [[nodiscard]]：再进一步——连"忽略返回值"都不允许，忘了检查就编译警告。
    //       确实想忽略时写 (void)r.setBattery(x); 明确表态"我知道，故意不看"。
    [[nodiscard]] bool setBattery(double b);
    [[nodiscard]] bool setTemperature(double t);
    [[nodiscard]] bool recordBattery(double v);  // 记录一条电量历史

    // ---- 文件读写 ----
    // 【问】两个都在跟文件打交道，为什么一个带 const 一个不带？
    // 【答】const 管的是"改不改 Robot 对象"，不是"碰不碰文件"。
    //       saveToFile 只读成员、写磁盘 → 不改对象 → 可以 const；
    //       loadFromFile 要把文件内容写进成员 → 改对象 → 不能 const。
    [[nodiscard]] bool saveToFile(const std::string& path) const;  // 内存→文件
    [[nodiscard]] bool loadFromFile(const std::string& path);      // 文件→内存

    void print() const;  // 打印当前状态

private:
    // 私有成员：外部不能直接访问，只能通过上面的公开接口操作
    // 【问】既然是 private，为什么 setBattery 内部又能改 battery_？
    // 【答】private 的边界在"类"，不在"对象"，也不在"函数"。类自己的成员函数当然能改；
    //       甚至同一个类的不同对象之间也能互相访问私有成员（a 的方法里可以读 b.battery_）。
    //       它挡住的是"类之外的所有代码"：main 里写 r.battery_ = 50; 编译不过。
    // 【问】那"能改"和"不能改"到底怎么分？
    // 【答】两条独立规则：private 管"谁能碰"，const 管"能不能改"。
    //       r 是普通对象 → 可以通过 setBattery 改；
    //       const Robot& 或 const 成员函数里 → 不能改。
    double battery_;              // 当前电量（0~100）
    double temperature_;          // 当前温度（-20~80）
    std::vector<double> history_; // 电量历史记录
};
