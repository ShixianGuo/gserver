# A C++ High Performance TCP Server
[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)

## Introduction


| Part Ⅰ | Part Ⅱ | Part Ⅲ | Part Ⅳ | Part Ⅴ | Part Ⅵ |
| :--------: | :---------: | :---------: | :---------: | :---------: | :---------: |
|[项目目的](docs/项目目的.md)|[项目测试](docs/项目测试.md) |[并发模型](docs/[并发模型.md)|[基础组件](docs/基础组件.md) |[网络通信](docs/网路通信.md) |[逻辑收发](docs/逻辑收发.md) |

## Envoirment

* OS: Ubuntu 16.04  
* Complier: g++ 4.8

## Build
```
make
```

## Technical points

* 使用Epoll水平触发的IO多路复用技术，非阻塞IO，使用Reactor模式
* 使用连接池 
* 使用多线程充分利用多核CPU，并使用线程池避免线程频繁创建销毁的开销
* 专门处理数据逻辑发送的一整套数据发送逻辑
* 其他：信号 日志 守护进程等

## Model 

## Code statistics

## Others