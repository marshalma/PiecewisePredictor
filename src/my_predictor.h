// my_predictor.h
// This file contains a sample my_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.
// Note that this predictor doesn't use the whole 8 kilobytes available
// for the CBP-2 contest; it is just an example.

class my_update : public branch_update {
public:
	unsigned int index;
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

class Piecewise : public branch_predictor {
#define M 32
#define N 32
#define H 20
#define THETA 2.14 * (H+1) + 20.58

private:
	char W[N][M][H+1];
	unsigned int GHR = 0;
	AddressQueue GA;
	my_update u;
	branch_info bi;
	
public:
	Piecewise(void): GA(H) {
		memset(W, 0, sizeof(W));
	}
	
	branch_update* predict(branch_info &b) {
		int i = b.address % N, j = b.address % M;	
		int res = W[b.address][0][0];
		
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) { // if the branch is conditional, it should be investigated further. Otherwise, it should always be taken.
			for (int i = 1; i <= H; i++) {
				if (GHR & (1 << i-1) > 0) res += W[b.address][GA[i]][i];
				else res -= W[b.address][GA[i]][i];
			}
			u.direction_prediction(res>0);
		} 
		else {
			u.direction_prediction (true);
		}
		u.target_prediction (0);
		return &u;
	}	
	
	void update(branch_update* u, bool taken, unsigned int target) {
		
	}
};
