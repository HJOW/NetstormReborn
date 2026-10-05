// 두 원본 x86의 실제 결과로 다리 추첨·반복·열린 끝·수명 접두 구간을 대조한다.
#include "TestSupport.h"
#include "o/Bridge.h"
#include "o/CanonDecoder.h"
#include "o/TerrainBuilder.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace {
// TSV와 상태 목록을 지정 구분자로 분리한다.
std::vector<std::string> Split(std::string_view text, char separator) {
    std::vector<std::string> result;
    std::istringstream input{std::string(text)};
    std::string part;
    // 비어 있지 않은 필드들을 순서대로 보존한다.
    while (std::getline(input, part, separator)) result.push_back(part);
    return result;
}
// 원본 실행 파일이 없는 빌드 환경에서도 Git 포함 기대값을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows = [] {
        std::ifstream input(NETSTORM_BRIDGE_FIXTURE);
        if (!input) throw std::runtime_error("Missing bridge x86 fixture");
        std::vector<std::vector<std::string>> result;
        std::string line;
        // 주석만 건너뛰고 모든 기대값을 보존한다.
        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty() && line.front() != '#') result.push_back(Split(line, '\t'));
        }
        return result;
    }();
    return rows;
}
// 원본 타입 코드 바이트를 새로운 파서와 관계없이 읽는다.
std::vector<FrameCode> FrameCodes(char kind, bool subset = false) {
    // 기대값 머리의 타입별 코드 표를 찾는다.
    for (const auto& row : Fixture()) {
        if (row[0] != "Frames" || row[1].front() != kind) continue;
        std::vector<FrameCode> codes;
        const auto& hex = row[2];
        // 네 바이트 코드마다 원본 순서를 유지한다.
        for (std::size_t i = 0; i < hex.size(); i += 8) {
            FrameCode code{};
            auto* bytes = reinterpret_cast<std::uint8_t*>(&code);
            // 각 바이트를 16진수 두 글자에서 복원한다.
            for (std::size_t byte = 0; byte < 4; ++byte)
                bytes[byte] = static_cast<std::uint8_t>(std::stoul(hex.substr(i+byte*2, 2), nullptr, 16));
            if (!subset || (code.side != 'F' && code.side != 'L')) codes.push_back(code);
        }
        return codes;
    }
    throw std::runtime_error("Missing bridge frame codes");
}
// TSV의 정수에 저장한 단정도 좌표를 손실 없이 복원한다.
float Coordinate(const std::string& text) { return std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(text))); }
// 새로운 안전 처리 경로가 예외를 보고하는지 확인한다.
bool Throws(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return true; }
    return false;
}
}

// 패치판과 CD판의 실제 가중치/추첨 차이를 보존하고 난수 반환과 다음 상태를 함께 검사한다.
TEST_CASE(Bridge_X86PatternSelectionAndRandomState) {
    std::size_t draws = 0, randoms = 0, different = 0;
    // 실제 두 판본 각각의 기대값과 같은 입력을 대조한다.
    for (const auto& row : Fixture()) {
        if (row[0] == "Draw") {
            const auto value = static_cast<std::int32_t>(std::stoll(row[1]));
            const auto patch = SelectBridgePattern(value), cd = SelectBridgePattern(value, OriginalEdition::Cd1072);
            CHECK(patch == std::stoul(row[2])); CHECK(cd == std::stoul(row[4]));
            CHECK(row[3] == "287" && row[5] == "306");
            different += patch != cd; ++draws;
        } else if (row[0] == "Random") {
            auto state = static_cast<std::uint32_t>(std::stoul(row[1]));
            CHECK(TerrainBuilder::Next(state, std::stoi(row[2])) == std::stoi(row[3]));
            CHECK(state == std::stoul(row[4])); ++randoms;
        }
    }
    CHECK(draws == 10005 && randoms == 1056 && different > 0);
}

// 두 원본 생성자·Advance의 전체 출력 목록과 C++ 반복자의 프레임/좌표 비트/라벨/방향을 대조한다.
TEST_CASE(Bridge_X86CanonDecoderFullSequences) {
    std::size_t count = 0;
    // 다리 26개와 영역 68개의 모든 회전·누락 프레임 사례를 사용한다.
    for (const auto& row : Fixture()) {
        if (row[0] != "Decode") continue;
        const char kind = row[1].front();
        const RiftTypeFrames frames(FrameCodes(kind, row[4] == "1"));
        const auto patterns = kind == 'B' ? BridgePatterns() : TerritoryPatterns();
        CanonDecoder decoder(frames, patterns[std::stoul(row[2])], std::stoi(row[3]), Coordinate(row[5]), Coordinate(row[6]));
        const auto expected = row[7] == "-" ? std::vector<std::string>{} : Split(row[7], ';');
        // 각 호출이 반환하는 유효 셀만 기대값 순서로 검사한다.
        for (const auto& entry : expected) {
            CHECK(decoder.Valid()); if (!decoder.Valid()) break;
            const auto fields = Split(entry, ',');
            CHECK(decoder.Frame() == std::stoi(fields[0]));
            CHECK(std::bit_cast<std::uint32_t>(decoder.X()) == std::stoul(fields[1]));
            CHECK(std::bit_cast<std::uint32_t>(decoder.Y()) == std::stoul(fields[2]));
            CHECK(decoder.Label() == std::stoi(fields[3])); CHECK(decoder.Side() == std::stoi(fields[4]));
            decoder.Advance();
        }
        CHECK(!decoder.Valid()); ++count;
    }
    CHECK(count == 752);
}

