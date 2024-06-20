#include <iostream> // 用于输入输出流
#include <chrono> // 用于高精度时间测量
#include <cstdlib> // 包含一些通用的工具函数，如随机数生成
#include <pthread.h> // 用于多线程编程
#include <time.h> // 用于时间处理函数
#include "skiplist.h" // 引入自定义的跳表实现
#include <unistd.h>

#define NUM_THREADS 2
#define TEST_COUNT 100000
SkipList<int, std::string> list(18);

void* insertElement(void* threadid)
{
	int tid;
	tid = *(int *)threadid;
	std::cout << "insertElement: " << tid << std::endl;
	int tmp = TEST_COUNT / NUM_THREADS;
	for (int count=0; count<tmp; count++)
	{
		list.insert_element(rand()%TEST_COUNT, "kazu");
	}
	pthread_exit(NULL);
}

void* searchElement(void* threadid)
{
	int tid;
	tid = *(int *)threadid;
	std::cout << "searchElement: " << tid << std::endl;
	int tmp = TEST_COUNT / NUM_THREADS;
	for (int count=0; count<tmp; count++)
	{
		list.search_element(rand()%TEST_COUNT);
	}
	pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
	srand(time(NULL));
	{
		pthread_t threads[NUM_THREADS];
		int rc;
		int i;

		auto start = std::chrono::high_resolution_clock::now();

		for(i=0;i<NUM_THREADS;i++)
		{
			std::cout << "create threads: " << i << std::endl;
			rc = pthread_create(&threads[i], NULL, insertElement, (void*)&i);

			if(rc!=0)
			{
				std::cout << "create threads failed: " << i << std::endl;
				exit(-1);
			}
		}

		void *ret; 	
		for(i=0;i<NUM_THREADS;i++)
		{
			if(pthread_join(threads[i], (void**)&ret)!=0)
			{
				perror("pthread_join() error\n");
				exit(3);
			}
		}		

		auto finish = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> eplased = finish - start;
		std::cout << "insert elapsed: " << eplased.count() << std::endl;
	}

	sleep(10);
	list.dump_file();


	{
		pthread_t threads[NUM_THREADS];
		int rc;
		int i;

		auto start = std::chrono::high_resolution_clock::now();

		for(i=0;i<NUM_THREADS;i++)
		{
			std::cout << "create threads: " << i << std::endl;
			rc = pthread_create(&threads[i], NULL, searchElement, (void*)&i);

			if(rc!=0)
			{
				std::cout << "create threads failed: " << i << std::endl;
				exit(-1);
			}
		}

		void *ret; 	
		for(i=0;i<NUM_THREADS;i++)
		{
			if(pthread_join(threads[i], (void**)&ret)!=0)
			{
				perror("pthread_join() error\n");
				exit(3);
			}
		}		

		auto finish = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> eplased = finish - start;
		std::cout << "search elapsed: " << eplased.count() << std::endl;
	}	

	pthread_exit(NULL); // 主线程退出
	return 0;
}