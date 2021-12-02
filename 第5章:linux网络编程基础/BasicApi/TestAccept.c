#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>

#include <libgen.h>

int main(int argc,char *argv[]){
    if(argc<=2){
        printf("usage:%s ip_address port_number\n",basename(argv[0]));
	return 1;
    }
    //参数赋值
    const char* ip = argv[1];
    int port = atoi(argv[2]);//atoi是字符串转为整型的函数。
    
    //创建socket
    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock>=0);

    //创建socket地址
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    //命名socket
    int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);
    
    //监听socket
    ret = listen(sock,5);
    assert(ret!=-1);

    sleep(20);

    //接受连接。通过前面的创建、命名、监听socket，一个服务器的socket已经建立完成且可以被客户端连接。但是客户端连接之后怎么得到客户端的具体信息呢？利用accpet得到。    
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock,(struct sockaddr*)&client,&client_addrlength);  
    if(connfd<0){
        printf("error is: %d\n",errno);
    }
    else {
        char remote[INET_ADDRSTRLEN];
        printf("connected with ip:%s and port: %d\n",inet_ntop(AF_INET,&client.sin_addr,remote,INET_ADDRSTRLEN),ntohs(client.sin_port));//inet_ntop函数将网络字节序的ip地址转化为字符串的ip地址，并把结果存放到remote里面。ntos函数将网络字节序的端口号整型转换为主机字节序的端口号。
    }
    close(sock);
    return 0;
} 
