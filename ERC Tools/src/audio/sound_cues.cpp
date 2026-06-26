#include "audio/sound_cues.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

#pragma comment(lib, "winmm.lib")

namespace
{
    constexpr size_t kCueCount = 6;

    std::atomic<bool> g_soundCuesEnabled{ true };
    std::mutex g_soundSettingsMutex;
    std::array<bool, kCueCount> g_cueEnabled{ true, true, true, true, true, true };
    unsigned int g_soundOutputDeviceId = kDefaultSoundOutputDeviceId;

    struct WavData
    {
        WAVEFORMATEX format{};
        std::vector<BYTE> samples;
        DWORD durationMs = 1000;
    };

    void CALLBACK WaveDoneCallback(HWAVEOUT, UINT message, DWORD_PTR instance, DWORD_PTR, DWORD_PTR)
    {
        if (message == WOM_DONE && instance != 0)
            SetEvent(reinterpret_cast<HANDLE>(instance));
    }

    size_t CueIndex(SoundCue cue)
    {
        switch (cue) {
        case SoundCue::Message:
            return 0;
        case SoundCue::PrivateMessage:
            return 1;
        case SoundCue::Notification:
            return 2;
        case SoundCue::TimerStart:
            return 3;
        case SoundCue::TimerWarning:
            return 4;
        case SoundCue::TimerComplete:
            return 5;
        default:
            return 0;
        }
    }

    const wchar_t* CueFilename(SoundCue cue)
    {
        switch (cue) {
        case SoundCue::Message:
            return L"message.wav";
        case SoundCue::PrivateMessage:
            return L"private_message.wav";
        case SoundCue::Notification:
            return L"notification.wav";
        case SoundCue::TimerStart:
            return L"timer_start.wav";
        case SoundCue::TimerWarning:
            return L"timer_warning.wav";
        case SoundCue::TimerComplete:
            return L"timer_complete.wav";
        default:
            return L"notification.wav";
        }
    }

    std::filesystem::path ExecutableDirectory()
    {
        std::array<wchar_t, MAX_PATH> path{};
        const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0 || length >= path.size())
            return std::filesystem::current_path();
        return std::filesystem::path(path.data()).parent_path();
    }

    std::filesystem::path CuePath(SoundCue cue)
    {
        return ExecutableDirectory() / L"assets" / L"sounds" / CueFilename(cue);
    }

    bool ReadBytes(std::ifstream& stream, void* destination, std::streamsize count)
    {
        stream.read(static_cast<char*>(destination), count);
        return stream.gcount() == count;
    }

    bool ReadChunkId(std::ifstream& stream, char (&id)[4])
    {
        return ReadBytes(stream, id, 4);
    }

    std::uint32_t ReadU32LE(std::ifstream& stream, bool& ok)
    {
        std::array<unsigned char, 4> bytes{};
        ok = ReadBytes(stream, bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!ok)
            return 0;
        return static_cast<std::uint32_t>(bytes[0])
            | (static_cast<std::uint32_t>(bytes[1]) << 8)
            | (static_cast<std::uint32_t>(bytes[2]) << 16)
            | (static_cast<std::uint32_t>(bytes[3]) << 24);
    }

    std::uint16_t ReadU16LE(const std::vector<unsigned char>& bytes, size_t offset)
    {
        return static_cast<std::uint16_t>(bytes[offset])
            | static_cast<std::uint16_t>(bytes[offset + 1] << 8);
    }

    std::uint32_t ReadU32LE(const std::vector<unsigned char>& bytes, size_t offset)
    {
        return static_cast<std::uint32_t>(bytes[offset])
            | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
            | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
            | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
    }

    bool ChunkEquals(const char (&id)[4], const char* expected)
    {
        return std::memcmp(id, expected, 4) == 0;
    }

    bool LoadPcmWav(const std::filesystem::path& path, WavData& wav)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
            return false;

        char riff[4]{};
        char wave[4]{};
        bool ok = false;
        if (!ReadChunkId(stream, riff))
            return false;
        (void)ReadU32LE(stream, ok);
        if (!ok || !ReadChunkId(stream, wave))
            return false;
        if (!ChunkEquals(riff, "RIFF") || !ChunkEquals(wave, "WAVE"))
            return false;

        bool foundFormat = false;
        bool foundData = false;

        while (stream && (!foundFormat || !foundData)) {
            char chunkId[4]{};
            if (!ReadChunkId(stream, chunkId))
                break;

            const std::uint32_t chunkSize = ReadU32LE(stream, ok);
            if (!ok)
                break;

            if (ChunkEquals(chunkId, "fmt ")) {
                std::vector<unsigned char> formatBytes(chunkSize);
                if (chunkSize < 16 || !ReadBytes(stream, formatBytes.data(), chunkSize))
                    return false;

                wav.format.wFormatTag = ReadU16LE(formatBytes, 0);
                wav.format.nChannels = ReadU16LE(formatBytes, 2);
                wav.format.nSamplesPerSec = ReadU32LE(formatBytes, 4);
                wav.format.nAvgBytesPerSec = ReadU32LE(formatBytes, 8);
                wav.format.nBlockAlign = ReadU16LE(formatBytes, 12);
                wav.format.wBitsPerSample = ReadU16LE(formatBytes, 14);
                wav.format.cbSize = 0;

                if (wav.format.wFormatTag != WAVE_FORMAT_PCM || wav.format.nAvgBytesPerSec == 0)
                    return false;
                foundFormat = true;
            } else if (ChunkEquals(chunkId, "data")) {
                wav.samples.resize(chunkSize);
                if (chunkSize > 0 && !ReadBytes(stream, wav.samples.data(), chunkSize))
                    return false;
                foundData = true;
            } else {
                stream.seekg(chunkSize, std::ios::cur);
                if (!stream)
                    return false;
            }

            if ((chunkSize & 1u) != 0)
                stream.seekg(1, std::ios::cur);
        }

        if (!foundFormat || !foundData || wav.samples.empty())
            return false;

        wav.durationMs = static_cast<DWORD>(
            (static_cast<std::uint64_t>(wav.samples.size()) * 1000ull) / wav.format.nAvgBytesPerSec);
        wav.durationMs = std::max<DWORD>(wav.durationMs, 250);
        return true;
    }

    bool PlayWaveOnDevice(const std::filesystem::path& path, unsigned int deviceId)
    {
        WavData wav;
        if (!LoadPcmWav(path, wav))
            return false;

        HANDLE doneEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!doneEvent)
            return false;

        HWAVEOUT waveOut = nullptr;
        MMRESULT result = waveOutOpen(
            &waveOut,
            deviceId,
            &wav.format,
            reinterpret_cast<DWORD_PTR>(&WaveDoneCallback),
            reinterpret_cast<DWORD_PTR>(doneEvent),
            CALLBACK_FUNCTION);
        if (result != MMSYSERR_NOERROR) {
            CloseHandle(doneEvent);
            return false;
        }

        WAVEHDR header{};
        header.lpData = reinterpret_cast<LPSTR>(wav.samples.data());
        header.dwBufferLength = static_cast<DWORD>(wav.samples.size());

        result = waveOutPrepareHeader(waveOut, &header, sizeof(header));
        if (result != MMSYSERR_NOERROR) {
            waveOutClose(waveOut);
            CloseHandle(doneEvent);
            return false;
        }

        result = waveOutWrite(waveOut, &header, sizeof(header));
        if (result == MMSYSERR_NOERROR) {
            const DWORD timeoutMs = wav.durationMs + 3000;
            if (WaitForSingleObject(doneEvent, timeoutMs) != WAIT_OBJECT_0)
                waveOutReset(waveOut);
        } else {
            waveOutReset(waveOut);
        }

        while (waveOutUnprepareHeader(waveOut, &header, sizeof(header)) == WAVERR_STILLPLAYING) {
            waveOutReset(waveOut);
            Sleep(5);
        }

        waveOutClose(waveOut);
        CloseHandle(doneEvent);
        return result == MMSYSERR_NOERROR;
    }

    unsigned int SelectedOutputDeviceId()
    {
        std::lock_guard<std::mutex> lock(g_soundSettingsMutex);
        return g_soundOutputDeviceId;
    }

    bool CueAllowed(SoundCue cue)
    {
        if (!g_soundCuesEnabled.load())
            return false;

        std::lock_guard<std::mutex> lock(g_soundSettingsMutex);
        return g_cueEnabled[CueIndex(cue)];
    }
}

