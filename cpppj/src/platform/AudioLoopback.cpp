// 프로세스 단위 WASAPI 루프백 캡처 구현이다. 캡처 스레드가 COM 활성화부터 WAV 마무리까지 모두 맡는다.
#include "platform/AudioLoopback.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <initguid.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <audioclientactivationparams.h>
#include <objidl.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

namespace netstorm::platform { // OS 장치 경계다.
namespace { // 캡처 형식과 COM 보조다.
constexpr std::uint32_t kSampleRate = 44100;       // 녹음 주파수(Hz)다.
constexpr std::uint16_t kChannels = 2;             // 채널 수(스테레오)다.
constexpr std::uint16_t kBitsPerSample = 16;       // 표본 비트 수다.
constexpr std::uint32_t kBlockAlign = kChannels * kBitsPerSample / 8; // 프레임 하나의 바이트 수다.
constexpr std::uint64_t kMaxSilenceFrames = static_cast<std::uint64_t>(kSampleRate) * 3600; // 한 번에 채울 무음의 상한(1시간)이다.

// 프로세스 루프백 활성화 완료를 기다리는 COM 처리기다. 어느 스레드에서 불려도 되는 agile 객체다.
class ActivationHandler final : public IActivateAudioInterfaceCompletionHandler, public IAgileObject {
public:
    ActivationHandler() : event_(CreateEventW(nullptr, TRUE, FALSE, nullptr)) {}
    ~ActivationHandler() { if (event_) CloseHandle(event_); if (client_) client_->Release(); }
    // 완료를 기다린다. 성공하면 IAudioClient(참조 한 개 이전)를 돌려준다.
    IAudioClient* Wait(HRESULT& result) {
        if (WaitForSingleObject(event_, 5000) != WAIT_OBJECT_0) { result = HRESULT_FROM_WIN32(ERROR_TIMEOUT); return nullptr; }
        result = result_;
        auto* client = client_;
        client_ = nullptr; // 호출자가 해제한다.
        return client;
    }
    STDMETHODIMP QueryInterface(REFIID id, void** object) override {
        if (id == __uuidof(IUnknown) || id == __uuidof(IActivateAudioInterfaceCompletionHandler)) *object = static_cast<IActivateAudioInterfaceCompletionHandler*>(this);
        else if (id == __uuidof(IAgileObject)) *object = static_cast<IAgileObject*>(this);
        else { *object = nullptr; return E_NOINTERFACE; }
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references_)); }
    STDMETHODIMP_(ULONG) Release() override { const auto count = InterlockedDecrement(&references_); if (!count) delete this; return static_cast<ULONG>(count); }
    STDMETHODIMP ActivateCompleted(IActivateAudioInterfaceAsyncOperation* operation) override {
        HRESULT activate = E_FAIL;
        IUnknown* unknown = nullptr;
        result_ = operation->GetActivateResult(&activate, &unknown);
        if (SUCCEEDED(result_)) result_ = activate;
        if (SUCCEEDED(result_) && unknown) result_ = unknown->QueryInterface(__uuidof(IAudioClient), reinterpret_cast<void**>(&client_));
        if (unknown) unknown->Release();
        SetEvent(event_);
        return S_OK;
    }
private:
    LONG references_{1};       // COM 참조 수다.
    HANDLE event_{};           // 완료 신호다.
    HRESULT result_{E_FAIL};   // 활성화 결과다.
    IAudioClient* client_{};   // 활성화된 오디오 클라이언트다.
};

// 44바이트 PCM WAV 머리를 쓴다(크기는 마무리 때 채운다).
void WriteWavHeader(FILE* file, std::uint32_t dataBytes) {
    const std::uint32_t riffSize = 36 + dataBytes, formatSize = 16, byteRate = kSampleRate * kBlockAlign;
    const std::uint16_t pcm = 1;
    std::fwrite("RIFF", 1, 4, file); std::fwrite(&riffSize, 4, 1, file); std::fwrite("WAVEfmt ", 1, 8, file);
    std::fwrite(&formatSize, 4, 1, file); std::fwrite(&pcm, 2, 1, file); std::fwrite(&kChannels, 2, 1, file);
    std::fwrite(&kSampleRate, 4, 1, file); std::fwrite(&byteRate, 4, 1, file); std::fwrite(&kBlockAlign, 2, 1, file); std::fwrite(&kBitsPerSample, 2, 1, file);
    std::fwrite("data", 1, 4, file); std::fwrite(&dataBytes, 4, 1, file);
}
// 현재 QPC를 100ns 단위로 돌려준다(WASAPI 패킷 시각과 같은 단위다).
std::uint64_t Qpc100ns() {
    LARGE_INTEGER frequency{}, counter{};
    QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&counter);
    return static_cast<std::uint64_t>(counter.QuadPart) * 10000000ULL / static_cast<std::uint64_t>(frequency.QuadPart);
}
}

