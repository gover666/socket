#include"ThreadPool.h"
#include<pthread.h>

const int NUMBER = 2;
typedef struct Task
{
	void* (*function)(void* arg);
	void* arg;
}Task;

struct ThreadPool
{
	Task* taskQ;
	int queueCapacity;
	int queueSize;
	int queueFront;
	int queueRear;

	int minNum;
	int maxNum;
	int busyNum;
	int liveNum;
	int exitNum;

	pthread_t ManagerId;
	pthread_t* WorkerIDS;

	pthread_mutex_t busymutex;
	pthread_mutex_t threadpoolmutex;

	pthread_cond_t notFull;
	pthread_cond_t notEmpty;

	int shutdown;
};

ThreadPool* ThreadPoolcreate(int min, int max, int queuesize)
{
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
	do
	{
		if (pool == NULL)
		{
			printf("malloc pool error");
			break;
		}

		pool->WorkerIDS = (pthread_t*)malloc(sizeof(pthread_t) * max);
		if (pool->WorkerIDS == NULL)
		{
			printf("malloc workersIDs error");
			break;
		}
		memset(pool->WorkerIDS, 0, sizeof(pthread_t) * max);
		pool->maxNum = max;
		pool->minNum = min;
		pool->liveNum = min;
		pool->busyNum = 0;
		pool->exitNum = 0;

		if (pthread_mutex_init(&pool->threadpoolmutex, NULL) ||
			pthread_mutex_init(&pool->busymutex, NULL) ||
			pthread_cond_init(&pool->notFull, NULL) ||
			pthread_cond_init(&pool->notEmpty, NULL))
		{
			printf("mutex or condition init fail");
			break;
		}
		pool->taskQ = (Task*)malloc(sizeof(Task) * queuesize);
		pool->queueCapacity = queuesize;
		pool->queueSize = 0;
		pool->queueFront = 0;
		pool->queueRear = 0;
		pool->shutdown = 0;


		pthread_create(&pool->ManagerId, NULL, manager, pool);
		for (int i = 0; i < min; i++)
		{
			pthread_create(&pool->WorkerIDS[i], NULL, worker, pool);

		}
		return pool;
	} while (0);

	if (pool && pool->WorkerIDS)
	{
		free(pool->WorkerIDS);
	}
	if (pool && pool->taskQ)
	{
		free(pool->taskQ);
	}
	if (pool)
	{
		free(pool);
	}

	return NULL;
}

void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void* arg)
{
	pthread_mutex_lock(&pool->threadpoolmutex);
	while (pool->queueSize == pool->queueCapacity && !pool->shutdown)
	{
		pthread_cond_wait(&pool->notFull, &pool->threadpoolmutex);
	}
	if (pool->shutdown)
	{
		pthread_mutex_unlock(&pool->threadpoolmutex);
		return;
	}

	
	pool->taskQ[pool->queueRear].function = func;
	pool->taskQ[pool->queueRear].arg = arg;
	pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
	pool->queueSize++;

	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->threadpoolmutex);
}

int getThreadPoolLiveNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->threadpoolmutex);
	int busyNum = pool->busyNum;
	pthread_mutex_unlock(&pool->threadpoolmutex);

	return busyNum;
}

int getThreadPoolBusyNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->threadpoolmutex);
	int liveNum = pool->liveNum;
	pthread_mutex_unlock(&pool->threadpoolmutex);

	return liveNum;
}

int destroyThreadPool(ThreadPool* pool)
{
	if (pool == NULL)
	{
		return -1;
	}


	pool->shutdown = 1;

	
	pthread_join(pool->ManagerId, NULL);

	
	for (int i = 0; i < pool->liveNum; i++)
	{
		pthread_cond_signal(&pool->notEmpty);
	}



	if (pool->taskQ)
	{
		free(pool->taskQ);
	}

	if (pool->WorkerIDS)
	{
		free(pool->WorkerIDS);
	}
	pthread_mutex_destroy(&pool->threadpoolmutex);
	pthread_mutex_destroy(&pool->busymutex);
	pthread_cond_destroy(&pool->notEmpty);
	pthread_cond_destroy(&pool->notFull);

	free(pool);
	pool = NULL;

	return 0;
}

void* worker(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (1)
	{
		pthread_mutex_lock(&pool->threadpoolmutex);
		
		while (pool->queueSize == 0 && !pool->shutdown)
		{
			
			pthread_cond_wait(&pool->notEmpty, &pool->threadpoolmutex);

			if (pool->exitNum > 0)
			{
				pool->exitNum--;
				if (pool->liveNum > pool->minNum)
				{
					pool->liveNum--;
					pthread_mutex_unlock(&pool->threadpoolmutex);
					ExitThread(pool);
				}
			}

		}
		
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->threadpoolmutex);
			ExitThread(pool);
		}
		
		Task task;
		task.function = pool->taskQ[pool->queueFront].function;
		task.arg = pool->taskQ[pool->queueFront].arg;
		
		pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
		pool->queueSize--;

		
		pthread_cond_signal(&pool->notFull);
		pthread_mutex_unlock(&pool->threadpoolmutex);

		printf("thread %ld start working...\n", pthread_self());
		pthread_mutex_lock(&pool->busymutex);
		pool->busyNum++;
		pthread_mutex_unlock(&pool->busymutex);
		task.function(task.arg);
		free(task.arg);
		task.arg = NULL;


		printf("thread %ld end working...\n", pthread_self());
		pthread_mutex_lock(&pool->busymutex);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->busymutex);

	}
	return NULL;
}

void* manager(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown)
	{
		sleep(3);

		
		pthread_mutex_lock(&pool->threadpoolmutex);
		int queuesize = pool->queueSize;
		int liveNum = pool->liveNum;
		pthread_mutex_unlock(&pool->threadpoolmutex);

		
		pthread_mutex_lock(&pool->threadpoolmutex);
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->threadpoolmutex);

		
		if (liveNum-busyNum < queuesize && liveNum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->threadpoolmutex);
			int counter = 0;
			for (int i = 0; counter < NUMBER && liveNum < pool->maxNum && i < pool->maxNum; i++)
			{
				if (pool->WorkerIDS[i] == 0)
					pthread_create(&pool->WorkerIDS[i], NULL, worker, pool);
				counter++;
				pool->liveNum++;
			}
			pthread_mutex_unlock(&pool->threadpoolmutex);
		}

		
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->threadpoolmutex);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->threadpoolmutex);


			for (int i = 0; i < NUMBER; i++)
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}
	}
	return NULL;
}

void* ExitThread(ThreadPool* pool)
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < pool->maxNum; i++)
	{
		if (tid == pool->WorkerIDS[i])
		{
			pool->WorkerIDS[i] = 0;
			printf("threadExit() called, %ld exiting...\n", tid);
			break;
		}
	}
	pthread_exit(NULL);
}