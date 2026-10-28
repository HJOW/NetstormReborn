// 세 원본 PE의 실제 x86 결과로 다리 한 칸 붕괴 처리·수명 감소·프레임 전환·destroy 재정의·스캔 커서를 대조한다.
#include "TestSupport.h"
#include "o/Bridge.h"
#include "o/RawBridgeDecay.h"
#include "o/SquidFinder.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace { //기존 동결 기대값을 새 raw 실행기에 그대로 입력하며 기대 결과는 다시 계산하지 않는다.
//직접 fixture 설치는 non-const 소유자의 물리 바이트에만 한정한다.
std::span<std::uint8_t> RawDecayFixtureSlot(netstorm::o::SidPool& pool,std::uint16_t number) {
    const auto bytes=pool.Bytes();return std::span(const_cast<std::uint8_t*>(bytes.data()),bytes.size()).subspan(static_cast<std::size_t>(number)*50U,50U);
}
//비정렬 WORD/DWORD의 원문 폭만 조립한다.
std::uint32_t RawDecayRead(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);return value; //기록 비교용 바이트 읽기다.
}
//생산 코드의 쓰기 보조 함수를 기대 입력 구성에 사용하지 않는다.
void RawDecayPut(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); //명시 입력을 원문 폭으로 배치한다.
}
}

