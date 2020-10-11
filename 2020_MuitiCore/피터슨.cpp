#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
using namespace std;
using namespace chrono;

volatile int sum;
mutex sum_lock;

volatile bool flag[2] = { false, false };
volatile int victim;

void p_lock(const int tid) {
	int other = 1 - tid;
	flag[tid] = true;
	victim = tid;
	atomic_thread_fence(memory_order_seq_cst);
	while (flag[other] && victim == tid);
}

void p_unlock(const int tid) {
	flag[tid] = false;
}

void worker(int tid) {
	for (int i = 0; i < 25'000'000; ++i) {
		p_lock(tid);
		sum += 2;
		p_unlock(tid);
	}
}

int main() {
	auto start = high_resolution_clock::now();
	thread t1{ worker, 0 };
	thread t2{ worker, 1 };
	t1.join();
	t2.join();
	auto end = high_resolution_clock::now();
	auto exec = end - start;
	cout << "Exec Time : " << duration_cast<milliseconds>(exec).count() << "ms\n";
	cout << "sum : " << sum << endl;
}

/*
½Ì±Û½º·¹µå 118ms
½Ì±Û¹ÂÅØ½º 1135ms
µà¾ó½º·¹µå 249ms °ª Àß¸ø³ª¿È
¹ÂÅØ½º	  1519ms
ÇÇÅÍ½¼	  3385ms (99924458) °ª ¿À·ù
mfence	  2074ms
½º·¹µåÆæ½º 3193ms
*/

/*
¾ÆÅä¹Í ºñ±³
ÇÇÅÍ½¼		3645ms	99630506
¾ÆÅä¹Í		4979ms	100000000
½º·¹µåÆæ½º	3585ms	100000000
*/