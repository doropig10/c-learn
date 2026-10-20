#include <iostream>
#include <string>
#include <vector>
#include <memory>

// ============================================================
// 面向对象多态示例：基类 Node + 派生类 BatteryNode/CameraNode
// 核心：虚函数 + 基类指针数组 + 多态调用
// ============================================================
// 【注意】这个文件自带 main()，和 src/main.cpp 里的 main() 是两个同名函数，
//       不能一起链接（会报 multiple definition of `main'）。
//       它是独立练习，单独编译：g++ -std=c++17 class.cpp -o class_demo
//       不要把它加进 CMakeLists 的 add_executable。

class Node {
public:
    // explicit：禁止隐式转换，必须显式写 Node n("lidar")，不能 Node n = "lidar"
    // 构造时把传进来的 name 存进成员 name_
    explicit Node(const std::string& name) : name_(name) {}

    // public getter：外部不能直接碰 protected 的 name_，通过这个只读接口读
    // 返回 const&：不拷贝字符串，效率高，且外部改不了
    const std::string& name() const { return name_; }

    // ① 虚函数：加 virtual 后，派生类可以重新实现
    // 【问】虚函数是什么？加不加 virtual 区别？
    // 【答】不加 virtual：基类指针调用时都调基类版本（静态绑定）。
    //       加了 virtual：运行时根据对象真实类型调对应版本（动态绑定/多态）。
    virtual void onStart() {
        std::cout << "[" << name_ << "] started (default)\n";
    }

    // ② 虚析构 = default：让编译器自动生成，但加上 virtual
    // 【问】为什么析构必须是 virtual？
    // 【答】通过基类指针 delete 派生类对象时，如果析构不是 virtual，
    //       只调 ~Node()，派生类部分不清理 → 内存泄漏。
    //       加了 virtual 才会先调派生类析构再调基类析构。
    virtual ~Node() = default;

protected:
    // ③ protected：派生类能直接访问，外部代码不能
    // 【问】name_ 是什么？为什么用 protected？
    // 【答】name_ 是存节点名字的字符串成员（如 "battery"）。
    //       protected 让 BatteryNode/CameraNode 里能直接用 name_，
    //       但 main 里不能写 b.name_（编译报错），只能通过 public 的 name() 读。
    //       结尾下划线 _ 是命名约定，表示"成员变量"，区别于参数/局部变量。
    std::string name_;
};

// ④ 公有继承：BatteryNode "是一种" Node
class BatteryNode : public Node {
public:
    // ⑤ 构造函数：初始化列表调基类构造，把 "battery" 传进去
    // 【问】BatteryNode() : Node("battery") {} 什么意思？
    // 【答】冒号后是初始化列表，先调基类 Node 的构造函数设置 name_="battery"。
    //       顺序永远是 基类构造 → 成员构造 → 函数体；析构正好相反：派生类 → 基类。
    //       不能在 {} 里"补调"基类构造：进函数体时基类早就构造完了，
    //       写 Node("battery") 只会造个临时对象再立刻销毁，什么都没改到。
    //       而不写初始化列表的话，编译器会去找 Node()（无参版）——Node 只有带参构造，
    //       于是直接报 no matching function for call to 'Node::Node()'。
    BatteryNode() : Node("battery") {}

    // ⑥ override：显式声明"我在重写基类的虚函数"
    // 写错签名（名字、参数、const）编译器直接报错，是安全检查
    // 【问】override 和 virtual 是一回事吗？
    // 【答】不是。基类标了 virtual 之后，派生类里同名同签名的函数自动就是虚函数，
    //       不需要再写 virtual；override 的作用只是让编译器核对"确实重写上了"。
    //       两个都写也可以（virtual void f() override），但 override 是关键的那个。
    void onStart() override {
        std::cout << "[" << name_ << "] started, checking battery\n";
    }
};

class CameraNode : public Node {
public:
    CameraNode() : Node("camera") {}
    void onStart() override {
        std::cout << "[" << name_ << "] started, opening device\n";
    }
};

int main() {
    // 栈上对象：离开 main 自动销毁，不用手动 delete
    BatteryNode b;
    CameraNode c;

    // 直接调用：静态类型就是 BatteryNode/CameraNode，调到各自版本
    b.onStart();
    c.onStart();

    // ⑦ vector<unique_ptr<Node>>：基类智能指针对象数组
    // 【问】为什么不能直接 vector<Node>？
    // 【答】直接装 Node 会发生"对象切片"——派生类部分被切掉，多态失效。
    //       具体过程：vector<Node> 只能存纯 Node 对象，push 一个 BatteryNode 时，
    //       编译器只按 Node 那部分复制（基类拷贝构造），BatteryNode 独有的成员全丢，
    //       之后调 onStart() 走的也是 Node 的版本。
    //       必须装基类指针（这里是智能指针）才能保留完整对象、触发多态。
    // 【问】unique_ptr 是什么？和裸指针 Node* p = new ... 区别？
    // 【答】unique_ptr 是智能指针，包着堆上对象，离开作用域自动 delete，不用手动释放，防泄漏。
    //       make_unique<BatteryNode>() 就是 new BatteryNode() 的安全版。
    // 【问】为什么用 vector<unique_ptr<Node>> 而不是 vector<Node*>？
    // 【答】Node* 不说明"谁负责 delete"，很容易漏；unique_ptr 把"我拥有它"写进了类型，
    //       vector 析构时会逐个 delete，连抛异常的路径也覆盖。用裸指针就得自己写循环释放，
    //       中途 throw 一下就泄漏。
    std::vector<std::unique_ptr<Node>> nodes;
    nodes.push_back(std::make_unique<BatteryNode>());  // 堆上造电池节点
    nodes.push_back(std::make_unique<CameraNode>());  // 堆上造摄像头节点

    // ⑧ 多态调用：n 是 Node*，但实际指向不同对象，虚函数自动调到对应版本
    // 【问】n->onStart() 为什么能跑出不同结果？
    // 【答】底层靠虚表(vtable)：每个类有一张函数地址表，对象里藏指针指向它。
    //       调用时顺对象找到对应类的虚表，查到正确函数再跳过去。
    //       所以调谁的版本取决于对象真实类型，不是指针类型。
    // 【问】基类指针 Node* 和派生类指针 BatteryNode* 区别？
    // 【答】Node* 能指向 Node 或任何派生类对象（向上转型，安全自动）；
    //       BatteryNode* 只能指向 BatteryNode 对象，范围窄。
    for (auto& n : nodes) {
        n->onStart();
    }

    // 【问】栈和堆的区别？
    // 【答】栈：局部对象自动管理，离开 } 自动回收，快但小。
    //       堆：new 出来的对象，要手动 delete（或用智能指针自动管理），大但灵活。
    //       上面 b、c 是栈对象，vector 里的是堆对象（智能指针自动回收）。
}