// 실제 표면 번호/이웃 목록으로 한 방향 검사는 두 판본, 전체 판단은 패치판 기대값에 대조한다.
TEST_CASE(Bridge_X86OpenDirectionAndSurfaceBoundaries) {
    std::vector<std::uint16_t> surface(kWorldCells*kWorldCells);
    std::size_t count = 0;
    // A~P와 8방향, 지도 가장자리·소수·번호 0·이웃 포함 여부를 모두 검사한다.
    for (const auto& row : Fixture()) {
        if (row[0] != "Open") continue;
        const auto candidate = std::stoul(row[5]);
        if (candidate == 65536) {
            // 65536은 비균일 시험 지도 표시다. 실제 표면 번호는 0/7/8/9다.
            constexpr std::uint16_t ids[]{0, 7, 8, 9};
            // 입력 지도의 각 칸을 원본에 전달한 것과 같은 번호로 채운다.
            for (int y = 0; y < kWorldCells; ++y) {
                // 각 행의 x에 따라 번호를 바꾼다.
                for (int x = 0; x < kWorldCells; ++x)
                    surface[static_cast<std::size_t>(y*kWorldCells+x)] = ids[(x+3*y) % 4];
            }
        } else {
            std::fill(surface.begin(), surface.end(), static_cast<std::uint16_t>(candidate));
        }
        std::vector<int> members;
        // '-'는 빈 이웃 목록이다.
        if (row[6] != "-") for (const auto& id : Split(row[6], ',')) members.push_back(std::stoi(id));
        const char side = row[1].front();
        const float x = Coordinate(row[3]), y = Coordinate(row[4]);
        CHECK(Bridge::IsOpen(side, std::stoi(row[2]), x, y, surface, members) == (row[7] == "1"));
        CHECK(Bridge::OpenDirection(side, x, y, surface, members) == std::stoi(row[8])); ++count;
    }
    CHECK(count == 5760);
}

// 수명 접두 구간은 실제 저장 단어와 실제 가상 삭제 호출의 인자를 검증한다.
TEST_CASE(Bridge_X86LifetimePrefixPreservesUnrelatedBits) {
    std::size_t count = 0;
    // 전투/편집기·보통/끝/접합 칸·0 제한·수명 증가를 검사한다.
    for (const auto& row : Fixture()) {
        if (row[0] != "Life") continue;
        const auto result = Bridge::ReduceLife(static_cast<std::uint16_t>(std::stoul(row[1])), std::stoi(row[2]), row[3] == "1", row[4].front());
        CHECK(result.word == std::stoul(row[5])); CHECK(result.remove == (row[6] == "1"));
        CHECK(result.removalFlags == std::stoul(row[7])); ++count;
    }
    CHECK(count == 480);
}

// 형상은 같지만 추첨 가중치는 달라야 한다. 0 경계와 CD에서만 가능한 24번도 확인한다.
TEST_CASE(Bridge_WeightsPreserveEditionDifferences) {
    const auto patch = BridgePatterns(), cd = BridgePatterns(OriginalEdition::Cd1072);
    CHECK(patch.size() == 26 && cd.size() == 26);
    CHECK(patch[22].first == 1 && cd[22].first == 10);
    CHECK(patch[23].first == 1 && cd[23].first == 10);
    CHECK(patch[24].first == 0 && cd[24].first == 1);
    CHECK(SelectBridgePattern(0) == 0 && SelectBridgePattern(0, OriginalEdition::Cd1072) == 0);
    // 모든 도형은 판본 사이에서 폭·높이·셀 바이트가 같다.
    for (std::size_t i = 0; i < patch.size(); ++i)
        CHECK(patch[i].width == cd[i].width && patch[i].height == cd[i].height && patch[i].cells == cd[i].cells);
    bool reached = false;
    // CD의 전체 나머지 구간에서 24번을 찾는다.
    for (int i = 0; i < 306; ++i) reached |= SelectBridgePattern(i, OriginalEdition::Cd1072) == 24;
    CHECK(reached);
}

// 원본의 비정상 입력 assert/정의되지 않은 좌표 변환은 새 코드에서 예외로 보고한다.
TEST_CASE(Bridge_RejectsInvalidDirectionsMapsAndLife) {
    std::vector<std::uint16_t> surface(kWorldCells*kWorldCells);
    CHECK(Throws([&] { Bridge::IsOpen('J', 8, 0, 0, surface, {}); }));
    CHECK(Throws([&] { Bridge::IsOpen('J', 0, 0, 0, {}, {}); }));
    CHECK(Throws([&] { Bridge::IsOpen('J', 0, std::numeric_limits<float>::infinity(), 0, surface, {}); }));
    CHECK(Throws([&] { Bridge::OpenDirection('Q', 0, 0, surface, {}); }));
    CHECK(Throws([&] { Bridge::ReduceLife(8 << 3, 1, false, 'J'); }));
    CHECK(Throws([&] { Bridge::ReduceLife(7 << 3, -1, false, 'J'); }));
}
