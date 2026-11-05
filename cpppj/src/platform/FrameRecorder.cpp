// 영상 녹화기 구현이다. 게임 스레드는 프레임 복사만 하고 PNG 인코딩/파일 쓰기는 작업 스레드가 한다.
#include "platform/FrameRecorder.h"
#include "platform/Json.h"
#include "platform/Png.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <stdexcept>

namespace netstorm::platform { // 파일 형식/OS 경계의 범용 부품이다.
namespace { // 파일 이름 규칙이다.
// 순번 프레임 파일 이름(frame_000123.png)이다.
std::string FrameName(std::uint64_t index) { char text[32]; std::snprintf(text, sizeof(text), "frame_%06llu.png", static_cast<unsigned long long>(index)); return text; }
// 바이트열을 파일로 쓴다. 성공하면 true다.
bool WriteBytes(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    return file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())).good();
}
}

FrameRecorder::FrameRecorder() = default;
FrameRecorder::~FrameRecorder() { if (running_) Stop(); }

bool FrameRecorder::Start(const FrameRecorderOptions& options, std::string& error) {
    if (running_) { error = "이미 녹화 중"; return false; }
    if (options.fps < 1 || options.fps > 60) { error = "fps는 1..60이어야 합니다"; return false; }
    std::error_code code;
    std::filesystem::create_directories(options.directory, code);
    if (code || !std::filesystem::is_directory(options.directory)) { error = "녹화 폴더를 만들 수 없음"; return false; }
    options_ = options;
    if (options_.maxQueued < 1) options_.maxQueued = 1;
    hasFirst_ = false; firstTime_ = nextTime_ = 0; accepted_ = dropped_ = 0; width_ = height_ = 0;
    { const std::lock_guard lock(mutex_); queue_.clear(); stopping_ = false; times_.clear(); written_ = 0; workerError_.clear(); }
    running_ = true;
    worker_ = std::thread([this] { Worker(); });
    return true;
}

bool FrameRecorder::Due(double nowSeconds) const { return running_ && (!hasFirst_ || nowSeconds >= nextTime_); }

bool FrameRecorder::Offer(std::uint32_t width, std::uint32_t height, std::uint32_t pitch, std::span<const std::uint8_t> pixels, std::span<const std::uint8_t, 768> palette, double nowSeconds) {
    if (!Due(nowSeconds)) return false;
    if (!width || !height || pitch < width || static_cast<std::uint64_t>(pitch) * (height - 1) + width > pixels.size()) return false;
    const double period = 1.0 / options_.fps;
    if (!hasFirst_) { hasFirst_ = true; firstTime_ = nowSeconds; nextTime_ = nowSeconds; }
    nextTime_ = (nowSeconds - nextTime_ > period * 2) ? nowSeconds + period : nextTime_ + period; // 많이 밀렸으면 따라잡지 않고 다시 시작한다.
    Item item;
    item.width = width; item.height = height;
    item.pixels.resize(static_cast<std::size_t>(width) * height);
    for (std::uint32_t y = 0; y < height; ++y) std::copy_n(pixels.begin() + static_cast<std::ptrdiff_t>(y) * pitch, width, item.pixels.begin() + static_cast<std::ptrdiff_t>(y) * width); // 행 간격 여백은 버리고 촘촘히 복사한다.
    std::copy(palette.begin(), palette.end(), item.palette.begin());
    item.time = nowSeconds - firstTime_;
    {
        const std::lock_guard lock(mutex_);
        if (queue_.size() >= options_.maxQueued) { ++dropped_; return false; } // 인코더가 밀리면 프레임을 버린다.
        item.index = accepted_++;
        queue_.push_back(std::move(item));
    }
    width_ = width; height_ = height;
    condition_.notify_one();
    return true;
}

void FrameRecorder::Worker() {
    while (true) { // 대기열이 비고 종료 요청이 올 때까지 프레임을 저장한다.
        Item item;
        {
            std::unique_lock lock(mutex_);
            condition_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
            if (queue_.empty()) return; // 종료 요청 뒤 남은 프레임이 없다.
            item = std::move(queue_.front());
            queue_.pop_front();
        }
        try {
            const auto png = EncodePngIndexed(item.width, item.height, item.width, item.pixels, item.palette);
            if (!WriteBytes(options_.directory / FrameName(item.index), png)) throw std::runtime_error("프레임 파일 쓰기 실패");
            const std::lock_guard lock(mutex_);
            if (times_.size() <= item.index) times_.resize(item.index + 1, -1.0);
            times_[item.index] = item.time;
            ++written_;
        } catch (const std::exception& error) {
            const std::lock_guard lock(mutex_);
            if (workerError_.empty()) workerError_ = error.what();
        }
    }
}

FrameRecorderResult FrameRecorder::Stop() {
    FrameRecorderResult result;
    if (!running_) return result;
    { const std::lock_guard lock(mutex_); stopping_ = true; }
    condition_.notify_all();
    worker_.join();
    running_ = false;
    const double period = 1.0 / options_.fps;
    std::vector<double> times;
    std::uint64_t written;
    std::string workerError;
    { const std::lock_guard lock(mutex_); times = times_; written = written_; workerError = workerError_; }
    std::vector<std::uint64_t> saved; // 실제로 저장된 프레임 번호다(오류로 빠진 번호는 제외한다).
    for (std::uint64_t index = 0; index < times.size(); ++index) if (times[index] >= 0) saved.push_back(index);
    result.frames = saved.size(); result.dropped = dropped_; result.width = width_; result.height = height_;
    result.manifest = options_.directory / "manifest.json"; result.concat = options_.directory / "frames.ffconcat";
    std::string concat = "ffconcat version 1.0\n"; // ffmpeg concat demuxer 목록이다. 마지막 파일은 길이 없이 한 번 더 적는다.
    Json timestamps = Json::MakeArray();
    for (std::size_t position = 0; position < saved.size(); ++position) { // 프레임 시각 목록과 각 프레임의 표시 길이를 만든다.
        const double start = times[saved[position]];
        const double end = position + 1 < saved.size() ? times[saved[position + 1]] : start + period;
        char text[96];
        std::snprintf(text, sizeof(text), "duration %.6f\n", std::max(end - start, 0.001));
        concat += "file " + FrameName(saved[position]) + "\n" + text;
        timestamps.Push(static_cast<std::int64_t>(start * 1000.0 + 0.5));
        result.seconds = end;
    }
    if (!saved.empty()) concat += "file " + FrameName(saved.back()) + "\n";
    WriteBytes(result.concat, {reinterpret_cast<const std::uint8_t*>(concat.data()), concat.size()});
    Json manifest;
    manifest.Set("fps", options_.fps).Set("width", static_cast<std::int64_t>(width_)).Set("height", static_cast<std::int64_t>(height_));
    manifest.Set("frames", static_cast<std::int64_t>(result.frames)).Set("dropped", static_cast<std::int64_t>(result.dropped)).Set("duration_seconds", result.seconds);
    manifest.Set("frame_pattern", "frame_%06d.png").Set("concat", "frames.ffconcat").Set("timestamps_ms", std::move(timestamps));
    if (!workerError.empty()) manifest.Set("error", workerError);
    const auto text = manifest.Dump();
    WriteBytes(result.manifest, {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
    static_cast<void>(written);
    return result;
}
}
