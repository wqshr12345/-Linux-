#include<sys/socket.h>
#include<assert.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#define BUFFER_SIZE 1024
int main(int argc,char* argv[]){

    //参数赋值
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    //创建socket地址。
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = port;

    //创建socket。
    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock>=0);
    
    //命名socket。
    int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);
    
    //设置缓冲区大小
    int recvbuf = atoi(argv[3]);
    int len = sizeof(recvbuf);
    setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&recvbuf,sizeof(recvbuf)); 
    getsockopt(sock,SOL_SOCKET,SO_RCVBUF,&recvbuf,(socklen_t*)&len);
    printf("the tcp receive buffer size after setting is %d\n",recvbuf);

    //监听socket。
    ret = listen(sock,5);
    assert(ret!=-1);

    //accept连接。
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock,(struct sockaddr*)&client,&client_addrlength);
    if(connfd<0){
        printf("errno is :%d\n",errno);
    }
    else{
	char buffer[BUFFER_SIZE];
	memset(buffer,'\0',BUFFER_SIZE);
	while(recv(connfd,buffer,BUFFER_SIZE-1,0)>0){}
	close(connfd);
}
    close(sock);
    return 0;
}
