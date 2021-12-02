#include<sys/socket.h>
#include<stdio.h>
#include <libgen.h>
#include<stdlib.h>
#include <arpa/inet.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<assert.h>
int main(int argc,char* argv[]){
    if(argc<=2){
        printf("usage: %s ip_address port_number\n",basename(argv[0]));
	return 1;
}
    //参数赋值
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    //创建服务器socket地址。
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&server_address.sin_addr);
    server_address.sin_port = htons(port);

    //创建客户端socket。注意，客户端的socket一般不需要bind命名，其采用的端口是由操作系统自动分配的。
    int sockfd = socket(PF_INET,SOCK_STREAM,0);
    assert(sockfd>=0);
 
    //发起连接。
    if(connect(sockfd,(struct sockaddr*)&server_address,sizeof(server_address))<0){
	printf("connection failed\n");
}
  else{
	const char* oob_data = "abc";
        const char* normal_data = "123";
        send(sockfd,normal_data,strlen(normal_data),0);
	send(sockfd,oob_data,strlen(oob_data),MSG_OOB);
	send(sockfd,normal_data,strlen(normal_data),0);
 }
    close(sockfd);
    return 0;

}
