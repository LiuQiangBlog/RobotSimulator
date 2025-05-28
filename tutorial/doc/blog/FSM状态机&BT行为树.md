有限状态机框架

- QP/C++ https://github.com/QuantumLeaps/qpcpp
QP/C++实时事件框架（RTEF）是一种轻量级的异步、事件驱动型主动对象（又称 Actor）计算模型实现，专为实时嵌入式系统（如微控制器 MCU）设计。
QP/C++既是构建由主动对象（Actor）组成的应用程序的软件基础设施，也是以确定性实时方式执行这些主动对象的运行时环境。
此外，QP/C++框架支持分层状态机，用于规范主动对象的行为[参考 UML 2.5 标准][Sutter:10][ROOM:94]。
该框架可视为一种现代化的、异步且真正事件驱动的实时操作系统（RTOS）。
- https://github.com/QuantumLeaps/qpcpp-examples
- TinyFSM
- Boost.Statechart

建议优先选择QP/C++，其"活动对象"模式天然适配您的多线程解析场景，且能通过内置的线程安全机制避免手动管理锁带来的风险。
若需快速验证原型，可先用TinyFSM搭建基础逻辑，再逐步迁移至QP框架。

行为树框架 https://github.com/BehaviorTree/BehaviorTree.CPP

针对QP状态机库，有一本中文教材 《uml状态图的实用cc++设计第二版pdf》



