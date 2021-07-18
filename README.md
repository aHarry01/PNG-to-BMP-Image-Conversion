# PNG-to-BMP-Image-Conversion
Converts compressed PNG images to uncompressed BMP format. PNGs must have color type of RGB or RGBA, with a bit depth of 8. Implements an algorithm to decompress PNG image data compressed with a [combination of LZSS and Huffman coding](https://en.wikipedia.org/wiki/Deflate). 

## Usage:
Compile png.cpp, BitReadingFuncs.cpp, and HuffmanTree.cpp. Run with 1 required argument specifying the PNG file to convert, and 1 optional argument specifying output BMP file name. Default BMP filename is output.bmp if there is no second argument.

Example with g++:

g++ png.cpp BitReadingFuncs.cpp HuffmanTree.cpp -o main.out

./main.out png_test.png output_test.bmp

## Dependencies: 
* [C++20 Compiler](https://en.cppreference.com/w/cpp/20)

## References
*  [W3C PNG format specification](https://www.w3.org/TR/PNG/)
* [Decompression algorithm](https://pyokagan.name/blog/2019-10-18-zlibinflate/)

## License
Licensed under MIT license.
