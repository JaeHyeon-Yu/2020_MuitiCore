#include <iostream>
#include <thread>
#include <atomic>
using namespace std;

constexpr int SIZE = 50'000'000;
atomic_int x, y;
int trace_x[SIZE], trace_y[SIZE];

void core_0() {
	for (int i = 0; i < SIZE; ++i) {
		x = i;
		// atomic_thread_fence(memory_order_seq_cst);
		trace_y[i] = y;
	}
}

void core_1() {
	for (int i = 0; i < SIZE; ++i) {
		y = i;
		// atomic_thread_fence(memory_order_seq_cst);
		trace_x[i] = x;
	}
}

int main() {
	thread t0{ core_0 };
	thread t1{ core_1 };
	t0.join();
	t1.join();

	int mem_err = 0;
	for (int i = 0; i < SIZE-1; ++i) {
		if (trace_x[i] == trace_x[i + 1])
			if (trace_y[trace_x[i]] == trace_y[trace_x[i] + 1])
				if (trace_y[trace_x[i]] == i) {
					mem_err++;
				}
	}


	cout << "num of memory error : " << mem_err << endl;
}