#ifndef MIN_HEAP_TIMER
#define MIN_HEAP_TIMER

#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include<time.h>
#include "../log/log.h"

using std::exception;

#define BUFFER_SIZE 64

class heap_timer;  //前向声明
//绑定socket和定时器
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    heap_timer* timer;
};

//定时器类
class heap_timer{
public:
    time_t expire;  //定时器生效的绝对时间
    void (*cb_func)(client_data*);  //定时器的回调函数
    client_data* user_data;  //用户数据

    heap_timer(int delay)
    {
        expire = time(NULL) + delay;
    }
};

//时间堆容器类
class time_heap{
public:
    //构造函数之一，初始化一个大小为cap的空堆
    time_heap(int cap);
    //构造函数之二，用已有数组来初始化堆
    time_heap(heap_timer** init_array, int size, int capacity);
    //析构函数，销毁时间堆
    ~time_heap();

    //添加目标定时器timer
    void add_timer( heap_timer* timer );
    //删除目标定时器timer
    void del_timer( heap_timer* timer );
    //调整定时器
    void adjust_timer(heap_timer* timer, time_t timeout);
    //获得堆顶部的定时器
    heap_timer* top() const;  //声明成员函数后加const修饰，表明这个函数是只读函数，不会修改任何数据成员
    //删除堆顶部的定时器
    void pop_timer();
    //心搏函数
    void tick();
    //判空函数
    bool empty() const;

private:
    //最小堆的下滤操作，确保堆数组中以第hole个节点作为根的子树拥有最小堆性质
    void percolate_down(int hole);
    //将堆数组容量扩大1倍
    void resize();

    heap_timer** array;  //堆数组
    int m_capacity;  //堆数组的容量
    int m_cur_size;  //堆数组当前包含的元素个数
};


class Utils
{
public:
    Utils() {
        m_time_heap = new time_heap(10000);
    }
    ~Utils() {
        delete m_time_heap;
    }

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    time_heap *m_time_heap;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);


#endif