#include "robot.hpp"

#include <algorithm>
#include <iostream>
#include <string>

int main() {
    // ===== 存档路径（相对路径，相对程序运行时的工作目录）=====
    // 【问】写到哪里？为什么一定要在项目根目录运行？
    // 【答】这是相对路径，相对的是"当前工作目录"，不是 exe 所在位置。
    //       在 E:\code\C++\3 运行 → 解析成 E:\code\C++\3\data\robot_state.txt        ✓
    //       在 build\ 里运行     → 解析成 E:\code\C++\3\build\data\...（目录不存在） ✗
    //       这不是代码错，是位置错。以后遇到"文件明明在，程序说找不到"，先查工作目录。
    const std::string path = "data/robot_state.txt";

    Robot r;   // 构造默认机器人：电量100、温度20

    // ===== STL 算法 + Lambda 练习 =====
    std::vector<double> h = {80, 55.5, 12};

    // count_if：遍历数组，数满足 lambda 条件的元素个数
    // lambda [](double v){ return v > 50; } = 判断 v 是否大于50
    // 80✓ 55.5✓ 12✗ → over50 = 2
    // 【问】count_if 和这个 lambda 是干嘛的？
    // 【答】count_if 遍历整个数组，lambda 是判断规则，返回满足条件的元素个数。
    int over50 = std::count_if(h.begin(), h.end(), [](double v) { return v > 50; });
    std::cout << "over 50: " << over50 << "\n";       // 2

    // sort + lambda：a>b 表示降序（从大到小）
    std::sort(h.begin(), h.end(), [](double a, double b) { return a > b; });
    std::cout << "sorted desc: ";
    for (double v : h) std::cout << v << " ";          // 80 55.5 12（本来就是降序）
    std::cout << "\n";

    // lambda 值捕获 [threshold]：把外部变量 threshold=40 拷贝进 lambda
    // 数 v < 40 的个数：80✗ 55.5✗ 12✓ → below = 1
    // 【问】[threshold] 方括号里写变量名是什么意思？
    // 【答】值捕获：lambda 创建时把 threshold=40 拷贝一份存进自己内部。
    //       这样 lambda 就能用外部变量。如果用 [&threshold] 引用捕获，局部变量函数返回后地址会悬空。
    double threshold = 40.0;
    auto below = std::count_if(h.begin(), h.end(),
                               [threshold](double v) { return v < threshold; });
    std::cout << "below " << threshold << ": " << below << "\n";   // 1


    // ===== 读档：从文件加载状态 =====
    // 三道校验门全过才会真正修改对象；失败则保持默认值
    // 【问】读档成功后，main 接下来干什么？
    // 【答】①打印状态 ②判断历史是否为空，空才补录 ③把读进来的数据原样存回文件。
    //       因为文件里已有3条历史，补录条件不成立，所以这次运行不写新数据。
    if (r.loadFromFile(path)) {
        std::cout << "loaded from " << path << "\n";
    } else {
        std::cout << "no valid data, using defaults\n";
    }
    r.print();   // 打印读档后的状态

    // 只在没有历史时才记录，避免每次运行都追加
    // 【问】为什么要这个判断？
    // 【答】不判断的话每运行一次就多 3 条，跑 10 次变 30 条。加上判断后程序是"幂等"的：
    //       反复运行，结果一样。真实项目里这里会换成传感器读数或 ROS2 话题消息，
    //       而不是写死的 80 / 55.5 / 12——那三条只是让第一次运行有东西可看的脚手架。
    // 【问】主程序怎么没使用 setBattery/setTemperature？
    // 【答】这两个公开方法当前 demo 用不上——main 靠读档和 recordBattery 设置电量，
    //       靠默认值和读档设置温度。它们是给别的场景预留的接口。
    if (r.historySize() == 0) {
        // 这三个值是写死的常量，确定合法，显式忽略返回值
        // 【补充】检查返回值有三种写法，按场景选：
        //   ① if (ok) { 成功处理 } else { 失败处理 }   —— 两种结果都要管
        //   ② if (!ok) { 报错 }                        —— 只关心失败
        //   ③ (void)expr;                              —— 明确忽略（配合 [[nodiscard]] 表达"我知道，故意不看"）
        (void)r.recordBattery(80.0);
        (void)r.recordBattery(55.5);
        (void)r.recordBattery(12.0);
        std::cout << "recorded 3 samples\n";
        r.print();
    }

    // ===== 存档：把当前状态写回文件 =====
    if (!r.saveToFile(path)) {
        std::cout << "save failed\n";
    } else {
        std::cout << "saved to " << path << "\n";
    }
}
