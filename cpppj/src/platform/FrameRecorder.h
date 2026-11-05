// 4차 로컬 MCP의 영상 녹화기다. 게임 화면 프레임을 인덱스 PNG 순번 파일로 저장하고, 시간 정보 목록(manifest.json)과
// ffmpeg concat 목록(frames.ffconcat)을 함께 만든다. 8비트 팔레트 프레임을 팔레트째 보존하므로 원본 색이 손실되지 않는다.
// 원본에는 없는 새 장치이며 PNG 인코딩은 작업 스레드에서 해 게임 프레임을 막지 않는다.
#pragma once
#include <array>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace netstorm::platform { // 파일 형식/OS 경계의 범용 부품이다.
// 녹화 설정이다.
struct FrameRecorderOptions {
    std::filesystem::path directory; // 프레임/목록을 쓸 폴더(없으면 만든다)다.
    int fps{15};                     // 목표 캡처 속도(1..60)다. 게임이 더 느리면 그만큼만 저장한다.
    std::size_t maxQueued{8};        // 인코딩 대기 프레임 상한이다. 넘치면 새 프레임을 버리고 dropped로 센다.
};
// 녹화 결과 요약이다.
struct FrameRecorderResult {
    std::uint64_t frames{};            // 저장한 프레임 수다.
    std::uint64_t dropped{};           // 대기열 포화로 버린 프레임 수다.
    double seconds{};                  // 첫 프레임부터 마지막 프레임 끝까지의 길이(초)다.
    std::filesystem::path manifest;    // manifest.json 경로다.
    std::filesystem::path concat;      // frames.ffconcat 경로다.
    std::uint32_t width{}, height{};   // 프레임 크기다.
};
// 프레임 녹화기다. Start→Offer(매 게임 프레임)→Stop 순서로 쓴다. Offer/Start/Stop은 게임 스레드에서만 부른다.
class FrameRecorder {
public:
    FrameRecorder();
    ~FrameRecorder();
    FrameRecorder(const FrameRecorder&) = delete;
    FrameRecorder& operator=(const FrameRecorder&) = delete;
    // 폴더를 만들고 작업 스레드를 시작한다. 이미 녹화 중이거나 폴더를 만들 수 없으면 false이며 error에 이유를 쓴다.
    bool Start(const FrameRecorderOptions& options, std::string& error);
    // 현재 프레임을 후보로 낸다. nowSeconds는 단조 증가 시각(초)이다. 캡처 주기가 안 됐거나 대기열이 가득 차면 false다.
    // pixels는 pitch 간격의 8비트 화면이고 palette는 256개 RGB(768바이트)다. 내용은 복사하므로 호출 뒤 바뀌어도 된다.
    bool Offer(std::uint32_t width, std::uint32_t height, std::uint32_t pitch, std::span<const std::uint8_t> pixels, std::span<const std::uint8_t, 768> palette, double nowSeconds);
    // 이번 프레임을 캡처할 주기가 됐는지만 묻는다(불필요한 화면 복사를 피하기 위함). 상태를 바꾸지 않는다.
    bool Due(double nowSeconds) const;
    // 대기열을 모두 저장하고 manifest.json/frames.ffconcat을 쓴 뒤 결과를 돌려준다.
    FrameRecorderResult Stop();
    bool Running() const { return running_; } // 녹화 중인지다.
    const std::filesystem::path& Directory() const { return options_.directory; } // 현재(마지막) 녹화 폴더다.
private:
    struct Item { std::uint64_t index{}; double time{}; std::uint32_t width{}, height{}; std::vector<std::uint8_t> pixels; std::array<std::uint8_t, 768> palette{}; }; // 대기 프레임 하나다.
    void Worker();                                 // 작업 스레드 본문(PNG 인코딩+쓰기)이다.
    FrameRecorderOptions options_;                 // 현재 설정이다.
    bool running_{false};                          // 녹화 중 표시(게임 스레드 전용)다.
    double firstTime_{}, nextTime_{};              // 첫 프레임 시각과 다음 캡처 시각이다(게임 스레드 전용).
    bool hasFirst_{false};                         // 첫 프레임을 받았는지다.
    std::uint64_t accepted_{0}, dropped_{0};       // 받은/버린 프레임 수다(게임 스레드 전용).
    std::uint32_t width_{}, height_{};             // 마지막 프레임 크기다.
    std::mutex mutex_;                             // 대기열/작업 상태 보호다.
    std::condition_variable condition_;            // 작업 스레드 깨우기다.
    std::deque<Item> queue_;                       // 인코딩 대기열이다.
    bool stopping_{false};                         // 작업 스레드 종료 요청이다.
    std::vector<double> times_;                    // 저장한 프레임 시각(초, 첫 프레임 기준)이다. mutex_로 보호한다.
    std::uint64_t written_{0};                     // 저장 완료 프레임 수다. mutex_로 보호한다.
    std::string workerError_;                      // 작업 스레드가 겪은 첫 오류다. mutex_로 보호한다.
    std::thread worker_;                           // PNG 인코딩 스레드다.
};
}
