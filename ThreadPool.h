#pragma once
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
typedef struct ThreadPool ThreadPool;

ThreadPool* ThreadPoolcreate(int min, int max, int queuesize);



void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void* arg);


int getThreadPoolLiveNum(ThreadPool* pool);


int getThreadPoolBusyNum(ThreadPool* pool);


int destroyThreadPool(ThreadPool* pool);


void* worker(void* arg);
void* manager(void* arg);
void* ExitThread(ThreadPool* pool);