std::vector<SoundOutputDevice> EnumerateSoundOutputDevices()
{
    std::vector<SoundOutputDevice> devices;
    devices.push_back({ kDefaultSoundOutputDeviceId, L"Windows default output", true });

    const UINT count = waveOutGetNumDevs();
    for (UINT i = 0; i < count; ++i) {
        WAVEOUTCAPSW caps{};
        if (waveOutGetDevCapsW(i, &caps, sizeof(caps)) != MMSYSERR_NOERROR)
            continue;

        std::wstring name = caps.szPname;
        if (name.empty())
            name = L"Audio output " + std::to_wstring(i + 1);

        devices.push_back({ static_cast<unsigned int>(i), name, false });
    }

    return devices;
}

void SetSoundCuesEnabled(bool enabled)
{
    g_soundCuesEnabled.store(enabled);
    if (!enabled)
        PlaySoundW(nullptr, nullptr, 0);
}

bool SoundCuesEnabled()
{
    return g_soundCuesEnabled.load();
}

void SetSoundCueEnabled(SoundCue cue, bool enabled)
{
    std::lock_guard<std::mutex> lock(g_soundSettingsMutex);
    g_cueEnabled[CueIndex(cue)] = enabled;
}

bool SoundCueEnabled(SoundCue cue)
{
    std::lock_guard<std::mutex> lock(g_soundSettingsMutex);
    return g_cueEnabled[CueIndex(cue)];
}

void SetSoundOutputDeviceId(unsigned int deviceId)
{
    std::lock_guard<std::mutex> lock(g_soundSettingsMutex);
    g_soundOutputDeviceId = deviceId;
}

unsigned int SoundOutputDeviceId()
{
    std::lock_guard<std::mutex> lock(g_soundSettingsMutex);
    return g_soundOutputDeviceId;
}

void PlaySoundCue(SoundCue cue)
{
    if (!CueAllowed(cue))
        return;

    const auto path = CuePath(cue);
    if (!std::filesystem::is_regular_file(path)) {
        const std::wstring message = L"ERC Tools sound asset missing: " + path.wstring() + L"\n";
        OutputDebugStringW(message.c_str());
        return;
    }

    const unsigned int deviceId = SelectedOutputDeviceId();
    if (deviceId != kDefaultSoundOutputDeviceId) {
        std::thread([path, deviceId]() {
            if (!PlayWaveOnDevice(path, deviceId)) {
                const std::wstring message = L"ERC Tools selected audio device failed, falling back to default: "
                    + path.wstring() + L"\n";
                OutputDebugStringW(message.c_str());
                PlaySoundW(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
            }
        }).detach();
        return;
    }

    PlaySoundW(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}
