# Khttpd
a simple http server by c

* 暂时只支持静态文件服务，将来有可能支持动态脚本，以及其他功能
* 支持以daemon进程运行

>编译：
* 由于使用了pthread_create，所以编译时加上-lpthread参数
* gcc -lpthread khttpd.c -o khttpd

>运行：
* 支持配置两个参数：端口和是否以daemon方式运行
* ./khttpd 默认监听8080端口，以daemon方式运行
* ./khttpd -p 8081 -d 1
* ./khttpd -p8081 -d0

* 请求的文件目录与程序同级目录,默认请求./index.html
