#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>

using namespace std;
using namespace chrono;

constexpr int NUM_TEST = 10000000;
constexpr int MAX_THREAD = 8;

class Node {
public:
	int key;
	Node* next;
	Node() {
		key = 0;
		next = nullptr;
	}
	Node(int key_value){
		key = key_value;
		next = nullptr;
	}

	~Node() = default;
};

class CQueue {
	Node *head, *tail;
	mutex mu;
public:
	CQueue() {
		head = tail = new Node();
	}
	~CQueue() {
		Init();
		delete head;
	}

	void Init() {
		Node* ptr;
		while (head!=tail) {
			ptr = head;
			head = ptr->next;
			delete ptr;
		}
	}

	void Enq(int key) {
		Node* e = new Node{ key };
		mu.lock();
		tail->next = e;
		tail = e;
		mu.unlock();
	}

	int Deq() {
		mu.lock();
		if (head == tail) {
			mu.unlock();
			return -1;
		}
		Node* ptr = head;
		head = head->next;
		delete ptr;
		mu.unlock();
		return head->key;
	}

	void Display20() {
		Node* curr = head->next;
		for (int i = 0; i < 20; ++i) {
			if (curr == nullptr) break;
			cout << curr->key << ", ";
			curr = curr->next;
		}
		cout << endl;
	}
};

CQueue cque;

void ThreadFunc(int num_thread) {
	int key;

	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		switch (rand() % 2) {
		case 0: 
			cque.Enq(i);
			break;
		case 1:
			cque.Deq();
			break;
		default:
			cout << "Error\n";
			exit(-1);
		}
	}
}


int main()
{
	for (int num = 1; num <= MAX_THREAD; num = num * 2)
	{
		cque.Init();
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