using namespace netstorm::o;
namespace {
// 기대값 파일의 행 종류별 개수(bridgedecay.json의 cases와 같다). 파일이 잘리면 검사가 실패한다.
constexpr std::size_t kCellRows = 4241, kLifeRows = 3264, kDestroyRows = 1152, kCrackRows = 348, kNormalRows = 174,
    kWeakenRows = 696, kInitRows = 12, kScanRows = 3941;
// Life 행 가운데 결과 수명이 7을 넘는 입력 수. 원본은 4비트로 쓰고 새 코드는 거부한다.
constexpr std::size_t kLifeGuardedRows = 400;
// 마지막 빈 필드도 보존하여 TSV 열 수를 유지한다.
std::vector<std::string> Split(std::string_view text, char separator) {
    std::vector<std::string> result;
    std::size_t start = 0;
    // 구분자가 더 없으면 남은 문자열을 추가하고 끝낸다.
    for (;;) {
        const auto next = text.find(separator, start);
        result.emplace_back(text.substr(start, next == std::string_view::npos ? next : next - start));
        if (next == std::string_view::npos) return result;
        start = next + 1;
    }
}
// 원본 파일이나 Python 없이 Git에 저장한 실제 기계어 기대값을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows = [] {
        std::ifstream input(NETSTORM_BRIDGEDECAY_FIXTURE);
        if (!input) throw std::runtime_error("Missing bridge decay x86 fixture");
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
// 기대값의 Frames 행(표 번호별 실제/변형 bridge 프레임 코드)을 읽는다.
RiftTypeFrames Frames(const std::string& table) {
    // 표 번호가 같은 행을 찾는다.
    for (const auto& row : Fixture()) {
        if (row[0] != "Frames" || row[1] != table) continue;
        std::vector<FrameCode> codes;
        const auto& hex = row[2];
        // 네 바이트 코드마다 원본 순서를 유지한다.
        for (std::size_t i = 0; i + 8 <= hex.size(); i += 8) {
            const auto byte = [&](std::size_t index) {
                return static_cast<std::uint8_t>(std::stoul(hex.substr(i + index * 2, 2), nullptr, 16));
            };
            codes.push_back({byte(0), byte(1), byte(2), byte(3)});
        }
        return RiftTypeFrames(std::move(codes));
    }
    throw std::runtime_error("Missing bridge decay frame table");
}
// 입력 객체 한 개: 번호, 기준점, 발자국, 타입 플래그, 종류(0 다리/1 섬), 프레임, state, extra, 그래프 번호, 수명 단어.
struct Input {
    // id: 대상을 구별하는 식별 번호. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::uint16_t id{};
    // x: X 좌표. 입력 구성 또는 실행 결과 대조에 사용한다.
    // y: Y 좌표. 입력 구성 또는 실행 결과 대조에 사용한다.
    // width: 가로 크기. 입력 구성 또는 실행 결과 대조에 사용한다.
    // height: 세로 크기. 입력 구성 또는 실행 결과 대조에 사용한다.
    int x{}, y{}, width{}, height{};
    // flags1: 타입의 첫 번째 플래그 단어. 입력 구성 또는 실행 결과 대조에 사용한다.
    // flags2: 타입의 두 번째 플래그 단어. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::uint32_t flags1{}, flags2{};
    // kind: 자료나 동작의 종류를 구별하는 값. 입력 구성 또는 실행 결과 대조에 사용한다.
    // frame: 타입에서 사용할 그림 프레임 번호. 입력 구성 또는 실행 결과 대조에 사용한다.
    int kind{}, frame{};
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    // extra: 원본 출력·상태 문맥의 추가 인자 값. 입력 구성 또는 실행 결과 대조에 사용한다.
    // graph: 서로 연결된 표면의 Graph 소속을 관리하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::uint8_t state{}, extra{}, graph{};
    // word: 단어. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::uint16_t word{};
};
// "a,b,c;d,e,f" 형식의 정수 묶음을 읽는다. '-'는 빈 목록이다.
std::vector<std::vector<long long>> Tuples(const std::string& text) {
    std::vector<std::vector<long long>> result;
    if (text == "-" || text.empty()) return result;
    // 묶음마다 쉼표로 나눈 정수를 모은다.
    for (const auto& entry : Split(text, ';')) {
        std::vector<long long> values;
        // 한 묶음의 정수들.
        for (const auto& value : Split(entry, ',')) values.push_back(std::stoll(value));
        result.push_back(std::move(values));
    }
    return result;
}
// 객체 열을 이름 있는 필드로 옮긴다.
std::vector<Input> Inputs(const std::string& text) {
    std::vector<Input> result;
    // 객체마다 13개 정수다.
    for (const auto& v : Tuples(text)) {
        result.push_back({static_cast<std::uint16_t>(v[0]), static_cast<int>(v[1]), static_cast<int>(v[2]), static_cast<int>(v[3]),
            static_cast<int>(v[4]), static_cast<std::uint32_t>(v[5]), static_cast<std::uint32_t>(v[6]), static_cast<int>(v[7]),
            static_cast<int>(v[8]), static_cast<std::uint8_t>(v[9]), static_cast<std::uint8_t>(v[10]),
            static_cast<std::uint8_t>(v[11]), static_cast<std::uint16_t>(v[12])});
    }
    return result;
}
// 그래프 번호 → 표면 수.
std::map<int, std::int16_t> Records(const std::string& text) {
    std::map<int, std::int16_t> result;
    // 레코드마다 (번호, 표면 수, 사용 여부)다.
    for (const auto& v : Tuples(text)) result[static_cast<int>(v[0])] = static_cast<std::int16_t>(v[1]);
    return result;
}
// 다리 객체의 가변 상태를 만든다. 섬은 방문 목록에 들어가지 않으므로 상태가 없다.
std::vector<BridgeDecayState> States(const std::vector<Input>& inputs, const std::map<int, std::int16_t>& records) {
    std::vector<BridgeDecayState> result;
    // 종류 0(다리)만 옮긴다.
    for (const auto& input : inputs) {
        if (input.kind != 0) continue;
        const auto record = records.find(input.graph);
        result.push_back({input.id, input.word, input.frame, record == records.end() ? std::int16_t{0} : record->second,
            record != records.end(), input.extra, (input.state & 2) != 0});
    }
    return result;
}
// 원본 사건 표기로 바꾼다: D:번호:플래그, R:번호, C:칸x:칸y, S:x비트:y비트.
std::string EventText(const std::vector<BridgeDecayEvent>& events, const std::vector<Input>& inputs) {
    std::string result;
    // 번호로 입력 좌표를 찾는다.
    const auto at = [&](std::uint16_t id) -> const Input& {
        return *std::find_if(inputs.begin(), inputs.end(), [id](const Input& input) { return input.id == id; });
    };
    // 사건을 호출 순서대로 공백으로 잇는다.
    for (const auto& event : events) {
        if (!result.empty()) result += ' ';
        const auto& input = at(event.id);
        switch (event.kind) {
        case BridgeDecayEventKind::Destroy:
            result += "D:" + std::to_string(event.id) + ":" + std::to_string(event.flags); break;
        case BridgeDecayEventKind::Redraw:
            result += "R:" + std::to_string(event.id); break;
        case BridgeDecayEventKind::Carriers:
            result += "C:" + std::to_string(input.x) + ":" + std::to_string(input.y); break;
        case BridgeDecayEventKind::CrackSound:
            result += "S:" + std::to_string(std::bit_cast<std::uint32_t>(static_cast<float>(input.x))) + ":" +
                std::to_string(std::bit_cast<std::uint32_t>(static_cast<float>(input.y))); break;
        }
    }
    return result.empty() ? "-" : result;
}
// 결과 상태를 기대값의 after 열 표기(번호,단어,프레임,state)로 바꾼다.
std::string AfterText(const std::vector<Input>& inputs, const std::vector<BridgeDecayState>& states) {
    std::string result;
    // 입력 순서대로 적는다. 섬은 입력 값 그대로다.
    for (const auto& input : inputs) {
        auto word = input.word;
        int frame = input.frame;
        std::uint8_t state = input.state;
        const auto found = std::find_if(states.begin(), states.end(),
            [&](const BridgeDecayState& entry) { return entry.id == input.id; });
        if (found != states.end()) {
            word = found->word; frame = found->frame;
            if (found->dead) state = static_cast<std::uint8_t>(state | 2);
        }
        if (!result.empty()) result += ';';
        result += std::to_string(input.id) + "," + std::to_string(word) + "," + std::to_string(frame) + "," + std::to_string(state);
    }
    return result;
}
// 표면 탐색기 입력(객체 목록, 번호 지도, spot 지도)을 만든다. 결과는 계산하지 않는다.
struct Scene {
    // objects: 현재 월드 또는 수집 단계의 객체 목록. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<SurfaceObject> objects;
    // map: 지도. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint16_t> map = std::vector<std::uint16_t>(kWorldCells * kWorldCells);
    // spots: 칸별 점유·통행 비트를 읽는 spot 지도. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint8_t> spots = std::vector<std::uint8_t>(kWorldCells * kWorldCells);
    Scene(const std::vector<Input>& inputs, const RiftTypeFrames& frames, const std::string& spotText) {
        const auto codes = frames.Codes();
        // 다리는 실제 프레임 코드, 섬은 사방 연결 코드 하나를 쓴다.
        for (const auto& input : inputs) {
            const FrameCode code = input.kind == 0 ? codes[static_cast<std::size_t>(input.frame)] : FrameCode{'A', 'A', 1, 0};
            objects.push_back({input.id, input.x, input.y, input.width, input.height, input.flags1, input.flags2, code,
                (input.state & 2) != 0, (input.extra & 8) != 0});
            // 오른쪽 아래 기준점의 발자국 전체에 번호를 적는다(원본에 넣은 입력 규약과 같다).
            for (int y = input.y - input.height + 1; y <= input.y; ++y)
                // 같은 행에서 그 표면이 차지하는 열.
                for (int x = input.x - input.width + 1; x <= input.x; ++x)
                    map[static_cast<std::size_t>(y * kWorldCells + x)] = input.id;
        }
        // 지정한 spot 비트만 넣는다.
        for (const auto& v : Tuples(spotText))
            spots[static_cast<std::size_t>(v[1] * kWorldCells + v[0])] = static_cast<std::uint8_t>(v[2]);
    }
};
// 새 보호 경로가 예외를 보고하는지 확인한다.
bool Throws(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return true; }
    return false;
}
// 기대값의 정수 열에 저장한 배정도 비트를 복원한다.
double Bits(const std::string& text) { return std::bit_cast<double>(static_cast<std::uint64_t>(std::stoull(text))); }
}

