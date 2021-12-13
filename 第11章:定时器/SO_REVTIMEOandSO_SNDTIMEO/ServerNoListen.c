#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<assert.h>

int main(int argc,char* argv[]){

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET,SOCK_STREAM,0);
    assert(sockfd>=0);
  
    int ret = bind(sockfd,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);

    ret = listen(sockfd,5);
    assert(ret!=-1);
    while(1){

}
}
