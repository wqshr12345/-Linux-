#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>

#include<stdbool.h>
#include <libgen.h>

static bool stop = false;
static void handle_term(int sig){
    stop = true;
}
int main(int argc,char* argv[]){//argv接收3个参数,IP地址、端口号和backlog值
    signal(SIGTERM,handle_term);
    if(argc<=3){
        printf("usage: %s ip_address port_number backlog\n",basename(argv[0]));
        return 1;
    }
    //从argv得到的参数赋值
    const char* ip = argv[1];
    int port = atoi(argv[2]);//atoi函数把字符串转换为整型。
    int backlog = atoi(argv[3]);
   
    //创建socket(使用IP和TCP协议)
    int sock = socket(PF_INET,SOCK_STREAM,0);
   
    //判断socket是否创建成功(即值是否 不是-1)
    assert(socket>=0);
    
    //创建socket地址
    struct sockaddr_in address;
    bzero(&address,sizeof(address));//0初始化
    address.sin_family = AF_INET;//设置地址族
    inet_pton(AF_INET,ip,&address.sin_addr);//设置IP地址。inet_pton表示将字符串表示的IP地址转换为网络字节序整数表示的IP地址。并把结果存到了sin_addr里。
    address.sin_port = htons(port);//设置端口号。htons表示将主机字节序的整型转换为网络字节序的整型。

    //命名socket(绑定socket与socket地址)
    int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));//注意这里用了一种强制转换，转换为sockaddr。所有socket地址结构体在使用的时候都要这么强转一次，因为所有socket编程使用的地址接口都是sockaddr。
    assert(ret!=-1);

    //监听socket
    ret = listen(sock,backlog);
    assert(ret!=-1);

    while(!stop){
        sleep(1);
    }
    close(sock);
    return 0;
}
