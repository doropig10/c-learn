#include "robot.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

// 匿名命名空间：里面的函数只在本文件内可见，不污染全局，避免命名冲突
// 相当于 C 语言的 static 函数
namespace {

// 剥掉字符串末尾的 \r（回车符）
// Windows 换行是 \r\n，Linux/Mac 是 \n；getline 遇到 \n 停下但会留下 \r
// 不剥掉的话 "20\r" 解析时 pos 对不上长度，会误判为非法
void stripCR(std::string& s) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
    // 【问】pop_back 是哪个类的？
    // 【答】s 是 std::string，所以是 std::string::pop_back()，删最后一个字符。
    //       vector/deque/list 也有同名 pop_back，但这里调用者是 string。
}

// 严格把字符串解析成 double
// s：输入字符串；out：解析出的数字通过引用写回
// 返回 true=成功，false=失败
bool parseDouble(const std::string& s, double& out) {
    if (s.empty()) return false;            // 空串直接失败
    std::size_t pos = 0;                    // 记录 stod 解析到第几个字符
    try {
        out = std::stod(s, &pos);          // string to double：字符串转浮点数
        // 【问】文件第二行什么时候变成 t 的？
        // 【答】就是这一行！std::stod 把字符串 "20" 解析成数字 20.0，通过引用写进 out（即 t）。
        //       前面 getline 读到的是字符串 line="20"，到这里才变成 double t=20.0。
    } catch (const std::exception&) {
        return false;                       // 串根本不是数字（如 "abc"），stod 抛异常
    }
    // 要求整个串都被解析完，不能有多余字符（如 "20abc" 只解析出 20，剩下 abc 没解析，拒绝）
    // 【问】return 不是 pos 吗？
    // 【答】不是。return 返回的是 pos == s.size() 这个比较的结果（true/false），
    //       不是 pos 本身。意思是"整个字符串是否被干净地解析完了"。
    return pos == s.size();
}

}  // namespace

// ---------------------------------------------------------------- 枚举

// 把 BatteryStatus 枚举转成可读字符串
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

// 重载 << 运算符：让枚举能直接用 cout 打印成文字（而不是底层数字 0/1/2）
// 返回 ostream& 是为了支持链式调用：cout << a << b << c
// 【问】这个函数干什么？
// 【答】给 BatteryStatus 枚举重载 <<，这样 cout << status() 会打印 "Empty" 而不是数字 3。
//       它内部调 toString(s) 把枚举转成字符串，再写进流。
std::ostream& operator<<(std::ostream& os, BatteryStatus s) {
    return os << toString(s);
}

// ---------------------------------------------------------------- 构造

// 构造函数：初始化列表直接给成员赋值，默认电量100、温度20
// 【问】battery_(battery) 和在函数体里写 battery_ = battery; 有什么区别？
// 【答】初始化列表是"对象出生时就是这个值"；函数体赋值是"先默认构造、再改一遍"。
//       对 double 结果一样，但有三种成员只能走初始化列表：const 成员、引用成员、
//       没有默认构造函数的类成员（包括基类）。
// 【问】形参 battery 和成员 battery_ 是同一个东西吗？
// 【答】不是。形参是调用时复制进来的临时副本，battery_ 是对象自己的成员。
//       battery_(battery) 就是"用副本初始化成员"。下划线后缀专门用来区分这两者。
Robot::Robot(double battery, double temperature)
    : battery_(battery), temperature_(temperature) {}

// ---------------------------------------------------------------- 读

// 下面都是 const 成员函数：不修改对象，只读
double Robot::battery() const     { return battery_; }       // 读当前电量
double Robot::temperature() const { return temperature_; }    // 读当前温度
std::size_t Robot::historySize() const { return history_.size(); }  // 历史记录条数

// 电量是否合法（0~100）
bool Robot::isValid() const {
    return battery_ >= 0.0 && battery_ <= 100.0;
}

// 温度是否合法（-20~80）
bool Robot::isTemperatureValid() const {
    return temperature_ >= -20.0 && temperature_ <= 80.0;
}

// 电量分档：≥75满电 ≥50中等 ≥25低 否则空；非法返回Invalid
// 【问】为什么用 >= 而不是 >？
// 【答】边界值必须由你明确指定。写成 > 75 的话，75 会掉进"中等"档，和"75 起算满电"的意图相反。
// 【问】第一行 if (!isValid()) 是干什么的？
// 【答】兼作"非法先拦"。电量 120 如果不先拦，会因为 >= 75 被误判成 Full；
//       先拦掉之后直接返回 Invalid，后面的分档根本不参与。
BatteryStatus Robot::status() const {
    if (!isValid())       return BatteryStatus::Invalid;
    if (battery_ >= 75.0) return BatteryStatus::Full;
    if (battery_ >= 50.0) return BatteryStatus::Medium;
    if (battery_ >= 25.0) return BatteryStatus::Low;
    return BatteryStatus::Empty;
}