// 실제 스캔 한 칸 처리(패치 004227e0, CD/10.37은 스캔 함수 안의 인라인)의 사건 순서와 결과 상태를 대조한다.
TEST_CASE(BridgeDecay_X86CellProcessingMatchesThreeBinaries) {
    const auto frames = Frames("0");
    std::size_t count = 0;
    std::map<BridgeDecayOutcome, std::size_t> outcomes;
    // 고정 장면과 무작위 장면의 모든 시작 칸.
    for (const auto& row : Fixture()) {
        if (row[0] != "Cell") continue;
        const auto inputs = Inputs(row[1]);
        const Scene scene(inputs, frames, row[3]);
        const SurfaceFinder finder(scene.objects, scene.map, scene.spots);
        const auto states = States(inputs, Records(row[2]));
        const BridgeDecayMode mode{row[4] == "1", true, false, row[5] == "1", row[6] == "1"};
        const auto result = Bridge::DecayCell(finder, frames, states, static_cast<std::uint16_t>(std::stoul(row[7])), mode);
        CHECK(result.outcome != BridgeDecayOutcome::Incomplete && result.outcome != BridgeDecayOutcome::Faulted);
        CHECK(EventText(result.events, inputs) == row[8]);
        CHECK(AfterText(inputs, result.states) == row[9]);
        ++outcomes[result.outcome]; ++count;
    }
    CHECK(count == kCellRows);
    // 기대값이 주요 분기를 모두 지났는지 확인한다.
    for (const auto outcome : {BridgeDecayOutcome::SmallGraph, BridgeDecayOutcome::Hard, BridgeDecayOutcome::Isolated,
            BridgeDecayOutcome::Closed, BridgeDecayOutcome::Blocked, BridgeDecayOutcome::Decayed})
        CHECK(outcomes[outcome] > 0);
}

