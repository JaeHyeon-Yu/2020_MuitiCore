#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
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

	bool CAS(Node* volatile* top, Node* old_top, Node* new_top) {
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int volatile*>(top), reinterpret_cast<int*>(&old_top), reinterpret_cast<int>(new_top));
	}

	void Push(int key) {
		Node* e = new Node{ key };
		while (true) {
			Node* first = top;
			e->next = first;
			if (CAS(&top, first, e)) return;
		}
	}

	int Pop() {
		while (true) {
			Node* first = top;
			if (first == nullptr) return -1;
			
			int value = first->key;
			if (CAS(&top, first, first->next)) {
				// delete first;
				return value;
			}
		}
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

