// 메뉴의 클릭 경계·정지 중 전환·원본 조건·GIF 손상 입력을 실제 게임 없이 검사한다.
#include "TestSupport.h"
#include "client/DialogScript.h"
#include "client/GifImage.h"
#include "client/Gump.h"
#include "client/Renderer.h"
#include "client/State.h"
#include "o/BaseFile.h"
#include <fstream>
#include <chrono>
#include <stdexcept>

namespace {
// 잘린 자료가 정상 경로로 받아들여지지 않는지 확인한다.
template<class Function> bool Throws(Function function) { try { function(); } catch (const std::exception&) { return true; } return false; }
// GIF 최소 코드 크기 2의 고정 테스트 자료. RGB 표와 팔레트 번호가 다르도록 구성한다.
std::vector<std::uint8_t> Gif(bool interlaced, bool transparent) {
    std::vector<std::uint8_t> bytes{'G','I','F','8','9','a',2,0,3,0,0x81,0,0, 255,0,0, 0,255,0, 0,0,255, 99,87,65};
    if (transparent) bytes.insert(bytes.end(), {0x21,0xf9,4,1,0,0,2,0});
    bytes.insert(bytes.end(), {0x2c,0,0,0,0,2,0,3,0,static_cast<std::uint8_t>(interlaced ? 0x40 : 0),2});
    const std::vector<unsigned> codes = interlaced ? std::vector<unsigned>{4,0,4,1,4,0,4,3,4,2,4,3,5} : std::vector<unsigned>{4,0,4,1,4,2,4,3,4,0,4,3,5};
    std::vector<std::uint8_t> packed((codes.size() * 3 + 7) / 8);
    // 각 리터럴 앞 clear를 넣어 코드 폭을 3으로 고정한다.
    for (std::size_t i = 0; i < codes.size(); ++i)
        // 낮은 비트부터 저장한다.
        for (unsigned bit = 0; bit < 3; ++bit) packed[(i * 3 + bit) / 8] |= static_cast<std::uint8_t>(((codes[i] >> bit) & 1) << ((i * 3 + bit) % 8));
    bytes.push_back(static_cast<std::uint8_t>(packed.size())); bytes.insert(bytes.end(), packed.begin(), packed.end()); bytes.insert(bytes.end(), {0,0x3b});
    return bytes;
}
// 단위 테스트 전용 임시 디렉터리. 예외에도 이 테스트가 만든 폴더만 정리한다.
struct TemporaryDirectory {
    std::filesystem::path path;
    // 서로 다른 실행이 같은 이름을 쓰지 않도록 현재 시계값을 붙인다.
    TemporaryDirectory() : path(std::filesystem::temp_directory_path() / ("netstorm-menu-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) { std::filesystem::create_directory(path); }
    // 원본 경로와 무관한 생성 폴더만 삭제한다.
    ~TemporaryDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
};
}
// 돌 버튼은 누름/뗌이 같은 버튼에 있어야 한다. 밖으로 나갔다 돌아오는 캡처는 유지된다.
TEST_CASE(Gump_StoneCaptureAndInputBoundaries) {
    using namespace netstorm::client;
    GumpInput input; input.SetControls({{7,{10,20,85,40},false,true,false},{9,{90,20,165,40},false,false,false}});
    CHECK(!input.Event({InputCode::kRightButton,20,25})); CHECK(!input.Pressed());
    CHECK(!input.Event({InputCode::kLeftButton,20,25})); CHECK(input.Pressed() == 7);
    CHECK(input.Move({85,25})); CHECK(!input.Pressed());
    CHECK(input.Move({10,39})); CHECK(input.Pressed() == 7);
    CHECK(input.Event({InputCode::kLeftButton | InputCode::kRelease,10,39}) == 7);
    CHECK(!input.Event({InputCode::kLeftButton,20,40})); CHECK(!input.Pressed());
    CHECK(!input.Event({InputCode::kLeftButton,90,20})); CHECK(!input.Pressed());
    input.Event({InputCode::kLeftButton,20,25}); input.Move({0,0});
    CHECK(!input.Event({InputCode::kLeftButton | InputCode::kRelease,0,0}));
    input.Event({InputCode::kLeftButton,20,25}); input.Move({20,25},false);
    CHECK(!input.Event({InputCode::kLeftButton | InputCode::kRelease,20,25}));
}
// 목록은 누름 즉시 실행하며 화면 변경 뒤 이전 캡처를 재사용하지 않는다.
TEST_CASE(Gump_MenuRowsAndSceneChange) {
    using namespace netstorm::client; GumpInput input;
    input.SetControls({{2,{0,0,100,18},true,true,false}});
    CHECK(input.Event({InputCode::kLeftButton,99,17}) == 2);
    CHECK(!input.Event({InputCode::kLeftButton | InputCode::kRelease,99,17}));
    input.SetControls({{2,{0,0,100,18},false,true,false}}); input.Event({InputCode::kLeftButton,10,10});
    input.SetControls({{2,{0,0,100,18},false,true,false}});
    CHECK(!input.Event({InputCode::kLeftButton | InputCode::kRelease,10,10}));
}
// 원본은 중첩 조건 스택이 아니라 숨김 한 상태와 첫 닫힘으로 표시를 복구한다.
TEST_CASE(Dialog_ConditionsAndQuotedCommandArguments) {
    using namespace netstorm::client;
    CHECK(FilterDialogConditions("a<?0>x<?1>y</?>z</?>b") == "azb");
    CHECK(FilterDialogConditions("<?!0>a</?><??\"A=B\"=\"A=B\">b</?><?ge3=3>c</?><?l4=3>x</?>") == "abc");
    CHECK(FilterDialogConditions("<??evil=evil>a</?><??ge3=3>b</?><?!g!4=3>x</?><?\"1\">c</?>") == "abc");
    const auto page = ParseDialogPage("<background=\"nsstart\"><h2>Campaign</h2><p>Body.\n$Checked=Locked,MissionBegin,next,0,0\n$Button=\"Back, please\",Tell,Blank\n$Timeout=120,Tell,Blank");
    CHECK(page.background == "nsstart"); CHECK(page.paragraphs.size() == 2); CHECK(page.paragraphs[0].heading == 2);
    CHECK(page.items.size() == 2); CHECK(!page.items[0].enabled); CHECK(page.items[0].menu); CHECK(page.items[1].label == "Back, please");
    CHECK(page.items[1].action.argument == "Blank"); CHECK(page.timeoutSeconds == 120);
}
// 같은 프레임의 입력 두 개로 여러 장면을 건너뛰지 않는다.
TEST_CASE(State_OneDeferredTransitionPerInputBatch) {
    netstorm::client::State state; state.Post({"Tell","UCampaign"}); state.Post({"Quit","0"});
    CHECK(state.phase == netstorm::client::ClientPhase::MainMenu);
    const auto first = state.Take(); CHECK(first && first->argument == "UCampaign"); CHECK(!state.Take());
    state.Post({"MissionBegin","TheWarBegins"}); CHECK(state.Take()->command == "MissionBegin");
}
// 원본 GIF의 번호를 RGB 색의 순서로 바꾸지 않으며 인터레이스/투명 마스크도 보존한다.
TEST_CASE(Gif_IndicesInterlaceTransparencyAndTruncation) {
    const std::vector<std::uint8_t> expected{0,1,2,3,0,3};
    const auto image = netstorm::client::DecodeGif(Gif(false,true));
    CHECK(image.width == 2 && image.height == 3); CHECK(image.indices == expected);
    CHECK(image.opacity == std::vector<std::uint8_t>({255,255,0,255,255,255}));
    CHECK(netstorm::client::DecodeGif(Gif(true,false)).indices == expected);
    const auto full = Gif(false,false);
    // 첫 이미지의 끝 앞 모든 잘린 길이는 실패해야 한다(마지막 GIF trailer는 선택적).
    for (std::size_t size = 0; size + 1 < full.size(); ++size) CHECK(Throws([&] { netstorm::client::DecodeGif(std::span(full).first(size)); }));
    auto bad = full; bad[6] = 0; bad[7] = 0; CHECK(Throws([&] { netstorm::client::DecodeGif(bad); }));
}
// 메뉴가 만든 배경은 호출자 임시 이미지가 없어져도 남고, 새 배경으로 완전히 지워진다.
TEST_CASE(Renderer_OwnedBackgroundAndReplacement) {
    using namespace netstorm::client; Renderer renderer(2,2); std::vector<std::uint8_t> pixels(4);
    auto background = std::make_shared<IndexedImage>(IndexedImage{2,2,{1,2,3,4},{255,255,255,255}});
    renderer.SetBackground(background); background.reset(); CHECK(renderer.SceneImage().indices == std::vector<std::uint8_t>({1,2,3,4}));
    CHECK(renderer.DrawCount() == 0); renderer.Draw(pixels,2); CHECK(pixels == std::vector<std::uint8_t>({1,2,3,4}));
    renderer.Present([](ScreenRect) {}); renderer.SetBackground(nullptr); renderer.Draw(pixels,2,9); CHECK(pixels == std::vector<std::uint8_t>({9,9,9,9}));
}
// 원본 캠페인 목록은 파일 이름을 대소문자 없이 정렬하고 디스크 두 경로의 중복을 합친다.
TEST_CASE(BaseFile_WildcardCampaignDiscovery) {
    TemporaryDirectory base, secondary;
    std::filesystem::create_directory(base.path / "d"); std::filesystem::create_directory(secondary.path / "d");
    std::ofstream(base.path / "d/offical3.english") << "disk"; std::ofstream(base.path / "d/offical1.english") << "first";
    std::ofstream(base.path / "d/unrelated.english") << "other"; std::ofstream(secondary.path / "d/offical3.english") << "shadowed";
    std::ofstream(secondary.path / "d/offical2.english") << "second";
    netstorm::o::BaseFileSystem files(base.path, secondary.path);
    CHECK(files.Match("D\\OFFICAL?.ENGLISH") == std::vector<std::string>({"d/offical1.english","d/offical2.english","d/offical3.english"}));
    CHECK(files.Match("d/missing*.english").empty()); CHECK(files.Read("d/offical3.english") == std::vector<std::uint8_t>({'d','i','s','k'}));
}
