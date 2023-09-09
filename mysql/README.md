# 数据库连接池

- 单例模式，保证唯一
- list实现连接池
- 连接池为静态大小
- 互斥锁实现线程安全

## 单例模式创建

单例模式是一种设计模式，它确保一个类只有一个实例，并提供全局访问点。

## RAII机制释放数据库连接

- RAII全称是“Resource Acquisition is Initialization”，直译过来是“资源获取即初始化”。
- 在构造函数中申请分配资源，在析构函数中释放资源。因为C++的语言机制保证了，当一个对象创建的时候，自动调用构造函数，当对象超出作用域的时候会自动调用析构函数。所以，在RAII的指导下，我们应该使用类来管理资源，将资源和对象的生命周期绑定。
- RAII的核心思想是将资源或者状态与对象的生命周期绑定，通过C++的语言机制，实现资源和状态的安全管理,智能指针是RAII最好的例子。