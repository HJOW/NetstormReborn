// 4차 로컬 MCP 서버 구현이다. 프로토콜 처리(McpProtocol)와 stdio/게임 스레드 연결(McpServer)을 나눠 앞쪽만 게임 없이 검사한다.
#include "app/McpServer.h"
#include "client/ClientMain.h"
#include "client/DisplayTextInput.h" // UTF-8 text 동작을 실행 전에 전체 검증한다.
#include "client/Screen.h"
#include "client/UberGump.h"
#include "platform/Png.h"
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <set>

namespace netstorm::app { // 앱 계층의 검사/분석 구동기다.
using platform::Json;
namespace { // 도구 이름/허용 동작 표와 문서 문자열이다.
constexpr const char* kServerName = "netstorm-cpp";                                  // MCP serverInfo.name이다.
constexpr const char* kDefaultProtocol = "2024-11-05";                               // 클라이언트가 요청하지 않을 때 쓰는 프로토콜 버전이다.
constexpr std::uint64_t kMaxWaitFrames = 36000;                                     // 한 번의 대기 호출이 허용하는 최대 프레임 수다.
constexpr std::size_t kMaxActions = 200;                                            // 한 번의 입력 호출이 허용하는 최대 동작 수다.
// 입력 도구가 허용하는 UI 스크립트 동작이다. 검사 전용(test-*)/상태 합성 동작은 노출하지 않는다.
const std::set<std::string> kAllowedOperations{"click", "click-if", "down", "up", "up-if", "right", "outside", "pointer", "esc", "key", "keyup", "text", "script-command", "command"};
// 도구 이름 목록이다(tools/list와 tools/call 검증이 같은 표를 쓴다).
const std::set<std::string> kToolNames{"netstorm_status", "netstorm_report", "netstorm_screenshot", "netstorm_input", "netstorm_wait_frames", "netstorm_record_start", "netstorm_record_stop", "netstorm_quit"};

// 속성 하나의 JSON 스키마 조각을 만든다.
Json Property(const char* type, const char* description) { Json property; property.Set("type", type).Set("description", description); return property; }
// 도구 하나의 설명자를 만든다.
Json Tool(const char* name, const char* description, Json properties, Json required = Json::MakeArray()) {
    Json schema; schema.Set("type", "object").Set("properties", std::move(properties)).Set("required", std::move(required));
    Json tool; tool.Set("name", name).Set("description", description).Set("inputSchema", std::move(schema));
    return tool;
}
// 응답 봉투(jsonrpc/id/result)를 만든다.
Json Envelope(const Json& id, Json result) { Json response; response.Set("jsonrpc", "2.0").Set("id", id).Set("result", std::move(result)); return response; }
}

std::string McpBase64(std::span<const std::uint8_t> bytes) {
    static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((bytes.size() + 2) / 3 * 4);
    for (std::size_t index = 0; index < bytes.size(); index += 3) { // 세 바이트를 네 글자로 바꾼다.
        const std::uint32_t chunk = (static_cast<std::uint32_t>(bytes[index]) << 16) | (index + 1 < bytes.size() ? static_cast<std::uint32_t>(bytes[index + 1]) << 8 : 0U) | (index + 2 < bytes.size() ? bytes[index + 2] : 0U);
        out.push_back(alphabet[(chunk >> 18) & 63U]); out.push_back(alphabet[(chunk >> 12) & 63U]);
        out.push_back(index + 1 < bytes.size() ? alphabet[(chunk >> 6) & 63U] : '=');
        out.push_back(index + 2 < bytes.size() ? alphabet[chunk & 63U] : '=');
    }
    return out;
}

Json McpProtocol::ToolsList() {
    Json tools = Json::MakeArray();
    tools.Push(Tool("netstorm_status", "현재 게임 상태 요약(프레임 번호, 화면 크기, 단계/페이지)을 돌려준다.", Json::MakeObject()));
    Json reportProperties; reportProperties.Set("camera_only", Property("boolean", "true이면 큰 월드 덤프를 생략한 간결한 카메라 관측만 돌려준다."));
    tools.Push(Tool("netstorm_report", "게임 내부 관측 보고서(메뉴/월드/카탈로그/카메라 등 탭 구분 텍스트)를 돌려준다.", std::move(reportProperties)));
    Json screenshotProperties; screenshotProperties.Set("presented", Property("boolean", "true이면 논리 화면 대신 GPU 후처리(안티앨리어싱/FSR 1.0)가 실제로 표시한 클라이언트 영역 전체를 RGBA PNG로 돌려준다. 후처리가 꺼져 있으면 논리 화면이다."));
    tools.Push(Tool("netstorm_screenshot", "현재 게임 화면을 PNG 이미지로 돌려준다(기본은 논리 화면 버퍼, 원본 팔레트 색 그대로).", std::move(screenshotProperties)));
    Json action; action.Set("type", "object").Set("properties", Json::MakeObject().Set("op", Property("string", "동작: click, click-if, down, up, up-if, right, outside, pointer, esc, key, keyup, text, script-command, command")).Set("arg", Property("string", "동작 인자. click은 컨트롤 라벨 또는 screen:x,y, key는 문자/키 이름(예: enter, f5, shift+a), text는 UTF-8 확정 본문(제어문자 제외·최대39문자), script-command는 'Tell,Blank' 같은 원문 명령.")));
    Json actions; actions.Set("type", "array").Set("items", std::move(action)).Set("description", "프레임마다 하나씩 순서대로 실행할 입력 동작 목록이다.");
    Json inputProperties;
    inputProperties.Set("actions", std::move(actions)).Set("wait_frames", Property("integer", "마지막 동작 뒤 더 기다릴 프레임 수(기본 3).")).Set("screenshot", Property("boolean", "true이면 완료 뒤 화면 PNG를 함께 돌려준다(기본 true)."));
    Json required = Json::MakeArray(); required.Push("actions");
    tools.Push(Tool("netstorm_input", "마우스/키보드/원문 명령 입력을 게임 입력 큐에 프레임마다 하나씩 넣고 완료 뒤 상태(와 화면)를 돌려준다.", std::move(inputProperties), std::move(required)));
    Json waitProperties; waitProperties.Set("frames", Property("integer", "기다릴 프레임 수(1..36000).")).Set("screenshot", Property("boolean", "true이면 화면 PNG를 함께 돌려준다."));
    Json waitRequired = Json::MakeArray(); waitRequired.Push("frames");
    tools.Push(Tool("netstorm_wait_frames", "지정한 프레임 수만큼 게임이 진행되길 기다린 뒤 상태를 돌려준다.", std::move(waitProperties), std::move(waitRequired)));
    Json recordProperties;
    recordProperties.Set("fps", Property("integer", "영상 캡처 속도(1..60, 기본 15).")).Set("audio", Property("boolean", "true이면 게임 프로세스의 소리를 audio.wav로 함께 녹음한다(기본 true).")).Set("name", Property("string", "폴더 이름(영문/숫자/-/_). 생략하면 recording-날짜시각."));
    tools.Push(Tool("netstorm_record_start", "화면 프레임(인덱스 PNG 순번 파일)과 게임 소리(WAV, 프로세스 루프백)를 사용자 Captures 폴더에 녹화하기 시작한다.", std::move(recordProperties)));
    Json stopProperties; stopProperties.Set("mux", Property("boolean", "true이고 ffmpeg가 있으면 영상과 소리를 recording.mp4로 합친다."));
    tools.Push(Tool("netstorm_record_stop", "녹화를 끝내고 폴더/프레임 수/소리 통계를 돌려준다. manifest.json과 frames.ffconcat도 만든다.", std::move(stopProperties)));
    tools.Push(Tool("netstorm_quit", "게임 창을 닫고 서버를 끝낸다.", Json::MakeObject()));
    return tools;
}
Json McpProtocol::TextResult(const Json& id, std::string text, bool isError) {
    Json item; item.Set("type", "text").Set("text", std::move(text));
    Json content = Json::MakeArray(); content.Push(std::move(item));
    Json result; result.Set("content", std::move(content)).Set("isError", isError);
    return Envelope(id, std::move(result));
}
Json McpProtocol::ImageResult(const Json& id, std::string text, std::span<const std::uint8_t> png) {
    Json content = Json::MakeArray();
    Json textItem; textItem.Set("type", "text").Set("text", std::move(text)); content.Push(std::move(textItem));
    Json imageItem; imageItem.Set("type", "image").Set("data", McpBase64(png)).Set("mimeType", "image/png"); content.Push(std::move(imageItem));
    Json result; result.Set("content", std::move(content)).Set("isError", false);
    return Envelope(id, std::move(result));
}
Json McpProtocol::Error(const Json& id, int code, std::string message) {
    Json error; error.Set("code", code).Set("message", std::move(message));
    Json response; response.Set("jsonrpc", "2.0").Set("id", id).Set("error", std::move(error));
    return response;
}
std::optional<Json> McpProtocol::Handle(const Json& message, const Enqueue& enqueue) const {
    if (!message.IsObject()) return Error(Json(), -32600, "Invalid Request");
    const auto method = message["method"].AsString();
    if (method.empty() || !message.Has("id")) return std::nullopt; // 알림이거나 클라이언트가 보낸 응답이다.
    const auto& id = message["id"];
    if (method == "initialize") { // 클라이언트가 요청한 프로토콜 버전을 지원 목록에서 고른다.
        static const std::set<std::string> supported{"2024-11-05", "2025-03-26", "2025-06-18"};
        const auto requested = message["params"]["protocolVersion"].AsString(kDefaultProtocol);
        Json info; info.Set("name", kServerName).Set("version", "11.01");
        Json tools; tools.Set("listChanged", false);
        Json capabilities; capabilities.Set("tools", std::move(tools));
        Json result; result.Set("protocolVersion", supported.contains(requested) ? requested : std::string(kDefaultProtocol)).Set("capabilities", std::move(capabilities)).Set("serverInfo", std::move(info));
        return Envelope(id, std::move(result));
    }
    if (method == "ping") return Envelope(id, Json::MakeObject());
    if (method == "tools/list") { Json result; result.Set("tools", ToolsList()); return Envelope(id, std::move(result)); }
    if (method == "tools/call") {
        const auto name = message["params"]["name"].AsString();
        if (!kToolNames.contains(name)) return Error(id, -32602, "Unknown tool: " + name);
        Json arguments = message["params"]["arguments"];
        if (!arguments.IsObject()) arguments = Json::MakeObject();
        enqueue(McpToolCall{id, name, std::move(arguments)});
        return std::nullopt;
    }
    return Error(id, -32601, "Method not found: " + method);
}

McpServer::McpServer() = default;
McpServer::~McpServer() {
    if (reader_.joinable()) reader_.detach(); // stdin 읽기는 블록되어 있으므로 프로세스 종료에 맡긴다.
}

void McpServer::Start() {
    HANDLE duplicate{};
    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_OUTPUT_HANDLE), GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS); // MCP 메시지 전용 stdout 복제본이다.
    output_ = duplicate;
    std::fflush(stdout);
    _dup2(_fileno(stderr), _fileno(stdout)); // 게임의 printf/cout 로그가 MCP 스트림을 오염시키지 않도록 stderr로 보낸다.
    SetStdHandle(STD_OUTPUT_HANDLE, GetStdHandle(STD_ERROR_HANDLE));
    reader_ = std::thread([this] { ReadLoop(); });
}

