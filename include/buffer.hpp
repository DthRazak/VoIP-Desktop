#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

template<class T, class Container = std::vector<T>>
class ThreadSafeCircleBuffer {
private:
	mutable std::mutex read_mut, write_mut;
	std::condition_variable cond_data;
	std::atomic<size_t> distance;
	//size_t distance;
	Container buffer;
	typename Container::iterator write_it, read_it, end_it;

	void moveNext(typename Container::iterator &it);
public:
	ThreadSafeCircleBuffer();
	ThreadSafeCircleBuffer(size_t buffSize);
	ThreadSafeCircleBuffer(ThreadSafeCircleBuffer const & other);

	~ThreadSafeCircleBuffer() = default;

	void write(T &val);
	void write_copy(T val);
	void writeWithMove(T &&val);
	void read(T &val);

	bool empty() const;
	bool full() const;
};



template<class T, class Container>
void ThreadSafeCircleBuffer<T, Container>::moveNext(typename Container::iterator &it) {
	if (it != end_it)
		++it;
	else
		it = buffer.begin();
}

template<class T, class Container>
ThreadSafeCircleBuffer<T, Container>::ThreadSafeCircleBuffer() :
	distance(0),
	buffer(100)
{
	std::for_each(buffer.begin(), buffer.end(), [](T& el) { el = std::move(T()); });
	write_it = buffer.begin();
	read_it = std::prev(buffer.end());
	end_it = std::prev(buffer.end());
}

template<class T, class Container>
ThreadSafeCircleBuffer<T, Container>::ThreadSafeCircleBuffer(size_t buffSize) :
	distance(0),
	buffer(buffSize)
{
	std::for_each(buffer.begin(), buffer.end(), [](T& el) { el = std::move(T()); });
	write_it = buffer.begin();
	read_it = std::prev(buffer.end());
	end_it = std::prev(buffer.end());
}

template<class T, class Container>
ThreadSafeCircleBuffer<T, Container>::ThreadSafeCircleBuffer(ThreadSafeCircleBuffer const & other) {
	std::lock(other.read_mut, other.write_mut);
	std::lock_guard<std::mutex> read_lk(other.read_mut, std::adopt_lock);
	std::lock_guard<std::mutex> write_lk(other.write_mut, std::adopt_lock);
	buffer = other.buffer;
	distance = other.distance;
	write_it = buffer.begin() + std::distance(other.buffer.begin(), other.write_it);
	read_it = buffer.begin() + std::distance(other.buffer.begin(), other.read_it);
	end_it = std::prev(buffer.end());
}

template<class T, class Container>
void ThreadSafeCircleBuffer<T, Container>::write(T & val) {
	std::unique_lock<std::mutex> lk(write_mut);
	*write_it = val;
	distance++;
	cond_data.wait(lk, [this] { return !(distance == buffer.capacity()); });
	cond_data.notify_all();
	moveNext(write_it);
}

template<class T, class Container>
void ThreadSafeCircleBuffer<T, Container>::write_copy(T val) {
	std::unique_lock<std::mutex> lk(write_mut);
	*write_it = val;
	distance++;
	cond_data.wait(lk, [this] { return !(distance == buffer.capacity()); });
	cond_data.notify_all();
	moveNext(write_it);
}

template<class T, class Container>
void ThreadSafeCircleBuffer<T, Container>::writeWithMove(T &&val) {
	std::unique_lock<std::mutex> lk(write_mut);
	*write_it = std::move(val);
	distance++;
	cond_data.wait(lk, [this] { return !(distance == buffer.capacity()); });
	cond_data.notify_all();
	moveNext(write_it);
}

template<class T, class Container>
void ThreadSafeCircleBuffer<T, Container>::read(T & val) {
	std::unique_lock<std::mutex> lk(read_mut);
	cond_data.wait(lk, [this] {return !(distance == 0); });
	moveNext(read_it);
	val = *read_it;
	distance--;
	cond_data.notify_all();
}

template<class T, class Container>
bool ThreadSafeCircleBuffer<T, Container>::empty() const {
	std::lock(read_mut, write_mut);
	std::lock_guard<std::mutex> read_lk(read_mut, std::adopt_lock);
	std::lock_guard<std::mutex> write_lk(write_mut, std::adopt_lock);
	return distance == 0;
}

template<class T, class Container>
bool ThreadSafeCircleBuffer<T, Container>::full() const {
	std::lock(read_mut, write_mut);
	std::lock_guard<std::mutex> read_lk(read_mut, std::adopt_lock);
	std::lock_guard<std::mutex> write_lk(write_mut, std::adopt_lock);
	return distance == buffer.capacity();
}
