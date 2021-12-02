#include<sys/types.h>
#include<sys/socket.h>

//创建一个socket

int socket(int domain,int type,int protocol);//domain代表底层协议族（ipv4还是ipv6） type代表服务类型（选定tcp还是udp） protocol代表选择的具体协议(一般由前两个确定，所以设置为0，默认就好)



//命名socket

//前面创建socket仅仅指定了该socket的协议类型(比如ip tcp)，但并没有给定具体的地址(比如端口3 端口2241)。将一个socket与一个socket地址绑定，叫做给socket命名。

int bind(int sockfd,const struct sockaddr* my_addr,socklen_t addrlen);//该函数就是把my_addr所指向的socket地址分配给未命名的sockfd文件描述符，addrlen指socket地址的长度。


//监听socket

//socket被命名后，不能立刻接受客户连接，需要创建一个监听队列存放待处理的客户连接。这里默认是服务器，可能会接收到来自多个端口的客户的连接。

int listen(int sockfd,int backlog);//sockfd表示被监听的服务器socket，backlog表示监听队列的最大数量。
