// 자산 로더의 손상된 입력·레이어 순서·픽셀 투명도를 검사한다.
#include "TestSupport.h"
#include "client/Screen.h"
#include "client/VFXDraw.h"
#include "o/RiftType.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <functional>
#include <sstream>
#include <stdexcept>

namespace {
// 정상 동작과 구분해야 하는 잘린 파일·부적합 입력의 예외를 검사한다.
bool Throws(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return true; }
    return false;
}
// 작은 VFX 입력의 정수를 리틀 엔디언으로 기록한다.
void Put(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value, std::size_t size = 4) {
    // 지정한 정수 크기만큼 낮은 바이트부터 쓴다.
    for (std::size_t i = 0; i < size; ++i) bytes.at(offset+i) = static_cast<std::uint8_t>(value >> (i*8));
}
// 두 테이블 항목이 같은 프레임을 참조하는 입력으로 공유 오프셋을 검사한다.
std::vector<std::uint8_t> SharedShape() {
    std::vector<std::uint8_t> data(48);
    Put(data, 0, 0x30312e31); Put(data, 4, 2); Put(data, 8, 24); Put(data, 16, 24);
    Put(data, 24, 6, 2); Put(data, 26, 3, 2); Put(data, 28, 2, 2); Put(data, 30, 1, 2);
    Put(data, 32, std::bit_cast<std::uint32_t>(-2)); Put(data, 36, std::bit_cast<std::uint32_t>(-1));
    Put(data, 40, 3); Put(data, 44, 1);
    const std::vector<std::uint8_t> runs{1, 1, 5, 0, 255, 6, 4, 0, 12, 7, 0, 0};
    data.insert(data.end(), runs.begin(), runs.end());
    return data;
}
// 고정 x86 기대값의 16진수 바이트를 변환한다.
std::vector<std::uint8_t> Unhex(const std::string& hex) {
    if (hex == "-") return {};
    if (hex.size() % 2) throw std::runtime_error("Invalid graphics fixture hex");
    std::vector<std::uint8_t> bytes;
    bytes.reserve(hex.size()/2);
    // 기대값을 두 자리씩 원본 바이트로 되돌린다.
    for (std::size_t i = 0; i < hex.size(); i += 2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    return bytes;
}
}

// 프레임 코드와 도움말/기본 프레임의 덮어쓰기 순서는 원본/CD 로더에서 확인한 규칙이다.
TEST_CASE(TypeParser_PreservesCodesAndSpecialFrameOrder) {
    const auto type = netstorm::o::RiftTypeDefinition::Parse(
        "typename Sample client constructor typeflags shadow dontSave;\n"
        "{ description=\"한글; //본문\\경로\"; foot_x=7; foot_y=6; COST=1; cost=2; }\n"
        "A00: default Fringe: \"a.gif\" #01 : \"shadow.gif\" #00;\n"
        "AA260: HELP hard suck rim lit unlit cracked: \"b.gif\" #2;\n"
        "B00: default GumpFrame BaseFrame dirt: \"c.gif\" #3;\n");
    CHECK(type.name == "Sample"); CHECK(type.constructor == "client constructor");
    CHECK(type.flags.size() == 2); CHECK(type.String("DESCRIPTION") == "한글; //본문\\경로");
    CHECK(type.Number("cost") == 2); CHECK(!type.Number("description")); CHECK(!type.String("cost"));
    CHECK(!type.Property("missing"));
    CHECK((type.Footprint() == std::array<int, 2>{7, 8}));
    CHECK(type.clusters.size() == 3); CHECK(type.clusters[0].images.size() == 2);
    CHECK(type.clusters[0].images[0].number == 1); CHECK(type.clusters[0].code.variant == 'P');
    CHECK(type.clusters[1].code.side == 'A'); CHECK(type.clusters[1].code.variant == 'A');
    CHECK(type.clusters[1].code.number == 4); CHECK(type.clusters[1].code.flags == 126);
    CHECK(type.clusters[2].code.flags == 0);
    CHECK(type.specialFrames.defaultFrame == 2); CHECK(type.specialFrames.gumpFrame == 2);
    CHECK(type.specialFrames.helpFrame == 1); CHECK(type.specialFrames.baseFrame == 2);
    CHECK(type.FrameTable().FindNumber('A', 'A', 4) == 1);
    const auto defaults = netstorm::o::RiftTypeDefinition::Parse("typename Empty {} A00:default:\"a\"#0; B00:default:\"b\"#0;");
    CHECK(defaults.specialFrames.defaultFrame == 1); CHECK(defaults.specialFrames.helpFrame == 1);
    CHECK(defaults.specialFrames.gumpFrame == 0); CHECK(defaults.specialFrames.baseFrame == -1);
}

// 패치 전후의 로딩 순서 길이는 같지 않으며 앞부분만 공통이다.
TEST_CASE(TypeLoadOrder_MatchesEditionLengths) {
    const auto patch = netstorm::o::TypeLoadOrder(netstorm::o::OriginalEdition::Patch1078);
    const auto cd = netstorm::o::TypeLoadOrder(netstorm::o::OriginalEdition::Cd1072);
    CHECK(patch.size() == 116); CHECK(cd.size() == 101);
    CHECK(patch.front() == "dude"); CHECK(patch[58] == "sunCannon");
    CHECK(patch.back() == "Monument"); CHECK(cd.back() == "fenceShield");
    CHECK(std::equal(cd.begin(), cd.end(), patch.begin()));
}

