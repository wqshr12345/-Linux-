#include<netdb.h>
#include<stdio.h>
#include<string.h>
int main(int argc,char *argv[]){

    const char* name = "www.baidu.com";
    const char* ip = "192.168.80.129";
    
    struct hostent* ptemp1 = gethostbyname(name);
   // struct hostent* ptemp2 = gethostbyaddr(ip,sizeof(ip),AF_INET);
    printf("%s",ptemp1->h_addr_list[0]);
   // printf("%s",ptemp2->h_aliases[0]);
    return 0;
}
