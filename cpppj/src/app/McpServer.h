// 4차 로컬 MCP 서버다. Netstorm.exe가 stdio(JSON-RPC 2.0, 한 줄 한 메시지)로 MCP 클라이언트에 게임 조작/화면 캡처/상태 조회를 제공한다.
// dotnetpj 개발을 위한 게임 분석용이며 원본에는 없는 새 기능이다. 입력 어휘는 기존 UI 스크립트 엔진(`--ui-script`)과 같다.
#pragma once
#include "platform/AudioLoopback.h"
#include "platform/FrameRecorder.h"
#include "platform/Json.h"
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace netstorm::client { class Client; } // 게임 스레드에서만 접근하는 실제 클라이언트다.

namespace netstorm::app { // 앱 계층의 검사/분석 구동기다.
// 바이트열을 표준 Base64로 인코딩한다(MCP 이미지 콘텐츠용).
std::string McpBase64(std::span<const std::uint8_t> bytes);

// tools/call 한 건이다. 게임 스레드가 처리한 뒤 같은 id로 응답한다.
struct McpToolCall {
    platform::Json id;         // 요청 id(숫자/문자열)다. 응답에 그대로 돌려준다.
    std::string name;          // 호출할 도구 이름이다.
    platform::Json arguments;  // 도구 인자 객체다(없으면 빈 객체).
};

// 전송과 무관한 MCP 프로토콜 처리기다. 게임/스레드를 모르므로 단위 검사할 수 있다.
class McpProtocol {
public:
    using Enqueue = std::function<void(McpToolCall)>; // 유효한 tools/call을 게임 스레드 대기열에 넣는 장치다.
    // 메시지 하나를 처리한다. 즉시 보낼 응답이 있으면 돌려주고, 알림이거나 대기열로 넘긴 tools/call이면 nullopt다.
    std::optional<platform::Json> Handle(const platform::Json& message, const Enqueue& enqueue) const;
    // 도구 목록(이름/설명/입력 스키마)이다.
    static platform::Json ToolsList();
    // 성공/오류 도구 결과(텍스트)를 JSON-RPC 응답으로 만든다.
    static platform::Json TextResult(const platform::Json& id, std::string text, bool isError = false);
    // 텍스트와 PNG 이미지를 함께 담은 도구 결과다.
    static platform::Json ImageResult(const platform::Json& id, std::string text, std::span<const std::uint8_t> png);
    // JSON-RPC 오류 응답이다.
    static platform::Json Error(const platform::Json& id, int code, std::string message);
};

// 도구 하나의 진행 상태다. 게임 프레임을 기다리는 호출을 위해 응답을 미룬다.
struct McpPending {
    McpToolCall call;                          // 원래 요청이다.
    std::deque<std::pair<std::string, std::string>> actions; // 아직 실행하지 않은 입력 동작(op, arg)이다.
    std::uint64_t nextActionFrame{};           // 다음 입력 동작을 실행할 프레임이다.
    std::uint64_t completeFrame{};             // 이 프레임 이후(afterFrame)에 응답한다. 입력 동작이 모두 끝난 뒤 계산한다.
    bool waitAfterActions{};                   // 입력 동작이 끝난 뒤 추가 대기 프레임 수를 더해야 하는지다.
    std::uint64_t extraFrames{};               // 마지막 동작 뒤 더 기다릴 프레임 수다.
    bool screenshot{};                         // 응답에 화면 PNG를 포함할지다.
    std::uint64_t actedFrame{~0ULL};           // 마지막으로 입력 동작을 내보낸 프레임이다(입력 오류를 그 호출에 귀속시키기 위함).
    std::string error;                         // 입력 실행 중 오류 메시지다. 비어 있지 않으면 오류로 응답한다.
};

// stdio MCP 서버다. 읽기 스레드는 메시지를 해석하고, 게임 스레드가 프레임 훅에서 도구를 실행한다.
class McpServer {
public:
    McpServer();
    ~McpServer();
    McpServer(const McpServer&) = delete;
    McpServer& operator=(const McpServer&) = delete;
    // stdout을 MCP 전용으로 분리(CRT stdout은 stderr로 우회)하고 stdin 읽기 스레드를 시작한다. 한 번만 호출한다.
    void Start();
    // 게임 스레드, 프레임 입력 단계 직전: 새 요청을 받아 즉답 도구를 처리하고 이번 프레임에 실행할 입력 동작(op, arg)을 돌려준다.
    std::vector<std::pair<std::string, std::string>> BeforeInput(client::Client& client);
    // 게임 스레드, 프레임 그리기 뒤: 화면 캡처/대기 완료를 처리해 응답을 보낸다.
    void AfterFrame(client::Client& client);
    // 게임 스레드: 이번 프레임의 입력 동작 실행이 실패했을 때 해당 호출들을 오류로 끝낸다(남은 동작은 버린다).
    void ReportInputError(client::Client& client, const std::string& message);
    // stdin이 닫혔거나 quit 도구가 호출되어 게임을 끝내야 하는지다.
    bool QuitRequested() const;
private:
    void ReadLoop();                                       // stdin을 한 줄씩 읽어 처리하는 스레드 본문이다.
    void Send(const platform::Json& message);              // 한 줄 JSON을 stdout에 쓴다(스레드 안전).
    void Finish(const McpPending& pending, client::Client& client); // 대기 중인 호출에 최종 응답을 보낸다.
    platform::Json StatusJson(client::Client& client) const;   // 게임 상태 요약 객체다.
    std::vector<std::uint8_t> ScreenPng(client::Client& client) const; // 현재 논리 화면을 PNG로 만든다.
    std::vector<std::uint8_t> PresentedPng(client::Client& client) const; // 7차: GPU 후처리가 실제 표시한 클라이언트 영역을 RGBA PNG로 만든다(꺼져 있으면 논리 화면).
    platform::Json StartRecording(client::Client& client, const platform::Json& arguments); // 녹화 시작(영상+소리)이다. 결과 정보 객체를 돌려준다.
    platform::Json StopRecording(const platform::Json& arguments);                          // 녹화 종료와 선택적 ffmpeg 합성이다.
    platform::FrameRecorder recorder_;                     // 영상 프레임 녹화기다(게임 스레드 전용).
    std::unique_ptr<platform::ProcessAudioCapture> audio_; // 소리 캡처다(녹화 중에만 있다).
    std::string audioError_;                               // 소리 캡처를 시작하지 못한 이유다(영상만 녹화).
    McpProtocol protocol_;                                 // 프로토콜 처리기다.
    std::thread reader_;                                   // stdin 읽기 스레드다.
    std::mutex queueMutex_;                                // 요청 대기열 보호다.
    std::deque<McpToolCall> queue_;                        // 게임 스레드가 처리할 호출 목록이다.
    std::mutex writeMutex_;                                // stdout 쓰기 직렬화다.
    void* output_{};                                       // MCP 전용으로 복제한 stdout 핸들(HANDLE)이다.
    std::vector<McpPending> pending_;                      // 응답을 미룬 호출이다(게임 스레드 전용).
    bool quitCalled_{};                                    // quit 도구가 호출되었는지다(게임 스레드 전용).
    bool inputClosed_{};                                   // stdin이 EOF/오류로 닫혔는지다(읽기 스레드가 쓰고 queueMutex_로 보호).
};
}
