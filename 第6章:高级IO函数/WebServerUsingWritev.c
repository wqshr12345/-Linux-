#include<sys/socket.h>
#include<string.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc,char *argv[]){
    //参数赋值
    const char* ip = argv[1];
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
    int ret = bind(sock,address,sizeof(address));
    assert(ret!=-1);

    //监听socket
    ret = listen(sock,5);
    assert(ret!=-1);

    //accept连接
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock,(struct sockaddr*)&client,&client_addrlength);
    if(connfd<0){
	printf("errno is: %d\n",errno);

    }
    else{
	char header_buf[BUFFER_SIZE];
        memset(header_buf,'\0',BUFFER_SIZE);
	char* file_buf;
        struct stat file_stat;
        bool valid = true;
        int len = 0;
        //---to do---//


}




}
