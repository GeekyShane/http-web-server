#include "min_heap_timer.h"
#include "../http/http_conn.h"

//定义构造函数一，初始化一个大小为cap的空堆
time_heap::time_heap(int cap) : m_capacity(cap), m_cur_size(0)
{
    array = new heap_timer* [m_capacity];  //创建堆数组
    if(!array){
        throw std::exception();
    }
    for(int i = 0;i<m_capacity;++i){
        array[i] = nullptr;
    }
}

//定义构造函数二，用已有数组初始化堆
time_heap::time_heap(heap_timer** init_array, int size, int capacity) : m_cur_size(size), m_capacity(capacity)
{
    if(capacity < size){
        throw std::exception();
    }
    array = new heap_timer*[capacity];  //创建堆数组
    if(!array){
        throw std::exception();
    }
    for(int i = 0;i<capacity;++i){
        array[i] = nullptr;
    }
    if(size!=0){
        //初始化堆数组
        for(int i = 0;i<size;++i){
            array[i] = init_array[i];
        }
        for(int i = (m_cur_size-1)/2;i>=0;--i){
            //对数组中的第[m_cur_size/2-1]～0个元素执行下虑操作
            percolate_down(i);
        }
    }
}

//析构函数，销毁时间堆
time_heap::~time_heap()
{
    for(int i = 0;i<m_cur_size;++i){
        delete array[i];
    }
    delete[] array;
}

//添加目标定时器timer
void time_heap::add_timer(heap_timer* timer)
{
    if(!timer){
        return;
    }
    //如果当前堆容量不够，则将其扩大一倍
    if(m_cur_size >= m_capacity){
        resize();
    }
    //新插入了一个元素，当前堆数组大小加1，hole是新建空穴的位置
    int hole = m_cur_size++;
    int parent = 0;
    //对空穴到根节点的路径上的所有节点执行上虑操作
    for( ;hole>0;hole=parent){
        parent = (hole-1)/2;
        if(array[parent]->expire <= timer->expire){
            break;
        }
        array[hole] = array[parent];
    }
    array[hole] = timer;
}

//删除目标定时器timer
void time_heap::del_timer(heap_timer *timer)
{
    if(!timer){
        return;
    }
    //仅仅将目标定时器的回调函数设置为空，即所谓的延迟销毁。这将节省真正删除该定时器造成的开销，但这样做容易使得数组膨胀。
    timer->cb_func = nullptr;
}

//调整定时器堆
void time_heap::adjust_timer(heap_timer *timer, time_t timeout)
{
    if(timer == nullptr){
        return;
    }
    timer->expire = timeout;
    percolate_down(0);
}

//获得堆顶的定时器
heap_timer* time_heap::top() const
{
    if(empty()){
        return nullptr;
    }
    return array[0];
}

//删除堆顶的定时器
void time_heap::pop_timer()
{
    if(empty()){
        return;
    }
    if(array[0]){
        delete array[0];
        //将原来的堆顶元素替换为堆数组中的最后一个元素
        array[0] = array[--m_cur_size];
        percolate_down(0);  //对新的堆顶元素执行下虑操作
    }
}

//心搏函数
void time_heap::tick()
{
    heap_timer* tmp = array[0];
    time_t timeout = time(NULL);  //循环处理堆中到期的定时器
    while(!empty()){
        if(!tmp){
            break;
        }
        //如果堆顶定时器没到期，则退出循环
        if(tmp->expire > timeout){
            break;
        }
        //否则就执行堆顶定时器中的任务
        if(array[0]->cb_func){
            array[0]->cb_func(array[0]->user_data);
        }
        //将堆顶元素删除，同时生成新的堆顶定时器（array[0]）
        pop_timer();
        tmp = array[0];
    }
}

//判读时间堆容器是否为空
bool time_heap::empty() const
{
    return m_cur_size == 0;
}

//最小堆的下虑操作
void time_heap::percolate_down(int hole)
{
    heap_timer* temp = array[hole];
    int child = 0;
    for( ;(hole*2+1) <= (m_cur_size-1); hole=child){
        if((child < (m_cur_size-1)) && (array[child+1]->expire < array[child]->expire)){
            ++child;
        }
        if(array[child]->expire < temp->expire){
            array[hole] = array[child];
        }else{
            break;
        }
    }
    array[hole] = temp;
}

//将堆数组容量扩大一倍
void time_heap::resize()
{
    heap_timer **temp = new heap_timer* [2*m_capacity];
    for(int i = 0;i<2*m_capacity;i++){
        temp[i] = nullptr;
    }
    if(!temp){
        throw std::exception();
    }
    m_capacity = 2 * m_capacity;
    for(int i = 0;i<m_cur_size;++i){
        temp[i] = array[i];
    }
    delete[] array;
    array = temp;
}






void Utils::init(int timeslot){
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_time_heap->tick();
    //以堆顶定时器定时事件设置为alarm，更精确
    //将所有定时器中时间最小的一个定时器的超时值作为心搏间隔，所以取最小堆的堆顶
    if(!m_time_heap->empty()){
        heap_timer *temp = m_time_heap->top();
        alarm(temp->expire-time(NULL));
    }
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}