// 是否可运行：电量温度都合法 + 电量≥25 + 温度在[-10,60]
bool Robot::isOperational() const {
    if (!isValid() || !isTemperatureValid())          return false;
    if (battery_ < 25.0)                              return false;
    if (temperature_ < -10.0 || temperature_ > 60.0)  return false;
    return true;
}

// 历史电量平均值（历史为空返回0）
double Robot::averageBattery() const {
    if (history_.empty()) return 0.0;
    double sum = 0.0;
    for (const double& v : history_) {
        sum += v;
    }
    return sum / history_.size();
}

// 历史电量最小值（历史为空返回0）
// 【问】空历史返回 0 有问题吗？
// 【答】有。电量真的为 0 时也返回 0，于是"没有数据"和"最低就是 0"分不清。
//       更诚实的写法是返回 std::optional<double>，或用 bool minBattery(double& out)
//       让返回值表示"有没有数据"。现阶段够用，但要知道这个 0 是"善意的谎言"。
double Robot::minBattery() const {
    if (history_.empty()) return 0.0;
    return *std::min_element(history_.begin(), history_.end());
}

// 历史电量最大值（历史为空返回0）
double Robot::maxBattery() const {
    if (history_.empty()) return 0.0;
    return *std::max_element(history_.begin(), history_.end());
}

// ---------------------------------------------------------------- 写

// 设置电量（带校验），不合法返回false且不修改
// 【问】值是怎么进到对象里的？函数怎么知道改的是哪个对象？
// 【答】调用 r.setBattery(45) 时：45 先复制给形参 b，再赋值给 this->battery_。
//       编译器给每个成员函数加了一个隐藏参数 this（类型 Robot*），
//       相当于 setBattery(&r, 45)——同一份代码，操作的是不同对象的数据。
// 【问】为什么必须"先校验、再赋值"，不能反过来？
// 【答】反过来写的话，非法值已经写进对象了，再 return false 也救不回来。
//       所有校验都必须在"改任何成员之前"完成。
bool Robot::setBattery(double b) {
    if (b < 0.0 || b > 100.0) return false;
    battery_ = b;
    return true;
}

// 设置温度（带校验）
bool Robot::setTemperature(double t) {
    if (t < -20.0 || t > 80.0) return false;
    temperature_ = t;
    return true;
}

// 记录一条电量历史：追加到历史数组，同时更新当前电量
// 【问】为什么一个函数里既 push_back 又改 battery_？会不会职责太多？
// 【答】一次"采样"天然同时意味着两件事：记进历史 + 当前值更新为最新读数。
//       判断标准是需求，不是函数长短：将来若出现"只记历史、不改当前值"的需求
//       （比如导入一批旧数据），就该拆成两个函数。
bool Robot::recordBattery(double v) {
    if (v < 0.0 || v > 100.0) return false;
    history_.push_back(v);   // 追加到历史末尾
    battery_ = v;           // 当前电量也设为这个值
    return true;
}

// ---------------------------------------------------------------- 文件

// 写入存档：内存 → 文件
// 格式：3行 = 版本头 + 温度 + 历史(逗号分隔)
// 【问】saveToFile 是写入，loadFromFile 是读取，那为什么一个带 const 一个不带？
// 【答】const 管的是"改不改 Robot 对象"，不是"碰不碰文件"。
//       saveToFile：只读成员、写磁盘 → 不改对象 → 可以 const；
//       loadFromFile：把文件内容写进成员 → 改对象 → 不能 const。
//       对照 print() const：它写屏幕，同样不改对象，所以也是 const。
// 【问】除了方向相反，两者还有什么区别？
// 【答】信任程度不同。save 端的数据是内存里的合法值，直接按格式写；
//       load 端的数据来自外部（可能被手改、可能是旧版本、可能写了一半），必须逐行校验。
// 【问】写到哪里？
// 【答】path 参数传进来的路径，main 里写死 "data/robot_state.txt"（相对路径，相对程序运行目录）。
bool Robot::saveToFile(const std::string& path) const {
    // ios::trunc：截断重写，每次写都覆盖旧文件
    // 【注意】打开的瞬间旧内容就没了。如果写到一半失败（磁盘满、拔盘），
    //       原数据也回不来——"保存失败反而把好数据毁了"。
    //       真正的原子写是：先写 path + ".tmp"，全部成功后用 rename 替换正式文件。
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return false;   // 文件打不开就失败

    out << "# robot_state v1\n";        // 第1行：版本头
    out << temperature_ << "\n";        // 第2行：温度

    // 第3行：历史电量，逗号分隔（第一个前面不加逗号）
    for (std::size_t i = 0; i < history_.size(); ++i) {
        if (i != 0) out << ",";
        out << history_[i];
    }
    out << "\n";

    // 检查流状态：写成功才返回true
    return static_cast<bool>(out);
}

