//之前写的服务器demo，如果客户端想从服务器得到一个文件，需要先读取这个文件到一个char[]数组，然后把这个char[]数组的内容send到传输socket，这样经历了一个内核缓冲区(文件)到用户缓冲区(char[])的拷贝，影响效率。如果使用sendfile函数，就可以直接在文件描述符中间传输数据，避免了拷贝，增加效率。
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<assert.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<errno.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<unistd.h>
#include<libgen.h>
#include<fcntl.h>
int main(int argc,char* argv[]){
    if(argc<=3){
	printf("usage: %s ip_address port_number filename\n",basename(argv[0]));
	return 1;
    }
    //参数赋值
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* file_name = argv[3];
 
    //读取文件
    int filefd = open(file_name,O_RDONLY);
    assert(filefd>0);
    struct stat stat_buf;
    fstat(filefd,&stat_buf);
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

    //监听sokcet
    ret = listen(sock,5);
    assert(ret!=-1);

    //accept连接
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock,(struct sockaddr*)&client,&client_addrlength);
    if(connfd<0){
	printf("errno is:%d\n",errno);

    }
    else{
        sendfile(connfd,filefd,NULL,stat_buf.st_size);
        close(connfd);
    }
    close(sock);
    return 0;
}



















