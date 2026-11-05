// 4차 로컬 MCP의 소리 포함 녹화용 오디오 캡처다. Windows 10 2004(빌드 19041) 이상의 프로세스 단위 WASAPI 루프백으로
// 지정한 프로세스(게임 자신)가 내보내는 소리만 WAV로 받는다. 원본에는 없는 새 장치이며 다른 앱의 소리는 섞이지 않는다.
#pragma once
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

namespace netstorm::platform { // OS 장치 경계다.
// 캡처 결과 통계다.
struct AudioCaptureStats {
    std::uint64_t frames{};        // WAV에 쓴 전체 표본 프레임 수(무음 보충 포함)다.
    std::uint64_t silenceFrames{}; // 그중 시간축을 맞추려고 채운 무음 프레임 수다.
    std::uint64_t nonZeroFrames{}; // 실제로 소리가 있던(0이 아닌 표본을 가진) 프레임 수다.
    std::uint32_t sampleRate{};    // 표본 주파수(Hz)다.
    double seconds{};              // 녹음 길이(초)다.
};
// 프로세스 루프백 캡처 하나다. Start부터 Stop까지 별도 스레드가 WAV를 쓴다.
class ProcessAudioCapture {
public:
    ProcessAudioCapture();
    ~ProcessAudioCapture();
    ProcessAudioCapture(const ProcessAudioCapture&) = delete;
    ProcessAudioCapture& operator=(const ProcessAudioCapture&) = delete;
    // processId 프로세스 트리의 출력을 wavPath(44.1kHz 16비트 스테레오 PCM)에 녹음한다. 실패하면 false이며 error에 이유를 쓴다.
    // 소리가 없는 구간은 시간축을 맞추도록 무음으로 채운다.
    bool Start(std::uint32_t processId, const std::filesystem::path& wavPath, std::string& error);
    // 캡처를 멈추고 WAV 머리를 마무리한 뒤 통계를 돌려준다. 시작하지 않았으면 빈 통계다.
    AudioCaptureStats Stop();
    bool Running() const { return running_.load(); } // 캡처 스레드가 도는 중인지다.
private:
    struct Impl;                         // COM 객체/스레드/파일을 숨기는 구현이다.
    std::unique_ptr<Impl> impl_;         // 구현 소유자다.
    std::atomic<bool> running_{false};   // 실행 표시다.
};
}
