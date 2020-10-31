// 게으른 동기화 with shared ptr

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
	shared_ptr<Node> next = nullptr;
	mutex nodeLock;
	bool isRemoved; // marked

	Node() {
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
	shared_ptr<Node> _head;
	shared_ptr<Node>_tail;

public:
	CList()
	{
		_head = make_shared<Node>(0x8000000);
		_tail = make_shared<Node>(0xFFFFFFF);
		_head->next = _tail;
	}

	bool is_valided(const shared_ptr<Node>& pred, const shared_ptr<Node>& curr) {	// &로 넘겨줘야 생성자 소멸자가 호출되지 않음
		return !pred->isRemoved && !curr->isRemoved && pred->next == curr;
	}

	bool Add(int key)
	{
		while (true) {
			shared_ptr<Node> pred, curr;
			pred = _head;
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
				else {
					shared_ptr<Node> node = make_shared<Node>(key);
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
			shared_ptr<Node> pred, curr;;
			pred = _head;;
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
		shared_ptr<Node> curr = _head;
		while (curr->key < key) {
			curr = curr->next;
		}
		return curr->key == key && !curr->isRemoved;
	}

	void Display20() {
		shared_ptr<Node> ptr = _head->next;
		for (int i = 0; i < 20; ++i) {
			if (ptr == _tail) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}

	void Init() {
		_head->next = _tail;
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