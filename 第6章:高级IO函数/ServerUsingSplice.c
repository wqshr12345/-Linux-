#define _GNU_SOURCE
#include<stdio.h>
#include<libgen.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<assert.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
int main(int argc,char* argv[]){
    if(argc<=2){
        printf("usage: %s ip_address port_number",basename(argv[0]));
	return 1;
    } 
    //参数赋值
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    //创建socket地址
    struct sockaddr_in address;
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
	printf("errno is %d",errno);
    }
    else{
	int pipefd[2];
	ret = pipe(pipefd);
	assert(ret!=-1);
        //下面的过程，将传输socket的内容先零拷贝到管道内，再从管道零拷贝回socket里。全程没有使用send和recv(这两个函数需要把数据从socket拷贝到用户态空间)，很高效。
	ret = splice(connfd,NULL,pipefd[1],NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
        assert(ret!=-1);
        ret = splice(pipefd[1],NULL,connfd,NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
 	assert(ret!=-1);
        close(connfd);
}
    close(sock);
    return 0;
}
