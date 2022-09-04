#define WIN32_LEAN_AND_MEAN

#include <Windows.h> 
#include <iostream> 
#include <future>
#include <xaudio2.h>

#include "sounds.h"


struct win32_audio_buffer
{
    float Memory[Sounds::sampleRate * 1];  // samples per buffer (44100) * channels 1
    int BytesPerBuffer;
    XAUDIO2_BUFFER XBuffer;
    IXAudio2* XEngine;
    IXAudio2SourceVoice* SourceVoice;
    WAVEFORMATEX WaveFormat;
};

HRESULT Win32XAudioInit(win32_audio_buffer* AudioBuffer);

bool Sounds::Setup()
{
    AudioBuffer = new win32_audio_buffer();
    Win32XAudioInit(AudioBuffer);

    bufferMem = AudioBuffer->Memory;

    return true;
}

// NOTE XAUDIO2
HRESULT Win32XAudioInit(win32_audio_buffer* AudioBuffer)
{
    // Initialize a COM:
    HRESULT HRes;
    HRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(HRes)) { return(HRes); }

    // Init XAUDIO Engine
    AudioBuffer->XEngine = {};
    if (FAILED(HRes = XAudio2Create(&AudioBuffer->XEngine, 0, XAUDIO2_DEFAULT_PROCESSOR)))
    {
        return HRes;
    }

    // MASTER VOICE
    IXAudio2MasteringVoice* XAudioMasterVoice = nullptr;
    if (FAILED(HRes = AudioBuffer->XEngine->CreateMasteringVoice(&XAudioMasterVoice)))
    {
        return HRes;
    }

    AudioBuffer->WaveFormat = {};

    //int32 SamplesPerBuffer = 44100;
    unsigned int SampleHz = Sounds::sampleRate;
    WORD Channels = 1;
    WORD BitsPerChannel = 32; // 4 byte samples
    int BufferSize = Channels * BitsPerChannel * SampleHz;
    AudioBuffer->BytesPerBuffer = SampleHz * Channels;

    AudioBuffer->WaveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT; // or could use WAVE_FORMAT_PCM WAVE_FORMAT_IEEE_FLOAT
    AudioBuffer->WaveFormat.nChannels = Channels;
    AudioBuffer->WaveFormat.nSamplesPerSec = SampleHz;
    AudioBuffer->WaveFormat.wBitsPerSample = BitsPerChannel; // 32
    AudioBuffer->WaveFormat.nBlockAlign = (Channels * BitsPerChannel) / 8;
    AudioBuffer->WaveFormat.nAvgBytesPerSec = SampleHz * Channels * BitsPerChannel / 8;
    AudioBuffer->WaveFormat.cbSize = 0;    // set to zero for PCM or IEEE float

    AudioBuffer->XBuffer.Flags = XAUDIO2_END_OF_STREAM;
    AudioBuffer->XBuffer.AudioBytes = SampleHz * Channels * BitsPerChannel / 8;
    AudioBuffer->XBuffer.PlayBegin = 0;
    AudioBuffer->XBuffer.PlayLength = 0;
    AudioBuffer->XBuffer.LoopBegin = 0;
    AudioBuffer->XBuffer.LoopLength = 0;
    AudioBuffer->XBuffer.LoopCount = 0;
    AudioBuffer->XBuffer.pContext = NULL;
    AudioBuffer->XBuffer.pAudioData = (BYTE*)&AudioBuffer->Memory;

    if (FAILED(HRes = AudioBuffer->XEngine->CreateSourceVoice(&AudioBuffer->SourceVoice, (WAVEFORMATEX*)&AudioBuffer->WaveFormat)))
    {
        return HRes;
    }

    if (FAILED(HRes = AudioBuffer->SourceVoice->SubmitSourceBuffer(&AudioBuffer->XBuffer)))
    {
        return HRes;
    }

    AudioBuffer->SourceVoice->SetVolume(0.1f);

    return S_OK;
}

