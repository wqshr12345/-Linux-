#include<pthread.h>
#include<unistd.h>
#include<stdio.h>

//按不同顺序访问互斥锁导致死锁

int a = 0;
int b = 0;
pthread_mutex_t mutex_a;
pthread_mutex_t mutex_b;

void* another(void* arg){
    //原子操作给b上锁。
    pthread_mutex_lock(&mutex_b);
    printf("in child thread,got mutex b,waiting for mutex a\n");
    sleep(5);//为什么sleep5秒？只有这样才能模拟出出现死锁的那种父子线程代码运行顺序，不加这行，可能运行一万次才会出现父进程上锁a，然后正好线程切换，子进程上锁b的情况。
    ++b;
    //原子操作给a上锁。
    pthread_mutex_lock(&mutex_a);
    b += a++;
    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);
    pthread_exit(NULL);
}

int main(){
    pthread_t id;
    pthread_mutex_init(&mutex_a,NULL);
    pthread_mutex_init(&mutex_b,NULL);
    pthread_create(&id,NULL,another,NULL);

    pthread_mutex_lock(&mutex_a);
    printf("in parent thread,got mutex a,waiting for mutex b\n");
    sleep(5);
    ++a;
    pthread_mutex_lock(&mutex_b);
    a+=b++;
    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_a);

    pthread_join(id,NULL);
    pthread_mutex_destroy(&mutex_a);
    pthread_mutex_destroy(&mutex_b);
    return 0;
}