void McpServer::Send(const Json& message) {
    auto line = message.Dump();
    line.push_back('\n');
    const std::lock_guard lock(writeMutex_);
    DWORD written{};
    std::size_t offset = 0;
    while (offset < line.size()) { // 짧은 쓰기에 대비해 모두 쓸 때까지 반복한다.
        if (!WriteFile(static_cast<HANDLE>(output_), line.data() + offset, static_cast<DWORD>(line.size() - offset), &written, nullptr) || !written) return; // 파이프가 끊기면 조용히 포기한다.
        offset += written;
    }
}

void McpServer::ReadLoop() {
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    std::string buffer;
    std::array<char, 8192> chunk{};
    const auto process = [this](std::string line) { // 한 줄(메시지 하나 또는 배치)을 해석해 응답/대기열로 보낸다.
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) return;
        const auto parsed = Json::Parse(line);
        if (!parsed) { Send(McpProtocol::Error(Json(), -32700, "Parse error")); return; }
        const auto enqueue = [this](McpToolCall call) { const std::lock_guard lock(queueMutex_); queue_.push_back(std::move(call)); };
        if (parsed->IsArray()) { for (const auto& item : parsed->Items()) if (const auto reply = protocol_.Handle(item, enqueue)) Send(*reply); return; }
        if (const auto reply = protocol_.Handle(*parsed, enqueue)) Send(*reply);
    };
    while (true) { // 입력이 끝날 때까지 읽는다.
        DWORD count{};
        if (!ReadFile(input, chunk.data(), static_cast<DWORD>(chunk.size()), &count, nullptr) || !count) break;
        buffer.append(chunk.data(), count);
        for (auto newline = buffer.find('\n'); newline != std::string::npos; newline = buffer.find('\n')) { // 완성된 줄마다 처리한다.
            process(buffer.substr(0, newline));
            buffer.erase(0, newline + 1);
        }
    }
    if (!buffer.empty()) process(buffer);
    const std::lock_guard lock(queueMutex_);
    inputClosed_ = true; // EOF이면 게임을 끝낸다.
}

