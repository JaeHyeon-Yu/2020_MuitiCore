#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>

using namespace std;
using namespace chrono;

constexpr int NUM_TEST = 10000000;
constexpr int MAX_THREAD = 8;

thread_local int thread_id;
int num_threads;

class Node {
public:
	int key;
	Node* next;
	Node() {
		key = 0;
		next = nullptr;
	}
	Node(int key_value) {
		key = key_value;
		next = nullptr;
	}

	~Node() = default;
};

class CStack {
	Node* volatile top;
	mutex mu;
public:
	CStack() {
		top = nullptr;
	}
	~CStack() {
		Clear();
	}

	void Clear() {
		while (top != nullptr) {
			Node* to_delete = top;
			top = top->next;
			delete to_delete;
		}
	}

	void Push(int key) {
		Node* e = new Node{ key };
		mu.lock();
		e->next = top;
		top = e;
		mu.unlock();
	}

	int Pop() {
		mu.lock();
		if (top == nullptr) {
			mu.unlock();
			return -1;
		}
		
		Node* p = top;
		top = top->next;
		int value = p->key;
		mu.unlock();
		delete p;
		return value;
	}

	void Display20() {
		Node* curr = top;
		for (int i = 0; i < 20; ++i) {
			if (curr == nullptr) break;
			cout << curr->key << ", ";
			curr = curr->next;
		}
		cout << endl;
	}
};

CStack cque;

void ThreadFunc(int num_thread) {
	int key;
	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		if (rand() % 2 == 0 || (i < 1000 / num_thread))
			cque.Push(i);
		else cque.Pop();
	}
}


int main()
{
	for (int num = 1; num <= MAX_THREAD; num = num * 2)
	{
		cque.Clear();
		// num 개수의 스레드를 생성해서 벤치마크 함수 실행
		// 실행 시간을 측정한 후에 출력
		auto s_t = high_resolution_clock::now();

		vector<thread> ths;
		for (int i = 0; i < num; ++i)
			ths.emplace_back(ThreadFunc, num);

		for (auto& th : ths) th.join();
		auto e_t = high_resolution_clock::now();
		auto exec_t = e_t - s_t;
		cout << num << " Thread, " << ", ";
		cout << "Exec Time = " << duration_cast<milliseconds>(exec_t).count() << "ms, " << endl;
		cque.Display20();

	}
}

