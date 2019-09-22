/*
 * Copyright (c) 2000-2004 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _EVENT_INTERNAL_H_
#define _EVENT_INTERNAL_H_

/* 这个头文件是只供库内部使用的，相应的event.h是供外部使用的 */

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "min_heap.h"
#include "evsignal.h"

/* eventop 是多种 IO 多路复用的一个句柄，在 eventop 中有
** init, add, del, dispatch, dealloc 几种方法，作用分别是：
** 初始化IO多路复用对象，添加事件，删除事件，事件分发，释放资源。
** 也就是说对于epoll，poll，select等都需要实现这几种方法。
** 通过将eventop中成员的函数指针指向对应IO多路复用的函数就实现了动态绑定
*/
struct eventop {
	const char *name;
	void *(*init)(struct event_base *);
	int (*add)(void *, struct event *);
	int (*del)(void *, struct event *);
	int (*dispatch)(struct event_base *, void *, struct timeval *);
	void (*dealloc)(struct event_base *, void *);
	/* set if we need to reinitialize the event base */
	int need_reinit;
};

struct event_base {
    /* eventop 对象指针，决定了使用哪种IO多路复用资源 
    ** 但是 eventop 实际上只保存了函数指针，最后资源的句柄是保存在 evbase 中。
    ** 比如要使用 epoll，那么就应该有一个 epoll 的文件描述符，eventop 中只保存了epoll相关的add，del等函数
    ** epoll 的文件描述符是保存在 evbase 中的，因此调用的形式就是 evsel->add(evbase, ev);
    */
	const struct eventop *evsel;
	void *evbase;
    /* event base 上所有事件的数量包括注册事件和激活事件
    ** 在 event_queue_insert 函数中加 1 */
	int event_count;		/* counts number of total events */
    /* event base 上被激活的事件的数量 */
	int event_count_active;	/* counts number of active events */

	int event_gotterm;		/* Set to terminate loop */
	int event_break;		/* Set to terminate loop immediately */

	/* active event management */
    /* libevent 支持事件的优先级，对于激活的事件，不同优先级的事件存储在不同的链表中 
    ** 然后再用一个链表把这些链表串起来
    */
	struct event_list **activequeues;
    /* 事件可以设定的最大优先级 */
	int nactivequeues;

	/* signal handling info */
	struct evsignal_info sig;
    /* 保存所有注册事件的链表 */
	struct event_list eventqueue;
    /* 上一次进行事件循环的时间 */
	struct timeval event_tv;
    /* 管理时间事件的小顶堆 */
	struct min_heap timeheap;

	struct timeval tv_cache;
};

/* Internal use only: Functions that might be missing from <sys/queue.h> */
#ifndef HAVE_TAILQFOREACH
#define	TAILQ_FIRST(head)		((head)->tqh_first)
#define	TAILQ_END(head)			NULL
#define	TAILQ_NEXT(elm, field)		((elm)->field.tqe_next)
#define TAILQ_FOREACH(var, head, field)					\
	for((var) = TAILQ_FIRST(head);					\
	    (var) != TAILQ_END(head);					\
	    (var) = TAILQ_NEXT(var, field))
#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	(elm)->field.tqe_next = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &(elm)->field.tqe_next;		\
} while (0)
#endif /* TAILQ_FOREACH */

int _evsignal_set_handler(struct event_base *base, int evsignal,
			  void (*fn)(int));
int _evsignal_restore_handler(struct event_base *base, int evsignal);

/* defined in evutil.c */
const char *evutil_getenv(const char *varname);

#ifdef __cplusplus
}
#endif

#endif /* _EVENT_INTERNAL_H_ */