const char* Sounds::GetKeyName(Key key, Scale scale)
{
        switch (key)
        {
        case Key_As: return "A#"; break;
        case Key_Bb: return "Bb"; break;
        case Key_B: return "B"; break;
        case Key_Bs: return "B#"; break;
        case Key_Cb: return "Cb";break;
        case Key_C: return "C";break;
        case Key_Cs: return "C#";break;
        case Key_Db: return "Db";break;
        case Key_D: return "D";break;
        case Key_Ds: return "D#";break;
        case Key_Eb: return "Eb";break;
        case Key_E: return "E";break;
        case Key_Es: return "E#";break;
        case Key_Fb: return "Fb";break;
        case Key_F: return "F";break;
        case Key_Fs: return "F#";break;
        case Key_Gb: return "Gb";break;
        case Key_G: return "G";break;
        case Key_Gs: return "G#";break;
        case Key_Ab: return "Ab";break;
        case Key_A: return "A";break;
        default: return "";break;
        }
    //if(!isFlat)
    //    switch (idx)
    //    {
    //    case A: return "A"; break;
    //    case As: return "A#"; break;
    //    case B: return "B"; break;
    //    case C: return "C"; break;
    //    case Cs: return "C#"; break;
    //    case D: return "D"; break;
    //    case Ds: return "D#"; break;
    //    case E: return "E"; break;
    //    case F: return "F"; break;
    //    case Fs: return "F#"; break;
    //    case G: return "G"; break;
    //    case Gs: return "G#"; break;
    //    default: break;
    //    }
    //else
    //    switch (idx)
    //    {
    //    case A: return "A"; break;
    //    case D: return "D"; break;
    //    case G: return "G"; break;
    //    case C: return "C"; break;
    //    case Bb: return "Bb"; break;
    //    //case Bs: return "B#"; break;
    //    case Cb: return "Cb"; break;
    //    case Db: return "Db"; break;
    //    case Eb: return "Eb"; break;
    //    case Fb: return "Fb"; break;
    //    case Es: return "E#"; break;
    //    case Gb: return "Gb"; break;
    //    case Ab: return "Ab"; break;
    //    default:break;
    //    }
}

Note Sounds::Key2Note(Key key)
{
    switch (key)
    {   
    case Key_A: return (Note)A; break;
    case Key_As: return (Note)As; break;
    case Key_Bb: return (Note)Bb; break;
    case Key_B: return (Note)B; break;
    case Key_Bs: return (Note)Bs; break;
    case Key_Cb: return (Note)Cb; break;
    case Key_C: return (Note)C; break;
    case Key_Cs: return (Note)Cs; break;
    case Key_Db: return (Note)Db; break;
    case Key_D: return (Note)D; break;
    case Key_Ds: return (Note)Ds; break;
    case Key_Eb: return (Note)Eb; break;
    case Key_E: return (Note)E; break;
    case Key_Es: return (Note)Es; break;
    case Key_Fb: return (Note)Fb; break;
    case Key_F: return (Note)F; break;
    case Key_Fs: return (Note)Fs; break;
    case Key_Gb: return (Note)Gb; break;
    case Key_G: return (Note)G; break;
    case Key_Gs: return (Note)Gs; break;
    case Key_Ab: return (Note)Ab; break;
    default: break;
    }
}

Key Sounds::Note2Key(Note note)
{
    switch (note)
    {
    case A: return Key_A; break;
    case As: return Key_As; break;
    case B: return Key_B; break;
    case C: return Key_C; break;
    case Cs: return Key_Cs; break;
    case D: return Key_D; break;
    case Ds: return Key_Ds; break;
    case E: return Key_E; break;
    case F: return Key_F; break;
    case Fs: return Key_Fs; break;
    case G: return Key_G; break;
    case Gs: return Key_Gs; break;
    default: break;
    }
}

