// 다리 이벤트 처리기·끝 칸 변환·글자별 프레임 구간 표를 저장된 실제 PE 제한 x86 관찰과 비교한다.
#include "TestSupport.h"
#include "o/RawBridgeEvents.h"
#include "o/SquidOwner.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 분석 도구와 같은 합성 풀 크기와 입력 번호. 실제 게임 월드 번호 배치를 뜻하지 않는다.
constexpr std::uint32_t kCapacity = 24000;
constexpr Sid kBridge{50}, kNewborn{60}, kNeighbor{70};
// 분석 도구가 쓰는 합성 vtable 주소 기록값과 다리/다른 타입 번호다. 호스트에서 역참조하지 않는다.
constexpr std::uint32_t kVtable = 0x11010000, kBridgeType = 82, kOtherType = 83;
// 명시적인 C++ 보호 예외를 원본 assert 실행 없이 확인한다.
template<class F> bool Throws(F action) { try { action(); return false; } catch (const std::exception&) { return true; } }
// 주석/빈 행을 제외하되 마지막 빈 필드도 보존하는 구분자 판독기다.
std::vector<std::string> Split(std::string_view text, char separator) {
    std::vector<std::string> result;
    std::size_t begin = 0;
    // 남은 마지막 필드까지 추가한다.
    for (;;) {
        const auto end = text.find(separator, begin);
        result.emplace_back(text.substr(begin, end == std::string_view::npos ? end : end - begin));
        if (end == std::string_view::npos) return result;
        begin = end + 1;
    }
}
// Git에 저장된 실제 기계어 기대값만 읽는다. Python/원본 PE는 테스트에 필요 없다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows = [] {
        std::ifstream input(NETSTORM_BRIDGEEVENT_FIXTURE);
        if (!input) throw std::runtime_error("Missing bridge event fixture");
        std::vector<std::vector<std::string>> result;
        std::string line;
        // 설명 주석만 건너뛴다.
        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty() && line.front() != '#') result.push_back(Split(line, '\t'));
        }
        return result;
    }();
    return rows;
}
// fixture의 부호 없는 DWORD 필드를 읽는다.
std::uint32_t Number(const std::string& value) { return static_cast<std::uint32_t>(std::stoul(value)); }
// 16진 문자열을 바이트로 바꾼다.
std::vector<std::uint8_t> Bytes(const std::string& hex) {
    std::vector<std::uint8_t> result;
    // 두 글자가 한 바이트다.
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    return result;
}
// 프레임 코드 바이트 배열을 타입 프레임 표로 만든다.
RiftTypeFrames Frames(const std::vector<std::uint8_t>& bytes) {
    std::vector<FrameCode> codes;
    // 네 바이트가 한 코드다.
    for (std::size_t i = 0; i + 3 < bytes.size(); i += 4) codes.push_back({bytes[i], bytes[i + 1], bytes[i + 2], bytes[i + 3]});
    return RiftTypeFrames(std::move(codes));
}
// Python zlib와 같은 전체 버퍼 Adler-32다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // 표준 소수와 중간 넘침을 막는 누적 폭이다.
    constexpr std::uint64_t prime = 65521;
    std::uint64_t a = 1, b = 0;
    // 슬롯은 짧아 매 바이트마다 나머지를 취해도 된다.
    for (auto byte : bytes) { a = (a + byte) % prime; b = (b + a) % prime; }
    return static_cast<std::uint32_t>((b << 16) | a);
}
// little endian으로 raw 필드를 쓴다.
void Write(std::span<std::uint8_t> raw, std::size_t offset, std::uint32_t value, std::size_t size) {
    // 낮은 바이트부터.
    for (std::size_t i = 0; i < size; ++i) raw[offset + i] = static_cast<std::uint8_t>(value >> (i * 8));
}
// 반환 float을 분석 도구와 같은 칸으로 쓴다. NaN은 정규화로 비트가 달라지므로 한 이름이다.
std::string ResultText(float value) { return std::isnan(value) ? "nan" : std::to_string(std::bit_cast<std::uint32_t>(value)); }
// 분석 도구의 EventOracle 입력/대체 효과를 같은 계약으로 재현한다.
struct Scene {
    OriginalEdition edition;
    SidPool pool;
    BridgeEventState state;
    std::map<std::uint32_t, RiftTypeFrames> variants;
    std::uint32_t variant{};
    std::uint32_t neighbor{};
    std::vector<std::string> events;
    RawBridgeEvents bridge;
    // 풀 슬롯 5~70을 미리 할당해 합성 번호 50·60·70을 준비한다. 복사/이동하면 훅이 가리키는 주소가 바뀐다.
    Scene(OriginalEdition which, const std::map<std::uint32_t, std::vector<std::uint8_t>>& frameBytes)
        : edition(which), pool(which, kCapacity, false), bridge(pool, state, Hooks()) {
        // 클라이언트 영역의 앞 슬롯들을 할당한다.
        for (int i = 5; i <= 70; ++i) CHECK(pool.Allocate(2).value == i);
        // 변형마다 프레임 표를 만든다.
        for (const auto& [number, bytes] : frameBytes) variants.emplace(number, Frames(bytes));
    }
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    bool Patch() const { return edition == OriginalEdition::Patch1078; }
    // 판본별 필드 위치다.
    std::size_t Owner() const { return Patch() ? 0x22 : 0x20; }
    std::size_t FrameOffset() const { return Patch() ? 0x24 : 0x22; }
    std::size_t FlagOffset() const { return Patch() ? 0x28 : 0x23; }
    // 분석 도구가 기록하는 프레임 값: 패치는 DWORD, CD는 바이트를 부호 없는 수로 읽는다.
    std::uint32_t RawFrame(Sid sid) const {
        const auto raw = pool.Slot(sid);
        std::uint32_t value = 0;
        // 패치는 네 바이트, CD는 한 바이트다.
        for (std::size_t i = 0; i < (Patch() ? 4U : 1U); ++i) value |= static_cast<std::uint32_t>(raw[FrameOffset() + i]) << (8 * i);
        return value;
    }
    // 외부 효과를 분석 도구의 대체 함수와 같은 사건 문자열로 기록한다.
    BridgeEventHooks Hooks() {
        return {
            // 타입 번호는 다리/다른 타입 두 개만 합성했고 프레임 표는 같다.
            [this](std::uint32_t type) -> const RiftTypeFrames& {
                CHECK(type == kBridgeType || type == kOtherType);
                return variants.at(variant);
            },
            // 임시 프레임이 쓰인 채로 호출된다. 그 프레임 값을 기록하고 입력의 첫 이웃을 돌려준다.
            [this](Sid sid) {
                events.push_back("F:" + std::to_string(sid.value) + ":" + std::to_string(RawFrame(sid)) + ":0");
                return Sid{static_cast<std::uint16_t>(neighbor)};
            },
            [this](Sid sid) { events.push_back("N:" + std::to_string(sid.value)); },
            // 새 객체의 헤더(vtable·타입·상태 0)만 만든다. 실제 할당/postCreate는 대체 경계다.
            [this](std::uint32_t type) {
                events.push_back("C:" + std::to_string(type) + ":0");
                auto raw = pool.AllocatedBytes(kNewborn);
                Write(raw, 0, kVtable, 4);
                raw[10] = static_cast<std::uint8_t>(type);
                raw[11] = 0;
                return kNewborn;
            },
            [this](Sid sid, std::uint32_t flags) { events.push_back("D:" + std::to_string(sid.value) + ":" + std::to_string(flags)); },
            [this](Sid sid, std::uint8_t owner) { events.push_back("O:" + std::to_string(sid.value) + ":" + std::to_string(owner)); },
            [this](Sid sid, float x, float y, std::uint32_t flags) {
                events.push_back("P:" + std::to_string(sid.value) + ":" + std::to_string(std::bit_cast<std::uint32_t>(x)) + ":" +
                                 std::to_string(std::bit_cast<std::uint32_t>(y)) + ":" + std::to_string(flags));
            },
        };
    }
    // 행의 입력(권한·디버그·전역 타입·변형·프레임·위치·단어·부모·소유자·플래그·이웃)으로 슬롯과 전역을 만든다.
    void Prepare(const std::vector<std::string>& row) {
        events.clear();
        state.authority = Number(row[2]) != 0;
        state.debugKeep = Number(row[3]) != 0;
        state.bridgeType = Number(row[4]);
        variant = Number(row[5]);
        neighbor = Number(row[13]);
        // 두 슬롯을 0으로 지우고 다리 슬롯만 채운다. 새 슬롯은 생성 대체가 헤더를 쓴다.
        for (Sid sid : {kBridge, kNewborn}) {
            const auto raw = pool.AllocatedBytes(sid);
            std::fill(raw.begin(), raw.end(), std::uint8_t{});
        }
        auto raw = pool.AllocatedBytes(kBridge);
        Write(raw, 0, kVtable, 4);
        raw[10] = static_cast<std::uint8_t>(kBridgeType);
        raw[11] = 0;
        Write(raw, 8, Number(row[10]), 2);
        Write(raw, 12, Number(row[9]), 2);
        Write(raw, 14, Number(row[7]), 4);
        Write(raw, 18, Number(row[8]), 4);
        raw[Owner()] = static_cast<std::uint8_t>(Number(row[11]));
        Write(raw, FrameOffset(), Number(row[6]), Patch() ? 4 : 1);
        raw[FlagOffset()] = static_cast<std::uint8_t>(Number(row[12]));
    }
};
// 판본 이름으로 고른 행을 모두 재생한다.
void ReplayEdition(const std::string& name, std::size_t minimumHandler, std::size_t minimumEnd) {
    std::map<std::uint32_t, std::vector<std::uint8_t>> frameBytes;
    // 프레임 변형 행을 먼저 모은다.
    for (const auto& row : Fixture()) if (row[0] == "Frames") frameBytes[Number(row[1])] = Bytes(row[2]);
    CHECK(frameBytes.size() == 3);
    Scene scene(name == "originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072, frameBytes);
    std::size_t handler = 0, end = 0;
    // 한 줄이 한 입력이다.
    for (const auto& row : Fixture()) {
        if (row[0] == "Frames" || row[0] == "Table" || row[1] != name) continue;
        CHECK(row.size() == 22);
        scene.Prepare(row);
        std::string result = "-";
        if (row[0] == "Handler") {
            result = ResultText(scene.bridge.Handle(kBridge, Number(row[14]), 0, std::bit_cast<float>(Number(row[15]))));
            ++handler;
        } else {
            scene.bridge.ConvertEnd(kBridge, std::bit_cast<float>(Number(row[16])), std::bit_cast<float>(Number(row[17])));
            ++end;
        }
        std::string joined;
        for (const auto& event : scene.events) { if (!joined.empty()) joined += ';'; joined += event; }
        const std::string events = joined.empty() ? "-" : joined;
        const bool same = result == row[18] && events == row[19] && Adler(scene.pool.Slot(kBridge)) == Number(row[20]) &&
                          Adler(scene.pool.Slot(kNewborn)) == Number(row[21]);
        CHECK(same);
        if (!same) {
            std::printf("  %s %s 이벤트 %s 프레임 %s 위치 %s/%s\n    기대 %s | %s | %s | %s\n    실제 %s | %s | %u | %u\n", name.c_str(), row[0].c_str(),
                row[14].c_str(), row[6].c_str(), row[7].c_str(), row[8].c_str(), row[18].c_str(), row[19].c_str(), row[20].c_str(),
                row[21].c_str(), result.c_str(), events.c_str(), Adler(scene.pool.Slot(kBridge)), Adler(scene.pool.Slot(kNewborn)));
            break;
        }
    }
    CHECK(handler >= minimumHandler);
    CHECK(end >= minimumEnd);
}
}

