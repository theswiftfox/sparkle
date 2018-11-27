#ifndef FILE_READER_H
#define FILE_READER_H

#include <vector>
#include <string>

namespace Engine {
	namespace Tools {
		namespace FileReader {
			/**
			 * \brief ImageFile representation
			 */
			struct ImageFile {
				int width, height, channels;
				unsigned char* imageData;

				/**
				 * \brief release the bound memory for the image
				 */
				void free();
			};

			/**
			 * \brief read file from given path into a character array
			 * \param fileName file path to read from
			 * \return vector with all characters from the file
			 */
			std::vector<char> readFile(const std::string& fileName);
			/**
			 * \brief read file from given path line by line into a string array
			 * \param fileName file path to read from
			 * \return vector with all lines from the file
			 */
			std::vector<std::string> readFileLines(const std::string& fileName);
			/**
			 * \brief load image from path
			 * \param imagePath image path to read from
			 * \return @ImageFile with data read from @imagePath
			 */
			ImageFile loadImage(std::string imagePath);
		}
	}
}


#endif
