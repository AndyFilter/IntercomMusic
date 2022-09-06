#pragma once

#include "structs.h"
#include <thread>

struct win32_audio_buffer;

namespace Sounds
{
	bool Setup();

	inline int* pBaseOctave = nullptr;
	inline bool* pUseSawWave = nullptr;
	void PlaySound(Note, DWORD dwMillis = 50, FunctionType funcType = (FunctionType)(*pUseSawWave), int octave = *pBaseOctave);

	void PlayReplay(Recording& rec, bool* is_playing, bool* is_paused, int* progress);

	const char* GetKeyName(Key key);
	Note Key2Note(Key key);
	Key Note2Key(Note note);
	void GetKeySignature(Scale scale, Key baseKey, Key outSignature[7]);

	const unsigned int sampleRate = 44100; // 44100;
	inline float* pVolume;
	inline bool isPlaying = false;

	inline win32_audio_buffer* AudioBuffer = nullptr;
	inline float* bufferMem = nullptr;

	inline std::thread replayThread;
}