bool McpServer::QuitRequested() const { return quitCalled_ || inputClosed_; }

Json McpServer::StatusJson(client::Client& client) const {
    Json status;
    status.Set("frame", client.Time().number);
    Json screen; screen.Set("width", client.GetScreen().Width()).Set("height", client.GetScreen().Height());
    status.Set("screen", std::move(screen));
    if (client.Menu()) { // 보고서의 phase/page 줄만 요약한다.
        const auto report = client.Menu()->Report();
        std::size_t begin = 0;
        while (begin < report.size()) { // 줄 단위로 훑어 첫 phase/page 값을 얻는다.
            const auto end = report.find('\n', begin);
            const auto line = report.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
            if (line.starts_with("phase\t")) status.Set("phase", line.substr(6));
            else if (line.starts_with("page\t")) status.Set("page", line.substr(5));
            if (end == std::string::npos) break;
            begin = end + 1;
        }
    }
    return status;
}

std::vector<std::uint8_t> McpServer::ScreenPng(client::Client& client) const {
    auto& screen = client.GetScreen();
    const auto width = static_cast<std::uint32_t>(screen.Width()), height = static_cast<std::uint32_t>(screen.Height()), pitch = static_cast<std::uint32_t>(screen.Pitch());
    const std::uint8_t* pixels = screen.Lock();
    std::array<std::uint8_t, 768> palette{};
    const auto colors = screen.Palette();
    for (std::size_t index = 0; index < 256 && index < colors.size(); ++index) { palette[index * 3] = colors[index].red; palette[index * 3 + 1] = colors[index].green; palette[index * 3 + 2] = colors[index].blue; }
    std::vector<std::uint8_t> png;
    try { png = platform::EncodePngIndexed(width, height, pitch, {pixels, static_cast<std::size_t>(pitch) * height}, palette); } catch (...) { screen.Unlock(); throw; }
    screen.Unlock();
    return png;
}

