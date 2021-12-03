#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<assert.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>
#include<libgen.h>

//本服务器主要考察了dup函数的使用。
int main(int argc,char* argv[]){
    if(argc<=2){
	printf("usage:%s ip_address port_number\n",basename(argv[0]));
	return 1;
	}
    //参数赋值
    const char* ip =argv[1];
    int port = atoi(argv[2]);

    //创建socket地址
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);
    
    //创建socket
    int sock = socket(PF_INET,SOCK_STREAM,0); 
    assert(sock>=0);

    //命名socket
    int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);
    
    //监听socket
    ret = listen(sock,5);
    assert(ret!=-1);

    //accept连接
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock,(struct sockaddr*)&client,&client_addrlength);
    if(connfd<0){
	printf("errno is:%d\n",errno);
}
    else{
	close(STDOUT_FILENO);
	dup(connfd);
	printf("abcd\n");//可以这么理解。本来printf是向文件描述符是1的地方写入东西。本来1是属于标准输出的，所以printf就是往标准输出(命令行)写东西。但是通过dup函数，把1这个地方指向了connfd，所以现在往1写就是往connfd写，就是往客户端写。
	close(connfd);
}
    close(sock);
    return 0;
}
