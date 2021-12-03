#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<assert.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>

#define BUFFER_SIZE 512


int main(int argc,char *argv[]){

    //参数赋值
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    //创建服务器端socket地址
    struct sockaddr_in server_address;
    bzero(&server_address,sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&server_address.sin_addr);
    server_address.sin_port = htons(port);
    
    //创建客户端socket
    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock>=0);
    
    //设置客户端socket的属性。
    int sendbuf = atoi(argv[3]);//发送缓冲区的大小
    int len = sizeof(sendbuf);
    setsockopt(sock,SOL_SOCKET,SO_SNDBUF,&sendbuf,sizeof(sendbuf));
    getsockopt(sock,SOL_SOCKET,SO_SNDBUF,&sendbuf,(socklen_t*)&len);
    printf("the tcp send buffer size after setting is %d\n",sendbuf); 
    
    if(connect(sock,(struct sockaddr*)&server_address,sizeof(server_address))!=-1){
	char buffer[BUFFER_SIZE];
	memset(buffer,'a',BUFFER_SIZE);
	send(sock,buffer,BUFFER_SIZE,0);
}
    close(sock);
    return 0;
}
