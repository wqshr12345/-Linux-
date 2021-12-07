#include<libgen.h>
#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<assert.h>
#include<errno.h>
#include<unistd.h>
#include<sys/select.h>
#include<stdlib.h>
#include<string.h>
int main(int argc,char* argv[]){
    if(argc<=2){
	printf("usage: %s ip_address port_number\n",basename(argv[0]));
	return 1;
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

    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(listenfd,(struct sockaddr*)&client_address,&client_addrlength);
    if(connfd<0){
	printf("errno is: %d\n",errno);
        close(listenfd);
    }
    
    char buf[1024];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&read_fds);

    while(1){
	memset(buf,'\0',sizeof(buf));
	//只监听一个connfd的select，意义何在？
	
	//每次调用select前都要重新在read_fds和exception_fds中设置文件描述符connfd，因为事件发生后，文件描述符集合将被内核修改。也就是说在read_fds中的集合，select只监听他们的可读事件，其他不管；在exception_fds中的集合，select只监听他们的异常事件，其他不管。
	FD_SET(connfd,&read_fds);
	FD_SET(connfd,&exception_fds);
	ret = select(connfd+1,&read_fds,NULL,&exception_fds,NULL);
	//ret返回就绪文件描述符的总数。
	if(ret<0){
	    printf("selection failure\n");
	    break;
	}
	//用IO复用的exception_fds找带外数据，是两大方法之一。

        //如果read_fds中的connfd描述符就绪，就返回true。
	if(FD_ISSET(connfd,&read_fds)){
            ret = recv(connfd,buf,sizeof(buf)-1,0);
            if(ret<=0){
                break;
            }
            printf("get %d bytes of normal data: %s\n",ret,buf);
        }
	memset(buf,'\0',sizeof(buf));
	if(FD_ISSET(connfd,&exception_fds)){
	    ret = recv(connfd,buf,sizeof(buf)-1,MSG_OOB);
	    if(ret<=0){
		break;
	    }
	    printf("get %d bytes of oob data:%s \n",ret,buf);
	}
	}
    close(connfd);
    close(listenfd);
    return 0;
}
