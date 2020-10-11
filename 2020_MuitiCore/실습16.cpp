#include <iostream>
#include <thread>
#include <atomic>
using namespace std;

constexpr int SIZE = 50'000'000;
volatile int* bound;
volatile bool done = false;
int g_error = 0;
void core_0() {
	for (int i = 0; i < SIZE; ++i) {
		*bound = -(1 + *bound);
	}
	done = true;
}

void core_1() {
	while (!done) {
		int v = *bound;
		if (v == -1) continue;
		if (v == 0) continue;
		g_error++;
	}
}

int main() {
	int* bigdata = new int[32];
	int addr = reinterpret_cast<int>(&bigdata[31]);
	int mod64 = addr % 64;
	addr = addr - mod64;
	addr = addr - 1;
	bound = reinterpret_cast<int*>(addr);
	*bound = 0;
	thread t0{ core_0 };
	thread t1{ core_1 };
	t0.join();
	t1.join();

	cout << "num of memory error : " << g_error << endl;
	delete bigdata;
}