#include<sys/socket.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<pthread.h>
#include "lst_timer.h"
#include<libgen.h>

#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define TIMESLOT 5

static int pipefd[2];
static sort_timer_lst timer_lst;
static int epollfd = 0;

int setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

void addfd(int epollfd,int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

//信号触发函数
void sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

//为具体的一个信号绑定一个触发函数
void addsig(int sig){
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}

//定时处理任务，实际上就是调用tick函数。
void timer_handler(){
    timer_lst.tick();
    alarm(TIMESLOT);
}

//定时器回调函数,删除非活动连接socket上的注册事件，并关闭。
void cb_func(client_data* user_data){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);
    assert(user_data);
    close(user_data->sockfd);
    printf("close fd %d\n",user_data->sockfd);
}

int main(int argc,char* argv[]){
    if(argc<=2){
	printf("usage: %s ip_address port_number \n",basename(argv[0]));
	return -1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd>=0);

    int ret = bind(listenfd,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);

    ret = listen(listenfd,5);
    assert(ret!=-1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd!=-1);

    addfd(epollfd,listenfd);

    //设置管道，准备统一事件源。
    ret = socketpair(PF_UNIX,SOCK_STREAM,0,pipefd);
    assert(ret!=-1);
    setnonblocking(pipefd[1]);
    addfd(epollfd,pipefd[0]);

    //设置信号处理函数
    addsig(SIGALRM);
    addsig(SIGTERM);
    bool stop_server = 0;
    
    client_data* users = new client_data[FD_LIMIT];
    bool timeout = false;
    alarm(TIMESLOT);//在timeslot的时间之后，触发SIGALRM函数。
    
    while(!stop_server){
	//本来看到传的参数是event还以为写错了，转念一想，数组的名字就代表指向数组头部元素的指针，所以没毛病。
	int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
	if((number<0)&&(errno!=EINTR)){
	    printf("epoll failure\n");
	    break;
	}
	for(int i = 0;i<number;i++){
	    int sockfd = events[i].data.fd;
	    if(sockfd == listenfd){
		struct sockaddr_in client_address;
		socklen_t client_addrlength = sizeof(client_address);
		int connfd = accept(listenfd,(struct sockaddr*)&client_address,&client_addrlength);
		addfd(epollfd,connfd);
		users[connfd].address = client_address;
		users[connfd].sockfd = connfd;

		//设置定时器
		util_timer* timer = new util_timer;
		timer->user_data = &users[connfd];
		timer->cb_func = cb_func;
		time_t cur = time(NULL);
		timer->expire = cur+3*TIMESLOT;
		users[connfd].timer = timer;//这岂不是互相引用了？timer里有users[connfd],users[connfd]里有timer
		timer_lst.add_timer(timer);
	    }
	    //如果监听到pipefd1有可读事件，说明有信号发生。
	    else if((sockfd == pipefd[0])&&(events[i].events & EPOLLIN)){
		int sig;
		char signals[1024];
		ret = recv(pipefd[0],signals,sizeof(signals),0);
		if(ret == -1){
		//如果errno是eagain或者ewouldblock，说明没有数据可读
		    continue;
		}
		else if(ret == 0){
		    //对方关闭了连接
		    continue;
		}
		else{
		    for(int i = 0;i<ret;i++){
			switch(signals[i]){
			   //触发了SIGALRM信号，说明定时器到点了，按理说应该触发定时器函数(然后进行链表遍历，找到超时连接并关闭)。但是由于定时任务优先级不高，所以这里设置一个flag，最后再处理。
			    case SIGALRM:{
				timeout = true;
				break;
			    }
			    case SIGTERM:{
				stop_server = true;
			    }
			}
		    }
		}
	    }
	    //最后，说明是连接socket的事件，就进行读取等操作。
	    else if(events[i].events & EPOLLIN){
		memset(users[sockfd].buf,'\0',BUFFER_SIZE);
		ret = recv(sockfd,users[sockfd].buf,BUFFER_SIZE-1,0);//关于这里为什么是-1呢，因为要留一个空给\0作为字符串结束的标志。
		util_timer* timer = users[sockfd].timer;
		if(ret<0){
		    if(errno!=EAGAIN){
			cb_func(&users[sockfd]);
		   	if(timer){
			    timer_lst.del_timer(timer);
			}
		     }
		}
		else if(ret == 0){
		    cb_func(&users[sockfd]);
		    if(timer){
			timer_lst.del_timer(timer);
		    }
		}
		else{
		    //否则，说明这个socket有数据可读，那么我认为这个socket相对比较活跃，再让它多活一会，看看能不能再多收点数据。
		    if(timer){
			time_t cur = time(NULL);
			timer->expire = cur+3*TIMESLOT;
			printf("adjust timer once\n");
			timer_lst.adjust_timer(timer);
		    }
		}
	    }
	    //既不是listenfd，又不是connfd，又不是信号，那就不管了。
	    else{
		printf("something else happened\n");
	    }
	}
	if(timeout){
	    timer_handler();
	    timeout = false;
	}
    }
    close(listenfd);
    close(pipefd[1]);
    close(pipefd[2]);
    delete [] users;
    return 0;
}
