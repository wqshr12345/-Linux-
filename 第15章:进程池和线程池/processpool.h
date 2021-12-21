#ifndef PROCESSPOLL_H
#define PROCESSPOLL_H

//描述一个子进程的类
class process{
    public:
	process():m_pid(-1){}
    public:
	pid_t m_pid;//子进程的PID
	int m_pipefd[2];//父子进程通信的管道
}

//进程池类(使用了单体模式，保证程序最多创建一个processpoll实例，创建方法无非是私有化构造函数，静态实例，共有创建方法，和之前java学过的一样)
template<typename T>
class processpool{
    private:
	processpool(int listenfd,int process_number = 8);
    public:
	//单体模式，创造进程池实例的函数。
	static processpool<T>* create(int listenfd,int process_number=8){
	    if(!m_instance){
 		m_instance = new processpoll<T>(listenfd,process_number);
	    }
	    return m_instance;
	}
	//析构函数
	~processpool(){
	    delete [] m_sub_process;
	}
	//启动进程池函数
	void run();
    private:
	void setup_sig_pipe();
	void run_parent();
	void run_child();
    private:
	//进程池允许的最大子进程数量
	static const int MAX_PROCESS_NUMBER = 16;
	//每个子进程处理的最大客户数量
	static const int USER_PER_PROCESS = 65536;
	//epoll最多能处理的事件数
	static const int MAX_EVENT_NUMBER = 10000;
	//进程池中的进程总数
	int m_process_number;
	//子进程在池子中的编号
	int m_idx;
	//子进程的内核时间表id
	int m_epollfd;
	//监听socket
	int m_listenfd;
	//决定子进程是否停止运行
	int m_stop;
	//保留所有子进程的描述信息
	process* m_sub_process;
	//进程池静态实例
	static processpool<T>* m_instance;
};

template<typename T >
processpool<T>* processpool< T >::m_instance = NULL;//初始化单例？

//用于处理信号的管道，实现统一信号源，可以称之为信号管道。
static int sig_pipefd[2];

//将某个socket设置为非阻塞，用于使用ET模式。
static int setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

//添加某个fd到epoll的内核事件表。
static void addfd(int epollfd,int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);//设置ET模式要和非阻塞一起使用。
}

