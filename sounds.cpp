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

    return(S_OK);
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