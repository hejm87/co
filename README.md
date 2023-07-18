# co
simple coroutine framework

[存在问题]
1. channel存在两个问题（不休眠情况下）：
(1) channel close exception 框架需要捕获，避免协议泄露
(2) 显式设置协程状态的模式需要去除

[后续开发]
1. channel非协程环境下使用
2. channel带cache模式下功能逻辑~
3. create co 带返回值
4. co api封装
5. 非协程环境getcid()返回-1~
6. create_co可以强制多生成协程
7. 异常类型

[遗留问题]
1. 协程设置4k栈时，throw异常会导致堆栈数据破坏问题