// 读取存档：文件 → 内存
// 核心设计：先逐行校验，全部通过才真正修改对象（防脏数据）
// 【问】读档到底走了哪些步骤？
// 【答】三道校验门：①版本头 ②温度 ③历史 → 全过才写内存。
//       任何一道失败都 return false，对象保持读档前原样。
bool Robot::loadFromFile(const std::string& path) {
    std::ifstream in(path);             // 只读打开
    if (!in.is_open()) return false;    // 文件不存在/打不开，保持默认值

    std::string line;   // 临时存每行读到的字符串

    // 第 1 行：版本头校验
    if (!std::getline(in, line)) return false;  // 读不到行就失败
    stripCR(line);                               // 剥掉行尾 \r
    // rfind(...,0)==0 表示"以这个串开头"，即版本号匹配
    if (line.rfind("# robot_state v1", 0) != 0) return false;

    // 第 2 行：温度解析+校验
    if (!std::getline(in, line)) return false;
    stripCR(line);
    double t = 0.0;                     // 【问】t 是什么？
    // 【答】t 是 loadFromFile 内部的临时局部变量，用来存从文件第2行解析出来的温度。
    //       函数结束就销毁。之所以先放临时变量，是为了校验通过前不碰对象的 temperature_。
    if (!parseDouble(line, t)) return false;       // 字符串 → double
    if (t < -20.0 || t > 80.0) return false;     // 范围校验

    // 第 3 行：历史电量（允许为空行）
    if (!std::getline(in, line)) return false;
    stripCR(line);

    std::vector<double> loaded;         // 临时数组：先存解析好的历史
    if (!line.empty()) {
        // istringstream：把字符串包成输入流，就能用 getline 按逗号切分
        // 【问】std::istringstream iss(line) 这行干什么？
        // 【答】把字符串 line="80,55.5,12" 包成一个流，这样下一行 getline(iss, item, ',')
        //       就能像从文件读一样，按逗号逐段切出 "80"、"55.5"、"12"。
        std::istringstream iss(line);
        std::string item;
        while (std::getline(iss, item, ',')) {   // 按逗号逐段切
            double v = 0.0;
            if (!parseDouble(item, v)) return false;  // 每段都要严格解析
            if (v < 0.0 || v > 100.0) return false;  // 每段都要范围校验
            loaded.push_back(v);
        }
    }

    // ===== 全部校验通过，才真正改动对象（事务性：全有或全无）=====
    // 前面如果任何一步失败都 return false 了，走到这里说明数据全合法
    // 【问】为什么非要用临时变量 loaded，不能边解析边 push_back 进 history_？
    // 【答】如果直接写成员，读到第 3 个值是 "abc" 时，前两个值已经进对象了，
    //       对象就处于"一半新一半旧"的脏状态。先存临时 vector、全部通过再一次性赋值，
    //       才能保证"要么全成功，要么一个字节都不改"。
    //       （注意："事务性"保护的是内存；文件的原子性要靠 tmp 文件 + rename，是另一回事。）
    // 【问】temperature_ = t 这行什么意思？
    // 【答】把校验过的临时温度 t，正式写进对象成员 temperature_。
    //       前面 t 一直是临时变量，这行才真正改对象。
    temperature_ = t;
    // 【问】std::move(loaded) 干什么？
    // 【答】移动语义：不拷贝整个数组，直接把 loaded 的内存所有权转交给 history_。
    //       执行后 loaded 变空，但没人再用它，省掉一次大数组拷贝。
    history_ = std::move(loaded);
    if (!history_.empty()) {
        // 【问】为什么 battery_ 取 history_.back()（最后一个值）？
        // 【答】历史是按时间顺序 push_back 的，最后一条就是最新电量读数。
        //       比如 80→55.5→12，当前电量就是 12%。
        battery_ = history_.back();
    }
    return true;
}

// ---------------------------------------------------------------- 输出

// 打印当前状态：电量、温度、档位、是否可运行、历史统计
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
