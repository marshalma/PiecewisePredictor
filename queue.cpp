#include <iostream>

class AddressQueue {
	public:
		unsigned int* queue;
		int front;
		int back;
		int size;
		const int length;
		AddressQueue(int n) : length(n), front(0), back(0), size(0) {
			this->queue = new unsigned int[n];
		}
		
		void push(unsigned int x) {
			if (size == length) pop();
			queue[back] = x;
			back = (back+1) % length;
			size++;
		}
		
		unsigned int qsize() {
			return this->size;
		}
		
		unsigned int pop() {
			if (size == 0) return -1;
			int res = queue[front];
			front = (front + 1) % length;
			size--;
			return res;
		}
		
		~AddressQueue() {
			delete[] queue;
		}
	};

using namespace std;
int main(int argc, char *argv[]) {
	AddressQueue q(5);
	q.push(1);
//	q.push(2);
//	q.push(3);
//	q.push(4);
//	q.push(5);
//	q.push(5);
//	q.push(6);
//	q.push(6);
	cout << q.pop() << endl;
	cout << q.pop() << endl;
	cout << q.pop() << endl;
	return 0;
}