// 7차: GPU 후처리가 켜져 있으면 실제로 표시한 클라이언트 영역(BGRA)을 RGBA PNG로 만들고, 꺼져 있으면 논리 화면 PNG를 돌려준다.
std::vector<std::uint8_t> McpServer::PresentedPng(client::Client& client) const {
    std::vector<std::uint8_t> bgra;
    int width = 0, height = 0;
    if (!client.GetScreen().ReadPresented(bgra, width, height)) return ScreenPng(client);
    std::vector<std::uint8_t> rgba(bgra.size());
    for (std::size_t offset = 0; offset + 3 < bgra.size(); offset += 4) { // B,G,R,A 순서를 R,G,B,A로 바꾸고 알파는 불투명으로 둔다.
        rgba[offset] = bgra[offset + 2]; rgba[offset + 1] = bgra[offset + 1]; rgba[offset + 2] = bgra[offset]; rgba[offset + 3] = 255;
    }
    return platform::EncodePngRgba(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), rgba);
}

Json McpServer::StartRecording(client::Client& client, const Json& arguments) {
    Json info;
    if (recorder_.Running()) return info.Set("error", "이미 녹화 중입니다");
    std::string name = arguments["name"].AsString();
    if (name.empty()) { // 기본 이름은 시작 시각이다.
        const auto now = std::time(nullptr); std::tm local{}; localtime_s(&local, &now);
        char text[40]; std::strftime(text, sizeof(text), "recording-%Y%m%d-%H%M%S", &local); name = text;
    }
    for (const char ch : name) if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_')) return info.Set("error", "name은 영문/숫자/-/_만 허용합니다");
    const auto base = client.UserData() ? client.UserData()->Captures() : std::filesystem::temp_directory_path() / "netstorm-captures"; // 5차 저장 정책: 설치 폴더가 아닌 사용자 Captures다.
    platform::FrameRecorderOptions options;
    options.directory = base / std::filesystem::path(std::u8string(name.begin(), name.end()));
    options.fps = static_cast<int>(arguments["fps"].AsInt(15));
    std::string error;
    if (!recorder_.Start(options, error)) return info.Set("error", error);
    audioError_.clear();
    audio_.reset();
    if (arguments["audio"].AsBool(true)) { // 소리는 실패해도 영상 녹화는 계속한다.
        auto capture = std::make_unique<platform::ProcessAudioCapture>();
        if (capture->Start(GetCurrentProcessId(), options.directory / "audio.wav", audioError_)) audio_ = std::move(capture);
    }
    const auto text = options.directory.u8string();
    info.Set("directory", std::string(text.begin(), text.end())).Set("fps", options.fps).Set("audio", audio_ != nullptr);
    if (!audioError_.empty()) info.Set("audio_error", audioError_);
    return info;
}
Json McpServer::StopRecording(const Json& arguments) {
    Json info;
    if (!recorder_.Running()) return info.Set("error", "녹화 중이 아닙니다");
    platform::AudioCaptureStats audio{};
    const bool hadAudio = audio_ != nullptr;
    if (audio_) { audio = audio_->Stop(); audio_.reset(); }
    const auto directory = recorder_.Directory();
    const auto result = recorder_.Stop();
    const auto text = directory.u8string();
    info.Set("directory", std::string(text.begin(), text.end())).Set("frames", static_cast<std::int64_t>(result.frames)).Set("dropped", static_cast<std::int64_t>(result.dropped)).Set("duration_seconds", result.seconds);
    info.Set("width", static_cast<std::int64_t>(result.width)).Set("height", static_cast<std::int64_t>(result.height));
    if (hadAudio) {
        Json sound; sound.Set("seconds", audio.seconds).Set("frames", static_cast<std::int64_t>(audio.frames)).Set("silence_frames", static_cast<std::int64_t>(audio.silenceFrames)).Set("non_zero_frames", static_cast<std::int64_t>(audio.nonZeroFrames)).Set("sample_rate", static_cast<std::int64_t>(audio.sampleRate));
        info.Set("audio", std::move(sound));
    } else if (!audioError_.empty()) info.Set("audio_error", audioError_);
    if (arguments["mux"].AsBool(false)) { // ffmpeg가 있을 때만 영상+소리를 mp4로 합친다.
        wchar_t found[MAX_PATH]{};
        if (!SearchPathW(nullptr, L"ffmpeg.exe", nullptr, MAX_PATH, found, nullptr)) info.Set("mux_error", "ffmpeg.exe를 PATH에서 찾을 수 없음");
        else {
            const auto output = directory / "recording.mp4";
            std::wstring command = std::wstring(L"\"") + found + L"\" -y -loglevel error -f concat -safe 0 -i \"" + (directory / "frames.ffconcat").wstring() + L"\"";
            if (hadAudio) command += L" -i \"" + (directory / "audio.wav").wstring() + L"\"";
            command += L" -r 15 -c:v libx264 -pix_fmt yuv420p -vf scale=trunc(iw/2)*2:trunc(ih/2)*2";
            if (hadAudio) command += L" -c:a aac";
            command += L" \"" + output.wstring() + L"\"";
            STARTUPINFOW startup{}; startup.cb = sizeof(startup);
            PROCESS_INFORMATION process{};
            if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, directory.c_str(), &startup, &process)) info.Set("mux_error", "ffmpeg 실행 실패");
            else {
                const auto wait = WaitForSingleObject(process.hProcess, 180000);
                DWORD code = 1; GetExitCodeProcess(process.hProcess, &code);
                CloseHandle(process.hThread); CloseHandle(process.hProcess);
                if (wait == WAIT_OBJECT_0 && code == 0 && std::filesystem::exists(output)) { const auto out = output.u8string(); info.Set("mp4", std::string(out.begin(), out.end())); }
                else info.Set("mux_error", "ffmpeg 합성 실패(종료코드 " + std::to_string(code) + ")");
            }
        }
    }
    return info;
}
void McpServer::ReportInputError(client::Client& client, const std::string& message) {
    const std::uint64_t frame = client.Time().number;
    for (auto& pending : pending_) { // 이번 프레임에 동작을 내보낸 호출만 실패시킨다.
        if (pending.actedFrame != frame) continue;
        pending.error = message;
        pending.actions.clear();
        pending.completeFrame = frame;
    }
}
void McpServer::Finish(const McpPending& pending, client::Client& client) {
    if (!pending.error.empty()) { Send(McpProtocol::TextResult(pending.call.id, "입력 실행 오류: " + pending.error + "\n" + StatusJson(client).Dump(), true)); return; }
    const auto status = StatusJson(client).Dump();
    if (pending.screenshot) Send(McpProtocol::ImageResult(pending.call.id, status, pending.call.arguments["presented"].AsBool(false) ? PresentedPng(client) : ScreenPng(client)));
    else Send(McpProtocol::TextResult(pending.call.id, status));
}

