#ifndef LST_TIMER
#define LST_TIMER

#include<time.h>
#define BUFFER_SIZE 64
class util_timer;

//用户数据结构
struct client_data{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer* timer;
};

//定时器类。
class util_timer{
    public:
	util_timer():prev(NULL),next(NULL){}
    public:
	time_t expire;//任务的超时时间。
	void (*cb_func)(client_data*);//任务的回调函数。这是一个函数指针，名字是cb_func，指向一个参数列表是client_data*返回类型是void的函数。
	client_data* user_data;
	util_timer* prev;
	util_timer* next;
};

//定时器链表类
class sort_timer_lst{
    public:
	sort_timer_lst():head(NULL),tail(NULL){}
	//析构函数。因为这个类的成员只有两个指针，而默认析构函数无法删除指针指向的内存(有待商榷，今晚回去看看cpp primer)，更无法删除指针的后继指向的内存，所以需要析构函数自行删除。
	~sort_timer_lst(){
	    util_timer* tmp = head;
	    while(tmp){
		head = tmp->next;
		delete tmp;
		tmp = head;
	    }
	}
	//添加定时器
	void add_timer(util_timer* timer){
	    if(!time){
		return;
	    }
	    //如果head是空指针,那么链表长度为空,说明里面没有元素,就把timer设置为head。
	    if(!head){
		head = tail = timer;
	    }
	    //如果当前定时器位置。因为是按照超时时间从小到大排序，所以先和head比较一下。
	    if(timer->expire<head->expire){
		timer->next = head;
		head->prev = timer;
		head = timer;
		return;
	    }
	    //如果比head大，那么就在后面插入(感觉这里可以封装在一起阿，不明白为什么分两个函数).
	    add_timer(timer,head);
	}
	//调整定时器。当某个定时器的超时时间变化时，调整其在链表中的位置。这里只考虑时间延长，即往后调整。
	void adjust_timer(util_timer* timer){
	    if(!timer){
		return;
	    }
	    util_timer* tmp = timer->next;
	    //如果时间变化后不影响先后顺序，或者当前定时器就在末尾，那么就不管。
	    if(!tmp || timer->expire<tmp->expire){
		return;
	    }
	    //如果在头部，就把这个删了，然后在后面add。
	    if(timer == head){
		head = head->next;
		head->prev = NULL;
		timer->next = NULL;
		add_timer(timer,head);
	    }
	    //如果在中间，那么就把这个删了再在后面add，删的过程如下
	    else{
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		add_timer(timer,timer->next);
	    }
	}
	    //将目标定时器从链表中删除。
	void del_timer(util_timer* timer){
	    if(!timer){
	        return;
     	    }
	    //如果链表只有一个定时器
	    if((timer==head)&&(timer==tail)){
	        delete timer;
    	        head = NULL;
 	        tail = NULL;
 	        return;
	    }
	    //如果timer是头
	    if(timer == head){
	        head = head->next;
	        head->prev = NULL;
 	        delete timer;
	        return;
	    }
	    //如果timer是尾
	    if(timer == tail){
		 tail = tail->prev;
		 tail->next = NULL;
	         delete timer;
		 return;
	    }
	}
		//SIGALRM信号被触发后执行的函数。遍历链表，处理上面的到期任务。
	void tick(){
	    printf("timer tick\n");
	    if(!head){
		return;
	    }
//	    printf("timer tick\n");
	    time_t cur = time(NULL);
	    util_timer* tmp = head;
	    while(tmp){
		//如果遇到没有到期的任务，说明它后面的所有任务都没到期，就直接break吧。
		if(cur<tmp->expire){
		    break;
		}
		//否则，当前任务到期了，就执行这个任务，然后把这玩意删除掉。
		tmp->cb_func(tmp->user_data);
		head = tmp->next;
		if(head){
		    head->prev = NULL;
		}
		delete tmp;
		tmp = head;
	    }
	}	
    private:
	//将目标定时期添加到节点lst_head后面的链表中
	void add_timer(util_timer* timer,util_timer* lst_head){
	    util_timer* prev = lst_head;
	    util_timer* tmp = prev->next;
	    while(tmp){
		if(timer->expire<tmp->expire){
		    prev->next = timer;
		    timer->next = tmp;
		    tmp->prev = timer;
		    timer->prev = prev;
		    break;
		}
		prev = tmp;
		tmp = tmp->next;
	    }
	    //如果while循环遍历完了还没有找到合适的，说明已经遍历到了尾部。
	    if(!tmp){
		prev->next = timer;
		timer->prev = prev;
		timer->next = NULL;
		tail = timer;
	    }
	}
    private:
	util_timer* head;
	util_timer* tail;
};
#endif
