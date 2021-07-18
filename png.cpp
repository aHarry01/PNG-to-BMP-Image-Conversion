#include <iostream>
#include <fstream>
#include <string>
#include <cmath>

#include "Image.h"
#include "BitReadingFuncs.h"
#include "HuffmanTree.h"

void print_raw_data(const std::vector<uint8_t> &png_data){
	for(unsigned int i = 0; i < png_data.size(); i++){
		std::cout << (int)png_data[i] << " ";
	}
	std::cout << std::endl;
}

void print_raw_data(const std::vector<uint8_t> &png_data, int num){
	for(unsigned int i = 0; i < (num*30); i++){
		std::cout << (int)png_data[i] << " ";
	}
	std::cout << std::endl;
}


//PNG DECODING FUNCTIONS START:

//make sure that header is standard PNG header 137 80 78 71 13 10 26 10
bool check_header(std::ifstream &file){
	//read header information
	char header[8];
	file.seekg (0, std::ios::beg);
	file.read(header, 8);
	return ((int)(unsigned char)header[0] == 137 && (int)header[1] == 80 && (int)(header[2] == 78 && (int)header[3] == 71) 
			&& (int)header[4] == 13 && (int)header[5] == 10 && (int)header[6] == 26 && (int)header[7]==10);
}

//process the beginning chunk (code IHDR). i == index of the start of the data in png_data
//set width & height of img
void process_IHDR(int i, int length, const std::vector<uint8_t> &png_data, unsigned int &width, unsigned int &height, int &bytes_per_pixel){
	combine_bytes(png_data[i], png_data[i+1], png_data[i+2], png_data[i+3], width);
	combine_bytes(png_data[i+4], png_data[i+5], png_data[i+6], png_data[i+7], height);
	std::cout << "  INFO: " << width << " x " << height << std::endl;
	if (png_data[i+9] == 2) { bytes_per_pixel = 3; std::cout << "  RGB" << std::endl;}
	if (png_data[i+9] == 6) { bytes_per_pixel = 4;std::cout << "  RGBA - ANY TRANSPARENCY WILL BE LOST!" << std::endl;}

	if (png_data[i+8] != 8) //currently only supports PNGs with a bit depth of 8
		throw std::string("ERROR: only supports bit depth = 8, current bit depth = ") + std::to_string(int(png_data[i+8]));
	if (png_data[i+9] != 2 && png_data[i+9] != 6) //currently only supports truecolor (RGB) mode
		throw std::string("ERROR: only supports RGB and RGBA, current color type = ") + std::to_string(int(png_data[i+9]));
	if (png_data[i+10] != 0) //compression method must be 0
		throw std::string("ERROR: compression method must be 0");
	if (png_data[i+11] != 0) //filter method must be 0
		throw std::string("ERROR: filter method must be 0");
	if (png_data[i+12] != 0) //currently doesn't support interlace
		throw std::string("ERROR: doesn't support interlace");
	//AT SOME POINT SHOULD CHECK CRC DATA!!!
}

//copies data inside this IDAT block to IDAT_data
void add_to_IDAT(int i, int length, const std::vector<uint8_t> &png_data, std::vector<uint8_t> &IDAT_data){
	for(unsigned int a = i; a < i+length; a++){
		IDAT_data.push_back(png_data[a]);
	}
}

//process the file chunk by chunk, concatenates all IDAT chunk's data and stores it in IDAT_data
void processChunks(const std::vector<uint8_t> &png_data, std::vector<uint8_t> &IDAT_data, unsigned int& width, unsigned int& height, int &bytes_per_pixel){
	int i = 0;
	uint32_t length = 0;
	std::string chunk_code = "    ";
	while(i < png_data.size()){
		//get the length of this chunk
		combine_bytes(png_data[i], png_data[i+1], png_data[i+2], png_data[i+3], length);
		std::cout << length << " ";

		for(int pos = 0; pos < 4; pos++) chunk_code[pos] = png_data[i+pos+4];
		std::cout << chunk_code << std::endl;

		if (chunk_code == "IHDR") process_IHDR(i+8, length, png_data, width, height, bytes_per_pixel);
		else if (chunk_code == "IEND") return;
		else if (chunk_code == "IDAT") add_to_IDAT(i+8, length, png_data, IDAT_data);
		else if (chunk_code == "bKGD"){
			for(unsigned int a = i; a < i+length; a++)
				std::cout << (int)png_data[a] << " ";
			std::cout << std::endl;
		}

		i += 12 + length ; //4 length bytes, 4 chunk code bytes, 4 CRC bytes, length data bytes
	}
}

