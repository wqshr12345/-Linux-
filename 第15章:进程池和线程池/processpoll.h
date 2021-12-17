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
}
 

