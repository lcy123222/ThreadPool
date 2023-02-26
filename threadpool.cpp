#include"threadpool.h"
#include<functional>
#include<thread>
#include<iostream>
const int TASK_MAX_THRESHHOLD = 1024;
const int THREAD_MAX_THRESHHOLD = 1024;
const int THREAD_MAX_IDLE_TIME = 10;
ThreadPool::ThreadPool():initThreadSize_(4),
taskSize_(0),
idleThreadSize_(0),
curThreadSize_(0),
taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD),
threadSizeThreshHold_(THREAD_MAX_THRESHHOLD),
poolMode_(PoolMode::MODE_FIXED),
isPoolRunning_(false)
{}
ThreadPool::~ThreadPool()
{
	isPoolRunning_ = false;

	// 等待线程池里面所有的线程返回  有两种状态：阻塞 & 正在执行任务中
	std::unique_lock<std::mutex> lock(taskQueMtx_);
	notEmpty_.notify_all();
	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
}

void ThreadPool::setMode(PoolMode mode)
{
	if (checkRunningState())
		return;
	poolMode_ = mode;
}

void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
	taskQueMaxThreshHold_ = threshhold;
}
void ThreadPool::setThreadSizeThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
	if (poolMode_ == PoolMode::MODE_CACHED)
	{
		threadSizeThreshHold_ = threshhold;
	}
}
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	std::unique_lock<std::mutex> lock(taskQueMtx_);
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_;}))
	{
		std::cerr << "task queue is full, sumbit failed" << std::endl; 
		return Result(sp,false);
	}
	taskQue_.emplace(sp);
	taskSize_++;
	notEmpty_.notify_all();
	if (poolMode_ == PoolMode::MODE_CACHED
		&& taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeThreshHold_)
	{
		std::cout << ">>> create new thread..." << std::endl;

		// 创建新的线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		// 启动线程
		threads_[threadId]->start();
		// 修改线程个数相关的变量
		curThreadSize_++;
		idleThreadSize_++;
	}
	return Result(sp, true);
	
}
void  ThreadPool::start(int initThreadSize)
{
	isPoolRunning_ = true;

	// 记录初始线程个数
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;
	for (int i = 0; i < initThreadSize_;i++)
	{
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
	}
	for (int i = 0;i < initThreadSize_;i++)
	{
		threads_[i]->start();
		idleThreadSize_++;
	}

}
void ThreadPool::threadFunc (int threadid)
{
	auto lastTime = std::chrono::high_resolution_clock().now();
	for(;;)
	{
		std::shared_ptr<Task> task;
		{
			std::unique_lock<std::mutex> lock(taskQueMtx_);
			std::cout << "tid:" << std::this_thread::get_id() <<"尝试获取任务...！" << std::endl;
			
			while (taskQue_.size() == 0)
			{
				if (!isPoolRunning_)
				{

					threads_.erase(threadid);
					std::cout << "threadid:" << std::this_thread::get_id() << "exit" << std::endl;
					exitCond_.notify_all();
					return;
				}
				if (poolMode_ == PoolMode::MODE_CACHED)
				{
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_)
						{
							threads_.erase(threadid);
							curThreadSize_--;
							idleThreadSize_--;
							std::cout << "threadid:" << std::this_thread::get_id() << "exit" << std::endl;
							return;
						}
					}
				}
				else
				{
					notEmpty_.wait(lock);
				}
				/*if (!isPoolRunning_)
				{
					threads_.erase(threadid);
					std::cout << "threadid:" << std::this_thread::get_id() << "exit" << std::endl;
					return;
				}*/

			}
			
			
			
		
			idleThreadSize_--;
			std::cout << "tid:" << std::this_thread::get_id() << "获取任务成功！" << std::endl;
			task= taskQue_.front();
			taskQue_.pop();
			taskSize_--;
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}
			notFull_.notify_all();
		}
		
		if (task!=nullptr)
		{
			task->exec();
		}
		lastTime= std::chrono::high_resolution_clock().now();
		idleThreadSize_++;
		  

	}
	
	
}
void Thread::start()
{
	std::thread t(func_,threadId_);
	t.detach();
}
void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run()); // 这里发生多态调用
	}

};
int Thread::generateId_ = 0;
Thread::Thread(ThreadFunc func):func_(func),threadId_(generateId_++)
{}
Thread::~  Thread()
{}
Result::Result(std::shared_ptr<Task> task, bool isVaild):task_(task), isVaild_(isVaild)
{
	task_->setResult(this); 
}
Any Result::get()
{
	if (!isVaild_)
	{
		return "";
	 }
	sem_.wait();
	return std::move(any_);
};
void Result::setVal(Any any)
{
	this->any_ = std::move(any);
	sem_.post();
}
void Task::setResult(Result* res)
{
	result_ = res; 
}
Task::Task() :result_(nullptr)
{

}
bool ThreadPool::checkRunningState() const
{
	return isPoolRunning_;
}
int Thread::getId()const
{
	return threadId_;
}