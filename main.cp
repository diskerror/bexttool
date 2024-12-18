//
// Created by Reid Woodbury.
//

/**
 * Works like exiftool.
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <print>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Wave.h"

using namespace std;


int main(int argc, char **argv) {
    auto exitVal = EXIT_SUCCESS;

    try {
        //  Check for input parameter.
        //	if none, display help
        if (argc < 2) {
            cout << "Displays or writes to the 'bext' chunk of a WAVE file" << endl;
            cout << "    and displays contents of format ('fmt ') chunk." << endl;
            cout << "Display data:       " << argv[0] << " <file>" << endl;
            cout << "Write data to file: " << argv[0] << " <TAG \"<TAG_DATA>\"> <file>" << endl;
            cout <<
                 R"EOF(
TAGs can be one or more of (from Specification of the Broadcast Wave Format (BWF) pdf):
    -Description          ASCII [256] : Description of the sound sequence, null padded.
    -Originator           ASCII [32]  : Name of the originator, null padded.
    -OriginatorReference  ASCII [32]  : Reference of the originator, null padded.
    -OriginationDate      ASCII [10]  : yyyy:mm:dd
    -OriginationTime      ASCII [8]   : hh:mm:ss
    -TimeReference        unsigned 64-bit integer: First sample count since midnight.
    -Version              unsigned 16-bit integer: Version of the BWF with UMID and loudness fields.
    -UMID                 ASCII [64]  : Unique Material Identifier to standard SMPTE 330M.
    -LoudnessValue        decimal in the range -99.99 to 99.99
    -LoudnessRange        decimal in the range   0.00 to 99.99
    -MaxTruePeakLevel     decimal in the range -99.99 to 99.99
    -MaxMomentaryLoudness decimal in the range -99.99 to 99.99
    -MaxShortTermLoudness decimal in the range -99.99 to 99.99

Setting or leaving the version number at zero will clear UMID and all loudness fields.

)EOF";
            return exitVal;
        }

        HeaderChunk_t header;

        FormatExtensibleData_t format;
        size_t formatSize = 0;

        Size64Data_t size64data;
        size_t size64dataSize = sizeof(size64data);

        BroadcastAudioExt_t bExt;
        Chunk_t bextChunk;
        bextChunk.id = 'bext';
        bextChunk.size = sizeof(bExt);    //	602
        streampos bextPositon = 0;

        size_t dataChunkSize = 0;
        streampos dataStartPositon = 0;    //	data starts after chunkID and size

        //	Path to file is always the last argument.
        string fileName(argv[argc - 1]);

        ifstream inStream(fileName.c_str(), ios_base::in | ios_base::binary);
        if (!inStream.good()) {
            throw invalid_argument("There was a problem opening the input file.");
        }

        //	WAVE files always have a 12 byte header
        inStream.read((char *) &header, 12);
        if (header.id != 'RIFF' && header.id != 'RF64') {
            inStream.close();
            throw invalid_argument((fileName + "\n is not a RIFF or RF64 file.").c_str());
        }

        //	Identify and load pertinent Chunks.
        while (inStream.good() && !inStream.eof()) {
            Chunk_t chunkExam{};

            inStream.read((char *) &chunkExam, 8);

            switch (chunkExam.id) {
                case 'fmt ':
                    formatSize = chunkExam.size;
                    inStream.read((char *) &format, formatSize);
                    break;

                case 'bext':
                    bextPositon = inStream.tellg();
                    //	Using sizeof(bExt) because we only want the basic size read.
                    inStream.read((char *) &bExt, bextChunk.size);
                    //	Now reposition stream pointer in case actual bext size is larger.
                    inStream.seekg(chunkExam.size - bextChunk.size, ios_base::cur);
                    break;

                case 'ds64':
                    //	Reading this only to know what to skip over.
                    inStream.read((char *) &size64data, size64dataSize);
                    break;

                case 'data':
                    dataChunkSize = (chunkExam.size == 0xFFFFFFFF) ? size64data.dataSize : chunkExam.size;
                    dataStartPositon = inStream.tellg();
                    inStream.seekg(dataChunkSize, ios_base::cur);
                    break;

                    // skip over everything else
                default:
                    inStream.seekg(chunkExam.size, ios_base::cur);
                    break;
            }
        }

        inStream.close();

        if (argc == 2) {
            ios state(nullptr);
            state.copyfmt(cout);

            cout << fileName << endl;
            cout << "  Overall size: " << header.size << endl;
            cout << endl;
            cout << "FORMAT CHUNK ('fmt '):" << endl;
            cout << "  LENGTH:  " << formatSize << endl;
            cout << "  type:               " << format.type << endl;
            cout << "  channelCount:       " << format.channelCount << endl;
            cout << "  sampleRate:         " << format.sampleRate << endl;
            cout << "  bytesPerSecond:     " << format.bytesPerSecond << endl;
            cout << "  blockAlignment:     " << format.blockAlignment << endl;
            cout << "  bitsPerSample:      " << format.bitsPerSample << endl;

            if (formatSize > 16) {
                cout << "  cbSize:             " << format.cbSize << endl;
            }

            if (formatSize >= 40) {
                ios::fmtflags f(cout.flags());
                cout << "  validBitsPerSample: " << format.validBitsPerSample << endl;
                cout << "  channelMask:        " << format.channelMask << endl;
                cout << "  subFormat:          " << hex;
                cout << format.subFormat.data1 << "-";
                cout << format.subFormat.data2 << "-";
                cout << format.subFormat.data3 << "-";
                cout << format.subFormat.data4 << "-";
                cout << format.subFormat.data5 << endl;
                std::cout.flags(f);
            }

            cout << endl;
            cout << "BROADCAST AUDIO EXTENSION ('bext'):" << endl;
            cout << "  LENGTH:  " << bextChunk.size << endl;
            cout << "  Description:          " << string(bExt.Description, 256) << endl;
            cout << "  Originator:           " << string(bExt.Originator, 32) << endl;
            cout << "  OriginatorReference:  " << string(bExt.OriginatorReference, 32) << endl;
            cout << "  OriginationDate:      " << string(bExt.OriginationDate, 10) << endl;
            cout << "  OriginationTime:      " << string(bExt.OriginationTime, 8) << endl;
            cout << "  TimeReference:        " << bExt.TimeReference << endl;
            cout << "  Version:              " << bExt.Version << endl;
            cout << "  UMID[0-31]:           " << "0x";
            int c;
            for (c = 0; c < 32; c++) {
                cout << std::setfill('0') << std::setw(2) << hex << (unsigned int) bExt.UMID[c];
            }
            cout << endl;
            cout << "  UMID[32-63]:          " << "0x";
            for (; c < 64; c++) {
                cout << std::setfill('0') << std::setw(2) << hex << (unsigned int) bExt.UMID[c];
            }
            cout << endl;
            cout.copyfmt(state);

            if (bExt.LoudnessValue == 0x7FFF) {
                cout << "  LoudnessValue:" << endl;
            }
            else {
                cout << "  LoudnessValue:        " << (float_t) bExt.LoudnessValue / 100.0 << endl;
            }

            if (bExt.LoudnessRange == 0x7FFF) {
                cout << "  LoudnessRange:" << endl;
            }
            else {
                cout << "  LoudnessRange:        " << (float_t) bExt.LoudnessRange / 100.0 << endl;
            }

            if (bExt.MaxTruePeakLevel == 0x7FFF) {
                cout << "  MaxTruePeakLevel:" << endl;
            }
            else {
                cout << "  MaxTruePeakLevel:     " << (float_t) bExt.MaxTruePeakLevel / 100.0 << endl;
            }

            if (bExt.MaxMomentaryLoudness == 0x7FFF) {
                cout << "  MaxMomentaryLoudness:" << endl;
            }
            else {
                cout << "  MaxMomentaryLoudness: " << (float_t) bExt.MaxMomentaryLoudness / 100.0 << endl;
            }

            if (bExt.MaxShortTermLoudness == 0x7FFF) {
                cout << "  MaxShortTermLoudness:" << endl;
            }
            else {
                cout << "  MaxShortTermLoudness: " << (float_t) bExt.MaxShortTermLoudness / 100.0 << endl;
            }

            cout << endl;
            cout << "AUDIO DATA ('data'):" << endl;
            cout << "  LENGTH:  " << dataChunkSize << endl;
            cout << "  Audio sample start in file: " << dataStartPositon << endl;
            cout << "  Total time:                 ";
            double_t seconds = ((double_t) dataChunkSize / format.blockAlignment) / format.sampleRate;
            uint32_t hours = floor(seconds / 3600.0);
            seconds = remainder(seconds, 3600.0);
            uint32_t minutes = floor(seconds / 60.0);
            seconds = remainder(seconds, 60.0);
            cout << std::setfill('0') << std::setw(2) <<
                 hours << ":" << minutes << ":" << std::setw(4) << seconds << endl;
        }
        else {
            //	parse options
            for (uint32_t i = 1; i < (argc - 1); i += 2) {
                string tagName = argv[i];
                string tagData = argv[i + 1];
                tagData += string(255, '\0');   //  Set enough NULLs to fill unused part of field for all cases.

                if (tagName == "-Description") {
                    memcpy(bExt.Description, tagData.c_str(), 255);
                }
                else if (tagName == "-Originator") {
                    memcpy(bExt.Originator, tagData.c_str(), 32);
                }
                else if (tagName == "-OriginatorReference") {
                    memcpy(bExt.OriginatorReference, tagData.c_str(), 32);
                }
                else if (tagName == "-OriginationDate") {
                    memcpy(bExt.OriginationDate, tagData.c_str(), 10);
                }
                else if (tagName == "-OriginationTime") {
                    memcpy(bExt.OriginationTime, tagData.c_str(), 32);
                }
                else if (tagName == "-TimeReference") {
                    bExt.TimeReference = stoi(tagData);
                }
                else if (tagName == "-Version") {
                    bExt.Version = stoi(tagData);
                }
                else if (tagName == "-UMID") {
                    memcpy(bExt.UMID, tagData.c_str(), 64);
                }
                else if (tagName == "-LoudnessValue") {
                    bExt.LoudnessValue = stof(tagData) * 100;
                }
                else if (tagName == "-LoudnessRange") {
                    bExt.LoudnessRange = stof(tagData) * 100;
                }
                else if (tagName == "-MaxTruePeakLevel") {
                    bExt.MaxTruePeakLevel = stof(tagData) * 100;
                }
                else if (tagName == "-MaxMomentaryLoudness") {
                    bExt.MaxMomentaryLoudness = stof(tagData) * 100;
                }
                else if (tagName == "-MaxShortTermLoudness") {
                    bExt.MaxShortTermLoudness = stof(tagData) * 100;
                }
                else {
                    throw invalid_argument(string("Invalid argument: ") + tagName + ". Bad tag name.");
                }
            }

            //  0x7FFF means to ignore this value. Any value out of range is ignored.
            if (bExt.Version == 0) {
                memcpy(bExt.UMID, calloc(64, 1), 64);
                bExt.LoudnessValue = 0x7FFF;
                bExt.LoudnessRange = 0x7FFF;
                bExt.MaxTruePeakLevel = 0x7FFF;
                bExt.MaxMomentaryLoudness = 0x7FFF;
                bExt.MaxShortTermLoudness = 0x7FFF;
            }

            ofstream outStream(fileName.c_str(), ios_base::in | ios_base::out | ios_base::binary);
            if (!outStream.good()) {
                throw invalid_argument("There was a problem opening the file for writing.");
            }

            if (bextPositon >= 12) {
                //		write over old 'bext' chunk
                outStream.seekp(bextPositon);
                outStream.write((char *) &bExt, bextChunk.size);
            }
            else {
                //		write chunk at end of file
                outStream.seekp(0, ios_base::end);

                outStream.write((char *) &bextChunk, 8);
                outStream.write((char *) &bExt, bextChunk.size);

                //		update overall size in header
                header.size += bextChunk.size + 8;
                outStream.seekp(4);
                outStream.write((char *) &header.size, 4);
            }

            outStream.close();
        }


    } // try
    catch (exception &e) {
        cerr << e.what() << endl;
        exitVal = EXIT_FAILURE;
    }

    return exitVal;

} // main
