#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<assert.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<arpa/inet.h>
#include<signal.h>
#include<sys/epoll.h>

#define MAX_EVENT_NUMBER 1024
static int pipefd[2];//管道描述符

//设置文件描述符为非阻塞。
int setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

//向epoll内核事件表添加新的fd。
void addfd(int epollfd,int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}


//用户自定义的信号处理函数，接收参数只有一个sig，为信号类型。
void sig_handler(int sig){
    //保留原来的errno，以保证函数的可重入性...emm
    int save_errno = errno;
    int msg = sig;
    //把信号值写入管道，让主循环处理。
    send(pipefd[1],(char*)&msg,1,0);//为什么发送msg的指针呢...我不理解。
    errno = save_errno;
}

//设置信号的处理函数(本质上覆盖了sigaction方法，将具体信号值和信号处理函数进行绑定？SOS那为什么不在sig_handler里针对sig分类讨论呢.)
void addsig(int sig){
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    //在信号集中设置所有信号(这么说信号掩码包括所有信号？)
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}

int main(int argc,char* argv[]){
    if(argc<=2){
	printf("usage:%s ip_address port_number \n",basename(argv[0]));
	return -1;
    } 
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(AF_INET,SOCK_STREAM,0);
    assert(listenfd>=0);

    int ret = bind(listenfd,(struct sockaddr*)&address,sizeof(address));
    if(ret==-1){
	printf("errno is %d\n",errno);
	return 1;
    }

    ret = listen(listenfd,5);
    assert(ret!=-1);

    epoll_event events[MAX_EVENT_NUMBER];
    
    //创建内核事件表并注册listenfd描述符。
    int epollfd =epoll_create(5);
    assert(epollfd!=-1);
    addfd(epollfd,listenfd);
    
    //使用socketpair注册双向管道。并注册pipefd[0]到内核事件表
    ret = socketpair(PF_UNIX,SOCK_STREAM,0,pipefd);
    assert(ret!=-1);
    setnonblocking(pipefd[1]);
    addfd(epollfd,pipefd[0]);

    //设置一些信号的处理函数
    addsig(SIGHUP);
    addsig(SIGCHLD);
    addsig(SIGTERM);
    addsig(SIGINT);
    addsig(SIGQUIT);
    bool stop_server = false;

    while(!stop_server){
	ret = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
	if((ret<0)&&(errno!=EINTR)){
	    printf("epoll failure\n");
	    break;
	}
	for(int i = 0;i<ret;i++){
	    int sockfd = events[i].data.fd;
	    if(sockfd == listenfd){
		struct sockaddr_in client_address;
		socklen_t client_addrlength = sizeof(client_address);
		int connfd = accept(listenfd,(struct sockaddr*)&client_address,&client_addrlength);
		addfd(epollfd,connfd);
	    }
	    //如果就绪的是管道出口，且是可读事件就绪，那么就接受信号。
	    else if((sockfd == pipefd[0])&&(events[i].events & EPOLLIN)){
		int sig;
		char signals[1024];
		ret = recv(pipefd[0],signals,sizeof(signals),0);
		//出现错误errno。一般来说错误如果是EAGAIN，就是管道内没有数据可读。
		if(ret == -1){
		    continue;
		}
		//对方关闭了连接
		else if(ret == 0){
		    continue;
		}
		//正常接收到信号数据
		else{
		    for(int i = 0;i < ret;i++){
			switch(signals[i]){
			    case SIGCHLD:
			    case SIGHUP:
			    {
			         continue;
			    }
			    case SIGTERM:
			    case SIGINT:
			    {
				printf("啦啦啦就不退出气死你！\n");
				continue;
				//stop_server = true;
			    }
			    case SIGQUIT:
			    {
				printf("^_^，这个才是真正的退出！\n");
				stop_server = true;
			    }

			}
		    }
		    
		}
	    }
	    //其他情况说明是connfd。这里就不写了，这个程序的重点在于对signal的处理。
	    else{
	    }
	}

    }
    printf("close fds\n");
    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}
