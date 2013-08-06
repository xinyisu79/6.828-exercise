#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

// #define SOL

static int nthread = 1;
static int round = 0;

struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

static void 
barrier()
{
  pthread_mutex_lock(&bstate.barrier_mutex);
  bstate.nthread++;
  if (bstate.nthread  == nthread){
	  bstate.round++;
	  bstate.nthread = 0;
	  pthread_cond_broadcast(&bstate.barrier_cond);
	  //之前弄错了，还没有unlock就return了。
	  //但是有一个比较神奇的现象：
	  //thread()循环中的printf()一直没有打印出任何东西(已经被注释)
	  //十分纳闷：因为即使deadlock，printf还是可以分别在两个thread
	  //里运行1次，2次，一共应该能看到3条printf的结果啊。
	  //猜测是printf的buffer缓冲区，果然，printf加了\n之后就能看到了。
//	  return;
  }
  else
	  pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
  pthread_mutex_unlock(&bstate.barrier_mutex);
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
//    printf("%dth iteration\n", i);
    int t = bstate.round;
    assert (i == t);
	//如果不是同时到达barrier才执行bstate.round++的话，
	//那就有可能某个线程还没到barrier，另外的进程就已经round++了
	//这样后到barrier的线程会多对round++一次，所以最后i != bstate.round
	//的情况发生。我们要保证的是：所有进程都到barrier了，
	//才执行（且仅执行一次）round++
    barrier();
    usleep(random() % 100);
  }
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
}
