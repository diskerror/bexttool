//
// Created by Reid Woodbury.
//

/**
 * Works like exiftool.
 */

#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <print>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <string>

#include "Wave.h"

using namespace std;


int main(int argc, char **argv)
{
	auto exitVal = EXIT_SUCCESS;
	
	try
	{
		//  Check for input parameter.
		//	if none, display help
		if (argc < 2)
		{
			cout << "Displays or writes to the 'bext' chunk of a WAVE file" << endl;
			cout << "    and displays contents of format ('fmt ') chunk." << endl;
			cout << "Display data:       " << argv[0] << " <file>" << endl;
			cout << "Write data to file: " << argv[0] << " <TAG> <file>" << endl;
			cout <<
				 R"EOF(
TAGs can be one or more of (from Specification of the Broadcast Wave Format (BWF) pdf):
    -Description         ASCII [256] : Description of the sound sequence, null padded.
    -Originator          ASCII [32]  : Name of the originator, null padded.
    -OriginatorReference ASCII [32]  : Reference of the originator, null padded.
    -OriginationDate     ASCII [10]  : yyyy:mm:dd
    -OriginationTime     ASCII [8]   : hh:mm:ss
    -TimeReference       unsigned 64-bit integer: First sample count since midnight.
    -Version             unsigned 16-bit integer: Version of the BWF.
)EOF";
			return exitVal;
		}
		
		HeaderChunk_t          header;
		Chunk_t                chunkExam;
		FormatExtensibleData_t format;
		size_t                 formatSize     = 0;
		Size64Data_t           size64data;
		size_t                 size64dataSize = 0;
		BroadcastAudioExt_t    bExt;
		size_t                 bextSize       = 0;
		streampos              bextPositon    = 0;
		
		//	Path to file is always the last argument.
		string fileName(argv[argc - 1]);
		
		ifstream inStream(fileName.c_str(), ios_base::in | ios_base::binary);
		if (!inStream.good())
		{
			throw invalid_argument("There was a problem opening the input file.");
		}
		
		//	WAVE files always have a 12 byte header
		inStream.read(reinterpret_cast<char *>(&header), 12);
		if (!(header.id == 'RIFF' || header.id == 'RF64') || header.type != 'WAVE')
		{
			inStream.close();
			throw invalid_argument((fileName + "\n is not a WAVE file.").c_str());
		}
		
		//	Identify and load pertinent Chunks.
		while (inStream.good() && !inStream.eof())
		{
			inStream.read(reinterpret_cast<char *>(&chunkExam), 8);
			
			switch (chunkExam.id)
			{
				case 'fmt ':
					formatSize = chunkExam.size;
					inStream.read(reinterpret_cast<char *>(&format), formatSize);
					break;
				
				case 'bext':
					bextSize    = chunkExam.size;
					bextPositon = inStream.tellg();
					// Using sizeof(bExt) because we only want the basic size read.
					inStream.read(reinterpret_cast<char *>(&bExt), sizeof(bExt));
					inStream.seekg(inStream.tellg() + (streamoff) (bextSize - sizeof(bExt)));
					break;
				
				case 'ds64':
					//	Reading this only to know what to skip over.
					size64dataSize = chunkExam.size;
					inStream.read(reinterpret_cast<char *>(&size64data), size64dataSize);
					break;
				
				case 'data':
				{
					streamoff tempSize =
								  (chunkExam.size == 0xFFFFFFFF) ? size64data.dataSize : chunkExam.size;
					inStream.seekg(inStream.tellg() + tempSize);
				}
					break;
					
					// skip over everything else
				default:
					inStream.seekg((uint32_t) inStream.tellg() + chunkExam.size);
					break;
			}
		}
		
		inStream.close();
		
		if (argc == 2)
		{
			cout << fileName << endl;
			cout << endl;
			cout << "FORMAT CHUNK ('fmt '):" << endl;
			cout << "  LENGTH:  " << formatSize << endl;
			cout << "type:               " << format.type << endl;
			cout << "channelCount:       " << format.channelCount << endl;
			cout << "sampleRate:         " << format.sampleRate << endl;
			cout << "bytesPerSecond:     " << format.bytesPerSecond << endl;
			cout << "blockAlignment:     " << format.blockAlignment << endl;
			cout << "bitsPerSample:      " << format.bitsPerSample << endl;
			
			if (formatSize > 16)
			{
				cout << "cbSize:             " << format.cbSize << endl;
			}
			
			if (formatSize >= 40)
			{
				ios::fmtflags f(cout.flags());
				cout << "validBitsPerSample: " << format.validBitsPerSample << endl;
				cout << "channelMask:        " << format.channelMask << endl;
				cout << "subFormat:          " << hex;
				cout << format.subFormat.data1 << "-";
				cout << format.subFormat.data2 << "-";
				cout << format.subFormat.data3 << "-";
				cout << format.subFormat.data4 << "-";
				cout << format.subFormat.data5 << endl;
				std::cout.flags(f);
			}
			
			cout << endl;
			cout << "BROADCAST AUDIO EXTENSION ('bext'):" << endl;
			cout << "  LENGTH:  " << bextSize << endl;
			cout << "Description:         " << string(bExt.Description, 256) << endl;
			cout << "Originator:          " << string(bExt.Originator, 32) << endl;
			cout << "OriginatorReference: " << string(bExt.OriginatorReference, 32) << endl;
			cout << "OriginationDate:     " << string(bExt.OriginationDate, 10) << endl;
			cout << "OriginationTime:     " << string(bExt.OriginationTime, 8) << endl;
			cout << "TimeReference:       " << bExt.TimeReference << endl;
			cout << "Version:             " << bExt.Version << endl;
			cout << endl;
		}
		else
		{
			//	parse options
			for (uint32_t i = 1; i < (argc - 1); i += 2)
			{
				string tagName = argv[i];
				string tagData = argv[i + 1];
				tagData += string(255, '\0');
				
				if (tagName == "-Description")
				{
					memcpy(bExt.Description, tagData.c_str(), 255);
				}
				else if (tagName == "-Originator")
				{
					memcpy(bExt.Originator, tagData.c_str(), 32);
				}
				else if (tagName == "-OriginatorReference")
				{
					memcpy(bExt.OriginatorReference, tagData.c_str(), 32);
				}
				else if (tagName == "-OriginationDate")
				{
					memcpy(bExt.OriginationDate, tagData.c_str(), 10);
				}
				else if (tagName == "-OriginationTime")
				{
					memcpy(bExt.OriginationTime, tagData.c_str(), 32);
				}
				else if (tagName == "-TimeReference")
				{
					bExt.TimeReference = stoi(tagData);
				}
				else if (tagName == "-Version")
				{
					bExt.Version = stoi(tagData);
				}
				else
				{
					throw invalid_argument(string("Invalid argument: ") + tagName + ". Bad tag name.");
				}
			}
			
			ofstream outStream(fileName.c_str(), ios_base::in | ios_base::out | ios_base::binary);
			if (!outStream.good())
			{
				throw invalid_argument("There was a problem opening the file for writing.");
			}
			
			if (bextPositon >= 12)
			{
				//		write over old 'bext' chunk
				outStream.seekp(bextPositon);
				outStream.write(reinterpret_cast<char *>(&bExt), streamsize(bextSize));
			}
			else
			{
				//		write chunk at end of file
				outStream.seekp(0, std::ios_base::end);
				
				outStream.write(("bext"), 4);
				outStream.write(reinterpret_cast<char *>(&bextSize), streamsize(4));
				outStream.write(reinterpret_cast<char *>(&bExt), streamsize(bextSize));
				
				//		update overall size in header
				header.size += bextSize;
				outStream.seekp(4, ios::beg);
				outStream.write(reinterpret_cast<char *>(&(header.size)), streamsize(4));
			}
			
			outStream.close();
		}
		
		
	} // try
	catch (exception &e)
	{
		cerr << e.what() << endl;
		exitVal = EXIT_FAILURE;
	}
	
	return exitVal;
	
} // main
