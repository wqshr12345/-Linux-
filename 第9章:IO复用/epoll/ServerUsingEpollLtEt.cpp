#include<sys/socket.h>
#include<sys/epoll.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<libgen.h>
#include<string.h>
#include<assert.h>
#include<fcntl.h>
#include<sys/types.h>
#include<unistd.h>
#include<pthread.h>
#include<stdbool.h>
#include<errno.h>

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

//将文件描述符设置为非阻塞。
//fcntl函数，如其名file control一样，提供了各种对于文件描述符的控制操作。
int setnonblocking(int fd){
    //获取fd的当前状态。
    int old_option = fcntl(fd,F_GETFL);
    //设置新状态为老状态 并上一个 非阻塞。
    int new_option = old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}


//把文件描述符上的EOPLLIN(文件可读)事件注册进epoll内核事件表。参数enable_et指定是否对fd启用ET模式。
void addfd(int epollfd,int fd,bool enable_et){
    epoll_event event;
    event.data.fd = fd;//这个fd的用处何在？后面的epoll_ctl里明明有fd啊..
    event.events = EPOLLIN;//添加数据可读事件
    if(enable_et){
	event.events |= EPOLLET;//添加ET事件，EPOLL将使用ET模式处理该fd。
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

//LT模式下的工作流程
//注：这里的events是epoll_wait得到的已经就绪的事件，肯定就绪。number是events数组大小
void lt(epoll_event* events,int number,int epollfd,int listenfd){
    char buf[BUFFER_SIZE];
    for(int i = 0;i<number;i++){
	int sockfd = events[i].data.fd;//得到当前就绪事件的fd。
	//如果是listenfd，说明当前fd是监听socked，就accept啊什么的。
	if(sockfd == listenfd){
	    struct sockaddr_in client_address;
	    socklen_t client_addrlength = sizeof(client_address);
	    int connfd = accept(listenfd,(struct sockaddr*)&client_address,&client_addrlength);
	    //把监听到的connfd添加到epoll的内核时间表里。
	    addfd(epollfd,connfd,false);//不启用ET模式。
	}
	//其他情况下，说明当前fd是连接fd。就要判断当前fd有没有触发EPOLLIN事件。(因为events只是代表里面的fd都有事件，但不一定是触发了EPOLLIN事件，我们只对触发了EPOLLIN事件的进行recv，其他我们不care)
	else{
	    if(events[i].events & EPOLLIN){
		printf("event trigger once\n");
		memset(buf,'\0',BUFFER_SIZE);
		int ret = recv(sockfd,buf,BUFFER_SIZE-1,0);
		if(ret<=0){
		    close(sockfd);
		    continue;
		}
		printf("get %d bytes of content: %s\n",ret,buf);
	    }
	    else{
		printf("something else happened \n");
	    }

	}
    }
}

//ET模式下的工作流程
void et(epoll_event* events,int number,int epollfd,int listenfd){
    char buf[BUFFER_SIZE];
    for(int i = 0;i<number;i++){
	int sockfd = events[i].data.fd;
	if(sockfd == listenfd){
	    struct sockaddr_in client_address;
	    socklen_t client_addrlength = sizeof(client_address);
	    int connfd = accept(sockfd,(struct sockaddr*)&client_address,&client_addrlength);
	    addfd(epollfd,connfd,false);
	}
	else{
	    //如果当前fd是connfd，并且是可读事件，那么就进行recv等操作。
	    if(events[i].events & EPOLLIN){
		//因为ET模式下，一个事件只触发一次，所以要while保证完全读完。
		printf("event trigger once\n");
		while(1){
		    memset(buf,'\0',BUFFER_SIZE);
		    int ret = recv(sockfd,buf,BUFFER_SIZE-1,0);
		    if(ret<0){
			//对于非阻塞IO，下面条件成立表示数据已经全部读取完。之后，epoll就能再次触发当前sockfd上的EPOLLIN事件。
			if((errno = EAGAIN)||(errno = EWOULDBLOCK)){
			    printf("read later\n");
			    break;
			}
		    close(sockfd);
		    break;
		    }
		    //ret==0表明通信对方关闭了连接。
		    else if(ret == 0){
			close(sockfd);
		    }
		    else {
			printf("get %d bytes of content: %s\n",ret,buf);
	     	    }
		}
	    }
	    else{
                printf("something else happened \n");
	     }

	}

    }
}
int main(int argc,char* argv[]){
    if(argc <= 2){
	printf("usage:%s ip_address port_number\n",basename(argv[0]));
    }
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);
    
    int listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd>=0);

    int ret = bind(listenfd,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);

    ret = listen(listenfd,5);
    assert(ret!=-1);
    
    //创建epoll事件表
    int epollfd  = epoll_create(5);
    assert(epollfd!=-1); 
    //向epoll事件表里面注册fd的事件，addfd封装了epoll_ctl方法。
    addfd(epollfd,listenfd,true);  
    //创建epoll_event数组，用于存放就绪事件
    epoll_event events[MAX_EVENT_NUMBER];

    while(1){
	int ret = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
	if(ret<0){
	    printf("epoll failure\n");
	    break;
	}
	//et(events,ret,epollfd,listenfd);
	lt(events,ret,epollfd,listenfd);
    }
    close(listenfd);
    return 0;
}