void Sounds::GetKeySignature(Scale scale, Key baseKey, Key outSignature1[7])
{
    Key outSignature[7];

    // I guess I could also just remove these notes from the list itself, but this would introduce another mapping array and more calculations
    if (baseKey == Key_Bs || baseKey == Key_Es)
        baseKey = (Key)(baseKey + 2);
    else if(baseKey == Key_Cb || baseKey == Key_Fb)
        baseKey = (Key)(baseKey - 2);

    auto baseNote = Key2Note(baseKey);
    Note noteSignature[7]{};

    switch (scale)
    {
    case Major:
        noteSignature[0] = baseNote;
        noteSignature[1] = (Note)((baseNote + 2) % 12);
        noteSignature[2] = (Note)((baseNote + 4) % 12);
        noteSignature[3] = (Note)((baseNote + 5) % 12);
        noteSignature[4] = (Note)((baseNote + 7) % 12);
        noteSignature[5] = (Note)((baseNote + 9) % 12);
        noteSignature[6] = (Note)((baseNote + 11) % 12);
        break;
    case Minor:
        noteSignature[0] = baseNote;
        noteSignature[1] = (Note)((baseNote + 2) % 12);
        noteSignature[2] = (Note)((baseNote + 3) % 12);
        noteSignature[3] = (Note)((baseNote + 5) % 12);
        noteSignature[4] = (Note)((baseNote + 7) % 12);
        noteSignature[5] = (Note)((baseNote + 8) % 12);
        noteSignature[6] = (Note)((baseNote + 10) % 12);
        break;
    default:
        break;
    }

    //for (int i = 0; i < 7; i++)
    //    printf("Note %i: %s ", i, GetKeyName(Note2Key(noteSignature[i]), scale));
    //std::cout << std::endl;

    outSignature[0] = baseKey;

    bool isBaseKeySharp = (baseKey) % 3 == 1;
    bool isBaseKeyNeutral = ((baseKey) % 3 == 0);

    int retries = 0;

TryNewSignature:

    for (int i = 1; i < 7; i++)
        outSignature[i] = Note2Key(noteSignature[i]);

    for (int i = 1; i < 7; i++)
    {
        //if ((outSignature[i] + 1) % 3 == 0)
        //    continue;

        bool isSharp = (outSignature[i]) % 3 == 1;
        bool isNeutral = ((outSignature[i]) % 3 == 0);

        // Make sure the note has the same accidental as the base note
        //if (isSharp != isBaseKeySharp && !isNeutral && !isBaseKeyNeutral)
        if (((isSharp != isBaseKeySharp) || (isNeutral != isBaseKeyNeutral)) && !isNeutral && !isBaseKeyNeutral)
        {
            if (isSharp)
                outSignature[i] = (Key)abs((outSignature[i] + 1) % 21);
            else
                outSignature[i] = (Key)abs((outSignature[i] - 1) % 21);

            continue;
        }

        // Check if there are duplicate notes (ex. A and Ab, or Ab and A#)
        bool isHigher = isBaseKeySharp &&(std::find(outSignature, outSignature + 7, abs(((outSignature[i] + 1) + 21) % 21)) != outSignature + 7);
        bool isHigher2 = isBaseKeySharp &&(std::find(outSignature, outSignature + 7, abs(((outSignature[i] + 2) + 21) % 21)) != outSignature + 7) && (!isNeutral && !isSharp);
        bool isLower = (!isBaseKeyNeutral && !isBaseKeySharp) && (std::find(outSignature, outSignature + 7, abs(((outSignature[i] - 1) + 21) % 21)) != outSignature + 7);
        bool isLower2 = (!isBaseKeyNeutral && !isBaseKeySharp) && (std::find(outSignature, outSignature + 7, abs(((outSignature[i] - 2) + 21) % 21)) != outSignature + 7) && (!isNeutral && isSharp);

        if (isNeutral)
        {
            if (!(outSignature[i] == Key_F || outSignature[i] == Key_E || outSignature[i] == Key_C || outSignature[i] == Key_B))
                continue;
            // Move for example the note F to be E# if F# or Fb is taken. Same goes for E, C and B just with their respective neighbouring notes
            if (isHigher && isBaseKeySharp)
                outSignature[i] = (Key)abs((outSignature[i] - 2) % 21);
            if (isLower && !isBaseKeySharp)
                outSignature[i] = (Key)abs((outSignature[i] + 2) % 21);
        }
        else
        {
            // Try to avoid duplicates by moving for example a note G# to be Ab if there are no A, A#, or Ab notes.
            if ((isHigher || isHigher2) && !isSharp)// && !isNeutral)
                outSignature[i] = (Key)abs((outSignature[i] - (isHigher2 ? 1 : 1)) % 21);

            if ((isLower || isLower2) && isSharp)// && isNeutral)
                outSignature[i] = (Key)abs((outSignature[i] + (isLower2 ? 1 : 1)) % 21);
        }
    }

    // Check if is correct
    if (retries <= 2)
    {
        if(retries == 0)
            std::copy(outSignature, outSignature + 7, outSignature1);

        for (int i = 1; i < 7; i++)
        {
            bool isSharp = (outSignature[i]) % 3 == 1;
            bool isNeutral = ((outSignature[i]) % 3 == 0);

            bool isHigher = (std::find(outSignature, outSignature + 7, abs(((outSignature[i] + 1) + 21) % 21)) != outSignature + 7);
            bool isHigher2 = (std::find(outSignature, outSignature + 7, abs(((outSignature[i] + 2) + 21) % 21)) != outSignature + 7) && !isNeutral;
            bool isLower = (std::find(outSignature, outSignature + 7, abs(((outSignature[i] - 1) + 21) % 21)) != outSignature + 7);
            bool isLower2 = (std::find(outSignature, outSignature + 7, abs(((outSignature[i] - 2) + 21) % 21)) != outSignature + 7) && !isNeutral;

            // If is wrong
            if (((isHigher || isHigher2) && !isSharp) || ((isLower || isLower2) && isSharp))
            {
                if (retries < 2)
                {
                    if (!isBaseKeySharp)
                        outSignature[0] = (Key)abs((outSignature[0] + (isBaseKeySharp ? 1 : -1)) % 21);
                    isBaseKeySharp = !isBaseKeySharp;
                }
                else
                    return;
                if (isBaseKeyNeutral)
                {
                    isBaseKeySharp = false;
                    isBaseKeyNeutral = false;
                }
                retries++;
                // Try again with assuming a different accidental on a base note
                goto TryNewSignature;
            }
        }
    }
    // Good example of when the first run (assuming flats) is not correct is Db Minor. Trying to only use flats it would have to utilize double flat on B, and this is goofy ah, so It's better to just use sharps (assume it's C# minor).
    // But I can also see how this can be undesired behaviour. 

    //for (int i = 0; i < 7; i++)
    //{
    //    printf("Key %i: %s ", i, GetKeyName(outSignature[i], scale));
    //    outSignature1[i] = outSignature[i];
    //}
    //std::cout << std::endl;

    std::copy(outSignature, outSignature + 7, outSignature1);
}