// 잘린 문자열·구분자·방향·비유한 숫자는 무효 파일로 보고한다.
TEST_CASE(TypeParser_RejectsIncompleteOrInvalidDefinitions) {
    CHECK(Throws([] { netstorm::o::RiftTypeDefinition::Parse("typename A {x=\"unterminated"); }));
    CHECK(Throws([] { netstorm::o::RiftTypeDefinition::Parse("typename A {x=1}"); }));
    CHECK(Throws([] { netstorm::o::RiftTypeDefinition::Parse("typename A {} a00::\"x\"#0;"); }));
    CHECK(Throws([] { netstorm::o::RiftTypeDefinition::Parse("typename A {} A00::\"x\"#0"); }));
    CHECK(Throws([] { netstorm::o::RiftTypeDefinition::Parse("typename A {x=1e999;}"); }));
}

// 투명 건너뛰기·색 0/255·공유 오프셋·생략된 행 끝 픽셀을 보존한다.
TEST_CASE(ShapeDatabase_PreservesTransparencyAndSharedFrames) {
    const netstorm::client::ShapeDatabase database(SharedShape());
    CHECK(database.Blocks().size() == 1); CHECK(database.Blocks()[0].frames.size() == 2);
    CHECK(database.Blocks()[0].frames[0].offset == database.Blocks()[0].frames[1].offset);
    const auto image = database.Decode(0, 0);
    CHECK(image.width == 6); CHECK(image.height == 3);
    CHECK((image.indices == std::vector<std::uint8_t>{0,0,255,4,4,4, 7,7,7,7,7,7, 0,0,0,0,0,0}));
    CHECK((image.opacity == std::vector<std::uint8_t>{0,255,255,255,255,255, 255,255,255,255,255,255, 0,0,0,0,0,0}));
    CHECK(database.Decode(0, 1).indices == image.indices);
}

// 검증되지 않은 외부 파일이 다음 프레임까지 읽거나 거대한 할당을 유발하지 않게 한다.
TEST_CASE(ShapeDatabase_RejectsBrokenTablesAndRuns) {
    auto data = SharedShape(); Put(data, 4, 0xffffffff);
    CHECK(Throws([&] { netstorm::client::ShapeDatabase bad(data); }));
    data = SharedShape(); Put(data, 8, 0xffffffff);
    CHECK(Throws([&] { netstorm::client::ShapeDatabase bad(data); }));
    data = SharedShape(); Put(data, 8, 8);
    CHECK(Throws([&] { netstorm::client::ShapeDatabase bad(data); }));
    data = SharedShape(); data.pop_back();
    CHECK(Throws([&] { netstorm::client::ShapeDatabase(data).Decode(0, 0); }));
    data = SharedShape(); data[49] = 7;
    CHECK(Throws([&] { netstorm::client::ShapeDatabase(data).Decode(0, 0); }));
    data = SharedShape(); Put(data, 40, 0x7fffffff);
    CHECK(Throws([&] { netstorm::client::ShapeDatabase(data).Decode(0, 0); }));
    data = SharedShape(); Put(data, 32, 0x7fff0000); Put(data, 40, 0x7fff0001);
    const netstorm::client::ShapeDatabase special(data);
    CHECK(special.Blocks()[0].frames[0].IsSpecial());
    CHECK(Throws([&] { special.Decode(0, 0); }));
}

// COL은 RGB, 0x400바이트 형식은 BGRX다. 색 0도 임의로 투명화하지 않는다.
TEST_CASE(GamePalette_ReadsBothOriginalFormats) {
    std::vector<std::uint8_t> col(0x308), bgrx(0x400);
    col[8] = 10; col[9] = 20; col[10] = 30;
    bgrx[0] = 30; bgrx[1] = 20; bgrx[2] = 10; bgrx[3] = 93;
    const netstorm::client::GamePalette first(col), second(bgrx);
    CHECK(first.Color(0).red == second.Color(0).red); CHECK(first.Color(0).green == 20);
    CHECK(first.Color(0).blue == 30); CHECK(second.Color(0).red == 10);
    col.pop_back(); CHECK(Throws([&] { netstorm::client::GamePalette invalid(col); }));
}

// 두 원본 x86 그리기 함수의 반환값과 화면 전체를 새 C++ 구현과 대조한다.
TEST_CASE(VFXDraw_MatchesBothOriginalX86Editions) {
    std::ifstream fixture(NETSTORM_GRAPHICS_FIXTURE);
    CHECK(fixture.is_open());
    std::string line;
    std::size_t count = 0;
    // 색 0/255·모든 방향 클리핑·원점 이동·오류 반환이 고정 기대값으로 저장되어 있다.
    while (std::getline(fixture, line)) {
        if (line.empty() || line.front() == '#') continue;
        std::istringstream stream(line);
        std::string shapeHex, expectedHex;
        int width = 0, height = 0, x = 0, y = 0, background = 0, expectedResult = 0;
        netstorm::client::ShapeRect pane;
        stream >> shapeHex >> width >> height >> pane.left >> pane.top >> pane.right >> pane.bottom >> x >> y >> background >> expectedResult >> expectedHex;
        CHECK(!stream.fail());
        const netstorm::client::ShapeDatabase database(Unhex(shapeHex));
        const auto size = width > 0 && height > 0 ? static_cast<std::size_t>(width)*static_cast<std::size_t>(height) : 0;
        std::vector<std::uint8_t> canvas(size, static_cast<std::uint8_t>(background));
        const auto actual = netstorm::client::DrawShape(canvas, width, height, database, 0, 0, x, y, pane);
        const auto expected = Unhex(expectedHex);
        if (actual != expectedResult || canvas != expected) std::fprintf(stderr, "VFX oracle row %zu differs\n", count+1);
        CHECK(actual == expectedResult); CHECK(canvas == expected);
        ++count;
    }
    CHECK(count == 709);
}
