#include "HuffmanTree.h"

void HuffmanTree::destroy(Node* ptr){
	if (ptr == nullptr) return;
	destroy(ptr->left);
	destroy(ptr->right);
	delete ptr;
}

//do i even need this???
int HuffmanTree::get_symbol(int code, int codeLength) const{//gets the symbol stored at the given code 
	Node* curr = root;
	while(codeLength > 0){
		if((code >> codeLength-1)&1){ //if the current bit is 1
			if (curr->right == nullptr) curr->right = new Node;
			curr = curr->right;
		}
		else{ //if the current bit is 0
			if (curr->left == nullptr) curr->left = new Node;
			curr = curr->left;
		}
		codeLength--;
	}
	return curr->symbol;
}

//right is 1, left is 0
//will use the rightmost codeLentgth number of bits in the code integer
//will start from the most significant (leftmost) to the least significant (rightmost)
void HuffmanTree::insert(int code, int codeLength, int symbol){ //inserts symbol at code 
	Node* curr = root;
	while(codeLength > 0){
		if((code >> codeLength-1)&1){ //if the current bit is 1
			if (curr->right == nullptr) curr->right = new Node;
			curr = curr->right;
		}
		else{ //if the current bit is 0
			if (curr->left == nullptr) curr->left = new Node;
			curr = curr->left;
		}
		codeLength--;
	}
	curr->symbol = symbol;
}

//reads one symbol from the given bitstream 
//bitstream MUST be in flip mode before passing into this function ... tree needs to read codes from left to right
int HuffmanTree::get_next_symbol(BitReader &bitstream) const{
	Node* curr = root;
	while(curr->left || curr->right){ //while not at a leaf node containing a symbol
		int bit = bitstream.read_bit();
		//std::cout << bit << " " << std::endl;
		if (bit) {
			curr = curr->right; //if the next bit is a 1, go right
		}
		else{
			curr = curr->left;
		}
	}
	return curr->symbol;
} 

//constructs the tree based on the fixed data specified by DEFLATE
//eg 0-143 coded w 8 bits from 00110000, 144-255 coded w 9 bits 110010000 thru 111111111, 256-279 coded w 7 bits 000000 thru 0010111, 280-287 coded w 8 bits 11000000 thru 11000111
void HuffmanTree::construct_literal_length_fixed(){
	std::vector<int> next_codes = {0,0,0,0,0,0,0,0,48,400}; //7 starts at 0, 8 starts at 00110000 == 48, 9 starts ta 110010000 == 400
	int bitlength;
	//insert 0-287 into tree
	for(unsigned int i = 0; i < 288; i++){
		if (i <= 143 || i >= 280) bitlength = 8; //code should be 8 bits long
		else if (i >= 144 && i <= 255) bitlength = 9;//code should be 9 bits long
		else bitlength = 7; //code should be 7 bits long

		insert(next_codes[bitlength], bitlength, i);
		//std::cout << "Symbol = " << i << " Code = " << (int)next_codes[bitlength] << " Bitlength = " << (int)bitlength << std::endl;
		next_codes[bitlength] += 1;
	}
}

//constructs the tree based on the fixed data specified by DEFLATE
//MIGHT NOT NEED THIS .... THEY'RE JUST EQUAL TO THEMSELVES
void HuffmanTree::construct_distance_fixed(){
	int current_code = 0; //only one length (5 bits)
	for(unsigned int i = 0; i < 32; i++){
		insert(current_code, 5, i);
		//std::cout << "Symbol = " << i << " Code = " << current_code << " Bitlength = " << 5 << std::endl;
		current_code++;
	}
}

void HuffmanTree::construct_from_codelength(std::vector<uint8_t> &lengths){
//constructs the tree based on the lengths vector, with the alphabet ranging from 0-lengths.size()-1
	std::vector<int> length_count(lengths.size()); //count how many symbols have the length at a given index
	for(unsigned int i = 0; i < length_count.size(); ++i){
		int count = 0;
		for(unsigned int a = 0; a < lengths.size(); a++){ //search in lengths for how many codes with i length
			if (lengths[a] == i) count ++;
		}
		length_count[i] = count;
	}
	std::vector<int> next_codes = {0,0}; //index is the length, num stored there is the next (smallest) code w that length
	for(unsigned int i = 2; i < lengths.size(); i++){ //make a vector of the smallest code for each bit length
		next_codes.push_back((next_codes[i-1] + length_count[i-1]) << 1); //largest code of the next smallest length, shifted left
	}
	//insert things into the tree 
	for(unsigned int i = 0; i < lengths.size(); i++){
		if (lengths[i] == 0) continue; //can't have a bit length of zero
		insert(next_codes[lengths[i]], lengths[i],i);
		//std::cout << "Symbol = " << i << " Code = " << next_codes[lengths[i]] << " Bitlength = " << (int)lengths[i] << std::endl;
		next_codes[lengths[i]] += 1;
	}
}