struct ProcessAudioCapture::Impl {
    std::thread thread;                  // 캡처 스레드다.
    HANDLE stopEvent{};                  // 중지 신호다.
    HANDLE readyEvent{};                 // 초기화 결과(성공/실패) 신호다.
    std::string error;                   // 초기화 실패 이유다(readyEvent 전에만 쓴다).
    bool ready{};                        // 초기화에 성공했는지다.
    std::filesystem::path path;          // 출력 WAV 경로다.
    std::uint32_t processId{};           // 대상 프로세스 번호다.
    std::uint64_t startQpc{};            // 시작 시각(100ns)이다.
    AudioCaptureStats stats;             // 스레드 종료 뒤 읽는 결과다.
    // 한 줄 오류를 기록하고 준비 실패를 알린다.
    void Fail(std::string message) { error = std::move(message); SetEvent(readyEvent); }
    // 캡처 스레드 본문이다. COM 활성화→형식 초기화→루프→WAV 마무리 순서다.
    void Run() {
        const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool comReady = SUCCEEDED(init);
        IAudioClient* client = nullptr;
        IAudioCaptureClient* capture = nullptr;
        HANDLE bufferEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        FILE* file = nullptr;
        [&] { // 초기화 단계다. 중간에 실패하면 이유를 알리고 빠져나온다.
            if (!comReady) return Fail("COM 초기화 실패");
            AUDIOCLIENT_ACTIVATION_PARAMS params{};
            params.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
            params.ProcessLoopbackParams.TargetProcessId = processId;
            params.ProcessLoopbackParams.ProcessLoopbackMode = PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;
            PROPVARIANT variant{};
            variant.vt = VT_BLOB;
            variant.blob.cbSize = sizeof(params);
            variant.blob.pBlobData = reinterpret_cast<BYTE*>(&params);
            auto* handler = new ActivationHandler();
            IActivateAudioInterfaceAsyncOperation* operation = nullptr;
            HRESULT result = ActivateAudioInterfaceAsync(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK, __uuidof(IAudioClient), &variant, handler, &operation);
            if (SUCCEEDED(result)) client = handler->Wait(result);
            if (operation) operation->Release();
            handler->Release();
            if (FAILED(result) || !client) return Fail("프로세스 루프백 활성화 실패(Windows 10 2004 이상 필요) hr=0x" + [&] { char text[16]; std::snprintf(text, sizeof(text), "%08lx", static_cast<unsigned long>(result)); return std::string(text); }());
            WAVEFORMATEX format{};
            format.wFormatTag = WAVE_FORMAT_PCM; format.nChannels = kChannels; format.nSamplesPerSec = kSampleRate;
            format.nAvgBytesPerSec = kSampleRate * kBlockAlign; format.nBlockAlign = kBlockAlign; format.wBitsPerSample = kBitsPerSample;
            result = client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, 0, 0, &format, nullptr);
            if (FAILED(result)) return Fail("오디오 클라이언트 초기화 실패");
            if (FAILED(client->SetEventHandle(bufferEvent))) return Fail("오디오 이벤트 설정 실패");
            if (FAILED(client->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture)))) return Fail("캡처 서비스 조회 실패");
            file = _wfopen(path.c_str(), L"wb");
            if (!file) return Fail("WAV 파일을 열 수 없음");
            WriteWavHeader(file, 0);
            if (FAILED(client->Start())) return Fail("오디오 캡처 시작 실패");
            startQpc = Qpc100ns();
            ready = true;
            SetEvent(readyEvent);
        }();
        if (ready) {
            std::uint64_t written = 0, silence = 0, nonZero = 0; // 쓴 프레임 수, 무음 보충 수, 소리 있는 프레임 수다.
            const auto pad = [&](std::uint64_t frames) { // 무음 프레임을 채워 시간축을 맞춘다.
                frames = std::min(frames, kMaxSilenceFrames);
                std::vector<std::uint8_t> zeros(static_cast<std::size_t>(std::min<std::uint64_t>(frames, 4096)) * kBlockAlign, 0);
                while (frames) { const auto step = std::min<std::uint64_t>(frames, 4096); std::fwrite(zeros.data(), kBlockAlign, static_cast<std::size_t>(step), file); frames -= step; written += step; silence += step; }
            };
            HANDLE waits[2] = {stopEvent, bufferEvent};
            bool stop = false;
            while (!stop) { // 중지 신호까지 패킷을 받아 쓴다.
                const auto wait = WaitForMultipleObjects(2, waits, FALSE, 200);
                stop = wait == WAIT_OBJECT_0;
                UINT32 packet = 0;
                while (SUCCEEDED(capture->GetNextPacketSize(&packet)) && packet) { // 쌓인 패킷을 모두 비운다.
                    BYTE* data = nullptr; UINT32 frames = 0; DWORD flags = 0; UINT64 devicePosition = 0, qpc = 0;
                    if (FAILED(capture->GetBuffer(&data, &frames, &flags, &devicePosition, &qpc))) break;
                    const std::uint64_t expected = qpc > startQpc ? (qpc - startQpc) * kSampleRate / 10000000ULL : 0; // 패킷 시작이 있어야 할 프레임 번호다.
                    if (expected > written + kSampleRate / 50) pad(expected - written); // 20ms 넘는 공백은 무음으로 보충한다.
                    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) { std::vector<std::uint8_t> zeros(static_cast<std::size_t>(frames) * kBlockAlign, 0); std::fwrite(zeros.data(), kBlockAlign, frames, file); silence += frames; }
                    else {
                        std::fwrite(data, kBlockAlign, frames, file);
                        const auto* samples = reinterpret_cast<const std::int16_t*>(data);
                        for (UINT32 frame = 0; frame < frames; ++frame) if (samples[frame * 2] || samples[frame * 2 + 1]) ++nonZero; // 프레임마다 소리가 있는지 센다.
                    }
                    written += frames;
                    capture->ReleaseBuffer(frames);
                }
            }
            const std::uint64_t total = (Qpc100ns() - startQpc) * kSampleRate / 10000000ULL; // 전체 경과에 맞춰 끝을 무음으로 채운다.
            if (total > written) pad(total - written);
            stats.frames = written; stats.silenceFrames = silence; stats.nonZeroFrames = nonZero; stats.sampleRate = kSampleRate; stats.seconds = static_cast<double>(written) / kSampleRate;
            client->Stop();
        }
        if (file) { // WAV 머리에 최종 크기를 쓴다.
            const auto bytes = static_cast<std::uint32_t>(std::min<std::uint64_t>(stats.frames * kBlockAlign, 0xffffffffULL - 64));
            std::fseek(file, 0, SEEK_SET);
            WriteWavHeader(file, bytes);
            std::fclose(file);
        }
        if (capture) capture->Release();
        if (client) client->Release();
        if (bufferEvent) CloseHandle(bufferEvent);
        if (comReady) CoUninitialize();
    }
};

