// 성긴 동기화

#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>

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
	Node() {}
	Node(int key_value) : key(key_value) { }

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

	bool Add(int key)
	{
		Node* pred, * curr;
		_head.Lock();
		pred = &_head;
		curr = pred->next;
		curr->Lock();

		while (curr->key < key)
		{
			pred->Unlock();
			pred = curr;
			curr = curr->next;
			curr->Lock();
		}

		if (key == curr->key)
		{
			pred->Unlock();
			curr->Unlock();
			return false;
		}
		else
		{
			Node* node = new Node(key);
			node->next = curr;
			pred->next = node;
			pred->Unlock();
			curr->Unlock();
			return true;
		}
	}

	bool Remove(int key)
	{
		Node* pred, * curr;
		_head.Lock();
		pred = &_head;;
		curr = pred->next;
		curr->Lock();

		while (curr->key < key)
		{
			pred->Unlock();
			pred = curr;
			curr = curr->next;
			curr->Lock();
		}

		if (key == curr->key)
		{
			pred->next = curr->next;
			pred->Unlock();
			curr->Unlock();
			delete curr;
			return true;
		}
		else
		{
			pred->Unlock();
			curr->Unlock();
			return false;
		}

	}

	bool Contains(int key)
	{
		Node* pred, * curr;
		_head.Lock();
		pred = &_head;
		curr = pred->next;
		curr->Lock();

		while (curr->key < key)
		{
			pred->Unlock();
			pred = curr;
			curr = curr->next;
			curr->Lock();
		}

		if (key == curr->key)
		{
			pred->Unlock();
			curr->Unlock();
			return true;
		}
		else {
			pred->Unlock();
			curr->Unlock();
			return false;
		}
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