#include <vector>
#include <cstdint>
#include <iostream>
#include "BitReadingFuncs.h"

class Node{
public:
	Node(): symbol(0), left(nullptr), right(nullptr) {}
	Node(int s): symbol(s), left(nullptr), right(nullptr) {}
	int symbol;
	Node* left;
	Node* right;
};

//right is 1, left is 0 ... read code left to right (most significant to least significant) ... opposite of how BitReader reads things
class HuffmanTree{
public:
	HuffmanTree(): root(new Node){}
	~HuffmanTree() { destroy(root); }
	int get_symbol(int code, int codeLength) const; //gets the symbol stored at the given code
	void insert(int code, int codeLength, int symbol); //inserts symbol at code
	int get_next_symbol(BitReader &bitstream) const; //reads one symbol from the given bitstream 
	void construct_from_codelength(std::vector<uint8_t> &lengths); //constructs the tree based on the lengths vector, with the alphabet ranging from 0-alphabet_top
	void construct_literal_length_fixed(); //constructs the tree based on the fixed data specified by DEFLATE
	void construct_distance_fixed(); //constructs the tree based on the fixed data specified by DEFLATE
private:
	Node* root;
	void destroy(Node* ptr);
};