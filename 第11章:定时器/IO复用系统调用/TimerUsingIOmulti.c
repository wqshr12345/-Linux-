#define TIMEOUT 5000

int timeout = TIMEOUT;
time_t start = time(NULL);
time_t end = time(NULL);

while(1){
    printf("thie timeout is now %d mil-seconds\n",timeout);
    start = time(NULL);
    int number = epoll_wait(epollfd,events,MAX_EVNT_NUMBER,timeout);
    if((number<0)&&(errno!=EINTR)){
	printf("epoll failure\n");
	break;
    }
    //如果没有事件发生，而epoll返回，说明到时间了，触发定时事件。
    else if(number == 0){
	timeout = TIMEOUT;
	//handle
	continue;
    }
    //如果number>0，就去处理事件。
    else{
    }
    end = time(NULL);
    timeout -= (end-start)*1000;
    if(timeout<=0){
	timeout = TIMEOUT;
	//handle
    }

}
