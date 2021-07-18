#include <cstdint>
#include <vector>

#ifndef BITREADINGFUNCS_H_INCLUDED
#define BITREADINGFUNCS_H_INCLUDED

class BitReader{
public:
	BitReader(std::vector<uint8_t> &data) : bytes(data), index(0), bitPos(0){}
	int read_bit(); //read a bit from the bytestream, calling consecutively will read bits right to left
	unsigned int read_bits(int n); //read the n bits from the stream & combine into unsigned integer, flip from little endian to big endian (leftmost bit will go the right side on final unsigned integer)
	unsigned int read_bytes(int n); //reads the n bytes from the stream & combines them into unsigned integer
	//NOT NEEDED?? void turn_on_flip_mode(){ flip_mode = true; }
	//NOT NEEDED?? void turn_off_flip_mode(){ flip_mode = false; }
	const std::vector<uint8_t>& get_data(){ return bytes; };

private:
	std::vector<uint8_t> &bytes; //reference to vector of the bytes
	int index;
	int bitPos;
 
};

//combine 4 bytes into one 32 bit number, with leftmost byte parameter as the most significant byte
void combine_bytes(uint8_t most_significant, uint8_t two, uint8_t three, uint8_t least_significant, uint32_t &output);
//split the integer into 4 bytes and put them in the char array, from least significant to most signifcant (little endian just like bmp header)
void split_into_four_bytes(char* arr, int least_significant_index, int32_t integer);
void split_into_two_bytes(char* arr, int least_significant_index, uint16_t integer);

#endif
