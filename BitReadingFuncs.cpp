#include "BitReadingFuncs.h"

//combine 4 bytes into one 32 bit number, with leftmost byte parameter as most significant 
void combine_bytes(uint8_t most_significant, uint8_t two, uint8_t three, uint8_t least_significant, uint32_t &output){
	output = 0;
	output |= (uint32_t)most_significant << 24;
	output |= (uint32_t)two << 16;
	output |= (uint32_t)three << 8;
	output |= (uint32_t)least_significant;
}

//split the integer into 4 bytes and put them in the array starting at least_signicant_index and ending 4 spots later (little endian, just like bmp header)
//takes in a SIGNED integer like the bmp requires that width/height/others are 
void split_into_four_bytes(char* arr, int least_significant_index, int32_t integer){
	arr[least_significant_index] = (char)(integer);
	arr[least_significant_index+1] = (char)(integer >>  8);
	arr[least_significant_index+2] = (char) (integer >> 16);
	arr[least_significant_index+3] = (char)(integer >> 24); //most significant byte goes last
}

//split integer into 2 bytes and put them in the parameters from least signficiant to most significant
void split_into_two_bytes(char* arr, int least_significant_index, uint16_t integer){
	arr[least_significant_index] = (char)(unsigned char)(integer);
	arr[least_significant_index+1] = (char)(unsigned char) (integer >> 8); //most significant byte goes last
}


//read the rightmost bit and shift the byte so that calling read_bit 8 times will read bytes right to left
int BitReader::read_bit(){
	//if (flip_mode && bitPos == 0) //if it's in flip mode, reverse this byte before reading from it
	//	reverse_byte(index);
	
	bitPos++;
	int tmp = bytes[index] & 1;
	bytes[index] = bytes[index] >> 1;
	if (bitPos == 8){
		index++;
		bitPos = 0;	
	}
	return tmp;
}

//first bit read from stream is least significant bit, last bit is the most significant
//have to put the bits in big endian order (so first bit read from stream is rightmost bit in final integer)
unsigned int BitReader::read_bits(int n){
	unsigned int answer = 0;
	for(unsigned int i = 0; i < n; i++){
		answer |= read_bit() << i;
	}
	return answer;
}

//reads the next n bytes from the stream & combines them into one unsigned integer
unsigned int BitReader::read_bytes(int n){
	//if in the middle of a byte, discard any bits left in this byte & move to the next byte
	while(bitPos != 0){
		read_bit();
	}
	return read_bits(8*n); //read all the bits in the bytes

}