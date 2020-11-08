// 비멈춤 동기화 (Lock-Free

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

class LFNode;

class HAP {
	atomic_int value;
public:
	HAP() {
		value = 0;
	}
	~HAP() = default;

	void SetNext(LFNode* addr, bool isRemoved) {
		// 주소와 removed가 한번에 업데이트되기 위하여 중간변수 하나에 뒀다가 한번에 바꾼다.
		int v = reinterpret_cast<int>(addr);
		if (isRemoved) v = v | 1;
		value = v;
	}
	LFNode* GetNext() {
		return reinterpret_cast<LFNode*>(value & 0xFFFFFFFE);
	}
	LFNode* GetNext(bool* isRemoved) {
		int v = value;
		*isRemoved = (1 == (v & 1));
		return reinterpret_cast<LFNode*>(v & 0xFFFFFFFE);
	}

	bool CAS(LFNode* old_addr, LFNode* new_addr, bool old_mark, bool new_mark) {
		int old_v = reinterpret_cast<int>(old_addr);
		if (old_mark) old_v = old_v | 1;
		int new_v = reinterpret_cast<int>(new_addr);
		if (new_mark) new_v = new_v | 1;
		return atomic_compare_exchange_strong(&value, &old_v, new_v);
	}
};

class LFNode
{
public:
	int key = 0;
	HAP next;

	LFNode() = default;
	LFNode(int key_value) : key(key_value) {

	}

	~LFNode() {}
};

class CList
{
	LFNode _head;
	LFNode _tail;

public:
	CList()
	{
		_head.key = 0x80000000;
		_tail.key = 0xFFFFFFFF;
		_head.next.SetNext(&_tail, false);
	}
	~CList() {
		Init();
	}

	bool Add(int key)
	{
		LFNode* pred, * curr;
		while (true) {
			Find(key, pred, curr);

			if ( key == curr->key)
				return false;

			else {
				LFNode* node = new LFNode(key);
				node->next.SetNext(curr, false);
				if (pred->next.CAS(curr, node, false, false))
					return true;
			}
			
		}
	}

	bool Remove(int key) {
		bool is_removed;
		LFNode* pred, * curr;
		while (true) {
			Find(key, pred, curr);
			if (key != curr->key)
				return false;
			else {
				auto node = curr->next.GetNext();
				curr->next.GetNext(&is_removed);
				if (!is_removed) continue;
				pred->next.CAS(curr, node, false, false);
				return true;
			}
		}
	}

	bool Contains(int key) {
		bool marks = false;
		LFNode* curr = &_head;
		while (curr->key < key) {
			curr = curr->next.GetNext();
			auto succ = curr->next.GetNext(&marks);
		}
		return (curr->key == key && !marks);
	}

	void Display20() {
		LFNode* ptr = _head.next.GetNext();
		for (int i = 0; i < 20; ++i) {
			if (ptr == &_tail) break;
			cout << ptr->key << ", ";
			ptr = ptr->next.GetNext();
		}
		cout << endl;
	}

	void Init() {
		LFNode* ptr = _head.next.GetNext();
		while (_head.next.GetNext() != &_tail)
		{
			ptr = _head.next.GetNext();
			_head.next.SetNext(ptr->next.GetNext(), false);
			delete ptr;
		}
	}

	void Find(int key, LFNode*& pred, LFNode*& curr) {
		retry:
		pred = &_head;
		curr = pred->next.GetNext();
		while (true) {
			// 쓰레기 제거
			bool is_removed;
			LFNode *succ = curr->next.GetNext(&is_removed);
			while (is_removed) {
				if (!pred->next.CAS(curr, succ, false, false)) 
					goto retry;
				curr = succ;
				succ = curr->next.GetNext(&is_removed);
				
			}
			if (curr->key >= key) return;
			pred = curr;
			curr = curr->next.GetNext();
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

