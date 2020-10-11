#include <iostream>
#include <thread>
using namespace std;

volatile bool flag = false;
int g_data = 0;

void Receiver() {
	while (flag == false);
	int my_data = g_data;
	cout << "receive " << my_data << endl;
}
void Sender() {
	g_data = 999;
	flag = true;
}

int main() {
	thread r{ Receiver };
	thread s{ Sender };
	r.join();
	s.join();
}