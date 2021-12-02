#include<sys/socket.h>
#include<stdio.h>
#include <libgen.h>
#include<stdlib.h>
#include <arpa/inet.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<assert.h>
#include<errno.h>
#define BUF_SIZE 1024
int main(int argc,char* argv[]){
    if(argc<=2){
	printf("usage: %s ip_address port_number\n",basename(argv[0]));
	return 1;
    }
    //参数赋值
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    //创建socket。
    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock>=0);

    //创建socket地址。服务器需要显式地给一个地址，客户端可以由操作系统默认分配。
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);
    
    //命名socket。
    int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);
    
    //监听socket。创建监听队列来存放待处理的客户端列表。
    ret = listen(sock,5);
    assert(ret!=-1);
   
    //接受连接。 
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock,(struct sockaddr*)&client,&client_addrlength);
    if(connfd<0){
	printf("errno is : %d\n",errno);
	}
    else{
	char buffer[BUF_SIZE];
	memset(buffer,'\0',BUF_SIZE);
	ret = recv(connfd,buffer,BUF_SIZE-1,0);
	printf("got %d btyes of normal data '%s'\n",ret,buffer);

 	memset(buffer,'\0',BUF_SIZE);
        ret = recv(connfd,buffer,BUF_SIZE-1,MSG_OOB);
        printf("got %d btyes of normal data '%s'\n",ret,buffer);

	memset(buffer,'\0',BUF_SIZE);
        ret = recv(connfd,buffer,BUF_SIZE-1,0);
        printf("got %d btyes of normal data '%s'\n",ret,buffer);
 	
	close(connfd);
}
close(sock);
return 0;
}

