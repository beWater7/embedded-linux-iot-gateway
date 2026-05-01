### 介绍
    本项目实现了一个shellServer线程库，demo中启动了该线程，
    启动线程后支持使用regshellcmd注册自定义函数，目前注册了一个设置日志打印等级的函数

+ shellserver负责注册回调函数节点，接收客户端的数据，返回函数的执行结果给client
    
+ shelltools client客户端, 负责接收用户数据发送至服务端，并显示服务器返回的数据

+ tips ：
  ./demo & 启动shellserver服务线程
  ./shelltools 指令 xxx(参数) 或使用 ln -s  shelltools 指令创建软链接后执行./指令 xxx(参数)
  如果你想创建一个全局可用的指令软链接，可以在/usr/sbin 目录下创建此软链接
  然后使用/usr/sbin/setDebugLevel, 这样还是需要加路径，如果你希望直接调用指令，
  可以使用ln -s 指令 /usr/local/bin/linkname (全部使用绝对路径),
  可以使用 echo $PATH 查看 /usr/local/bin/ 路径是否在当前环境变量下
  直接执行setDebugLevel就可以，不需要./setDebugLevel
