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
	bool moveMode = false;

	char savePath[MAX_PATH];
};

struct ReplayState
{
	bool isPlaying = false;
	bool isPaused = false;
	int progress = -1;

	bool isPlayingAlong = false;
	bool waitForInput = false;
	bool wasInputNotePressed = false;
	bool autoScroll = true;
	int playbackSpeedPercent = 100;
	int64_t lastNotePlayTime = -1;
};

struct NoteStats
{
	int64_t playTime = -1;
	double actualWarmupRatio = 0;
	float delayTime = 0;
	int delayNum = 0;
	int warmupTime;
	Key key = (Key)-1;
	bool wasPlayed = false;
	bool wasClicked = false;
};

struct Score
{
	int badNotes = 0;
	int combo = 0;
	int64_t points = 0;
	float accuracy = 0.0f;
	int missedNotes = 0;
};