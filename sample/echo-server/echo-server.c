
// 系统头文件
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

// libevent2.x头文件
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h> // for evconnlistener
#include <event2/util.h>
#include <event2/event.h> // for event

// 端口号
static const int PORT = 9999;
static const char MESSAGE[] = "hello world\n";

static void listener_cb(struct evconnlistener* listener, evutil_socket_t fd, 
    struct sockaddr *addr, int len, void* user_data);
// static void conn_writecb(struct bufferevent *bev, void *user_data);
static void conn_readcb(struct bufferevent *bev, void *user_data);
static void conn_eventcb(struct bufferevent* bev, short events, void* user_data);
static void signal_cb(evutil_socket_t sig, short events, void* user_data);
static void listen_error_cb(struct evconnlistener *listener, void *user_data);

int main()
{
    // event_base 结构体是 libevent 实现时间循环的核心，每个事件循环通过 event_base 来调度
    struct event_base *base;
    // evconnlistener 机制提供了监听和接受 TCP 连接的方法
    struct evconnlistener *listener;
    // event 是操作的基本单元，libevent 将读、写、信号、超时等全都抽象为事件
    // 这里定义一个信号事件，捕捉指定的信号实现程序优雅的退出
    struct event *signal_event;

    struct sockaddr_in sin;

    // 初始化event_base
    base = event_base_new();
    if (!base)
    {
        fprintf(stderr, "Could not initialize libevent\n");
        return 1;
    }

    // 初始化 sockaddr_in 结构体，监听在0.0.0.0:9999 
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    // 创建套接字并绑定到上面指定的IP和端口上，将监听套接字注册到 event_base 的事件循环中
    // 选项 LEV_OPT_CLOSE_ON_FREE-释放连接监听器会关闭底层套接字
    // 选项 LEV_OPT_REUSEABLE-某些平台在默认情况下 ,关闭某监听套接字后 ,
    // 要过一会儿其他套接字才可以绑定到同一个 端口。设置这个标志会让 libevent 标记套接字是可重用的,
    // 这样一旦关闭,可以立即打开其 他套接字,在相同端口进行监听。
    listener = evconnlistener_new_bind(base, listener_cb, (void*)base, 
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1, (struct sockaddr*)&sin, sizeof(sin));
    if (!listener)
    {
        fprintf(stderr, "Could not create a listener\n");
        return 1;
    }
    else
    {
        printf("Listening on port:%d\n", PORT);
    }

    // 注册错误处理回调函数
    evconnlistener_set_error_cb(listener, listen_error_cb);

    // 创建一个信号事件
    signal_event = evsignal_new(base, SIGINT, signal_cb, (void*)base);
    // 将信号事件设置为未决的，只有未决的事件才会被处理
    if (!signal_event || event_add(signal_event, NULL) < 0)
    {
        fprintf(stderr, "Could not create/add a signal event\n");
        return 1;
    }

    // 启动时间循环，直到调用 event_base_loopbreak() 或者 event_base_loopexit() 为止
    event_base_dispatch(base);

    // 执行清理工作
    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);

    printf("Exit\n");
    return 0;
}

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, 
    struct sockaddr *addr, int len, void* user_data)
{
    struct event_base* base = user_data;
    struct bufferevent* bev;
    /* 新建立了一个连接套接字，为这个套接字新建一个bufferevent
       每个 bufferevent 都有一个输入缓冲区和输出缓冲区 */
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    // 每个 bufferevent 有两个数据相关的回调:一个读取回调和一个写入回调，会分别操作
    // 输入缓冲区和输出缓冲区
    // 这里为 bufferevent 设定回调函数，conn_writecb在写入数据后调用，conn_eventcb出错时调用
    bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, NULL);
    bufferevent_enable(bev, EV_WRITE | EV_READ);
    /* 向刚建立连接的客户端先打一个招呼，向 bufferevent 写入 hello-world */
    // bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}

// static void conn_writecb(struct bufferevent *bev, void *user_data)
// {
// 	struct evbuffer *output = bufferevent_get_output(bev);
// 	if (evbuffer_get_length(output) == 0) {
// 		printf("flushed answer\n");
// 		bufferevent_free(bev);
// 	}
// }

/* 当 bufferevent 中有数据可以读时，这个回调函数会被调用 */
static void conn_readcb(struct bufferevent *bev, void *user_data)
{
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);
    /* 将 input 缓存区的数据全部拷贝到 output 缓冲区中 */
    evbuffer_add_buffer(output, input);
}

static void conn_eventcb(struct bufferevent* bev, short events, void* user_data)
{
    if (events & BEV_EVENT_EOF)
    {
        // 客户端关闭连接
        printf("Connection closed\n");
    }
    else if (events & BEV_EVENT_ERROR)
    {
        fprintf(stderr, "Got an error on the connection:%s\n", strerror(errno));
    }
    bufferevent_free(bev);
}

static void signal_cb(evutil_socket_t sig, short events, void* user_data)
{
    struct event_base* base = user_data;
    struct timeval delay = {2, 0};
    printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");
    // 退出事件循环
    event_base_loopexit(base, &delay);
}

static void listen_error_cb(struct evconnlistener *listener, void *user_data)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error %d (%s) on the listener. Shutting down.\n", 
        err, evutil_socket_error_to_string(err));
    event_base_loopexit(base, NULL);
}