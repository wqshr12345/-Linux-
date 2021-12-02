#include<stdio.h>
void byteorder(){
  union {
    short value;
    char union_bytes[sizeof(short)];
    }test;
    test.value = 0x0102;
    //有个问题～这里的union_bytes是按照字节序存储吗？不应该是按照人的习惯大端存吗？
    if((union_bytes[0]==1)&&union_bytes[1]==2)){
	printf("big endian\n");
	}
    else if ((union_bytes[0]==2)&&union_bytes[1]==1)){
	printf("little endian\n");
	}
    else printf("unknow...\n");
}

