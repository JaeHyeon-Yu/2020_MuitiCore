#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include <set>

using namespace std;
using namespace chrono;

constexpr int NUM_TEST = 4000;
constexpr int KEY_RANGE = 1000;
constexpr int MAX_THREAD = 8;

thread_local int thread_id;

class STL_SET {
	set<int> m_set;
	mutex mu;

public:
	STL_SET() = default;
	~STL_SET() = default;

	void Init() {
		m_set.clear();
	}

	bool Add(int key) {
		lock_guard<mutex> L(mu);
		if (m_set.count(key) != 0) return false;
		m_set.insert(key);
		return true;
	}

	bool Remove(int key) {
		lock_guard<mutex> L(mu);
		if (m_set.count(key) != 0) return false;
		m_set.erase(key);
	}

	bool Contains(int key) {
		return m_set.count(key) != 0;
	}

	void Display20() {
		int cnt = 0;
		for (auto n : m_set) {
			if (cnt++ > 20) break;
			cout << n << ", ";
		}
		cout << endl;
	}
};

constexpr int ADD = 0;
constexpr int REMOVE = 1;
constexpr int CONTAINS = 2;
constexpr int CLEAR = 3;
constexpr int DISPLAY20 = 4;

class UNODE;

class Invocation {
public:
	int method;
	int param;
};

struct Response {
	bool ret;
};

class SeqObject_Set {
	set<int> m_set;
public:
	Response apply(const Invocation& inv) {
		switch (inv.method) {
		case ADD:
			if (m_set.count(inv.param) != 0) return Response{ false };
			m_set.insert(inv.param);
			return Response{ true };
			break;
		case REMOVE:
			if (m_set.count(inv.param) != 0) return Response{ false };
			m_set.erase(inv.param);
			return Response{ true };
		case CONTAINS:
			return Response{ m_set.count(inv.param) != 0 };
		case CLEAR:
			m_set.clear();
			return Response();
		case DISPLAY20:
			int cnt = 0;
			for (auto n : m_set) {
				if (cnt++ > 20) break;
				cout << n << ", ";
			}
			cout << endl;
			break;
		}
		return Response();

	}
};

class SEQ_SET {
	SeqObject_Set m_set;
public:
	SEQ_SET() = default;
	~SEQ_SET() = default;

	void Init(){
		m_set.apply(Invocation{ CLEAR, 0 });
	}
	bool Add(int key) {
		return m_set.apply(Invocation{ ADD, key }).ret;
	}
	bool Remove(int key) {
		return m_set.apply(Invocation{ REMOVE, key }).ret;
	}
	bool Contains(int key) {
		return m_set.apply(Invocation{ CONTAINS, key }).ret;
	}
	bool Display20() {
		return m_set.apply(Invocation{ DISPLAY20, NULL }).ret;
	}
};

class ConsensusNext {
	atomic_int cnext;

public:
	ConsensusNext() { cnext = 0; }
	~ConsensusNext() = default;
	UNODE* decide(UNODE* next) {
		int nv = reinterpret_cast<int>(next);
		int ov = 0;
		if (atomic_compare_exchange_strong(&cnext, &ov, nv) == true)
			return next;
		return reinterpret_cast<UNODE*>(ov);
	}
};

class UNODE {
public:
	int seq;
	UNODE* next;
	Invocation inv;
	ConsensusNext decidenext;

	UNODE(const Invocation& pinv) {
		inv = pinv;
		next = nullptr;
		seq = 0;
	}
};


class LFUniversalSET {
	UNODE* tail;
	UNODE* head[MAX_THREAD];
public:
	LFUniversalSET() {
		tail = new UNODE(Invocation());
		tail->seq = 1;
		for (auto& h : head) h = tail;
	}
	UNODE* get_last_node(){
		UNODE* p = head[0];
		for (auto h : head)
			if (h->seq > p->seq)
				p = h;
		return p;
	}
	Response apply(const Invocation& inv) {
		UNODE* prefer = new UNODE(inv);
		while (prefer->seq ==0){
			UNODE* before = get_last_node();
			UNODE* after = before->decidenext.decide(prefer);
			before->next = after;
			after->seq = before->seq + 1;
			head[thread_id] = after;
		}

		SeqObject_Set l_set;
		UNODE* exec = tail->next;
		while (exec!=prefer) {
			l_set.apply(exec->inv);
			exec = exec->next;
			if (exec->inv.method == DISPLAY20) break;
		}
		return l_set.apply(inv);
	}
};

class UN_SET {
	LFUniversalSET m_set;
public:
	UN_SET() = default;
	~UN_SET() = default;

	void Init() {
		m_set.apply(Invocation{ CLEAR, 0 });
	}
	bool Add(int key) {
		return m_set.apply(Invocation{ ADD, key }).ret;
	}
	bool Remove(int key) {
		return m_set.apply(Invocation{ REMOVE, key }).ret;
	}
	bool Contains(int key) {
		return m_set.apply(Invocation{ CONTAINS, key }).ret;
	}
	bool Display20() {
		return m_set.apply(Invocation{ DISPLAY20, NULL }).ret;
	}
};

UN_SET clist;

void ThreadFunc(int num_thread, int t_id)
{
	int key;
	thread_id = t_id;

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
			ths.emplace_back(ThreadFunc, num, i);

		for (auto& th : ths) th.join();
		auto e_t = high_resolution_clock::now();
		auto exec_t = e_t - s_t;
		cout << num << " Thread, " << ", ";
		cout << "Exec Time = " << duration_cast<milliseconds>(exec_t).count() << "ms, " << endl;
		clist.Display20();

	}
}