double SawWave(double t, float frequency) {
    double fullPeriodTime = 1.0 / frequency;
    double localTime = fmod(t, fullPeriodTime);

    auto value = ((localTime / fullPeriodTime) * 2 - 1.0);
    return value;
};

static double _currentPhase = 1;

HRESULT CreateFunction(win32_audio_buffer* Buffer, int frequency, FunctionType funcType)
{
    float PI2 = 6.28318f;
    float phaseStep = ((PI2 / (double)Sounds::sampleRate) * frequency);
    float period = 1.f / frequency;

    for (int i = 0; i < Buffer->BytesPerBuffer; i++)
    {
        _currentPhase += phaseStep;
        switch(funcType)
        {
        case sine:
        {
            float CurrentSample = sin(_currentPhase);//sinf(i * PI2 / Sounds::sampleRate * frequency);
            Buffer->Memory[i] = CurrentSample;
            break;
        }

        case saw:  
        {
            float CurrentSample = ((fmod((double)i / Sounds::sampleRate, period) / period) * 2 - 1.0);
            Buffer->Memory[i] = CurrentSample;
            break;
        }
        }
    }

    return(S_OK);
}

void StopSound(DWORD dwMillis)
{
        Sleep(dwMillis);
        //std::this_thread::sleep_for(std::chrono::milliseconds(dwMillis));
        Sounds::AudioBuffer->SourceVoice->SetVolume(0);
        //Sounds::AudioBuffer->SourceVoice->Stop(0);
        //Sounds::AudioBuffer->SourceVoice->ExitLoop();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        //AudioBuffer->SourceVoice->SetVolume(0);
        
        //AudioBuffer->SourceVoice->Discontinuity();
        Sounds::AudioBuffer->SourceVoice->Stop(0);
        Sounds::AudioBuffer->SourceVoice->ExitLoop();

        Sounds::AudioBuffer->SourceVoice->FlushSourceBuffers();
        Sounds::AudioBuffer->SourceVoice->SubmitSourceBuffer(&Sounds::AudioBuffer->XBuffer);

        Sounds::AudioBuffer->SourceVoice->SetVolume(*Sounds::pVolume);

        Sounds::isPlaying = false;
}

void Sounds::PlaySound(Note note, DWORD dwMillis, FunctionType funcType, int octave)
{
    if (isPlaying)
        return;

	int realNote = (note + 1) + (octave * 12);

    float frequency = pow<float>(2, (realNote - 49.f) / 12.f) * 440.f;

    Sounds::AudioBuffer->SourceVoice->SetVolume(*Sounds::pVolume);

	printf("playing freq: %f, real Note: %i\n", frequency, realNote);

    CreateFunction(AudioBuffer, frequency, funcType);

    isPlaying = true;

    AudioBuffer->XBuffer.PlayBegin = sampleRate - (sampleRate / (1000 / (dwMillis + 29))); // The value of 29 just doesn't cause any clicks / cracks
    //AudioBuffer->XBuffer.LoopBegin = sampleRate - (sampleRate / (1000 / dwMillis));
    //AudioBuffer->SourceVoice->SetVolume(1);
    AudioBuffer->SourceVoice->Start(0);

    std::thread stopThread(StopSound, dwMillis);
    stopThread.detach();

}