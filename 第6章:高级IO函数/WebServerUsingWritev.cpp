#include<sys/socket.h>
#include<string.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<stdbool.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<string.h>
#include<sys/types.h>

#define BUFFER_SIZE 1024

//定义两种HTTP状态码和状态信息
static const char* status_line[2] = {"200 OK","5OO Internal server error"};
int main(int argc,char *argv[]){
    //参数赋值
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* file_name = argv[3];   

    //创建socket地址
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    //创建socket
    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock>=0);
    
    //命名socket
    int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);

    //监听socket
    ret = listen(sock,5);
    assert(ret!=-1);

    //accept连接
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock,(struct sockaddr*)&client,&client_addrlength);
    if(connfd<0){
	printf("errno is: %d\n",errno);

    }
    else{
	//用于保存HTTP应答的状态行、头部字段和空行。
	char header_buf[BUFFER_SIZE];
        memset(header_buf,'\0',BUFFER_SIZE);
	//用于存放目标文件内容的缓存
	char* file_buf;
	//用于存放目标文件的属性的结构体
        struct stat file_stat;
	//记录目标文件是否有效
        bool valid = true;
        int len = 0;
        //判断目标文件是否存在。
	if(stat(file_name,&file_stat)<0){
	    valid = false;
	}
	else{
	    //判断目标文件是否是一个目录
	    if(S_ISDIR(file_stat.st_mode)){
	    valid = false;
            }
	    //判断当前用户是否有读取该文件的权限。
            else if(file_stat.st_mode & S_IROTH){
	        //进行对目标文件读取的工作。
		//1.首先是open目标文件。
		int fd = open(file_name,O_RDONLY);
		//2.建立缓存区并初始化，用于存放读取到的文件。
		file_buf = new char [file_stat.st_size+1];//a question,为什么这里要用new生成一块内存？直接char file_buf[file_stat.st_size+1]不好吗。
		memset(file_buf,'\0',file_stat.st_size+1);
		//3.将目标文件读取到缓存区。
		if(read(fd,file_buf,file_stat.st_size)<0){
		    valid = false;
		}
	}
	    else {
	        valid = false;
	    }
	
	}


	//如果目标文件有效，那么就发送正常的http应答。
	if(valid){
	    //将HTTP应答的状态行、头部字段和空行分别加入header_buf中。
	    ret = snprintf(header_buf,BUFFER_SIZE-1,"%s %s\r\n","HTTP/1.1",status_line[0]);
	    len += ret;
	    ret = snprintf(header_buf+len,BUFFER_SIZE-1-len,"Content-Length: %ld\r\n",file_stat.st_size);
	    len += ret;
	    ret = snprintf(header_buf+len,BUFFER_SIZE-1-len,"%s","\r\n");
	    //使用writev把两块内存里的东西集中写到connfd，而不用先拼接到一起，再写。
	    struct iovec iv[2];
	    iv[0].iov_base = header_buf;
	    iv[0].iov_len = strlen(header_buf);
	    iv[1].iov_base = file_buf;
	    iv[1].iov_len = strlen(file_buf);
	    ret = writev(connfd,iv,2);
	}
	else{
	    ret =  snprintf(header_buf,BUFFER_SIZE-1,"%s %s\r\n","HTTP/1.1",status_line[1]);
	    len += ret;
	    ret = snprintf(header_buf+len,BUFFER_SIZE-1-len,"%s","\r\n");
	    send(connfd,header_buf,strlen(header_buf),0);//如果单独发送，用send就行。

	}
	close(connfd);
	delete [] file_buf;//前面new的，后面总要delete掉。
}
    close(sock);
    return 0;
}