// 수명 감소 전체(제거·비트 갱신·금 간 프레임·디버그 갱신·이동체 확인)를 세 PE의 실제 결과와 대조한다.
TEST_CASE(BridgeDecay_X86LifeReductionEffects) {
    const auto frames = Frames("0");
    std::size_t count = 0, guarded = 0;
    // 방향 글자·프레임 종류·이전 수명·감소량·서버 여부의 조합.
    for (const auto& row : Fixture()) {
        if (row[0] != "Life") continue;
        const auto inputs = Inputs(row[1]);
        auto states = States(inputs, Records(row[2]));
        const BridgeDecayMode mode{row[3] == "1", true, false, row[4] == "1", row[5] == "1"};
        const int reduction = std::stoi(row[6]);
        std::vector<BridgeDecayEvent> events;
        ++count;
        // 원본은 수명을 4비트(0~15)로 그대로 쓴다. 새 코드는 정상 범위 0~7을 넘는 결과를 쓰기 전에 거부한다
        // (한 칸 처리는 목표 수명이 7 이하라 이 입력을 만들지 않는다). 기대값 행은 원본 동작의 기록으로 남긴다.
        if (((inputs[0].word & kBridgeLifeMask) >> 3) - reduction > 7) {
            const auto before = states[0];
            CHECK(Throws([&] { Bridge::ApplyLife(frames, states[0], reduction, mode, events); }));
            CHECK(states[0].word == before.word && states[0].frame == before.frame && events.empty());
            ++guarded;
            continue;
        }
        CHECK(Bridge::ApplyLife(frames, states[0], reduction, mode, events));
        CHECK(EventText(events, inputs) == row[7]);
        CHECK(AfterText(inputs, states) == row[8]);
    }
    CHECK(count == kLifeRows && guarded == kLifeGuardedRows);
}

// 다리 destroy 재정의(패치 004220f0, CD/10.37 00449820)가 기본 destroy로 넘어가는 조건을 대조한다.
TEST_CASE(BridgeDecay_X86DestroyOverride) {
    const auto frames = Frames("0");
    std::size_t count = 0, blocked = 0;
    // 편집기·권한·extra·표면 수·프레임·디버그 유지·인자의 조합.
    for (const auto& row : Fixture()) {
        if (row[0] != "Destroy") continue;
        const auto inputs = Inputs(row[1]);
        const auto states = States(inputs, Records(row[2]));
        const BridgeDecayMode mode{true, row[4] == "1", row[3] == "1", false, row[5] == "1"};
        const bool proceeds = Bridge::DestroyProceeds(mode, states[0].extra, states[0].graphSurfaces,
            frames.Codes()[static_cast<std::size_t>(states[0].frame)].flags);
        CHECK(proceeds == (row[7] != "-"));
        if (proceeds) CHECK(row[7] == "D:" + std::to_string(inputs[0].id) + ":" + row[6]);
        blocked += !proceeds; ++count;
    }
    CHECK(count == kDestroyRows && blocked > 0);
}

