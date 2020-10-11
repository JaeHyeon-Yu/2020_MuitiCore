// 실습18
// CAS를 사용한 Lock, Unlock

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
using namespace std;
using namespace chrono;

volatile int sum{ 0 };
mutex sum_lock;

volatile int LOCK = 0;

bool CAS(volatile int* L, int expected, int new_val) {
	return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(L), &expected, new_val);
}

void cas_lock() {
	while (!CAS(&LOCK, 0, 1));	
}

void cas_unlock() {
	LOCK = 0;
}

void thread_func(int num_threads) {
	volatile int local_sum = 0;
	for (int i = 0; i < 50'000'000 / num_threads; ++i) {
		// sum_lock.lock();
		cas_lock();
		sum += 2;
		cas_unlock();
		// sum_lock.unlock();
	}
}

int main() {
	for (int cnt = 1; cnt <= 16; cnt *= 2) {
		sum = 0;
		vector<thread> threads;
		auto start = high_resolution_clock::now();
		for (int i = 0; i < cnt; ++i)
			threads.emplace_back(thread_func, cnt);
		for (auto& t : threads)
			t.join();
		auto end = high_resolution_clock::now();

		auto exec_t = end - start;
		cout << "threads num : " << cnt << "\t";
		cout << "sum : " << sum << "\t";
		cout << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
	}
}