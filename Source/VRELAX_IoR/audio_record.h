/**
* MIT License
* Copyright (c) 2017 Hossein Mobasher
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
/**
*  This implementation is based on the WAVE PCM soundfile format.
*  http://soundfile.sapp.org/doc/WaveFormat/
*  Created by Hossein Mobasher <hoseinmobasher@gmail.com>
*/

#pragma once
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib,"winmm.lib")

#include <Windows.h>
#include <mmsystem.h>
#include <fstream>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <string.h>

using namespace std;
#define FREQUENCY 44100
#define LENGTH 2
//Length is typically 44100*time...

int StartRecord(char* data, int length)
{
	const int sampleRate = FREQUENCY;
	HWAVEIN hWaveIn;
	WAVEHDR WaveInHdr;

	WAVEFORMATEX aud_format;
	aud_format.wFormatTag = WAVE_FORMAT_PCM;     // simple, uncompressed format
	aud_format.nChannels = 1;                    //  1=mono, 2=stereo
	aud_format.nSamplesPerSec = sampleRate;      // 44100
	aud_format.wBitsPerSample = 8;     //  16 for high quality, 8 for telephone-grade
	aud_format.nBlockAlign = aud_format.wBitsPerSample * aud_format.nChannels / 8;                  // = n.Channels * wBitsPerSample/8
	aud_format.nAvgBytesPerSec = aud_format.nBlockAlign * aud_format.nSamplesPerSec / 8;
	aud_format.cbSize = 0;

	if (waveInOpen(&hWaveIn, WAVE_MAPPER, &aud_format, 0L, 0L, WAVE_FORMAT_DIRECT)) return 1;

	WaveInHdr.lpData = LPSTR(data);
	WaveInHdr.dwBufferLength = length;
	WaveInHdr.dwBytesRecorded = 0;
	WaveInHdr.dwUser = 0L;
	WaveInHdr.dwFlags = 0L;
	WaveInHdr.dwLoops = 0L;
	waveInPrepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));

	if (waveInAddBuffer(hWaveIn, &WaveInHdr, sizeof(WAVEHDR))) return 2;

	if (waveInStart(hWaveIn)) return 3;
	while (waveInUnprepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) Sleep(1);
	waveInClose(hWaveIn);
	return 0;
}

void raw_to_wav(std::string file_path, std::string file_name, int sample_rate, int bits_per_sample, int num_channels, int byte_rate);

void writeInt(std::vector<char> &output, const int value) {
	output.push_back((char)(value >> 0));
	output.push_back((char)(value >> 8));
	output.push_back((char)(value >> 16));
	output.push_back((char)(value >> 24));
}

void writeShort(std::vector<char> &output, const short value) {
	output.push_back((char)(value >> 0));
	output.push_back((char)(value >> 8));
}

void writeString(std::vector<char> &output, const std::string value) {
	for (int i = 0; i < value.length(); i++) {
		output.push_back(value[i]);
	}
}
// (file_path + "/" +) Use this if you want to save wav in different folder. Beware that .raw must be in the same folder.
void raw_to_wav(std::string file_path,
	std::string file_name,
	int srate = 44100, // sample rate
	int bps = 8, // bits per sample
	int nc = 1, // num channels (1 = mono, 2 = stereo)
	int brate = 192000) // byte rate
{
	std::vector<char> vector;

	std::ifstream file;
	file.exceptions(
		std::ifstream::badbit |
		std::ifstream::failbit |
		std::ifstream::eofbit);

	file.open(file_name + ".raw", std::ifstream::in | std::ifstream::binary);
	file.seekg(0, std::ios::end);
	std::streampos length(file.tellg());

	if (length) {
		file.seekg(0, std::ios::beg);
		vector.resize(static_cast<std::size_t>(length));
		file.read(&vector.front(), static_cast<std::size_t>(length));
	}

	std::vector<char> output;

	int n = 1;
	// little endian if true
	if (1 == *(char *)& n) {
		writeString(output, "RIFF"); // chunk id
	}
	else {
		writeString(output, "RIFX"); // chunk id
	}

	writeInt(output, (const int)(36 + vector.size())); // chunk size
	writeString(output, "WAVE"); // format
	writeString(output, "fmt "); // subchunk 1 id
	writeInt(output, 16); // subchunk 1 size
	writeShort(output, 1); // audio format (1 = PCM)
	writeShort(output, (const short)nc); // number of channels
	writeInt(output, srate); // sample rate
	writeInt(output, brate); // byte rate
	writeShort(output, (short)(nc * bps / 8)); // block align
	writeShort(output, (short)bps); // bits per sample
	writeString(output, "data"); // subchunk 2 id
	writeInt(output, (const int)vector.size()); // subchunk 2 size

	output.insert(output.end(), vector.begin(), vector.end());

	std::ofstream wav_file;
	wav_file.open(file_name + ".wav", std::ios::binary); // open wav
	wav_file.write(&output[0], output.size()); // write wav
	wav_file.close();
}