// 패치 10.78의 실제 기계어 관찰을 재생한다. 끝 칸 변환은 별도 진입점이 있어 직접 호출 행도 있다.
TEST_CASE(bridge_event_patch_x86_fixture) { ReplayEdition("originals", 1207, 726); }
// CD 10.72 배포본의 관찰. 끝 칸 변환은 처리기 안에 인라인되어 있어 직접 호출 행이 없다.
TEST_CASE(bridge_event_cd_x86_fixture) { ReplayEdition("originalCD", 1207, 0); }
// 추가 10.37 실행 파일은 CD와 같은 코드 배치지만 별도 PE의 출력을 재생한다.
TEST_CASE(bridge_event_1037_x86_fixture) { ReplayEdition("original1037", 1207, 0); }
// 글자별 첫 프레임·개수·글자 수 표는 세 PE와 두 정밀도에서 같았고 C++ 계산과 일치한다.
TEST_CASE(bridge_event_letter_run_table_matches_x86) {
    std::size_t rows = 0;
    // 한 줄이 한 프레임 코드 배열이다.
    for (const auto& row : Fixture()) {
        if (row[0] != "Table") continue;
        CHECK(row.size() == 6);
        const auto frames = Frames(row[2] == "-" ? std::vector<std::uint8_t>{} : Bytes(row[2]));
        const auto first = Split(row[3], ','), count = Split(row[4], ',');
        CHECK(first.size() == 16 && count.size() == 16);
        // 글자 'A'~'P' 16개를 하나씩 비교한다.
        for (int k = 0; k < 16 && k < static_cast<int>(first.size()); ++k) {
            const auto run = frames.Run(static_cast<std::uint8_t>('A' + k));
            CHECK(run.first == std::stoi(first[k]));
            CHECK(run.count == std::stoi(count[k]));
        }
        CHECK(frames.LetterKinds() == std::stoi(row[5]));
        ++rows;
    }
    CHECK(rows >= 300);
    CHECK(Throws([] { return Frames({}).Run('Q'); }));
    CHECK(Throws([] { return Frames({}).Run('@'); }));
}

