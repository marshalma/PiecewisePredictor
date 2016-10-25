//=============================================================================//
// This is a predictor implemented using the specifications discribed in the paper
// "Piecewise Linear Branch Predictor". There is also a customized queue class to
// store addresses which are used by the algorithm, which avoids using stl queue
// and may saves the computational overhead caused by stl queue.
// To limit the space of the 3-dimension matrix W, I chose to do mod operation on 
// first two indices using M and N.

class my_update_piece : public branch_update {
private:
	int output; // saves the last total weights, which will be needed in traning method
public:
	int get_output() {return this->output;} // getter of output
	void set_output(int arg) {this->output = arg;} // setter of output
};

// Customized queue to store address. The length is fixed. Serve as GA in the paper.
class AddressQueue {
	unsigned int* queue;
	int front;
	int back;
	int _size;
	const int length; // max length/capacity of the queue, which is fixed

public:
	AddressQueue(int n) : length(n), front(0), back(0), _size(0) {
		this->queue = new unsigned int[n];
	}

	void push_back(unsigned int x) {
		if (_size == length) pop_front();
		queue[back] = x;
		back = (back+1) % length;
		_size++;
	}

	unsigned int size() {
		return this->_size;
	}

	unsigned int pop_front() {
		if (_size == 0) return 0;
		int res = queue[front];
		front = (front + 1) % length;
		_size--;
		return res;
	}

	unsigned int operator[](unsigned int index) {
		// [0] is the element at the back of the queue
		if (index >= _size) return 0;
		int modulo_index = (back - (int)index - 1 + length) % length;
		return queue[modulo_index];
	}

	~AddressQueue() {
		delete[] queue;
	}
};


// Branch predictor from paper "Piecewise Linear Branch Prediction"
// Derived from abstract class branch_predictor
// ****************************************************************
// Variables that takes up space: W & GA
// W is a three-dimension matrix. 
// Each of the element is char typed which takes 1 byte. 
// Total space for W is M*N*(H+1) bytes.
// ****************************************************************
// GA is a queue(implemented using array) with max length H. 
// Each element is unsigned which takes 4 bytes. 
// Total 4*H bytes.
// ****************************************************************
// Other variables only takes constant space, no need to count them.
// Space: M*N*(H+1) + 4*H bytes 
class Piecewise : public branch_predictor {
#define M 256
#define N 1
#define H 32
#define THETA 2.14 * (H+1) + 20.58

private:
	char W[N][M][H+1];
	unsigned long long GHR = 0;
	AddressQueue GA;
	my_update_piece u;
	branch_info bi;

public:
	Piecewise(void): GA(H) {
		memset(W, 0, sizeof(W));
	}

	branch_update* predict(branch_info &b) {
		int address_modn = b.address % N;
		int res = W[address_modn][0][0];

		bi = b;
		if (bi.br_flags & BR_CONDITIONAL) { 
			// If the branch is conditional, it should be investigated further. 
			// Otherwise, the branch should always be taken.
			// The following for loop sums the weights.
			for (int i = 0; i < GA.size(); i++) {
				int second_index = GA[i] % M;
				char thisweight = W[address_modn][second_index][i];
				res += (GHR & ((unsigned long long)1 << (i-1)))?(int)thisweight:-(int)thisweight;
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
		
		// update bias
		if (abs(((my_update_piece*)u)->get_output()) < THETA || taken != u->direction_prediction()) {
			// using saturating arithmetic
			if (taken && W[address_modn][0][0] < 127) 
				W[address_modn][0][0]++;
			if (!taken && W[address_modn][0][0] > -127) 
				W[address_modn][0][0]--;
		}
		
		// update weights other than bias
		for (int i = 0; i < GA.size(); i++) {
			int second_index = GA[i] % M;
			// using saturating arithmetic
			if ((((GHR & ((unsigned long long)1 << (i-1))) > 0) == taken) && W[address_modn][second_index][i] < 127) 
				W[address_modn][second_index][i]++;
			if (!(((GHR & ((unsigned long long)1 << (i-1))) > 0) == taken) && W[address_modn][second_index][i] > -127) 
				W[address_modn][second_index][i]--;
		}
		
		// update GA
		GA.push_back(bi.address);
		
		// update GHR
		GHR <<= 1; // shifting GHR to make up a slot for current "taken"
		GHR |= taken; // pushing current "taken" into GHR
		GHR &= ((unsigned long long)1 << H) - 1; // masking GHR with 11..1 of length H
	}
};
