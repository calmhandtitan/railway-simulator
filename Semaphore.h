class Semaphore 
{
	mutex mtx;
	condition_variable cv;
	int count;
public:
	explicit Semaphore(int count_ = 1)
		: count(count_) {}

	inline void Notify()
	{
		unique_lock<mutex> lock(mtx);
		count++;
		cv.notify_one();
	}

	inline void Wait()
	{
		unique_lock<mutex> lock(mtx);

		while (count == 0){
			cv.wait(lock);
		}
		count--;
	}
};
