#pragma once

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
	bool useSawWave = false;

	Key selectedKey = Key_C;
	Scale selectedScale = Major;
};