ProcessAudioCapture::ProcessAudioCapture() = default;
ProcessAudioCapture::~ProcessAudioCapture() { if (impl_) Stop(); }

bool ProcessAudioCapture::Start(std::uint32_t processId, const std::filesystem::path& wavPath, std::string& error) {
    if (impl_) { error = "이미 녹음 중"; return false; }
    auto impl = std::make_unique<Impl>();
    impl->path = wavPath; impl->processId = processId;
    impl->stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    impl->readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    impl->thread = std::thread([raw = impl.get()] { raw->Run(); });
    const auto wait = WaitForSingleObject(impl->readyEvent, 15000);
    if (wait != WAIT_OBJECT_0 || !impl->ready) {
        error = wait != WAIT_OBJECT_0 ? "오디오 캡처 준비 시간 초과" : impl->error;
        SetEvent(impl->stopEvent);
        impl->thread.join();
        CloseHandle(impl->stopEvent); CloseHandle(impl->readyEvent);
        return false;
    }
    impl_ = std::move(impl);
    running_ = true;
    return true;
}

AudioCaptureStats ProcessAudioCapture::Stop() {
    if (!impl_) return {};
    SetEvent(impl_->stopEvent);
    impl_->thread.join();
    CloseHandle(impl_->stopEvent); CloseHandle(impl_->readyEvent);
    const auto stats = impl_->stats;
    impl_.reset();
    running_ = false;
    return stats;
}
}
