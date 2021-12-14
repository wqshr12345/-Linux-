




int main(int argc,char* argv[]){
    //第一个参数是key键值，唯一标识一个信号量集，如果这个参数的信号量集不存在，就新建;如果存在，就获取。有一个例外，使用IPC_PRIVATE的话，一定新建一个;第二个参数是信号集中信号量的个数;第三个参数是指定一组标志。

    int sem_id = semget(IPC_PRIVATE,1,0666);
    
    union semun sem_un;
    sem_un.val = 1;
    //第一个参数是由semget调用返回的信号量集标识符;第二个参数是被操作的信号量在信号量集中的位置;第三个参数是要执行的命令
    semctl(sem_id,0,SETVAL,sem_un);

}
