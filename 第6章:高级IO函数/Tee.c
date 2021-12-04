#define _GNU_SOURCE
#include<fcntl.h>
#include<assert.h>
#include<unistd.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>

int main(int argc,char* argv[]){
    if(argc!=2){
	printf("usage: %s <filename>\n",argv[0]);
	return 1;
}
    //打开指定文件。
    int filefd = open(argv[1],O_CREAT|O_WRONLY|O_TRUNC,0666);
    assert(filefd>0);
    
    int pipefd_stdout[2];
    int ret = pipe(pipefd_stdout);
    assert(ret!=-1);
 
    int pipefd_file[2];
     ret = pipe(pipefd_file);
    assert(ret!=-1);

    //把标准输入内容放到管道pipefd_stdout
    ret = splice(STDIN_FILENO,NULL,pipefd_stdout[1],NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
    assert(ret!=-1);
    //把管道pipefd_stdout内容复制到pipefd_file
    ret = tee(pipefd_stdout[0],pipefd_file[1],32768,SPLICE_F_NONBLOCK);
    //把管道pipefd_file内容定向到文件
    ret = splice(pipefd_file[0],NULL,filefd,NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
    //把管道pipefd_stdout内容定向到标准输出
    ret = splice(pipefd_stdout[0],NULL,STDOUT_FILENO,NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
    assert(ret!=-1);
    close(filefd);
    close(pipefd_stdout[0]);
    close(pipefd_stdout[1]);
    close(pipefd_file[0]);
    close(pipefd_file[1]);
    return 0;
}  
