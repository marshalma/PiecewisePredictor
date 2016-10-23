// my_predictor.h
// This file contains a sample my_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.
// Note that this predictor doesn't use the whole 8 kilobytes available
// for the CBP-2 contest; it is just an example.
#include <iostream>
#include <deque>

using namespace std;

class my_update : public branch_update {
public:
	unsigned int index;
};

class my_update_piece : public branch_update {
public:
	int output;
	void set_output(int arg) {this->output = arg;}
};

class my_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH	15
#define TABLE_BITS	15
	my_update u;
	branch_info bi;
	unsigned int history;
	unsigned char tab[1<<TABLE_BITS];

	my_predictor (void) : history(0) {
		memset (tab, 0, sizeof (tab));
	}

	branch_update *predict (branch_info & b) {
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) { // if the branch is conditional, it should be investigated further. Otherwise, it should always be taken.
			u.index =
				  (history << (TABLE_BITS - HISTORY_LENGTH))
				^ (b.address & ((1<<TABLE_BITS)-1));
			u.direction_prediction (tab[u.index] >> 1); // if tab[u.index] > 1 then predict it as taken, otherwise not taken.
		} else {
			u.direction_prediction (true);
		}
		u.target_prediction (0);
		return &u;
	}

	void update (branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			unsigned char *c = &tab[((my_update*)u)->index];
			// this may arise confusion about function variable u and class property u. Actually, I think there's no need for this method to take in an addition u, as update should be called everytime after predict is called. Test result shows that making u into this->u does not change the result.
			if (taken) {
				if (*c < 3) (*c)++;
			} else {
				if (*c > 0) (*c)--;
			}
			history <<= 1;
			history |= taken; // mask last bit with taken
			history &= (1<<HISTORY_LENGTH)-1; // mask history with 0b11111...1
		}
	}
};


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
		if (size == 0) return 0;
		int res = queue[front];
		front = (front + 1) % length;
		size--;
		return res;
	}

	unsigned int operator[](unsigned int index) {
		// [0] is the element at the back of the queue
		if (index >= size) return 0;
		int modulo_index = (back - (int)index - 1 + length) % length;
		return queue[modulo_index];
	}

	~AddressQueue() {
		delete[] queue;
	}
};



class Piecewise : public branch_predictor {
#define M 128
#define N 4
#define H 19
#define THETA 2.14 * (H+1) + 20.58

private:
	char W[N][M][H+1];
	unsigned int GHR = 0;
	deque<unsigned int> GA;
	my_update_piece u;
	branch_info bi;

public:
	Piecewise(void) {
		memset(W, 0, sizeof(W));
	}

	branch_update* predict(branch_info &b) {
		int address_modn = b.address % N;
		int res = W[address_modn][0][0];

		bi = b;
		if (bi.br_flags & BR_CONDITIONAL) { // if the branch is conditional, it should be investigated further. Otherwise, it should always be taken.
			for (int i = 0; i < GA.size(); i++) {
				int second_index = *(GA.rbegin()+i) % M;
				char thisweight = W[address_modn][second_index][i];
				int intweight = (GHR & (1 << (i-1)))?(int)thisweight:-(int)thisweight;
				res += intweight;
			}
			u.set_output(res);
			u.direction_prediction(res>=0);
		}
		else {
			u.direction_prediction (true);
		}
		u.target_prediction (0);
		return &u;
	}

	void update(branch_update* u, bool taken, unsigned int target) {
		if (!(bi.br_flags & BR_CONDITIONAL)) return;
		int address_modn = bi.address % N;
		if (abs(((my_update_piece*)u)->output) < THETA || taken != u->direction_prediction()) {
			if (taken && W[address_modn][0][0] < 127) W[address_modn][0][0]++;
			if (!taken && W[address_modn][0][0] > -127) W[address_modn][0][0]--;
		}
		for (int i = 0; i < GA.size(); i++) {
			int second_index = *(GA.rbegin()+i) % M;
			if ((((GHR & (1 << (i-1))) > 0) == taken) && W[address_modn][second_index][i] < 127) W[address_modn][second_index][i]++;
			if (!(((GHR & (1 << (i-1))) > 0) == taken) && W[address_modn][second_index][i] > -127) W[address_modn][second_index][i]--;
		}
		GA.push_back(bi.address);
		if (GA.size() > H) GA.pop_front();
		GHR <<= 1;
		GHR |= taken;
		GHR &= (1 << H) - 1;
	}
};