std::vector<std::pair<std::string, std::string>> McpServer::BeforeInput(client::Client& client) {
    std::deque<McpToolCall> calls;
    {
        const std::lock_guard lock(queueMutex_);
        calls.swap(queue_);
    }
    const std::uint64_t frame = client.Time().number;
    for (auto& call : calls) { // 이번 프레임에 도착한 호출을 도구별로 시작한다.
        try {
            if (call.name == "netstorm_status") Send(McpProtocol::TextResult(call.id, StatusJson(client).Dump()));
            else if (call.name == "netstorm_report") Send(McpProtocol::TextResult(call.id, client.Menu() ? client.Menu()->Report(call.arguments["camera_only"].AsBool()) : std::string("메뉴 없음")));
            else if (call.name == "netstorm_screenshot") { McpPending pending; pending.call = std::move(call); pending.screenshot = true; pending.completeFrame = frame; pending_.push_back(std::move(pending)); }
            else if (call.name == "netstorm_wait_frames") {
                const auto frames = call.arguments["frames"].AsInt(0);
                if (frames < 1 || static_cast<std::uint64_t>(frames) > kMaxWaitFrames) { Send(McpProtocol::TextResult(call.id, "frames는 1..36000이어야 합니다", true)); continue; }
                McpPending pending; pending.screenshot = call.arguments["screenshot"].AsBool(false); pending.completeFrame = frame + static_cast<std::uint64_t>(frames); pending.call = std::move(call); pending_.push_back(std::move(pending));
            }
            else if (call.name == "netstorm_input") {
                const auto& list = call.arguments["actions"].Items();
                if (list.empty() || list.size() > kMaxActions) { Send(McpProtocol::TextResult(call.id, "actions는 1..200개여야 합니다", true)); continue; }
                McpPending pending;
                bool valid = true;
                for (const auto& item : list) { // 허용된 동작만 받는다.
                    const auto op = item["op"].AsString();
                    if (!kAllowedOperations.contains(op)) { Send(McpProtocol::TextResult(call.id, "허용되지 않는 동작: " + op, true)); valid = false; break; }
                    if (op=="text") { const auto events=netstorm::client::MakeCommittedTextEvents(item["arg"].AsString());if (events.size()>=netstorm::client::InputQueue::kCapacity) throw std::length_error("text는 한 동작에 최대39문자이며 더 긴 본문은 여러 동작으로 전달해야 함"); } // 잘못된 UTF-8/미지원 문자는 첫 동작 이전에 호출 전체를 거부한다.
                    pending.actions.emplace_back(op, item["arg"].AsString());
                }
                if (!valid) continue;
                pending.nextActionFrame = frame; pending.extraFrames = static_cast<std::uint64_t>(std::clamp<std::int64_t>(call.arguments["wait_frames"].AsInt(3), 0, static_cast<std::int64_t>(kMaxWaitFrames)));
                pending.screenshot = call.arguments["screenshot"].AsBool(true); pending.call = std::move(call); pending_.push_back(std::move(pending));
            }
            else if (call.name == "netstorm_record_start") { const auto info = StartRecording(client, call.arguments); Send(McpProtocol::TextResult(call.id, info.Dump(), info.Has("error"))); }
            else if (call.name == "netstorm_record_stop") { const auto info = StopRecording(call.arguments); Send(McpProtocol::TextResult(call.id, info.Dump(), info.Has("error"))); }
            else if (call.name == "netstorm_quit") { Send(McpProtocol::TextResult(call.id, "게임을 닫습니다")); quitCalled_ = true; client.RequestClose(); }
        } catch (const std::exception& error) { Send(McpProtocol::TextResult(call.id, std::string("도구 실행 오류: ") + error.what(), true)); }
    }
    std::vector<std::pair<std::string, std::string>> actions;
    for (auto& pending : pending_) { // 입력 호출은 프레임마다 동작 하나를 내보낸다.
        if (pending.actions.empty() || pending.nextActionFrame > frame) continue;
        actions.push_back(std::move(pending.actions.front()));
        pending.actedFrame = frame;
        pending.actions.pop_front();
        pending.nextActionFrame = frame + 1;
        if (pending.actions.empty()) pending.completeFrame = frame + pending.extraFrames; // 마지막 동작 뒤 추가 대기 프레임을 더한다.
    }
    return actions;
}

