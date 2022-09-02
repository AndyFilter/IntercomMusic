#pragma once

//enum Note
//{
//	C,
//	Cs,
//	Db = Cs,
//	D,
//	Ds,
//	Eb = Ds,
//	E,
//	Es,
//	Fb = E,
//	F = Es,
//	Fs,
//	Gb = Fs,
//	G,
//	Gs,
//	Ab = Gs,
//	A,
//	As,
//	Bb = As,
//	B,
//	Bs = C,
//	Cb = B,
//};

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

//enum Note
//{
//	A,
//	As,
//	Bb = As,
//	B,
//	C,
//	Bs = C,
//	Cb = B,
//	Cs,
//	Db = Cs,
//	D,
//	Ds,
//	Eb = Ds,
//	E,
//	Es,
//	Fb = E,
//	F = Es,
//	Fs,
//	Gb = Fs,
//	G,
//	Gs,
//	Ab = Gs,
//};

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
};