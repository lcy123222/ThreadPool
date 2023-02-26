#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<vector>
#include<queue>
#include<memory>
#include<mutex>
#include<condition_variable>
#include<atomic>
#include<unordered_map>
#include<functional>
#include<thread>
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	// ������캯��������Any���ͽ�����������������
	template<typename T>  // T:int    Derive<int>
	Any(T data) : base_(std::make_unique<Derive<T>>(data))
	{}

	// ��������ܰ�Any��������洢��data������ȡ����
	template<typename T>
	T cast_()
	{
		// ������ô��base_�ҵ�����ָ���Derive���󣬴�������ȡ��data��Ա����
		// ����ָ�� =�� ������ָ��   RTTI
		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw "type is unmatch!";
		}
		return pd->data_;
	}
private:
	// ��������
	class Base
	{
	public:
		virtual ~Base() = default;
	};

	// ����������
	template<typename T>
	class Derive : public Base
	{
	public:
		Derive(T data) : data_(data)
		{}
		T data_;  // �������������������
	};

private:
	// ����һ�������ָ��
	std::unique_ptr<Base> base_;
};
class Semaphore
{
public:
	Semaphore(int limit = 0) :resLimit_(limit),isexit_(false) {};
	~Semaphore()
         {isexit_=true;};
	void wait()
	{        if(isexit_)
                  return;
		std::unique_lock<std::mutex> lock(mtx_);
	
			cond_.wait(lock, [&]()->bool {
				return resLimit_ > 0;
				});
			resLimit_--;
	}
	void  post()
	{       if(isexit_)
                return ;
		std::unique_lock<std::mutex> lock(mtx_);
		resLimit_++;
		cond_.notify_all();

	}
private:
	int resLimit_;
        std:: atomic_bool isexit_;
	std::mutex mtx_;
	std::condition_variable cond_;
};
class Task;
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isVaild = true);
	~Result() = default;
	void setVal(Any any_);
	Any get();
private:
	Any any_;
	Semaphore sem_;
	std::shared_ptr<Task> task_;
	std::atomic_bool isVaild_;
};
class Task
{
public:
	Task();
	void exec();
	void setResult(Result* res);
	virtual Any run() = 0;
private:
	Result *result_;
};



enum class PoolMode
{
	MODE_FIXED,
	MODE_CACHED,
};
class Thread
{
public:
	using ThreadFunc = std::function<void(int)>;
	Thread(ThreadFunc func);
	~Thread();
	void start();
	int getId()const;
private:
	ThreadFunc func_;
	static int generateId_;
	int threadId_;
};
class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();
	void start(int initThreadSize=std::thread::hardware_concurrency());
	//void setInitThreadSize(int size);
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	void setMode(PoolMode mode);
	void setTaskQueMaxThreshHold(int threshhold);
	void setThreadSizeThreshHold(int threshhold);
	Result submitTask(std::shared_ptr<Task> sp);
private:
	bool checkRunningState() const;
	void threadFunc(int threadid);
private:
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;
	size_t initThreadSize_;
	std::atomic_int curThreadSize_;	// ��¼��ǰ�̳߳������̵߳�������
	std::atomic_int idleThreadSize_;
	std::queue<std::shared_ptr<Task>> taskQue_;
	std::atomic_int taskSize_; 
	int threadSizeThreshHold_;
	int taskQueMaxThreshHold_;  
	std::mutex taskQueMtx_;
	std::condition_variable notFull_;
	std::condition_variable notEmpty_;
	PoolMode poolMode_;
	std::condition_variable exitCond_;
	std::atomic_bool isPoolRunning_;

};

#endif // !THREADPOOL_H
