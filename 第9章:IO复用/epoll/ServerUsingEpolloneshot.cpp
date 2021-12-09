#include<fcntl.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<stdio.h>
#include<arpa/inet.h>
#include<assert.h>
#include<errno.h>
#include<string.h>
#include<pthread.h>
#include<libgen.h>
#include<unistd.h>

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 1024

struct fds{
    int epollfd;
    int sockfd;
}; 

int setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

//向epoll的内核事件表中注册事件,本质是epoll_ctl函数
int addfd(int epollfd,int fd,bool oneshot){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if(oneshot){
	event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

//重置某个fd上的所有事件，这之后EPOLLIN可以再次被触发。
void reset_oneshot(int epollfd,int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

//工作线程
void* worker(void* arg){
    int sockfd = ((fds*)arg)->sockfd;
    int epollfd = ((fds*)arg)->epollfd;
    printf("start new thread to receive data on fd: %d\n",sockfd);
    char buf[BUFFER_SIZE];
    memset(buf,'\0',BUFFER_SIZE);
    //循环读取socked上的数据，直到没有数据可读(即遇到EAGAIN信号)
    while(1){
	int ret = recv(sockfd,buf,BUFFER_SIZE-1,0);
	if(ret==0){
	    close(sockfd);
	    printf("foreiner closed the connection\n");
	    break;
	}
	else if(ret<0){
	    //如果对当前socket读完了，那么就重设其事件，使得其可以再次被监听到。
	    if(errno == EAGAIN){
		reset_oneshot(epollfd,sockfd);
		printf("read later\n");
		break;
	    }
	}
	else{
	    printf("get content: %s\n",buf);
	    sleep(5);
	}
    }
    printf("end thread receiving data on fd: %d\n",sockfd);
}

int main(int argc,char* argv[]){
    if(argc<=2){
	printf("usage: %s ip_address port_Number\n",basename(argv[0]));
	return 1;
    }
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd!=-1);

    int ret = bind(listenfd,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);    

    ret = listen(listenfd,5);
    assert(ret!=-1);

    //注册epoll事件
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd!=-1);
    //这里监听listenfd不能注册EPOLLONESHOT事件，不然accept只能accept一个。
    addfd(epollfd,listenfd,false);

    while(1){
	int ret = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
	if(ret<0){
	    printf("epoll failure\n");
	    break;
	}
	for(int i = 0;i<ret;i++){
	    int sockfd = events[i].data.fd;
	    if(sockfd == listenfd){//啊！==和=的错误，很烦。
		struct sockaddr_in client_address;
		socklen_t client_addrlength = sizeof(client_address);
		int connfd = accept(listenfd,(struct sockaddr*)&client_address,&client_addrlength);
		addfd(epollfd,connfd,true);
	    }
	    else{
		if(events[i].events & EPOLLIN){
		    pthread_t thread;
		    fds fds_for_new_worker;
		    fds_for_new_worker.epollfd = epollfd;
		    fds_for_new_worker.sockfd = sockfd;
		    pthread_create(&thread,NULL,worker,(void*)&fds_for_new_worker);
		}
		else{
		    printf("something else happened\n");
		}
	    }
	}
    }
    close(listenfd);
    return 0;
}

