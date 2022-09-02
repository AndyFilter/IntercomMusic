#pragma once

#include "structs.h"

struct win32_audio_buffer;

namespace Sounds
{
	inline int* pBaseOctave = nullptr;
	inline bool* pUseSawWave = nullptr;
	void PlaySound(Note, DWORD dwMillis = 50, FunctionType funcType = (FunctionType)(*pUseSawWave), int octave = *pBaseOctave);

	bool Setup();

	const unsigned int sampleRate = 44100;//44100;
	inline float* pVolume;
	inline bool isPlaying = false;

	inline win32_audio_buffer* AudioBuffer = nullptr;
	inline float* bufferMem = nullptr;
}