//删除某个fd从epoll的内核事件表。
static void removefd(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

//某个信号的实际处理函数(可以看到，并没有在函数内处理该信号，而是通过管道把这个信号传给主进程，然后再利用epoll的监听实现统一事件处理。)
static void sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

//将信号与具体的信号处理函数绑定，本质上封装了sigaction函数。
stativ void addsig(int sig,void(handler)(int),bool restart = true){
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handler;
    if(restart){
	sa.sa_flags |= SA_RESTART;//这个参数的意思是重新调用被该信号终止的系统调用。
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}

//进程池类processpool的构造函数，listenfd是监听socket，process_number是进程池中的子进程数量。
template<typename T>
processpool<T>::processpool(int listenfd,int process_number):m_listenfd(listenfd),m_process_number(process_number),m_idx(-1),m_stop(false){
    assert((process_number>0)&&(process_number<=MAX_PROCESS_NUMBER));//进程数量应该在规定的范围内。
    m_sub_process = new process[process_number];//使用new创建一个process的数组
    //使用fork创建process_number个子进程，放到上面创建的数组里。
    for(int i = 0;i<process_number;++i){
	//创造双向管道，用于父子进程通信。但是fork之后两个进程要各自close一个。
	int ret = socketpair(PF_UNIX,SOCK_STREAM,0,m_sub_process[i].m_pipefd);
	assert(ret == 0);
	m_sub_process[i].m_pid = fork();
	assert(m_sub_process[i].m_pid>=0);
	//如果fork返回值>0,说明返回了子进程pid，是父进程；如果返回0，说明是子进程
	if(m_sub_process[i].m_pid>0){
	    close(m_sub_process[i].m_pipefd[1]);
	    continue;
	}
	else{
	    close(m_sub_process[i].m_pipefd[0]);
	    m_idx = i;//这行什么意思？
	    break;
	}
    }
}

//统一事件源的函数
template<typename T>
void processpool<T>::setup_sig_pipe(){
    m_epollfd = epoll_create(5);
    assert(m_epollfd!=-1);
    //信号管道的1用于发，0用于收。所以epoll监听0.
    setnonblocking(sig_pipefd[1]);
    addfd(m_epollfd,sig_pipefd[0]);

    //设置具体的信号处理函数
    addsig(SIGCHLD,sig_handler);
    addsig(SIGTERM,sig_handler);
    addsig(SIGINT,sig_handler);
    addsig(SIGPIPE,SIG_IGN);//linux自带的处理函数SIG_IGN，表示忽略该信号。
}

//根据m_idx，判断要运行父还是子。
template<typename T>
void processpool<T>::run(){
    if(m_idx!=-1){
	run_child();
	return;
    }
    run_parent();
}

template<typename T>
void processpool<T>::run_child(){
    //统一事件源
    setup_sig_pipe();
    //通过m_idx找到在进程池数组中自己对应的那个对象(有个问题，构造函数中fork了，所以父子进程都有m_sub_process[]数组吗？这岂不是浪费资源？答案是不是的，进程fork是写时复制，只有m_sub_process被改变了，才会复制，否则可以视为父子共享)
    int pipefd = m_sub_process[m_idx].m_pipefd[1]; 
    //子进程监听管道，父进程通过管道通知子进程accept新的连接
    addfd(m_epollfd,pipefd);
    epoll_event events[MAX_EVENT_NUMBER];
    T* users = new T[USER_PER_PROCESS];
    assert(users);
    int number = 0;
    int ret = -1;

    while(!m_stop){
	number = epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
	if((number<0)&&(errno!=EINTR)){
	    printf("epoll failure\n");
	    break;
	}
	for(int i = 0;i<number;i++){
	    int sockfd = events[i].data.fd;
	    //如果是pipefd，说明是父进程发来的，有新连接accpet，需要子进程处理
	    if((sockfd == pipefd)&& (events[i].events & EPOLLIN)){
		int client = 0;
		ret = recv(sockfd,(char*)&client,sizeof(client),0);
	    	if(((ret<0)&&(errno!=EAGAIN))||ret == 0){
		    continue;
		}
		else{
		    //纯粹的reactor模式(子进程自己读写连接socket),甚至更纯粹，因为这里子进程还自己监听连接socket。
		    struct sockaddr_in client_address;
		    socklen_t client_addrlength = sizeof(client_address);
		    int connfd = accept(m_listenfd,(struct sockaddr*)&client_address,&client_addrlength);
		    if(connfd<0){
			printf("errno is: %d\n",errno);
			continue;
		    }
		    addfd(m_epollfd,connfd);
		    //模板类T需要实现一个init方法,初始化一个客户连接。
		    users[connfd].init(m_epollfd,connfd,client_address);
		}
	    }
	    //处理子进程收到的信号
	    else if((sockfd == sig_pipefd[0])&&(events[i].events & EPOLLIN)){
		int sig;
		char signals[1024];
		ret = recv(sig_pipefd[0],signals,sizeof(signals),0);
		if(ret<=0){
		    continue;
		}
		else{
		    for(int i = 0;i<ret;++i){
			switch(signals[i]){
			    //子进程退出的时候向父进程发送SIGCHLD信号，然后父进程调用waitpid，对子进程善后处理，避免出现僵尸进程。
			    case SIGCHLD:{
				pid_t pid;
				int stat;
				while((pid = waitpid(-1,&stat,WNOHANG))>0){
				    //善后处理
				}
				break;
			    }
			    case SIGTERM:
			    //退出信号
			    case SIGINT:{
				m_stop = true;
				break;
			    }
			    default:{
				break;
			    }
			} 
		    }
		}
	    }
	    //如果不是信号也不是管道，说明一定是客户请求到来，就处理它。
	    else if(events[i].events & EPOLLIN){
		users[sockfd].process();
	    }
	    else{
		continue;
	    }
	}
    }
    delete [] users;
    users = NULL;//delete仅仅是删除指针指向的内容，但指针本身没有改变，还是指向那一块地方。如果不设为NULL，就会成为野指针，指向一块可能不存在的内存。
    close(pipefd);
    close(m_epollfd);
}

template<typename T
void processpool<T>::run_parent(){
    setup_sig_pipe();
    
    //父进程监听m_listenfd;
    addfd(m_epollfd,m_listenfd);

    epoll_event events[MAX_EVENT_NUMBER];
    int sub_process_counter = 0;
    int new_conn = 1;
    int number = 0
    int ret = -1;

    while(!m_stop){
	number = epoll_ctl(m_epollfd,events,MAX_EVENT_NUMBER,-1);
	if((number<0)&&(errno!=EINTR)){
	    printf("epoll failure\n");
	    break;
	}
	else{
	    for(int i = 0;i < number;i++){
		int sockfd = events[i].dafa.fd;
		//如果是lisntenfd，说明有新的连接到达，需要通过管道送给子进程。怎么送？轮盘调度分配。
		if(sockfd==m_listenfd){
		    int i = sub_process_counter;
		    do{
			if(m_sub_process[i].m_pid != -1){
			    break;
			}
			i = (i+1)%m_process_number;
		    }while(i!=sub_process_counter);
		    if(m_sub_process[i].m_pid == -1){
			m_stop = true;
			break;
		    }
		    sub_process_counter = (i+1)%m_process_number;
		    send(m_sub_process[i].m_pipefd[0],(char*)&new_conn,sizeof(new_conn),0);
		    printf("send request to child %d\n",i);
		}
		//如果是sig_pipefd，说明是信号管道
		else if((sockfd == sig_pipefd[0])&&(events[i].events&EPOLLIN)){
		    int sig;
		    char signals[1024];
		    ret = recv(sig_pipefd[0],signals,sizeof(signals),0);
		    if(ret<0){
			continue;
		    }
		    else{
			for(int i = 0;i<ret;i++){
			    switch(signals[i]){
				case SIGCHLD:{
				    pid_ pid;
				    int stat;
				    while((pid = waitpid(-1,&stat,WNOHANG))>0){
					for(int i = 0;i<m_process_number;i++){
					    //如果某个子进程退出，就要关闭对应的通信管道，并且设置pid为-1，表明这个进程已经g了。
					    if(m_sub_process[i].m_pid == pid){
						printf("child %d join\n",i);
						close(m_sub_process[i].m_pipefd[0]);
						m_sub_process[i].m_pid = -1;
					    }
					}
				    }
				    //如果所有的子进程都退出了，那么父进程也退出。
				    m_stop = true;
				    for(int i = 0;i<m_process_number;i++){
					if(m_sub_process[i].m_pid != -1){
					    m_stop = false;
					}
				    }
				    break;
				}
				case SIGTERM:
				case SIGINT:{
				    //如果父进程收到中断信号，就杀死所有子进程。可以直接用kill往子进程发送一个特殊的信号，但是这不符合统一事件源。更好的做法是父进程向父子进程通信管道发送一个特殊的数据。
				    printf("kill all the child now\n");
				    for(int i = 0;i<m_process_number;++i){
					int pid = m_sub_process[i].m_pid;
					if(pid! = -1){
					    kill(pid,SIGTERM);
					}
				    }
				    break;
				}
				default:{
				    break;
				}
			    }
			}
		    }
		}
		//父进程只处理listenfd和信号，其他的不管。
		else{
		    continue;
		}
	    }
	}
    }
    close(m_epollfd);
}
