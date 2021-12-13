#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<assert.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>
#include<libgen.h>

//虽然我的代码没有运行成功，但是我还是对于这个SO_SNDTIMEO有一定体会。我觉得这个本质就是为阻塞IO确定一个上限的阻塞时间，你最多阻塞这么久，如果到点了还没有IO事件，那你就返回一个errno。值得注意的是，在非阻塞IO下，如果当时没有数据接受，也会返回相同的ERRNO(比如send的EAGAIN和EWOULDBLOCK)，是不是可以这么理解，非阻塞IO可以认为是定时器为0的IO，而且如果没有数据返回，就返回errno。

int timeout_connect(const char* ip,int port,int time){
    int ret = 0;
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);    

    int sockfd = socket(PF_INET,SOCK_STREAM,0);
    assert(sockfd>=0);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = time;
    socklen_t len = sizeof(timeout);
    ret = setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,&timeout,len);
    assert(ret!=-1);

    ret = connect(sockfd,(struct sockaddr*)&address,sizeof(address));
    
    if(ret==-1){
	if(errno == EINPROGRESS){
	    printf("connecting timeout.process timeout \n");
	    return -1;
	}
	printf("error occur when connecting to server:%d\n",errno);
	return -1;
    }
    return sockfd;
}


int main(int argc,char* argv[]){
    if(argc<=2){
	printf("usage: %s ip_address port_number\n",basename(argv[0]));
	return -1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = timeout_connect(ip,port,10);
    if(sockfd<0){
	return 1;
    }
    return 0;
}
