#define _GNU_SOURCE 1
#include<sys/types.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<poll.h>

#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 65535

struct client_data{
    sockaddr_in address;
    char* write_buf;
    char buf[BUFFER_SIZE];
};

int setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

int main(int argc,char* argv[]){
    if(argc<=2){
	printf("usage:%s ip_address port_number\n",basename(argv[0]));
	return -1;
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

    //创建user数组，用以存储client对象。数组下标就是对应socket的值。由于文件描述符的最大值是65535，所以数组大小最大是65535. 
    client_data* users = new client_data[FD_LIMIT];
    //为了提高poll性能，poll监听的文件描述符应该做限制。
    pollfd fds[USER_LIMIT+1];
    for(int i = 1;i<USER_LIMIT;++i){
	fds[i].fd = -1;
	fds[i].events = 0;
    }
    fds[0].fd = listenfd;
    fds[0].events = POLLIN|POLLERR;
    fds[0].revents = 0;

    int user_counter = 0;//表示实际的poll监听的文件描述符数量。
    while(1){
	ret = poll(fds,user_counter+1,-1);
	if(ret < 0){
	    printf("poll failure\n");
	    break;
	}
	//for循环判断哪个fd的EPOLLIN被触发。
	for(int i = 0;i<user_counter+1;++i){
	    //如果fd是监听socket，且有新连接到达。因为是poll，所以需要判断revents。不像epoll直接返回一个好的数组。
	    if((fds[i].fd == listenfd) &&(fds[i].revents & POLLIN)){
		struct sockaddr_in client_address;
		socklen_t client_addrlength = sizeof(client_address);
		int connfd = accept(listenfd,(struct sockaddr*)&client_address,&client_addrlength);
	    	if(connfd<0){
		    printf("errno is:%d\n",errno);
		    continue;
		}
		//如果请求太多，就关闭新的客户端请求。
		if(user_counter>=USER_LIMIT){
		    const char* info = "too many users\n";
		    printf("%s",info);
		    send(connfd,info,strlen(info),0);
		    close(connfd);
		    continue;
		}
		//正常的客户端连接处理。首先把这个客户的数据存入数组，然后把这个客户的sock注册到poll监听事件中。
		user_counter++;
		users[connfd].address = client_address;
		setnonblocking(connfd);
		fds[user_counter].fd = connfd;
		fds[user_counter].events = POLLIN|POLLRDHUP|POLLERR;
		fds[user_counter].revents = 0;
		printf("comes a new user,now have %d users\n",user_counter);
	    }
	    //如果轮询到的fd不是监听socket，那么就是一个连接socket。
	    //第一种情况，连接socket的POLLERR事件，说明出现了错误。这时候不会首先去close这个socket，而是通过socket的“fcntl”——getsockopt,获取并清除这个错误。 
	    else if(fds[i].revents & POLLERR){
		printf("get an error from %d\n",fds[i].fd);
		char errors[100];
		memset(errors,'\0',100);
		socklen_t length = sizeof(errors);
		if(getsockopt(fds[i].fd,SOL_SOCKET,SO_ERROR,&errors,&length)<0){
		    printf("get socket option failed\n");
		}
		continue;
	    }
	    //第二种情况，连接socket的POLLRDHUP事件，客户端终止了连接。
	    else if(fds[i].revents & POLLRDHUP){
		//很显然，这时候应该删除这个socket对应的users[]信息，删除对应的fds[]监听信息。怎么删？用替换，把最后一个替换到这里，然后user_counter--.
		users[fds[i].fd] = users[fds[user_counter].fd];//这一行什么意思????
		close(fds[i].fd);
		fds[i] = fds[user_counter];
		i--;//这个是为了保证能处理到这个user_counter对应的socket的事件。
		user_counter--;
		printf("a client left\n");
	    }
	    //第三种情况，连接socket的POLLIN事件，客户端有数据送到。
	    else if(fds[i].revents & POLLIN){
		int connfd = fds[i].fd;
		memset(users[connfd].buf,'\0',BUFFER_SIZE);
		ret = recv(connfd,users[connfd].buf,BUFFER_SIZE-1,0);
		printf("get %d btyes of client data %s from %d\n",ret,users[connfd].buf,connfd);
		//ret<0.如果error是EAGAIN，说明socket中没有数据可以接受。如果是其他error，说明出问题了。
		if(ret<0){
		    if(errno!=EAGAIN){
			close(connfd);
			users[fds[i].fd] = users[fds[user_counter].fd];//意义何在？
			fds[i] = fds[user_counter];
			i--;
			user_counter--;
		    }
		}
		//ret=0，说明通信对方关闭了socket。按理说服务器应该close这个socket，但是吧，前面有POLLRDHUP处理这种情况，所以这里就不用处理了。
		else if(ret==0){
		
		}
		//ret>0,说明有数据可读。就把这个信息放到其他客户的write_buf里，准备发送。
		else{
		    for(int j =0;j<=user_counter;++j){
			if(fds[j].fd == connfd){
			    continue;
			}
			//下面的操作是保证，当有一个socket接收到客户端数据时，下一次调用poll，应该全部处理向其他socket传数据的请求，不能处理其他socket的接受客户端数据请求，不然的话，下次处理一个socket的接收数据请求，会把write_buf覆盖掉...这是一种简单的上锁机制？？
			fds[j].events |= ~POLLIN;
			fds[j].events |= POLLOUT;
			users[fds[j].fd].write_buf = users[connfd].buf;
		    }
		}
	    }
	    else if(fds[i].revents & POLLOUT){
		int connfd = fds[i].fd;
		if(! users[connfd].write_buf){
		    continue;
		}
		ret = send(connfd,users[connfd].write_buf,strlen(users[connfd].write_buf),0);
		users[connfd].write_buf = NULL;
		fds[i].events |= ~POLLOUT;
		fds[i].events |= POLLIN;

	    }	
	}

    }
    delete [] users;
    close(listenfd);
    return 0;

}
