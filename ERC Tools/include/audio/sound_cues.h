#pragma once

#include <string>
#include <vector>

enum class SoundCue
{
    Message,
    PrivateMessage,
    Notification,
    TimerStart,
    TimerWarning,
    TimerComplete
};

inline constexpr unsigned int kDefaultSoundOutputDeviceId = 0xFFFFFFFFu;

struct SoundOutputDevice
{
    unsigned int id = kDefaultSoundOutputDeviceId;
    std::wstring name;
    bool isDefault = false;
};

std::vector<SoundOutputDevice> EnumerateSoundOutputDevices();
void SetSoundCuesEnabled(bool enabled);
bool SoundCuesEnabled();
void SetSoundCueEnabled(SoundCue cue, bool enabled);
bool SoundCueEnabled(SoundCue cue);
void SetSoundOutputDeviceId(unsigned int deviceId);
unsigned int SoundOutputDeviceId();
void PlaySoundCue(SoundCue cue);
