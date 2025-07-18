/*
 Copyright © 2011 Apple  Inc. All rights reserved.
 
 IMPORTANT:  This Apple software is supplied to you by Apple Inc. (“Apple”) in consideration of your agreement to the following terms, and your use, installation, modification or redistribution of this Apple software constitutes acceptance of these terms.  If you do not agree with these terms, please do not use, install, modify or redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and subject to these terms, Apple grants you a personal, non-exclusive license, under Apple’s copyrights in this original Apple software (the “Apple Software”), to use, reproduce, modify and redistribute the Apple Software, with or without modifications, in source and/or binary forms; provided that if you redistribute the Apple Software in its entirety and without modifications, you must retain this notice and the following text and disclaimers in all such redistributions of the Apple Software.  Neither the name, trademarks, service marks or logos of Apple Inc. may be used to endorse or promote products derived from the Apple Software without specific prior written permission from Apple.  Except as expressly stated in this notice, no other rights or licenses, express or implied, are granted by Apple herein, including but not limited to any patent rights that may be infringed by your derivative works or by other works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS. 
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * main.cpp
 *
 * Converts pcm data contained in a .wav or .caf file into Apple Lossless (ALAC) put into a .caf file
 * or converts ALAC data from a .caf file into pcm data and put into a .wav or .caf file
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <out123.h>

// these are headers for the ALAC encoder and decoder
//#include "ALACEncoder.h"
#include "ALACDecoder.h"
#include "ALACBitUtilities.h"

// these are utility headers for this sample code
#include "CAFFileALAC.h"
#include "EndianPortable.h"

#define kMaxBERSize 5
#define kCAFFdataChunkEditsSize  4

#define kWAVERIFFChunkSize 12
#define kWAVEfmtChunkSize 24
#define kWAVEdataChunkHeaderSize 8

#define VERBOSE 0

// Helper functions
int32_t GetInputFormat(FILE * inputFile, AudioFormatDescription * theInputFormat, uint32_t * theFileType);
int32_t SetOutputFormat(AudioFormatDescription theInputFormat, AudioFormatDescription * theOutputFormat);
int32_t FindDataStart(FILE * inputFile, uint32_t inputFileType, int32_t * dataPos, int32_t * dataSize);
//int32_t DecodeALAC(FILE * inputFile, FILE * outputFile, AudioFormatDescription theInputFormat, AudioFormatDescription theOutputFormat, int32_t inputDataSize, uint32_t outputFileType);
int32_t DecodeALAC(FILE * inputFile, out123_handle *out, AudioFormatDescription theInputFormat, AudioFormatDescription theOutputFormat, int32_t inputDataSize, uint32_t outputFileType);
void GetOutputFileType(char * outputFileName, uint32_t * outputFileType);

// Adapted from CoreAudioTypes.h
enum
{
    kTestFormatFlag_16BitSourceData    = 1,
    kTestFormatFlag_20BitSourceData    = 2,
    kTestFormatFlag_24BitSourceData    = 3,
    kTestFormatFlag_32BitSourceData    = 4
};


int32_t main (int32_t argc, char * argv[]) 
{
    char * inputFileName = argv[1];
    char * outputFileName = argv[2];
    FILE * inputFile = NULL;
    FILE * outputFile = NULL;
    out123_handle *out;
		
	bool malformed = argc < 2;
	
    // Parse the commandline and open the necessary files
    for (int32_t i = 1; i < argc; ++i) 
	{
		if (strcmp (argv[i], "-h") == 0)
		{
			malformed = true;
		}
		else
		{
			if (argv[i][0] == '-')
            {
				printf ("unknown option: %s\n", argv[i]);
				malformed = true;
			}
			else
			{
                if (inputFile == NULL) inputFile = fopen (inputFileName, "rb"); // the b is necessary for Windows -- ignored by Unix
                if(inputFile == NULL)
                {
                    fprintf(stderr," Cannot open file \"%s\"\n", inputFileName);
                    exit (1);
                }

#if 1
	out = out123_new();
	if(!out)
		exit (1);
	if(out123_open(out, NULL, NULL))
		exit (1);
#else
                if (outputFile == NULL) outputFile = fopen (outputFileName, "w+b"); // the b is necessary for Windows -- ignored by Unix
                if(outputFile == NULL)
                {
                    fprintf(stderr," Cannot open file \"%s\"\n", outputFileName);
                    exit (1);
                }
#endif
			}
		}
				
		if (malformed)
		{
			break;
		}
	}
	
	if (!malformed) 
	{
        printf("Input file: %s\n", inputFileName);
//        printf("Output file: %s\n", outputFileName);
        // So at this point we have the input and output files open. Need to determine what we're dealing with
        int32_t theError = 0;
        AudioFormatDescription inputFormat;
        AudioFormatDescription outputFormat;
        int32_t inputDataPos = 0, inputDataSize = 0;
        uint32_t inputFileType = 0; // 'caff' or 'WAVE'
        uint32_t outputFileType = 0; // 'caff' or 'WAVE'
        
        theError = GetInputFormat(inputFile, &inputFormat, &inputFileType);
        if (theError)
        {
            fprintf(stderr," Cannot determine what format file \"%s\" is\n", inputFileName);
            exit (1);            
        }
        
        if (inputFileType != 'WAVE' && inputFileType != 'caff')
        {
            fprintf(stderr," File \"%s\" is of an unsupported type\n", outputFileName);
            exit (1);                        
        }
        
        if (inputFormat.mFormatID != kALACFormatAppleLossless && inputFormat.mFormatID != kALACFormatLinearPCM)
        {
            fprintf(stderr," File \"%s\'s\" data format is of an unsupported type\n", outputFileName);
            exit (1);                        
        }

        SetOutputFormat(inputFormat, &outputFormat);

        if (theError)
        {
            fprintf(stderr," Cannot determine what format file \"%s\" is\n", outputFileName);
            exit (1);            
        }
        FindDataStart(inputFile, inputFileType, &inputDataPos, &inputDataSize);
        fseek(inputFile, inputDataPos, SEEK_SET);
        
        // We know where we are and we know what we're doing
        if (outputFormat.mFormatID == kALACFormatAppleLossless)
        {
            // encoding
        }
        else
        {
#if 0
            // decoding
            GetOutputFileType(outputFileName, &outputFileType);
            
            if (outputFileType == 'WAVE' && outputFormat.mChannelsPerFrame > 2)
            {
                // we don't support WAVE because we don't want to reinterleave on output 
                fprintf(stderr," Cannot decode more than two channels to WAVE\n");
                exit (1);            
            }
#endif
            DecodeALAC(inputFile, out, inputFormat, outputFormat, inputDataSize, outputFileType);
        }
	}
	
	if (malformed) {
		printf ("Usage:\n");
		printf ("        alacconvert <input wav or caf file> <output caf file>\n");
		printf ("Decode:\n");
		printf ("        alacconvert <input caf file> <output wav or caf file>\n");
		printf ("\n");
		return 1;
	}
	
    if (inputFile) fclose(inputFile);
    if (outputFile) fclose(outputFile);
    
	return 0;
}


int32_t GetInputFormat(FILE * inputFile, AudioFormatDescription * theInputFormat, uint32_t * theFileType)
{
    // assumes the file is open
    uint8_t theReadBuffer[20];
    bool done = false;
    uint32_t chunkType = 0;

    fread(theReadBuffer, 1, 4, inputFile);
    
    if (theReadBuffer[0] == 'c' && theReadBuffer[1] == 'a' && theReadBuffer[2] == 'f'  & theReadBuffer[3] == 'f')
    {
        // It's a caff file!
        *theFileType = 'caff';
        // We support pcm data for encode and alac data for decode
        done = GetCAFFdescFormat(inputFile, theInputFormat);
    }
    else if (theReadBuffer[0] == 'R' && theReadBuffer[1] == 'I' && theReadBuffer[2] == 'F'  & theReadBuffer[3] == 'F')
    {
        fread(theReadBuffer, 1, 8, inputFile);
        if (theReadBuffer[4] == 'W' && theReadBuffer[5] == 'A' && theReadBuffer[6] == 'V'  & theReadBuffer[7] == 'E')
        {
            // It's a WAVE file!
            *theFileType = 'WAVE';
            // We only support pcm data             
            while (!done)
            {
                uint32_t theChunkSize = 0, theSampleRate = 0;
                fread(theReadBuffer, 1, 4, inputFile);
                chunkType = ((int32_t)(theReadBuffer[0]) << 24) + ((int32_t)(theReadBuffer[1]) << 16) + ((int32_t)(theReadBuffer[2]) << 8) + theReadBuffer[3];
                switch (chunkType)
                {
                    case 'fmt ':
                        fread(theReadBuffer, 1, 20, inputFile);
                        // Remember campers we're in little endian land
                        if (theReadBuffer[4] != 1 || theReadBuffer[5] != 0)
                        {
                            // we only support PCM
                            *theFileType = 0; // clear it
                            return -1;                        
                        }
                        theInputFormat->mFormatID = kALACFormatLinearPCM;
                        theInputFormat->mChannelsPerFrame = theReadBuffer[6];
                        theSampleRate = ((int32_t)(theReadBuffer[11]) << 24) + ((int32_t)(theReadBuffer[10]) << 16) + ((int32_t)(theReadBuffer[9]) << 8) + theReadBuffer[8];
                        theInputFormat->mSampleRate = theSampleRate;
                        theInputFormat->mBitsPerChannel = theReadBuffer[18];
                        theInputFormat->mFormatFlags = kALACFormatFlagIsSignedInteger | kALACFormatFlagIsPacked; // always little endian
                        theInputFormat->mBytesPerPacket = theInputFormat->mBytesPerFrame = (theInputFormat->mBitsPerChannel >> 3) * theInputFormat->mChannelsPerFrame;
                        theInputFormat->mFramesPerPacket = 1;
                        theInputFormat->mReserved = 0;
                        done = true;
                        break;
                    default:
                        // read the size and skip
                        fread(theReadBuffer, 1, 4, inputFile);
                        theChunkSize = ((int32_t)(theReadBuffer[3]) << 24) + ((int32_t)(theReadBuffer[2]) << 16) + ((int32_t)(theReadBuffer[1]) << 8) + theReadBuffer[0];
                        fseek(inputFile, theChunkSize, SEEK_CUR);
                        break;
                }
            }
        }
        else
        {
            *theFileType = 0; // clear it
            return -1;
        }
    }
    else
    {
        *theFileType = 0; // clear it
        return -1;
    }

    if (!done) return -1;

    return 0;
}

int32_t SetOutputFormat(AudioFormatDescription theInputFormat, AudioFormatDescription * theOutputFormat)
{
    if (theInputFormat.mFormatID == kALACFormatLinearPCM)
    {
        // encoding
        theOutputFormat->mFormatID = kALACFormatAppleLossless;
        theOutputFormat->mSampleRate =  theInputFormat.mSampleRate;
        
        switch(theInputFormat.mBitsPerChannel)
        {
            case 16:
                theOutputFormat->mFormatFlags = kTestFormatFlag_16BitSourceData;
                break;
            case 20:
                theOutputFormat->mFormatFlags = kTestFormatFlag_20BitSourceData;
                break;
            case 24:
                theOutputFormat->mFormatFlags = kTestFormatFlag_24BitSourceData;
                break;
            case 32:
                theOutputFormat->mFormatFlags = kTestFormatFlag_32BitSourceData;
                break;
            default:
                return -1;
                break;
        }
        
        theOutputFormat->mFramesPerPacket = kALACDefaultFramesPerPacket;
        theOutputFormat->mChannelsPerFrame = theInputFormat.mChannelsPerFrame;
        // mBytesPerPacket == 0 because we are VBR
        // mBytesPerFrame and mBitsPerChannel == 0 because there are no discernable bits assigned to a particular sample
        // mReserved is always 0
        theOutputFormat->mBytesPerPacket = theOutputFormat->mBytesPerFrame = theOutputFormat->mBitsPerChannel = theOutputFormat->mReserved = 0;
    }
    else
    {
        // decoding
        theOutputFormat->mFormatID = kALACFormatLinearPCM;
        theOutputFormat->mSampleRate =  theInputFormat.mSampleRate;
        
        switch(theInputFormat.mFormatFlags)
        {
            case kTestFormatFlag_16BitSourceData:
                theOutputFormat->mBitsPerChannel = 16;
                break;
            case kTestFormatFlag_20BitSourceData:
                theOutputFormat->mBitsPerChannel = 20;
                break;
            case kTestFormatFlag_24BitSourceData:
                theOutputFormat->mBitsPerChannel = 24;
                break;
            case kTestFormatFlag_32BitSourceData:
                theOutputFormat->mBitsPerChannel = 32;
                break;
            default:
                return -1;
                break;
        }
        
        theOutputFormat->mFramesPerPacket = 1;
        theOutputFormat->mChannelsPerFrame = theInputFormat.mChannelsPerFrame;
        theOutputFormat->mBytesPerPacket = theOutputFormat->mBytesPerFrame = theOutputFormat->mBitsPerChannel != 20 ? theInputFormat.mChannelsPerFrame * ((theOutputFormat->mBitsPerChannel) >> 3) : (int32_t)(theInputFormat.mChannelsPerFrame * 2.5 + .5);
        theOutputFormat->mFormatFlags = kALACFormatFlagsNativeEndian;
        theOutputFormat->mReserved = 0;
    }
    return 0;
}

int32_t FindDataStart(FILE * inputFile, uint32_t inputFileType, int32_t * dataPos, int32_t * dataSize)
{
    // returns the absolute position within the file
    int32_t currentPosition = ftell(inputFile); // record the current position
    uint8_t theReadBuffer[12];
    uint32_t chunkType = 0, fileSize = 0, chunkSize = 0;
    bool done = false;
    
    switch (inputFileType)
    {
        case 'WAVE':
            fseek(inputFile, 0, SEEK_SET); // start at 0
            fread(theReadBuffer, 1, 8, inputFile);
            fileSize = ((int32_t)(theReadBuffer[7]) << 24) + ((int32_t)(theReadBuffer[6]) << 16) + ((int32_t)(theReadBuffer[5]) << 8) + theReadBuffer[4];
            fseek(inputFile, 12, SEEK_SET); // start at 12!
            while (!done && ((uint32_t)(ftell(inputFile)) < fileSize))
            {
                fread(theReadBuffer, 1, 8, inputFile);
                chunkType = ((int32_t)(theReadBuffer[0]) << 24) + ((int32_t)(theReadBuffer[1]) << 16) + ((int32_t)(theReadBuffer[2]) << 8) + theReadBuffer[3];
                switch(chunkType)
                {
                    case 'data':
                        *dataPos = ftell(inputFile);
                        // little endian size
                        *dataSize = ((int32_t)(theReadBuffer[7]) << 24) + ((int32_t)(theReadBuffer[6]) << 16) + ((int32_t)(theReadBuffer[5]) << 8) + theReadBuffer[4];
                        done = true;
                        break;
                    default:
                        chunkSize = ((int32_t)(theReadBuffer[7]) << 24) + ((int32_t)(theReadBuffer[6]) << 16) + ((int32_t)(theReadBuffer[5]) << 8) + theReadBuffer[4];
                        fseek(inputFile, chunkSize, SEEK_CUR);
                        break;
                }
            }
            break;
        case 'caff':
            done = FindCAFFDataStart(inputFile, dataPos, dataSize);
            break;
    }
    
    fseek(inputFile, currentPosition, SEEK_SET); // start at 0

    if (!done) return -1;

    return 0;
}

// There's not a whole lot of difference between encode and decode on this level
//int32_t DecodeALAC(FILE * inputFile, FILE * outputFile, AudioFormatDescription theInputFormat, AudioFormatDescription theOutputFormat, int32_t inputDataSize, uint32_t outputFileType)
int32_t DecodeALAC(FILE * inputFile, out123_handle *out, AudioFormatDescription theInputFormat, AudioFormatDescription theOutputFormat, int32_t inputDataSize, uint32_t outputFileType)
{
    int32_t theInputPacketBytes = theInputFormat.mChannelsPerFrame * (theOutputFormat.mBitsPerChannel >> 3) * theInputFormat.mFramesPerPacket + kALACMaxEscapeHeaderBytes;
    int32_t theOutputPacketBytes = theInputPacketBytes - kALACMaxEscapeHeaderBytes;
    int32_t thePacketTableSize = 0, packetTablePos = 0, outputDataSizePos = 0, inputDataPos = 0;
    uint8_t * theReadBuffer = (uint8_t *)calloc(theInputPacketBytes, 1);
    uint8_t * theWriteBuffer = (uint8_t *)calloc(theOutputPacketBytes, 1);
    int32_t numBytes = 0;
    int64_t numDataBytes = 0;
    uint32_t numFrames = 0;
    BitBuffer theInputBuffer;
    uint8_t * theMagicCookie = NULL;
    uint32_t theMagicCookieSize = 0;
    
    ALACDecoder * theDecoder = new ALACDecoder;
    
    // We need to get the cookie from the file
    theMagicCookieSize = GetMagicCookieSizeFromCAFFkuki(inputFile);
    theMagicCookie = (uint8_t *)calloc(theMagicCookieSize, 1);
    GetMagicCookieFromCAFFkuki(inputFile, theMagicCookie, &theMagicCookieSize);
    
    // While we don't have a use for this here, if you were using arbitrary channel layouts, you'd need to run the following check:
    
    theDecoder->Init(theMagicCookie, theMagicCookieSize);
    free(theMagicCookie);
    
    BitBufferInit(&theInputBuffer, theReadBuffer, theInputPacketBytes);
    inputDataPos = ftell(inputFile);
    
#if 0
    if (outputFileType != 'WAVE')
    {
        // we only write out the caff header, the 'desc' chunk and the 'data' chunk
        // write out the caff header
        WriteCAFFcaffChunk(outputFile);
        
        // write out the desc chunk
        WriteCAFFdescChunk(outputFile, theOutputFormat);
        
        // We might be multi channel
        if (theOutputFormat.mChannelsPerFrame > 2)
        {
            // we are not rearranging the output data
            WriteCAFFchanChunk(outputFile, CAFFChannelLayoutTags[theOutputFormat.mChannelsPerFrame - 1]);
        }
        
        // We'll write out the data chunk next. The 'data' size will start past the 'data' chunk identifier
        outputDataSizePos = ftell(outputFile) + sizeof(uint32_t);
        
        // Finally, write out the data chunk
        WriteCAFFdataChunk(outputFile);
    }
    else
    {
        // We're writing a mono or stereo WAVE file
        WriteWAVERIFFChunk(outputFile);
        WriteWAVEfmtChunk(outputFile, theOutputFormat);
        WriteWAVEdataChunk(outputFile);
        outputDataSizePos = ftell(outputFile) - sizeof(uint32_t);
    }
#endif
    out123_start(out, theInputFormat.mSampleRate, 2, MPG123_ENC_SIGNED_16);

    // We do have to get the packet size from the packet table
    FindCAFFPacketTableStart(inputFile, &packetTablePos, &thePacketTableSize);
    
    fseek(inputFile, packetTablePos, SEEK_SET);
    numBytes = fread(theReadBuffer, 1, kMaxBERSize, inputFile);
    
    theInputPacketBytes = ReadBERInteger(theReadBuffer, &numBytes);
    packetTablePos += numBytes;
    fseek(inputFile, inputDataPos, SEEK_SET);
    inputDataPos += theInputPacketBytes;
    
    while ((theInputPacketBytes > 0) && ((size_t)theInputPacketBytes == fread(theReadBuffer, 1, theInputPacketBytes, inputFile)))
    {
#if VERBOSE
        printf ("Read %i bytes\n", theInputPacketBytes);
#endif
        theDecoder->Decode(&theInputBuffer, theWriteBuffer, theInputFormat.mFramesPerPacket, theInputFormat.mChannelsPerFrame, &numFrames);
//printf("MORIMORI %d %d\n", theInputPacketBytes, numFrames);
        numBytes = numFrames * theOutputFormat.mBytesPerFrame;
#if VERBOSE        
        printf ("Writing %i bytes\n", numBytes);
#endif
//        fwrite(theWriteBuffer, 1, numBytes, outputFile);
	out123_play(out, theWriteBuffer, numBytes);
        numDataBytes += numBytes;
        fseek(inputFile, packetTablePos, SEEK_SET);
        numBytes = fread(theReadBuffer, 1, kMaxBERSize, inputFile);
        
        theInputPacketBytes = ReadBERInteger(theReadBuffer, &numBytes);
#if VERBOSE
        printf ("theInputPacketBytes == %i bytes\n", theInputPacketBytes);
#endif
        packetTablePos += numBytes;
        fseek(inputFile, inputDataPos, SEEK_SET);
        inputDataPos += theInputPacketBytes;
        BitBufferReset(&theInputBuffer);
    }
#if 0
    if (outputFileType != 'WAVE')
    {
        // cleanup -- write out the data size
        fseek(outputFile, outputDataSizePos, SEEK_SET);
        numDataBytes += kCAFFdataChunkEditsSize; // add in the edit bytes
#if VERBOSE
        printf ("numDataBytes == %i bytes\n", numDataBytes);
#endif
        WriteCAFFChunkSize(outputFile, numDataBytes);
    }
    else
    {
        // cleanup -- write out the data size
        fseek(outputFile, outputDataSizePos, SEEK_SET);
        WriteWAVEChunkSize(outputFile, (uint32_t)numDataBytes);
        // write out the file size
        fseek(outputFile, 4, SEEK_SET);
        WriteWAVEChunkSize(outputFile, numDataBytes + sizeof(outputFileType) + kWAVEdataChunkHeaderSize + kWAVEfmtChunkSize); // add in the size for 'WAVE', size of the data' chunk header and the 'fmt ' chunk
    }
#endif

    delete theDecoder;

    out123_del(out);
    
    free(theReadBuffer);
    free(theWriteBuffer);

    return 0;
}

void GetOutputFileType(char * outputFileName, uint32_t * outputFileType)
{
    char * typeStr = strrchr(outputFileName, '.');
    
    *outputFileType = 'caff';

    if (typeStr != NULL)
    {
        if (strlen(typeStr) == 4)
        {
            if (strcmp(typeStr, ".wav") == 0)
            {
                *outputFileType = 'WAVE';
            }
        }
    }
}