void McpServer::AfterFrame(client::Client& client) {
    const std::uint64_t frame = client.Time().number;
    if (recorder_.Running()) { // 녹화 중이면 캡처 주기가 된 프레임만 화면을 복사해 녹화기에 낸다.
        const double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        if (recorder_.Due(now)) {
            auto& screen = client.GetScreen();
            const auto width = static_cast<std::uint32_t>(screen.Width()), height = static_cast<std::uint32_t>(screen.Height()), pitch = static_cast<std::uint32_t>(screen.Pitch());
            const std::uint8_t* pixels = screen.Lock();
            std::array<std::uint8_t, 768> palette{};
            const auto colors = screen.Palette();
            for (std::size_t index = 0; index < 256 && index < colors.size(); ++index) { palette[index * 3] = colors[index].red; palette[index * 3 + 1] = colors[index].green; palette[index * 3 + 2] = colors[index].blue; }
            recorder_.Offer(width, height, pitch, {pixels, static_cast<std::size_t>(pitch) * height}, palette, now);
            screen.Unlock();
        }
    }
    for (auto it = pending_.begin(); it != pending_.end();) { // 완료 조건을 채운 호출에 응답한다.
        if (it->actions.empty() && frame >= it->completeFrame) {
            try { Finish(*it, client); } catch (const std::exception& error) { Send(McpProtocol::TextResult(it->call.id, std::string("응답 생성 오류: ") + error.what(), true)); }
            it = pending_.erase(it);
        } else ++it;
    }
    bool closed;
    { const std::lock_guard lock(queueMutex_); closed = inputClosed_ && queue_.empty(); }
    if (closed && pending_.empty() && !quitCalled_) { quitCalled_ = true; client.RequestClose(); } // stdin이 닫히고 할 일이 없으면 게임을 끝낸다.
}
}
