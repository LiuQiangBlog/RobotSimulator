https://github.com/cross-platform/dspatch 这是一个数据流编程后端框架，其实可以跟imgui或qt的node前端框架结合，得到一个拖拽框架

DSPatch（发音为"dispatch"）是一个强大的 C++数据流框架。
该框架不局限于任何特定领域或数据类型，从响应式编程到流处理，DSPatch 通用的面向对象 API 允许您构建几乎任何能想象到的图处理系统。

https://github.com/cross-platform/dspatcher 这是对应的前端框架，不过它用的是qt来实现的

https://github.com/cross-platform/dspatchables


https://github.com/arximboldi/lager 基于单向数据流架构的 C++值导向设计库——C++版 Redux


https://github.com/paceholder/nodeeditor 基于Qt的节点编辑器，一个非常经典的节点编程库，我记得2019年就关注到了它，现在有3k的star。

需要说明的是，因为nodeeditor已经使用qt6框架了，所以你需要先安装qt6，然后在cmake中设置
set(Qt6_DIR "/home/liuqiang/Qt/6.9.0/gcc_64/lib/cmake/Qt6")
set(QT_DIR "/home/liuqiang/Qt/6.9.0/gcc_64/lib/cmake/Qt6")

nodeeditor库支持将数据流场景到.flow文件中，也支持从flow文件中加载数据流场景

https://github.com/chigraph/chigraph 类似nodeeditor
https://github.com/chigraph/chigraph-gui 这是它的前端




