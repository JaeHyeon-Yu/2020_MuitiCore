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

constexpr unsigned char ST_EMPTY = 0;
constexpr unsigned char ST_WAITING = 1;
constexpr unsigned char ST_BUSY = 2;
constexpr int EMPTY_VALUE = 0;

class Exchanger {
	atomic_int slot;
	unsigned char get_state() {
		int t = slot;
		return (t >> 30) & 0x3;
	}
	int get_value() {
		int t = slot;
		return t & 0x7FFFFFFF;
	}

	bool SLOT_CAS(int old_state, int new_state, int old_value, int new_value) {
		int old_v = old_state << 30 + old_value;
		int new_v = new_state << 30 + new_value;
		return atomic_compare_exchange_strong(&slot, &old_v, new_v);
	}

public:
	Exchanger() {
		slot = 0;
	}

	int Excange(int value, bool* time_out, bool* busy) {
		for (int i = 0; i < 1000; ++i) {
			unsigned char state = get_state();
			switch (state) {
			case ST_EMPTY:
				if (SLOT_CAS(ST_EMPTY, ST_WAITING, 0, value)) {
					int cnt = 0;
					while (get_state() != ST_BUSY) {
						cnt++;
						if (cnt > 1000) {
							*time_out = true;
							return 0;
						}
					}
					int ret = get_value();
					slot = EMPTY_VALUE;
					return ret;
				}
				else continue;
				break;
			case ST_WAITING: {
				int old_value = get_value();
				if (SLOT_CAS(ST_WAITING, ST_BUSY, old_value, value))
					return old_value;
				else continue;
			}break;
			case ST_BUSY:
				*busy = true;
				break;
			}
		}
		*busy = true;
		return 0;
	}
};

class ELBackOff {
	int range;
	Exchanger exchanger[MAX_THREAD];
public:
	ELBackOff() { range = 1; }
	~ELBackOff() = default;
	
	void Init() { range = 1; }

	int Visit(int value, bool* time_out) {
		int slot = rand() % range;
		bool busy;
		int ret = exchanger[slot].Excange(value, time_out, &busy);
		if (*time_out && range > 1) range--;
		if (busy && range <= num_threads / 2) range++;
		return ret;
	}
};

class LFELStack {
	Node* volatile top;
	ELBackOff el;
public:
	LFELStack() {
		el.Init();
		top = nullptr;
	}
	~LFELStack() {
		Clear();
	}

	void Clear() {
		el.Init();
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
			bool time_out;
			int ret = el.Visit(key, &time_out);
			if (!time_out && ret == 0) return;
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
			bool time_out;
			int ret = el.Visit(0, &time_out);
			if (!time_out && ret != 0) return ret;
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

LFELStack cque;

void ThreadFunc(int num_thread) {
	int key;
	num_threads = num_thread;
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

