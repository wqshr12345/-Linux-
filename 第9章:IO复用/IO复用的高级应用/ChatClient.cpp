#define _GNU_SOURCE 1

#include<stdio.h>
#include<libgen.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<assert.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<poll.h>
#include<fcntl.h>


#define BUFFER_SIZE 64

int main(int argc,char* argv[]){
    if(argc<=2){
	printf("usage: %s ip_address port_number\n",basename(argv[0]));
	return -1;
    }
 
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    //客户端的socket，无需命名，由操作系统随机分配端口号。
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd>=0);
    
    struct sockaddr_in server_address;
    bzero(&server_address,sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&server_address.sin_addr);
    server_address.sin_port = htons(port);

    if(connect(sockfd,(struct sockaddr*)&server_address,sizeof(server_address))<0){
	printf("connection failed\n");
	close(sockfd);
	return -1;
    }
    pollfd fds[2];
    //注册两个文件描述符：一个是标准输入0.一个是sock的sockfd。前者用于从客户这里读取输入，后者用于和服务器交互。
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char read_buf[BUFFER_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    assert(ret!=-1);

    while(1){
	ret = poll(fds,2,-1);
	if(ret<0){
	    printf("poll failure\n");
	    break;
	}
	//为什么不能用recv=0来表示对方关闭了连接？因为这个只能用于读取的时候对方关闭。如果是服务器往sock上写完了，处理完了，这时候客户端阻塞在poll，这时候服务器关闭，就没法用recv=0检测对方关闭连接了。
	if(fds[1].revents & POLLRDHUP){
	    printf("server close the connection\n");
	    break;
	}
	//如果sockfd有可读事件，说明是服务器发的数据
	else if(fds[1].revents & POLLIN){
	    memset(read_buf,'\0',BUFFER_SIZE);
	    recv(fds[1].fd,read_buf,BUFFER_SIZE-1,0);
	    printf("%s\n",read_buf);
	}
	//如果标准输入有可读事件，说明客户从键盘输入了数据。需要把这个数据传给服务器。可以先读到buf，再输入到sockfd。但这样的会经历一个内核到用户态的复制，影响效率。最好就是用splice。
	if(fds[0].revents & POLLIN){
	    ret = splice(0,NULL,pipefd[1],NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
	    ret = splice(pipefd[0],NULL,sockfd,NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);

	}
    }
    close(sockfd);
    return 0;
}
