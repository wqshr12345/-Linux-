#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>


#define BUFFER_SIZE 4096

//主状态机的两种状态：正在分析请求行，正在分析头部字段。
enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0,CHECK_STATE_HEADER};

//从状态机的三种状态：读到一个完整行、行出错、行数据不完整
enum LINE_STATUS {LINE_OK = 0,LINE_BAD,LINE_OPEN};

//服务器处理HTTP请求的结果
enum HTTP_CODE {NO_REQUEST,GET_REQUEST,BAD_REQUEST,FORBIDDEN_REQUEST,INTERNAL_ERROR,CLOSED_CONNECTION};

//模拟的HTTP应答结果
static const char* szret[] = {"I get a correct result\n","Something wrong\n"};

//从状态机,用于解析一行内容
LINE_STATUS parse_line(char* buffer,int& checked_index,int& read_index){
    char temp;
    //checked_index指向buffer中正在分析的字节，read_index指向buffer中客户数据的尾部的下一字节。
    for(;checked_index<read_index;++checked_index){
        temp = buffer[checked_index];
	if(temp = '\r'){
	    if((checked_index+1)==read_index){
		return LINE_OPEN;
	    }
	    else if(buffer[checked_index+1]=='\n'){
	        buffer[checked_index++]='\0';
	        buffer[checked_index++]='\0';
	        return LINE_OK;
	    }
	    return LINE_BAD;
	}
	else if(temp=='\n'){
	    if((checked_index>1)&&buffer[checked_index-1]=='\r'){
		buffer[checked_index-1]='\0';
		buffer[checked_index++]='\0';
		return LINE_OK;
	     }
	return LINE_BAD;

	}
    }
    return LINE_OPEN;

}

//分析请求行 诸如GET http://www.baidu.com/index.html HTTP/1.0
//吐槽：下面的方法里分割字符串的方法也太丑陋难读了吧。
HTTP_CODE parse_requestline(char* temp,CHECK_STATE& checkstate){
    char* url = strpbrk(temp," \t");
    if(!url){
	return BAD_REQUEST;
    }
    *url++ = '\0';//这样之后原字符串相当于 GET\0http...,url这时候指向h。这么说原来的temp就是GET了，因为遇到\0结束字符串读取，相当于原字符串分成了俩字符串，temp和url。
    
    char* method = temp;
    if(strcasecmp(method,"GET")==0){
	printf("The request method is GET\n");
    }
    else{
	return BAD_REQUEST;
    }
    url += strspn(url," \t");//跳过url字符串中的 \t字段。
    char* version = strpbrk(url," \t");
    if(!version){
	return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version," \t");
    if(strcasecmp(version,"HTTP/1.1")!=0){
	return BAD_REQUEST;
    }
    if(strncasecmp(url,"http://",7)==0){
	url+=7;
 	url = strchr(url,'/');
    }
    if(!url || url[0]!='/'){
	return BAD_REQUEST;
    }
    printf("The request URL is: %s\n",url);
    checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//分析头部字段 诸如 User-Agent Wget/1.12 (linux-gnu)  Host:www.baidu.com  Connection:close

HTTP_CODE parse_headers(char* temp){
    //遇到空行，说明得到正确的HTTP请求(?不懂。)
    if(temp[0]=='\0'){
	return GET_REQUEST;	
    }
    //处理HOST字段
    else if(strncasecmp(temp,"Host:",5)==0){
	temp += 5;
	temp += strspn(temp," \t");
	printf("the request host is: %s\n",temp);

    }
    //其他字段不处理
    else{
	printf("I can not handle this header\n");
    }
    return NO_REQUEST;
}


//正式分析HTTP请求的函数
HTTP_CODE parse_content(char* buffer,int& checked_index,CHECK_STATE& checkstate,int& read_index,int& start_line){
    LINE_STATUS linestatus = LINE_OK;
    HTTP_CODE retcode = NO_REQUEST;
    while((linestatus = parse_line(buffer,checked_index,read_index))==LINE_OK){
	char* temp = buffer+start_line;
	start_line = checked_index;
	switch(checkstate){
	    case CHECK_STATE_REQUESTLINE:
	    {
		retcode = parse_requestline(temp,checkstate);
		if(retcode == BAD_REQUEST){
		    return BAD_REQUEST;
		}
		break;
	    }
	    case CHECK_STATE_HEADER:
	    {
		retcode = parse_headers(temp);
		if(retcode == BAD_REQUEST){
		    return BAD_REQUEST;
		}
		else if(retcode == GET_REQUEST){
		    return GET_REQUEST;
		}
	        break;
	    }
	    default:{
		return INTERNAL_ERROR;
	    }
	}
    }
    if(linestatus == LINE_OPEN){
	return NO_REQUEST;
    }
    else{
	return BAD_REQUEST;
    }

}

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

    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock>=0);

    int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
    assert(ret!=-1);
    
    ret = listen(sock,5);
    assert(ret!=-1);
    
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock,(struct sockaddr*)&client,&client_addrlength);
    if(connfd<0){
	printf("errno is: %d\n",errno);
    }
    else{
	char buffer[BUFFER_SIZE];
	memset(buffer,'\0',BUFFER_SIZE);
	int data_read = 0;
	int read_index = 0;//当前缓冲区已经读了多少。
	int checked_index = 0;//从状态机已经分析了多少
	int start_line = 0;//主状态机中一行的起始位置
	CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
	while(1){
	    data_read = recv(connfd,buffer+read_index,BUFFER_SIZE-read_index,0);
	    if(data_read=-1){
		printf("reading failed\n");
		break;	
	    }
	    else if(data_read = 0){
		printf("remote client had closed the connection\n");
	   	break;
	    }
	    read_index += data_read;
	    HTTP_CODE result = parse_content(buffer,checked_index,checkstate,read_index,start_line);
	    if(result == NO_REQUEST){
		continue;	
	    }
	    else if(result == GET_REQUEST){
		send(connfd,szret[0],strlen(szret[0]),0);
	   	break;
	    }
	    else{
		send(connfd,szret[1],strlen(szret[1]),0);
		break;
	    }
	}
	close(connfd);
    }
    close(sock);
    return 0;
}