namespace {
// 실제 bridge.type의 프레임 코드 60개. 다리 붕괴 기대값 파일의 "Frames 0" 행에 원본 자산에서 읽은 값이 있다.
RiftTypeFrames RealBridgeFrames() {
    std::ifstream input(NETSTORM_BRIDGEDECAY_FIXTURE);
    if (!input) throw std::runtime_error("Missing bridge decay fixture");
    std::string line;
    // 첫 "Frames 0" 행만 쓴다.
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto row = Split(line, '\t');
        if (row.size() == 3 && row[0] == "Frames" && row[1] == "0") return Frames(Bytes(row[2]));
    }
    throw std::runtime_error("Missing bridge frame row");
}
}
// 실제 다리 타입에서 끝 칸 변환이 고르는 프레임: L·M·N·O의 첫 프레임은 보통(번호 1, 플래그 0) 프레임이고
// J·K에서 교체가 막히는 것은 단단한(번호 20, 플래그 0x40) 프레임뿐이다.
TEST_CASE(bridge_event_real_bridge_type_end_frames) {
    const auto frames = RealBridgeFrames();
    const auto codes = frames.Codes();
    CHECK(codes.size() == 60);
    CHECK(frames.LetterKinds() == 16);
    CHECK(frames.Run('J').first == 35 && frames.Run('J').count == 6);
    CHECK(frames.Run('K').first == 41 && frames.Run('K').count == 6);
    const std::array<std::pair<char, int>, 4> ends{{{'L', 47}, {'M', 50}, {'N', 53}, {'O', 56}}};
    // 끝 글자 네 개의 첫 프레임과 개수를 확인한다.
    for (const auto& [letter, first] : ends) {
        const auto run = frames.Run(static_cast<std::uint8_t>(letter));
        CHECK(run.first == first && run.count == 3);
        CHECK(codes[static_cast<std::size_t>(run.first)].number == 1 && codes[static_cast<std::size_t>(run.first)].flags == 0);
    }
    int hard = 0;
    // J·K 프레임 가운데 0x40이 켜진 것은 번호 20뿐이다.
    for (const auto& code : codes) {
        if (code.side != 'J' && code.side != 'K') continue;
        CHECK(((code.flags & 0x40) != 0) == (code.number == 20));
        hard += (code.flags & 0x40) != 0;
    }
    CHECK(hard == 2);
}
// 지연 낙하 예약 → Kernel 프레임 → 가상 표로 분배한 다리 처리기 → 끝 칸 변환까지 한 흐름으로 이어진다.
// 다리가 아닌 부모는 base 처리기처럼 payload가 주기가 된다.
TEST_CASE(bridge_event_runs_from_scheduled_fall_through_regular_process) {
    constexpr std::uint32_t capacity = 32768;
    SidPool pool(OriginalEdition::Patch1078, capacity, true);
    SquidHash hash;
    std::vector<std::uint8_t> spots(65536);
    std::vector<RiftTypeRecord> types(188);
    types[kBridgeType].flags2 = 4;
    SquidUnpop unpop(pool, hash, spots);
    RawSquidDestroy destroy(pool, unpop, types);
    Kernel kernel;
    SquidProcessState processState;
    processState.now = 10.0;
    BridgeEventState bridgeState;
    bridgeState.bridgeType = kBridgeType;
    const auto frames = RealBridgeFrames();
    std::vector<std::string> events;
    Sid born{};
    // 소유자 지정은 실제 복원 함수에 잇는다. 다리 genus는 작업장 목록 대상이 아니므로 소유자 바이트만 바뀐다.
    SquidPostPopState bookkeeping;
    SquidOwnerMode ownerMode;
    ownerMode.battle = true;
    SquidOwner owner(pool, types, bookkeeping, ownerMode);
    RawBridgeEvents bridgeEvents(pool, bridgeState, BridgeEventHooks{
        [&](std::uint32_t) -> const RiftTypeFrames& { return frames; },
        [&](Sid sid) { events.push_back("F:" + std::to_string(pool.Slot(sid)[0x24])); return Sid{1234}; },
        [&](Sid sid) { events.push_back("N:" + std::to_string(sid.value)); },
        // 새 객체는 실제 풀에서 번호를 받고 다리 가상 표와 타입만 쓴다.
        [&](std::uint32_t type) {
            born = pool.Allocate(2);
            auto raw = pool.AllocatedBytes(born);
            Write(raw, 0, kPatchBridgeVtable, 4);
            raw[10] = static_cast<std::uint8_t>(type);
            events.push_back("C:" + std::to_string(type));
            return born;
        },
        [&](Sid sid, std::uint32_t flags) { events.push_back("D:" + std::to_string(sid.value) + ":" + std::to_string(flags)); },
        [&](Sid sid, std::uint8_t player) {
            owner.Set(sid, player);
            events.push_back("O:" + std::to_string(sid.value == born.value) + ":" + std::to_string(player));
        },
        [&](Sid sid, float x, float y, std::uint32_t) {
            events.push_back("P:" + std::to_string(sid.value == born.value) + ":" + std::to_string(x) + ":" + std::to_string(y));
        }});
    SquidProcessHost host(pool, types, kernel, destroy, processState, MakeBridgeRegularHandler(pool, bridgeEvents),
        SquidDestroyHooks{[](const SquidDestroyEvent&) {}, [] { return Sid{}; }, {}});
    // 다리 칸: 실제 타입의 J 프레임(35), 위치 (20.75, 21.9), 소유자 3.
    const Sid bridge = pool.Allocate(2);
    auto raw = pool.AllocatedBytes(bridge);
    Write(raw, 0, kPatchBridgeVtable, 4);
    raw[10] = static_cast<std::uint8_t>(kBridgeType);
    Write(raw, 14, std::bit_cast<std::uint32_t>(20.75f), 4);
    Write(raw, 18, std::bit_cast<std::uint32_t>(21.9f), 4);
    raw[0x22] = 3;
    Write(raw, 0x24, 35, 4);
    // 가상 표가 다른 객체: 다리 처리기가 아니라 payload 주기로 반복한다.
    const Sid plain = pool.Allocate(2);
    auto other = pool.AllocatedBytes(plain);
    Write(other, 0, 0x00501dd0, 4);
    other[10] = static_cast<std::uint8_t>(kBridgeType);
    auto* fall = ScheduleBridgeFall(host, bridge, 20.0f, 21.0f);
    auto* periodic = host.AddRegular(plain, kBridgeFallEvent, 0.5f);
    CHECK(fall && periodic && HasScheduledBridgeFall(host, bridge));
    kernel.RunFrame();
    // 낙하 칸 y(21)가 객체 y(21.9)보다 크지 않으므로 L(첫 프레임 47)이다. 임시 프레임이 쓰인 채 이웃을 묻고 되돌린다.
    const std::vector<std::string> expected{"F:47", "N:" + std::to_string(bridge.value), "C:82", "O:1:3", "N:" + std::to_string(born.value),
        "D:" + std::to_string(bridge.value) + ":0", "P:1:" + std::to_string(20.75f) + ":" + std::to_string(21.9f)};
    CHECK(events == expected);
    CHECK(pool.Slot(bridge)[0x24] == 35);
    CHECK(pool.Slot(born)[0x24] == 47);
    // 옛 칸의 소유자(3)가 새 객체로 옮겨진다.
    CHECK(pool.Slot(born)[0x22] == 3);
    // 수명 비트는 4(0x20)로 쓰이고 새 객체로 옮겨진다.
    CHECK((pool.Slot(bridge)[12] & 0x78) == 0x20 && (pool.Slot(born)[12] & 0x78) == 0x20);
    // 다리 처리기는 -1을 돌려 예약을 그대로 두고, 다른 객체는 payload(0.5초) 뒤로 다시 예약된다.
    CHECK(kernel.Size() == 2 && fall->Count() == 0 && fall->Time() == 10.0);
    CHECK(periodic->Count() == 1 && periodic->Time() == 10.5);
    // 권한이 없으면 처리기가 0을 돌려 예약이 끝난다.
    events.clear();
    bridgeState.authority = false;
    kernel.RunFrame();
    CHECK(events.empty());
    CHECK(kernel.Size() == 1 && !HasScheduledBridgeFall(host, bridge));
}
