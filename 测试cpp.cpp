#include<iostream>
#include<chrono>
#include<thread>
#include"threadpool.h"

using namespace std;
class MyTask : public Task
{
public:
	MyTask(int begin, int end) :begin_(begin),end_(end)
	{

	}
	Any run()
	{
		long long sum = 0;
		cout << "tid:" << std::this_thread::get_id()<<"begin!" << endl;
		//std::this_thread::sleep_for(std::chrono::seconds(2));
		for (int i = begin_;i <= end_;i++)
		{
			sum+= i;
		}
		cout << "tid:" << std::this_thread::get_id() << "end!" << endl;
		return sum;
		
		
	}
private:
	int begin_;
	int	end_;
};
int main()
{
        {
	ThreadPool pool;
	pool.setMode(PoolMode::MODE_CACHED);
	pool.start(2);
	Result res1=pool.submitTask(std::make_shared<MyTask>(1,10000));
	Result res2 = pool.submitTask(std::make_shared<MyTask>(10000, 20000));
	Result res3 = pool.submitTask(std::make_shared<MyTask>(20000, 30000));
    pool.submitTask(std::make_shared<MyTask>(1, 10000));
	pool.submitTask(std::make_shared<MyTask>(1, 10000));
    pool.submitTask(std::make_shared<MyTask>(1, 10000));

	long long sum1 = res1.get().cast_<long long>();


	long long sum2 = res2.get().cast_<long long>();

	long long sum3 = res3.get().cast_<long long>();
	cout << (sum1 + sum2 + sum3) << endl;
       }
        cout<<"main over"<<endl;
	getchar();
}
