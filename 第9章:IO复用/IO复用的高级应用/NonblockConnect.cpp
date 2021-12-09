#include<sys/socket.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<time.h>
#include<errno.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<string.h>

#define BUFFER_SIZE 1023

int setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

int unblock_connect(const char* ip,int port,int time){
    int ret = 0;

    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET,SOCK_STREAM,0);
    int fdopt = setnonblocking(sockfd);
    //由于是非阻塞的socket，所以这个connect函数会立刻返回。
    ret = connect(sockfd,(struct sockaddr*)&address,sizeof(address));
    if(ret==0){
	//ret为0表示连接建立成功。这时候去掉该sockfd的非阻塞属性？
	printf("connect with server immediately\n");
	fcntl(sockfd,F_SETFL,fdopt);
	return sockfd;
    }
    else if(errno!=EINPROGRESS){
	printf("unblock connect not support\n");
	return -1;
    }
    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(sockfd,&writefds);

    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    //这里的connect和select的运用类似前面recv和select的运用。select监听sock端口，直到有可读事件返回，这时候再调用recv一定能读。同样，这里类似，只不过顺序换了一下。因为recv接受数据是被动的，connect是主动的，所以要先connect，再去到select里等待。这时候，一个客户端可以同时connect多个服务器，因为是非阻塞Socket，所以都是立刻返回。利用IO复用通知这些sock是否可写，再去查看该sock的错误码。
    ret = select(sockfd+1,NULL,&writefds,NULL,&timeout);
    if(ret<=0){
	printf("connection time out\n");
	close(sockfd);
	return -1;
    }
    if(!FD_ISSET(sockfd,&writefds)){
	printf("no events on sockfd found\n");
	close(sockfd);
	return -1;
    }
    int error = 0;
    socklen_t length = sizeof(error);
    if(getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&length)<0){
	printf("get socket option failed\n");
	close(sockfd);
	return -1;
    }
    if(error!=0){
	printf("connection failed after select with the error: %d \n",error);
	close(sockfd);
	return -1;
}
    //如果getsockopt不出错，error也为0.说明这个sock的connect成功了。
    printf("connection ready after select with the socket: %d \n",sockfd);
    fcntl(sockfd,F_SETFL,fdopt);
    return sockfd;
}

int main(int argc,char* argv[]){
    if(argc<=2){
	printf("usage: %s ip_address port_number",basename(argv[0]));
	return -1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    
    int sockfd = unblock_connect(ip,port,10);
    if(sockfd<10){
	return 1;
    }
    close(sockfd);
    return 0;
}
