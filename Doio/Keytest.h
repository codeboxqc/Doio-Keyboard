#pragma once

#include <vector>
#include <deque>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

struct KeyPressEvent {
    int         vkCode;        // Windows Virtual Key code
    std::string name;          // printable name
    bool        isDown;        // true = pressed, false = released
    float       timestamp;     // seconds since app start
};

struct KeyDef {
    int vk;
    const char* label;
    float x;
    float y;
    float w;
    float h;
    int freq;
};

class KeyTester {
public:
    KeyTester();
    ~KeyTester();

    void PollKeyTester(float appStartTime);
    void RenderKeyTesterPanel();

private:
    bool                       m_testerActive = false;
    std::vector<bool>          m_vkDown;          // vkDown[vk] = currently held
    std::deque<KeyPressEvent>  m_keyLog;           // rolling log
    std::vector<float>         m_vkAnimTime;      // Last press time for animation

    static const int           kLogMaxSize = 128;
    static std::string VkName(int vk);

    // Audio thread management
    void AudioThreadFunc();
    std::thread m_audioThread;
    std::atomic<bool> m_audioThreadExit;
    std::mutex m_audioMutex;
    int m_currentFreq;
    std::atomic<bool> m_beepActive;
};
