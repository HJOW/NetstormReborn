// cpppj의 실제 시간 Poll을 합성 전역에 실행하고 결과를 JSON으로 출력한다.
// 게임·소켓·창은 사용하지 않으며 등록 함수는 도달하면 중단한다.
#include "o/RawClockSynchronization.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <span>
#include <string>
#include <utility>
#include <vector>

// Poll 관찰에서 도달하지 않는 등록 함수의 링크 경계이다.
namespace netstorm::o {
// 일반 등록은 관측하지 않는다.
void RawPacketDispatch::Register(std::uint32_t, decltype(PacketRegistration::receive)) { std::abort(); }
// 고정 수신기 등록은 관측하지 않는다.
void RawPacketDispatch::RegisterCanonicalDirect(std::uint32_t, std::uint32_t, CanonicalPacketReceiver::Receive) { std::abort(); }
// 고정 기준선 승격은 관측하지 않는다.
void RawPacketDispatch::PersistCanonicalBaseline(std::uint32_t) { std::abort(); }
// 관찰기 등록은 관측하지 않는다.
void RawPacketDispatch::Observe(std::uint32_t, decltype(PacketRegistration::observer)) { std::abort(); }
}

// 관측 도구 전용 자료와 함수이다.
namespace {
// 저장소 한 바이트를 두 자리 16진수로 변환할 때 쓰는 문자표이다.
constexpr char kHexDigits[] = "0123456789abcdef";

// 입력 순서의 모든 바이트를 손실 없이 16진수 문자열로 만든다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    std::string result;
    result.reserve(bytes.size() * 2);
    // 각 바이트의 높은 자리와 낮은 자리를 차례로 출력한다.
    for (const auto byte : bytes) {
        result.push_back(kHexDigits[byte >> 4]);
        result.push_back(kHexDigits[byte & 15]);
    }
    return result;
}

// 70개 표본의 실제 32바이트 기록 전체를 순서대로 문자열로 만든다.
std::string SampleHex(const netstorm::o::ClockSynchronizationData& data) {
    std::string result;
    result.reserve(data.samples.size() * 64);
    // 표본의 헤더와 미사용 바이트도 함께 보존한다.
    for (const auto& sample : data.samples) result += Hex(sample);
    return result;
}

// 표본의 비정렬 위치에 host의 double 비트를 그대로 기록한다.
void PutTime(std::array<std::uint8_t, 32>& sample, std::size_t at, double value) {
    std::memcpy(sample.data() + at, &value, sizeof(value));
}

// 선택된 입력을 실제 Poll에 실행하고 입력·출력 전체를 한 JSON 객체로 출력한다.
void EmitCase(std::size_t index) {
    using namespace netstorm::o;
    const double now = 100.0 + static_cast<double>(index % 5);
    std::uint32_t connection = index % 11 == 0 ? 0 : 7;
    std::uint32_t replyMode = index % 13 == 0 ? 1 : 0;
    std::uint32_t gameTicks = 0x81234567U;
    ClockSynchronizationData data{};
    // 송신 제한과 부호 있는 비교에 쓰는 대표 DWORD 값이다.
    constexpr std::array<std::uint32_t, 6> sentValues{0, 30, 31, 59, 60, 0xffffffffU};
    data.sent = sentValues[index % sentValues.size()];
    data.received = index % 4 == 0 ? 0 : 3;
    data.latencyMilliseconds = 0xa1234567U;
    data.next = index % 3 == 0 ? 0 : index % 3 == 1 ? now - 1 : now + 1;
    const auto used = index % 6 == 0 ? data.samples.size() : std::size_t{8};
    // 각 표본의 헤더·빈 칸·유효/만료/느린 시각을 독립적으로 채운다.
    for (std::size_t slot = 0; slot < data.samples.size(); ++slot) {
        auto& sample = data.samples[slot];
        sample.fill(static_cast<std::uint8_t>((slot + index) & 0xff));
        double request = 0, server = 0, received = 0;
        if (slot < used) {
            const auto selector = (slot + index) % 5;
            const double age = selector == 4 ? 31.0 : selector == 3 ? 2.5 : 0.25 * (selector + 1);
            request = now - age;
            if ((slot + index) % 4 != 0) {
                received = request + 0.25 * (1 + ((slot + index) % 10));
                server = received - 0.125 * (1 + ((slot * 3 + index) % 9));
            }
        }
        PutTime(sample, 8, request);
        PutTime(sample, 16, server);
        PutTime(sample, 24, received);
    }
    const auto before = SampleHex(data);
    const auto initialSent = data.sent, initialReceived = data.received, initialLatency = data.latencyMilliseconds;
    const auto initialNext = data.next;
    const auto initialTicks = gameTicks;
    std::size_t wallCalls{};
    std::vector<std::pair<std::uint32_t, std::array<std::uint8_t, 32>>> sends;
    ClockSynchronizationHooks hooks{};
    // 실제 Poll이 고른 시계 조회와 송신만 순서대로 기록한다.
    hooks.wallSeconds = [&] { ++wallCalls; return now; };
    hooks.send = [&](std::uint32_t target, std::span<std::uint8_t> packet) {
        std::array<std::uint8_t, 32> copy{};
        if (packet.size() != copy.size()) std::abort();
        std::memcpy(copy.data(), packet.data(), copy.size());
        sends.emplace_back(target, copy);
    };
    RawClockSynchronization clock({connection, replyMode, gameTicks}, data, hooks);
    clock.Poll();
    std::cout << "{\"id\":" << index << ",\"now\":" << now << ",\"connection\":" << connection
              << ",\"replyMode\":" << replyMode << ",\"gameTicks\":" << initialTicks << ",\"sent\":" << initialSent
              << ",\"received\":" << initialReceived << ",\"latency\":" << initialLatency << ",\"next\":" << initialNext
              << ",\"samples\":\"" << before << "\",\"expected\":{\"gameTicks\":" << gameTicks
              << ",\"sent\":" << data.sent << ",\"received\":" << data.received << ",\"latency\":" << data.latencyMilliseconds
              << ",\"next\":" << data.next << ",\"samples\":\"" << SampleHex(data) << "\",\"wallCalls\":" << wallCalls
              << ",\"sends\":[";
    // 각 송신은 선택된 연결과 복사한 32바이트를 그대로 기록한다.
    for (std::size_t at = 0; at < sends.size(); ++at) {
        if (at) std::cout << ',';
        std::cout << "{\"connection\":" << sends[at].first << ",\"bytes\":\"" << Hex(sends[at].second) << "\"}";
    }
    std::cout << "]}}";
}
}

// 표본·기한·카운터 경계가 다른 24개 장면을 JSON 문서로 출력한다.
int main() {
    std::cout << std::setprecision(17) << "{\"format\":\"netstorm-cpp-clock-poll-v1\",\"cases\":[";
    // 각 장면은 실제 cpppj Poll을 독립 저장소에서 한 번 실행한다.
    for (std::size_t index = 0; index < 24; ++index) {
        if (index) std::cout << ',';
        EmitCase(index);
    }
    std::cout << "]}\n";
}