// 금 간/보통/약화/복구 전환(패치판에만 별도 함수가 있다)을 실제 표와 프레임이 빠진 변형 표에서 대조한다.
TEST_CASE(BridgeDecay_X86FrameTransitions) {
    std::map<std::string, std::size_t> counts;
    // 표 0은 실제 bridge.type, 1·2는 일부 프레임을 뺀 표다.
    for (const auto& row : Fixture()) {
        if (row[0] != "Crack" && row[0] != "Normal" && row[0] != "Weaken" && row[0] != "Restore") continue;
        const auto frames = Frames(row[1]);
        const int frame = std::stoi(row[2]);
        const std::string redraw = "R:6100";
        if (row[0] == "Crack") {
            const int found = Bridge::CrackedFrame(frames, frame, row[3] == "1");
            CHECK((found != -1) == (row[4] == "1"));
            CHECK((found == -1 ? frame : found) == std::stoi(row[5]));
            CHECK(row[6] == (found == -1 ? "-" : redraw));
        } else if (row[0] == "Normal") {
            const int found = Bridge::NormalFrame(frames, frame);
            CHECK((found == -1 ? frame : found) == std::stoi(row[5]));
            CHECK(row[6] == (found == -1 ? "-" : redraw));
        } else {
            const auto word = static_cast<std::uint16_t>(std::stoul(row[3]));
            const auto change = row[0] == "Weaken" ? Bridge::Weaken(frames, word, frame) : Bridge::Restore(frames, word, frame);
            if (row[0] == "Weaken") CHECK(change.changed == (row[4] == "1"));
            CHECK(change.word == std::stoul(row[5]));
            CHECK(change.frame == std::stoi(row[6]));
            CHECK(row[7] == (change.changed ? redraw : "-"));
        }
        ++counts[row[0]];
    }
    CHECK(counts["Crack"] == kCrackRows && counts["Normal"] == kNormalRows);
    CHECK(counts["Weaken"] == kWeakenRows && counts["Restore"] == kWeakenRows);
}

// 스캔 초기화와 프레임별 커서 전진·주기 되돌림·처리 대상 번호를 패치판과 CD/10.37 각각의 실제 결과와 대조한다.
TEST_CASE(BridgeDecay_X86ScanCursorAndPeriod) {
    std::size_t inits = 0, scans = 0, visitedTotal = 0;
    BridgeDecayScanState state{};
    std::vector<Input> objects;
    // 시퀀스는 Init → ScanScene → Scan 행들의 순서로 저장돼 있다.
    for (const auto& row : Fixture()) {
        if (row[0] == "Init") {
            const auto edition = row[1] == "originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
            state = BridgeDecayScan::Reset(edition, Bits(row[3]));
            CHECK(state.first == std::stoi(row[4]) && state.last == std::stoi(row[5]) && state.cursor == std::stoi(row[6]));
            CHECK(std::bit_cast<std::uint64_t>(state.next) == std::stoull(row[7]));
            ++inits;
        } else if (row[0] == "ScanScene") {
            objects = Inputs(row[3]);
        } else if (row[0] == "Scan") {
            const BridgeDecayMode mode{true, row[7] == "1", row[6] == "1", false, false};
            // 각 행은 실행 전 커서·다음 시각을 가진 독립 입력이다(64비트 정밀도에서만 다른 프레임은 기대값에서 빠져 있다).
            state.cursor = std::stoi(row[8]);
            state.next = Bits(row[9]);
            const auto step = BridgeDecayScan::Advance(state, Bits(row[3]), Bits(row[4]), std::stoi(row[5]) > 0, mode);
            std::string visited;
            // 구간 안의 번호를 오름차순으로 보며 처리 대상만 모은다.
            for (int sid = step.begin; sid < step.end; ++sid) {
                const auto found = std::find_if(objects.begin(), objects.end(), [sid](const Input& input) { return input.id == sid; });
                if (found == objects.end() || !BridgeDecayScan::Eligible(found->state, found->flags2, found->extra)) continue;
                if (!visited.empty()) visited += ',';
                visited += std::to_string(sid); ++visitedTotal;
            }
            CHECK((visited.empty() ? "-" : visited) == row[10]);
            CHECK(state.cursor == std::stoi(row[11]));
            CHECK(std::bit_cast<std::uint64_t>(state.next) == std::stoull(row[12]));
            ++scans;
        }
    }
    CHECK(inits == kInitRows && scans == kScanRows && visitedTotal > 0);
}

