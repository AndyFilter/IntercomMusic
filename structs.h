#pragma once
#include <vector>
#include <Windows.h>

enum Note
{
	A,
	As,
	B,
	C,
	Cs,
	D,
	Ds,
	E,
	F,
	Fs,
	G,
	Gs,
	Bb = As,
	Bs = C,
	Cb = B,
	Db = Cs,
	Eb = Ds,
	Fb = E,
	Es = F,
	Gb = Fs,
	Ab = Gs,
};

enum Key
{
	Key_A,
	Key_As,
	Key_Bb,
	Key_B,
	Key_Bs,
	Key_Cb,
	Key_C,
	Key_Cs,
	Key_Db,
	Key_D,
	Key_Ds,
	Key_Eb,
	Key_E,
	Key_Es,
	Key_Fb,
	Key_F,
	Key_Fs,
	Key_Gb,
	Key_G,
	Key_Gs,
	Key_Ab,
};


enum Scale
{
	Major,
	Minor,
};

enum FunctionType
{
	sine,
	saw,
};

struct Settings
{
	int octave = 6;
	float volume = 0.1f;
	bool useSawWave = false; // unused

	bool useScale = false;
	Key selectedKey = Key_C;
	Scale selectedScale = Major;

	bool isRecording = false;
	bool isRecordingPaused = false;
	bool useStaticDelay = true;
	int delay = 200;
};

enum RecordingEventType
{
	RecEv_Key,
	RecEv_Delay,
};

struct RecordingEvent
{
	RecordingEventType type;
	int value;
};

struct Recording
{
	std::vector<RecordingEvent> data;

	int64_t lastKeyTime = -1;
	int selectedEvent = -1;
	bool hideDealys = false;

	char savePath[MAX_PATH];
};

struct ReplayState
{
	bool isPlaying = false;
	bool isPaused = false;
};