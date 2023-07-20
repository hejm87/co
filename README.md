# co
simple coroutine framework

[存在问题]
1. channel存在两个问题（不休眠情况下）：
(1) channel close exception 框架需要捕获，避免协议泄露
(2) 显式设置协程状态的模式需要去除  --> 完成

[后续开发]
1. channel非协程环境下使用  --> 完成
2. channel带cache模式下功能逻辑 --> 完成
3. create co 带返回值	--> 完成
4. co api封装	--> # 还是取消co api的封装吧
5. 非协程环境getcid()返回-1 --> 完成
6. create_co可以强制多生成协程（信号量类进行修改，避免使用create_co）	--> 完成
7. 异常类型 --> 完成
8. 内核框架调试日志打印
9. 调度线程处理协程调度优化
   (1) 目前CoSchedule为空闲时每毫秒进行轮询，需优化   --> 完成
   (2) 定时器需要CoSchdule进行触发  --> 完成
10. CoSchedule代码优化
   (1) 把没用的lst_xxx队列删除   --> 完成
11. 单元测试使用gtest
12. 协程切换性能
13. 将测试拆分按功能拆分多个小单元

[异步IO操作开发]
1. 定义CoEpollEngine, SocketContext
2. hook socket相关函数：connect, send, recv, close, setsockopt, getsockopt, sleep, usleep
3. 支持tcp, udp协议类型


[遗留问题]
1. 协程设置4k栈时，throw异常会导致堆栈数据破坏问题
2. 好像遗留一个CoManager结束时，某个协程线程卡顿导致CoManager不能正常释放问题（概率非常低）