// 원본 기본 프레임 간격에서 스캔은 프레임마다 11개 번호를 전진하고 주기 끝 프레임에 나머지를 모두 훑는다.
TEST_CASE(BridgeDecay_ScanSpreadsRangeOverPeriod) {
    auto state = BridgeDecayScan::Reset(OriginalEdition::Patch1078, 0.0);
    CHECK(state.first == 15000 && state.last == 23001 && state.next == 10.0);
    double now = 0.0;
    std::int32_t covered = 0;
    int frames = 0;
    // 14ms 프레임으로 한 주기를 지난다.
    for (; now < 10.0; ++frames) {
        now += 0.014;
        const auto step = BridgeDecayScan::Advance(state, now, 0.014, false);
        if (now < 10.0) CHECK(step.end - step.begin == 11);
        covered += step.end - step.begin;
    }
    CHECK(covered == 8002);
    CHECK(state.cursor == 15000 && state.next == BridgeDecayScan::kPeriod + now);
    // 정지·편집기·권한 없음은 커서와 다음 시각을 건드리지 않는다.
    const auto before = state;
    BridgeDecayScan::Advance(state, now + 50.0, 0.014, true);
    BridgeDecayScan::Advance(state, now + 50.0, 0.014, false, {true, true, true, false, false});
    BridgeDecayScan::Advance(state, now + 50.0, 0.014, false, {true, false, false, false, false});
    CHECK(state.cursor == before.cursor && state.next == before.next);
    // CD판은 범위가 6000..14000이다.
    const auto cd = BridgeDecayScan::Reset(OriginalEdition::Cd1072, 2.5);
    CHECK(cd.first == 6000 && cd.last == 14000 && cd.cursor == 6000 && cd.next == 12.5);
    CHECK(BridgeDecayScan::Eligible(0, TypeFlag2::kBridge, 8) && !BridgeDecayScan::Eligible(0, TypeFlag2::kBridge, 1));
    CHECK(!BridgeDecayScan::Eligible(4, TypeFlag2::kBridge, 0) && !BridgeDecayScan::Eligible(0, TypeFlag2::kIsland, 0));
}

// 원본이 정상 반환하지 않는 입력(그래프 254, 접합 고리)은 새 코드에서 상태를 바꾸지 않고 구분해 알린다.
TEST_CASE(BridgeDecay_GuardsInvalidGraphRingAndInputs) {
    const auto frames = Frames("0");
    const auto codes = frames.Codes();
    // 방향 글자의 번호 1 프레임을 찾는다(입력 구성용).
    const auto frameOf = [&](char side) {
        for (std::size_t i = 0; i < codes.size(); ++i) if (codes[i].side == side && codes[i].number == 1) return static_cast<int>(i);
        throw std::runtime_error("Missing frame");
    };
    std::vector<Input> inputs;
    // 2×2 접합 고리: F(동·남) G(남·서) / I(북·동) H(북·서).
    const std::pair<char, std::pair<int, int>> ring[]{{'F', {100, 100}}, {'G', {101, 100}}, {'I', {100, 101}}, {'H', {101, 101}}};
    for (std::size_t i = 0; i < 4; ++i)
        inputs.push_back({static_cast<std::uint16_t>(200 + i), ring[i].second.first, ring[i].second.second, 1, 1, TypeFlag1::kSurface,
            TypeFlag2::kBridge, 0, frameOf(ring[i].first), 0, 0, 1, static_cast<std::uint16_t>(3 << 3)});
    // 고리에 붙은 열린 끝 판자를 시작 칸으로 삼는다.
    inputs.push_back({204, 99, 100, 1, 1, TypeFlag1::kSurface, TypeFlag2::kBridge, 0, frameOf('K'), 0, 0, 1, 3 << 3});
    inputs[0].frame = frameOf('C'); // 동·남·서로 바꿔 판자와 잇는다.
    const Scene scene(inputs, frames, "-");
    const SurfaceFinder finder(scene.objects, scene.map, scene.spots);
    auto states = States(inputs, {{1, std::int16_t{9}}});
    const auto ringResult = Bridge::DecayCell(finder, frames, states, 204);
    CHECK(ringResult.outcome == BridgeDecayOutcome::Incomplete && ringResult.events.empty());
    CHECK(AfterText(inputs, ringResult.states) == AfterText(inputs, states));
    // 시작 칸의 그래프가 254면 아무것도 하지 않는다.
    states.back().graphValid = false;
    const auto invalid = Bridge::DecayCell(finder, frames, states, 204);
    CHECK(invalid.outcome == BridgeDecayOutcome::InvalidGraph && invalid.events.empty());
    // 제거할 칸의 그래프가 254면 그 지점에서 멈추고 dead로 표시하지 않는다.
    std::vector<BridgeDecayEvent> events;
    BridgeDecayState lost{300, 1 << 3, frameOf('K'), 0, false, 0, false};
    CHECK(!Bridge::ApplyLife(frames, lost, 1, {}, events) && !lost.dead && events.empty());
    // 서버가 아니면 수명 0에서도 destroy 재정의에 닿지 않는다.
    CHECK(Bridge::ApplyLife(frames, lost, 1, {false, true, false, false, false}, events) && lost.word == 0);
    // 입력 누락·다리가 아닌 시작 칸·범위 밖 프레임은 예외다.
    CHECK(Throws([&] { Bridge::DecayCell(finder, frames, {}, 204); }));
    CHECK(Throws([&] { Bridge::CrackedFrame(frames, static_cast<int>(codes.size()), false); }));
    CHECK(Throws([&] { Bridge::NormalFrame(frames, -1); }));
}

