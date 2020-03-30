# A C++ High Performance TCP Server
[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)

## Introduction


| Part Ⅰ | Part Ⅱ | Part Ⅲ | Part Ⅳ | Part Ⅴ | Part Ⅵ |
| :--------: | :---------: | :---------: | :---------: | :---------: | :---------: |
|[项目目的](docs/项目目的.md)|[并发模型](docs/并发模型.md) |[核心流程](docs/[核心流程.md)|[核心结构](docs/核心结构.md) |[水平触发](docs/水平触发.md) |[项目测试](docs/逻辑收发.md) |

## Envoirment

* OS: Ubuntu 16.04  
* Complier: gcc 5.4.0

## Build
```
make
```

## Technical points

* 使用Epoll水平触发的IO多路复用技术，非阻塞IO，使用Reactor模式
* 使用连接池 维护一套空闲连接 减少连接创建时间 提高了性能
* 使用线程池来处理业务逻辑，调用适当的业务逻辑函数处理业务并返回结构
* 专门处理数据包的一整套数据发送逻辑以及对应的发送线程
* 其他：信号 日志 守护进程等

## Model 
* [Reactor模型:](docs/[并发模型.md) 同步事件循环 + 非阻塞I/O + 线程池

## Code statistics
![](https://gitee.com/shixianguo/blogimage/raw/master/img/20200330234155.png)

## Others