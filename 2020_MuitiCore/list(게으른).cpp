// 게으른 동기화

#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>

using namespace std;
using namespace chrono;

constexpr int NUM_TEST = 4000000;
constexpr int KEY_RANGE = 1000;
constexpr int MAX_THREAD = 8;



class Node
{
public:
	int key = 0;
	Node* next = nullptr;
	mutex nodeLock;
	bool isRemoved; // marked

	Node() {
		next = NULL;
		isRemoved = false;
	}
	Node(int key_value) : key(key_value) {
		isRemoved = false;
	}

	~Node() {}

	void Lock() {
		nodeLock.lock();
	}
	void Unlock() {
		nodeLock.unlock();
	}
};

class CList
{
	Node _head;
	Node _tail;

public:
	CList()
	{
		_head.key = 0x8000000;
		_tail.key = 0xFFFFFFF;
		_head.next = &_tail;
	}

	bool is_valided(Node* pred, Node* curr) {
		return !pred->isRemoved && !curr->isRemoved && pred->next == curr;
	}

	bool Add(int key)
	{
		while (true) {
			Node* pred, * curr;
			pred = &_head;
			curr = pred->next;

			while (curr->key < key)
			{
				pred = curr;
				curr = curr->next;
			}

			pred->Lock();
			curr->Lock();
			if (is_valided(pred, curr)) {
				if (curr->key == key) {
					pred->Unlock();
					curr->Unlock();
					return false;
				}
				else{
					Node* node = new Node(key);
					node->next = curr;
					pred->next = node;
					pred->Unlock();
					curr->Unlock();
					return true;
				}
			}
			else {
				pred->Unlock();
				curr->Unlock();
				return false;
			}
		}
	}

	bool Remove(int key)
	{
		while (true) {
			Node* pred, * curr;;
			pred = &_head;;
			curr = pred->next;

			while (curr->key < key)
			{
				pred = curr;
				curr = curr->next;
			}

			pred->Lock();
			curr->Lock();

			if (is_valided(pred, curr)) {
				if (curr->key != key) {
					pred->Unlock();
					curr->Unlock();
					return false;
				}
				else {
					curr->isRemoved = true;
					// 순서 존나 중요해서 컴파일러나 cpu가 바꾸지 않도록 해야함
					atomic_thread_fence(memory_order_seq_cst);
					pred->next = curr->next;
					pred->Unlock();
					curr->Unlock();
					return true;
				}
			}
			else {
				pred->Unlock();
				curr->Unlock();
				return false;
			}
		}
	}

	bool Contains(int key) {
		Node* curr = &_head;
		while (curr->key < key) {
			curr = curr->next;
		}
		return curr->key == key && !curr->isRemoved;
	}

	void Display20() {
		Node* ptr = _head.next;
		for (int i = 0; i < 20; ++i) {
			if (ptr == &_tail) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}

	void Init() {
		Node* ptr;
		while (_head.next != &_tail)
		{
			ptr = _head.next;
			_head.next = _head.next->next;
			delete ptr;
		}
	}
};

CList clist;

void ThreadFunc(int num_thread)
{
	int key;
	for (int i = 0; i < NUM_TEST / num_thread; i++) {
		switch (rand() % 3) {
		case 0: key = rand() % KEY_RANGE;
			clist.Add(key);
			break;
		case 1: key = rand() % KEY_RANGE;
			clist.Remove(key);
			break;
		case 2: key = rand() % KEY_RANGE;
			clist.Contains(key);
			break;
		default: cout << "Error\n";
			exit(-1);
		}
	}
}


int main()
{
	for (int num = 1; num <= MAX_THREAD; num = num * 2)
	{
		clist.Init();
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
		clist.Display20();

	}
}


/*
1	2276ms
2	1361ms
4	868ms
8	690ms

*/