namespace { //기존 실제 PE의 외부 효과 경계를 동일하게 유지한 raw 입력 장면이다.
struct RawDecayScene {
    SidPool pool{OriginalEdition::Patch1078,24000}; //실제 PATCH 풀의 물리 슬롯이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //입력 순서대로70부터 TYPE를 설치한다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames(std::vector<FrameCode>{})); //원문이 읽는 각 TYPE의 코드 표다.
    std::vector<std::uint16_t> surface=std::vector<std::uint16_t>(65536); //원문 fixture의 동일 hash0 입력이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); //원문 fixture의 동일 현재 spot 입력이다.
    std::array<GraphRecord,255> graphs{}; //한 칸 입구와 가상 삭제가 읽는 동일 표면 수다.
    BridgeDecayMode mode; //원문 행의 server/권한/개발용 표시 조건이다.
    std::string events; //실제 원문과 같은 외부 호출의 순서만 적는다.
    //기대 상태를 계산하지 않고 원문 행의13정수 슬롯/프레임/표면/spot을 설치한다.
    RawDecayScene(const std::vector<Input>& inputs,const RiftTypeFrames& codes,const std::string& recordText,const std::string& spotText,BridgeDecayMode selected):mode(selected) {
        for (const auto& record:Tuples(recordText)) graphs[static_cast<std::size_t>(record[0])].surfaces=static_cast<std::int16_t>(record[1]); //원문에 지정한 Graph 레코드만 쓴다.
        for (const auto& spot:Tuples(spotText)) spots[static_cast<std::size_t>(spot[1]*256+spot[0])]=static_cast<std::uint8_t>(spot[2]); //현재 bit 위치를 그대로 보존한다.
        for (std::size_t index=0;index<inputs.size();++index) { //객체마다 원문처럼 다른 TYPE 번호를 사용한다.
            const auto& entry=inputs[index];const auto type=70U+index;CHECK(type<types.size());types[type].flags1=entry.flags1;types[type].flags2=entry.flags2;types[type].footX=entry.width;types[type].footY=entry.height;
            frames[type]=entry.kind==0 ? codes : RiftTypeFrames({FrameCode{'A','A',1,0}});auto raw=RawDecayFixtureSlot(pool,entry.id);RawDecayPut(raw,0,0x5034c8);raw[10]=static_cast<std::uint8_t>(type);raw[11]=entry.state;RawDecayPut(raw,12,entry.word,2);RawDecayPut(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(entry.x)));RawDecayPut(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(entry.y)));raw[30]=entry.graph;raw[40]=entry.extra;RawDecayPut(raw,36,static_cast<std::uint32_t>(entry.frame));
            for (int y=entry.y-entry.height+1;y<=entry.y;++y) for (int x=entry.x-entry.width+1;x<=entry.x;++x) surface[static_cast<std::size_t>(y*256+x)]=entry.id; //명시 원문 지도는 각 발자국 전체를 번호로 채운 입력이다.
        }
    }
    //사건만 구분자로 잇고 수명/프레임 결과는 이 기록기로 만들지 않는다.
    void Event(std::string value) { if (!events.empty()) events+=' ';events+=value; }
    //원문도 외부 기본 삭제/이동체/소리/표시만 stub했고 새 raw 실행기도 같은 경계를 공급한다.
    RawBridgeDecayHooks Hooks() {
        RawBridgeDecayHooks hooks;
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { const auto raw=pool.Slot(sid);const auto graph=raw[30];if (graph==254) throw std::logic_error("원문 삭제의 무효 Graph 입력");const auto code=frames[raw[10]].Codes()[RawDecayRead(raw,36)];if (!Bridge::DestroyProceeds(mode,raw[40],graphs[graph].surfaces,code.flags)) return;Event("D:"+std::to_string(sid.value)+":"+std::to_string(flags));RawDecayFixtureSlot(pool,sid.value)[11]|=2; };
        hooks.notifySurface=[](Sid) {}; //원문004214a0도 정상 수명 진단 이외의 사건을 내지 않는다.
        hooks.refresh=[this](Sid sid) { Event("R:"+std::to_string(sid.value)); };
        hooks.carriers=[this](Sid sid) { const auto raw=pool.Slot(sid);Event("C:"+std::to_string(static_cast<int>(std::bit_cast<float>(RawDecayRead(raw,14))))+":"+std::to_string(static_cast<int>(std::bit_cast<float>(RawDecayRead(raw,18))))); };
        hooks.crackSound=[this](Sid sid) { const auto raw=pool.Slot(sid);Event("S:"+std::to_string(RawDecayRead(raw,14))+":"+std::to_string(RawDecayRead(raw,18))); };return hooks;
    }
    //실제 변경된 네 필드를 입력 순서대로 직렬화하며 기대 열은 수정하지 않는다.
    std::string After(const std::vector<Input>& inputs) const {
        std::string result;for (const auto& entry:inputs) { const auto raw=pool.Slot(Sid{entry.id});if (!result.empty()) result+=';';result+=std::to_string(entry.id)+","+std::to_string(RawDecayRead(raw,12,2))+","+std::to_string(RawDecayRead(raw,36))+","+std::to_string(raw[11]); }return result; //모든 입력 객체를 생략 없이 비교한다.
    }
};
}