//process the zlib header - 2 bytes
//returns the window size used in the compression
unsigned int process_zlib_header(BitReader &compressed){
	int CMF = (int)compressed.get_data()[0];
	int FLG = (int)compressed.get_data()[1];
	if ((CMF * 256 + FLG) % 31 != 0) throw std::string("ERROR while decompressing data: CMF and FLG checksum failed");

	if (compressed.read_bits(4) != 8) throw std::string("ERROR while decompressing data: invalid compression method"); //only compression method allowed is 8
	int cinfo = compressed.read_bits(4);
	if (cinfo >= 8) throw std::string("ERROR while decompressing data: invalid window size"); //cinfo can't be over 7 
	compressed.read_bits(5); //this is FCHECK, already did te checksum in the first 3 lines of the function
	if (compressed.read_bits(1)) throw std::string("ERROR while decompressing data: doesn't support preset dictionary");
	compressed.read_bits(2); //this is FLEVEL, indicates how much it was compressed, not necessary for decompression
	return (unsigned int)pow(2, cinfo + 8);
}

//inflates data was wasn't compressed
void inflate_no_compression(){
	std::cout << "no compression on this block ... NOT IMPLEMENTED YET " << std::endl;
}

//inflates the given data when given the Huffman Trees already constructed
void inflate_block_data(HuffmanTree &literal_length_tree, HuffmanTree &dist_tree, BitReader &compressed_bitstream, std::vector<uint8_t> &decompressed_data){
	int value = literal_length_tree.get_next_symbol(compressed_bitstream);
	int LengthExtraBits[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
	int LengthBase[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
	int DistanceExtraBits[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
	int DistanceBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
	while(value != 256){ //because 256 is the end of the block
		if (value <= 255) //actual data block
			decompressed_data.push_back(value);
		else{ //length value, need to get distance and go back that much
			int length = LengthBase[value-257] + compressed_bitstream.read_bits(LengthExtraBits[value-257]);
			int dist_value = dist_tree.get_next_symbol(compressed_bitstream);
			int distance = DistanceBase[dist_value] + compressed_bitstream.read_bits(DistanceExtraBits[dist_value]);

			std::vector<int> copy_data;
			int index = decompressed_data.size() - distance;
			//copy the the data to the copy vector
			while(copy_data.size() < length && index < decompressed_data.size()){
				copy_data.push_back(decompressed_data[index]);
				index++;
			}
			if (copy_data.size() == length){//if the copy_data is the correct length we were supposed to copy
				decompressed_data.insert(decompressed_data.end(), copy_data.begin(), copy_data.end());
			}
			else{//if copy_data is shorter than length, append the data multiple times to reach length bytes
				int i = 0;
				while(length > 0){
					decompressed_data.push_back(copy_data[i]);
					i++;
					length--;
					if (i == copy_data.size()) i = 0; //wrap back to beginning
				}
			}
		}
		value = literal_length_tree.get_next_symbol(compressed_bitstream);
	}
}

//inflates data compressed with fixed Huffman codes
void inflate_fixed(BitReader &compressed_bitstream, std::vector<uint8_t> &decompressed_data){
	std::cout << "FIXED HUFFMAN TREES" << std::endl;
	HuffmanTree literal_length_tree;
	literal_length_tree.construct_literal_length_fixed();
	HuffmanTree dist_tree;
	dist_tree.construct_distance_fixed();
	inflate_block_data(literal_length_tree, dist_tree, compressed_bitstream, decompressed_data);

}

//inflates data compressed with dynamic Huffman codes
void inflate_dynamic(BitReader &compressed_bitstream, std::vector<uint8_t> &decompressed_data){
	std::cout << "DYNAMIC HUFFMAN TREES" << std::endl;
	int num_lit_codes = compressed_bitstream.read_bits(5) +257;
	int num_dist_codes = compressed_bitstream.read_bits(5) + 1;
	int num_code_length_codes = compressed_bitstream.read_bits(4) + 4;

	//read code lengths for the code length alphabet
	//alphabet for code lengths is 1-18 and code lengths are given in this order: CodeLengthCodesOrder = [16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15]
	std::vector<uint8_t> code_length_tree_code_lengths(19,0);
	std::vector<int> codeLengthsCodesOrder = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
	for(unsigned int i = 0; i < num_code_length_codes; ++i){
		code_length_tree_code_lengths[codeLengthsCodesOrder[i]] = compressed_bitstream.read_bits(3);
	}
	HuffmanTree code_lengths_tree;
	code_lengths_tree.construct_from_codelength(code_length_tree_code_lengths); //this is the code to decode the code lengths for the literal/length and distance trees

	//get codelengths for literal/length tree and dist tree
	std::vector<uint8_t> all_codelengths(num_lit_codes+num_dist_codes);
	for(int i = 0; i < num_lit_codes+num_dist_codes; i++){
		int symbol = code_lengths_tree.get_next_symbol(compressed_bitstream);
		if (symbol <= 15) all_codelengths[i] = symbol;
		else{ //repeat code a certain number of times
			uint8_t value = 0;
			int repeat = 0;
			if (symbol == 16){//repeat previous code 3-6 times, using next 2 bits to determine how many times to copy
				value = all_codelengths[i-1];
				repeat = compressed_bitstream.read_bits(2) + 3;
			}
			else if (symbol == 17) repeat = compressed_bitstream.read_bits(3) + 3; //repeat 0 3-10 times
			else if (symbol == 18) repeat = compressed_bitstream.read_bits(7) + 11; //repeat 0 11-138 times
			for (int count = 0; count < repeat; count++){
				all_codelengths[i+count] = value;
			}
			i += repeat - 1; 
		} 
	}
	//split vector into literal/length tree codelengths and distance tree codelengths
	std::vector<uint8_t> literal_length_codelengths(all_codelengths.begin(), all_codelengths.begin() + num_lit_codes);
	std::vector<uint8_t> distance_codelengths(all_codelengths.begin()+num_lit_codes, all_codelengths.end());

	//construct trees & call function to use the trees to deocde the actual data
	HuffmanTree literal_length_tree;
	literal_length_tree.construct_from_codelength(literal_length_codelengths);
	HuffmanTree dist_tree;
	dist_tree.construct_from_codelength(distance_codelengths);
	inflate_block_data(literal_length_tree, dist_tree, compressed_bitstream, decompressed_data);

}

//decompresses compressed_data and puts it in decompressed_data
void decompress(std::vector<uint8_t> &compressed_data, std::vector<uint8_t> &decompressed_data){
	BitReader compressed_bitstream(compressed_data);
	unsigned int window_size = process_zlib_header(compressed_bitstream);

	int b_final = 0; 
	int b_type;
	while (!b_final){
		b_final = compressed_bitstream.read_bit();
		b_type = compressed_bitstream.read_bits(2);
		if (b_type == 3) throw std::string("ERROR while decompressing data: invalid BTYPE");
		else if (b_type == 0) inflate_no_compression();
		else if (b_type == 1) inflate_fixed(compressed_bitstream, decompressed_data);
		else if (b_type == 2) inflate_dynamic(compressed_bitstream, decompressed_data);
	}
	std::cout << std::endl << "decompress function complete" << std::endl;
}

//unfilter data that was originally filtered by using value to the left
void unfilter_sub(std::vector<uint8_t> &decompressed_data, int row_start_index, int row_length, int bytes_per_pixel){
	int value = 0;
	for(int i = row_start_index+bytes_per_pixel; i < row_start_index+row_length; i++){ //loop thru all pixels on scanline except start
		value = (int)decompressed_data[i] + (int)decompressed_data[i-bytes_per_pixel]; //add left & current pixels
		decompressed_data[i] = (value % 256);//put unfiltered value in vector, truncated to a byte
	}
}

//unfilter data that was originally filtered by using value above it
void unfilter_up(std::vector<uint8_t> &decompressed_data, int row_start_index, int row_length){
	int value = 0;
	for(int i = row_start_index; i < row_start_index+row_length; i++){ //loop thru all pixels on scanline
		value = (int)decompressed_data[i] + (int)decompressed_data[i-row_length-1]; //add current & above 
		decompressed_data[i] = (value % 256);//put unfiltered value in vector, truncated to a byte
	}
}

//unfilter data that was originally filtered using average of left and above
void unfilter_avg(std::vector<uint8_t> &decompressed_data, int row_start_index, int row_length, int bytes_per_pixel){
	int value = 0;
	for(int i = row_start_index; i < row_start_index+row_length; i++){ //loop thru all pixels on scanline
		if (i-bytes_per_pixel < row_start_index) value = int(decompressed_data[i]) + int(decompressed_data[i-row_length-1] / 2); //just do above if left is out of range
		else value = (int)((decompressed_data[i-bytes_per_pixel] + decompressed_data[i-row_length-1])/2) + (int)(decompressed_data[i]);
		decompressed_data[i] = (value % 256);//put unfiltered value in vector, truncated to a byte
	}
}

//unfilter data that was originally filtered with above, left or diagonal above/left, whichever was closest to above + left - diagonal
void unfilter_paeth(std::vector<uint8_t> &decompressed_data, int row_start_index, int row_length, int bytes_per_pixel){
	int paeth;
	int value;
	int left = 0;
	int above = 0;
	int diagonal = 0;
	for(int i = row_start_index; i < row_start_index+row_length; i++){ //loop thru all pixels on scanline except start
		if (i-bytes_per_pixel >= row_start_index){ //can only go left or diagonal left if it won't bring us to the previous scanline
			left = decompressed_data[i-bytes_per_pixel];
			diagonal = decompressed_data[i-row_length-bytes_per_pixel-1]; 
		}
		if (i-row_length-1 > 0) above = decompressed_data[i-row_length-1]; //make sure that going above isn't out of index
		paeth = above + left - diagonal;
		if (std::abs(above-paeth) <= std::abs(left-paeth) && std::abs(above-paeth) <= std::abs(diagonal-paeth)) value = above;
		else if (std::abs(left-paeth) <= std::abs(diagonal-paeth) && std::abs(left-paeth) <= std::abs(above-paeth)) value = left;
		else  value = diagonal;
		value += (int)decompressed_data[i];
		decompressed_data[i] = (value % 256); //add back in predicted value
	}
}

//unfilter the decompressed data and remove filter bytes from data
//also if bytes_per_pixel == 4 (RGBA image), then delete alpha data
void unfilter(std::vector<uint8_t> &decompressed_data, int width, int height, int bytes_per_pixel){
	//go to the beginning of all the rows, check the filter byte, remove it
	int index = 0; 
	for(unsigned int row = 0; row < height; row++){
		//std::cout << row << " FILTER METHOD " << (int)decompressed_data[index] << std::endl;
		if (decompressed_data[index] == 1) unfilter_sub(decompressed_data, index+1, width*bytes_per_pixel, bytes_per_pixel);
		else if(decompressed_data[index] == 2) unfilter_up(decompressed_data, index+1, width*bytes_per_pixel);
		else if (decompressed_data[index] == 3) unfilter_avg(decompressed_data, index+1, width*bytes_per_pixel, bytes_per_pixel);
		else if (decompressed_data[index] == 4) unfilter_paeth(decompressed_data, index+1, width*bytes_per_pixel, bytes_per_pixel);
		decompressed_data.erase(decompressed_data.begin() + index);
		index += (width*bytes_per_pixel);
	}
}

//PNG DECODING FUNCTIONS END

//BMP CONVERSION FUNCTIONS BEGIN

//bmp bytes are stored least significant byte first (this is opposite of the png header)
//if it's an RGBA image, will convert completely transparent pixels to white
void export_to_bitmap(std::vector<uint8_t> &decompressed_data, int width, int height, const std::string &output_file, int bytes_per_pixel){
	std::ofstream output(output_file, std::ios::out | std::ios::binary);
	output.seekp(0, std::ios::beg);

	int rowsize = width*3;
	if (rowsize % 4 != 0) //rows must have a multiple of 4 bytes, round up to next multiple of 32, times 3 assuming 24 bits per pixel
		rowsize = (width*3) - (width*3)%4 + 4;

	char header[14] = {66,77,0,0,0,0,0,0,0,0,0,0,0,0}; 
	split_into_four_bytes(header, 2, 14+40+(rowsize*height));//total file size
	split_into_four_bytes(header, 10, 14+40); //offset from beginning of file to pixel data (no color table, so just headers)
	
	char info_header[40] = {40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	split_into_four_bytes(info_header ,4, width);
	split_into_four_bytes(info_header, 8, height);
	split_into_two_bytes(info_header, 12, 1); //num of color planes, must be 1
	split_into_two_bytes(info_header, 14, 24); //color depth, 24 bits per pixel
	//info_header 16-19 are 4 bytes for the compression method ... leave at 0 (no compression) for now
	//?? info_header 20-23 are 4 bytes for raw image data size ... says it can be a dummy 0 if no compression, but i don't know how Paint got 320?
	//info_header 24-27 are preferred resolution in pixels per meter, X-axis, o means no preference
	//info_header 28-31 preferred resolution in pixels per meter, Y-axis, 0 means no preference
	//info_header 32-35 number of colors in color table ... 0 since this is rgb image
	//info_header 36-39 number of colors considered important, generally ignored, 0 as default

	char* pixel_data = new char[rowsize*height]; 
	//load rows into array, bottom row first
	int index = 0;
	for(int row = 0; row < height; row++){
		for(int col = decompressed_data.size()-(width*bytes_per_pixel*(row+1)); col < decompressed_data.size()-(width*bytes_per_pixel*row); col+=bytes_per_pixel){
			if (bytes_per_pixel == 4 && decompressed_data[col+3] == 0){//if this is a completely transparent pixel, make it white
				pixel_data[index+2] = 255;
				pixel_data[index+1] = 255;
				pixel_data[index] = 255;
			}
			else{//need to flip from RGB to BGR because of bmp format
				pixel_data[index+2] = decompressed_data[col];
				pixel_data[index+1] = decompressed_data[col+1];
				pixel_data[index] = decompressed_data[col+2];
			}
			index+=3;
		}	
		//extra bytes to get it to a multiple of 4 
		for(int i = width*3; i < rowsize; i++){
			pixel_data[index] = 0;
			index++;
		}
	}
	std::cout << std::endl;
	output.write(header, 14);
	output.write(info_header, 40);
	output.write(pixel_data, rowsize*height);
	delete [] pixel_data;
}

//BMP CONVERSION FUNCTIONS END

int main(int argc, char* argv[]){
	if (argc < 2) std::cerr << "Requires at least 1 argument - png filename" << std::endl;
	std::ifstream file (argv[1], std::ios::in|std::ios::binary|std::ios::ate);
	std::string output_filename = "output.bmp";
	if (argc == 3) output_filename = argv[2];

	if (file.is_open()){
	    unsigned int size = file.tellg();
	    if (!check_header(file)){ std::cerr << "ERROR: file is not a png" << std::endl; return 1; }
	    
	    //read everything beyond the header
	    file.seekg(8);
	    char* memblock = new char [size-8];
	    file.read (memblock, size-8);
	    file.close();

	    std::vector<uint8_t> png_data(size-8);
	   	for(unsigned int i = 0; i < size-8; i++){
	   		png_data[i] = (uint8_t)((unsigned char)memblock[i]);
	   	}
	    delete[] memblock;

	    //data processing of image
	   	unsigned int height;
	   	unsigned int width;
	    std::vector<uint8_t> compressed_data;
	    int bytes_per_pixel; //this will be 3 for RGB and 4 for RGBA
	    try{
	    	processChunks(png_data, compressed_data, width, height, bytes_per_pixel);
	    	std::vector<uint8_t> decompressed_data;
	    	//print_raw_data(compressed_data);
	    	decompress(compressed_data, decompressed_data);
	    	unfilter(decompressed_data, width, height, bytes_per_pixel);
 	    	//print_raw_data(decompressed_data, width*bytes_per_pixel);

	    	//print_raw_data(decompressed_data, width*bytes_per_pixel);
	    	//print_raw_data(decompressed_data);
	    	//print_raw_data(decompressed_data, width*bytes_per_pixel);
	    	export_to_bitmap(decompressed_data, width, height, output_filename, bytes_per_pixel);
	    }
		catch(std::string error){
			std::cerr << error << std::endl;
			return 1;
		}
	}
	else std::cout << "Unable to open file";

	return 0;
}