//독립 원문4241행의 한 칸 결과를 실제 raw float 이웃/WORD/프레임 쓰기로 재생한다.
TEST_CASE(RawBridgeDecay_NativeCellEventsAndPhysicalState) {
    const auto codes=Frames("0");std::size_t rows{};
    for (const auto& row:Fixture()) { //원본 기대값의 모든 Cell 행을 사용하고 새 golden은 만들지 않는다.
        if (row[0]!="Cell") continue;const auto inputs=Inputs(row[1]);RawDecayScene scene(inputs,codes,row[2],row[3],{row[4]=="1",true,false,row[5]=="1",row[6]=="1"});
        RawBridgeDecay decay(scene.pool,scene.types,scene.frames,scene.surface,scene.spots,scene.graphs,scene.mode,scene.Hooks());decay.Cell(Sid{static_cast<std::uint16_t>(std::stoul(row[7]))});
        const bool same=scene.events== (row[8]=="-" ? "" : row[8]) && scene.After(inputs)==row[9];CHECK(same);if (!same) { std::fprintf(stderr,"raw 다리 Cell 행%zu events=%s / %s after=%s / %s\n",rows,scene.events.c_str(),row[8].c_str(),scene.After(inputs).c_str(),row[9].c_str());break; }++rows;
    }
    CHECK(rows==kCellRows);
}

//논리 API가 보호했던 정상7 초과400행도 원문의 실제4비트 쓰기 그대로 대조한다.
TEST_CASE(RawBridgeDecay_NativeLifeIncludingWideValues) {
    const auto codes=Frames("0");std::size_t rows{};
    for (const auto& row:Fixture()) { //모든3264 Life 관측을 수명 값의 크기로 생략하지 않는다.
        if (row[0]!="Life") continue;const auto inputs=Inputs(row[1]);RawDecayScene scene(inputs,codes,row[2],"-",{row[3]=="1",true,false,row[4]=="1",row[5]=="1"});
        RawBridgeDecay decay(scene.pool,scene.types,scene.frames,scene.surface,scene.spots,scene.graphs,scene.mode,scene.Hooks());decay.ApplyLife(Sid{inputs[0].id},std::stoi(row[6]));
        const bool same=scene.events==(row[7]=="-" ? "" : row[7]) && scene.After(inputs)==row[8];CHECK(same);if (!same) { std::fprintf(stderr,"raw 다리 Life 행%zu events=%s / %s after=%s / %s\n",rows,scene.events.c_str(),row[7].c_str(),scene.After(inputs).c_str(),row[8].c_str());break; }++rows;
    }
    CHECK(rows==kLifeRows);
}
