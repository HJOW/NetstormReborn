// 실제 원본 x86 관찰과 번개의 화면 수명/정지 경계를 검사한다. 원본 게임·소리 장치는 실행하지 않는다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TestSupport.h"
#include "client/PaletteFlash.h"
#include "o/GameClock.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace netstorm::client;
namespace {
// fixture의 double 바이트를 little-endian으로 복원한다. NaN/무한대도 원본 저장 값을 유지한다.
double ReadDouble(const std::string& hex) {
    std::uint64_t word=0;
    // 원본의 8바이트를 각각 정해진 자리로 합친다.
    for (std::size_t i=0;i<8;++i) word|=std::stoull(hex.substr(i*2,2),nullptr,16)<<(i*8);
    return std::bit_cast<double>(word);
}
// RGBQUAD 배열을 little-endian byte hex로 바꾼다. 예약 바이트도 실제 관찰과 대조한다.
std::string ColorHex(const ScreenColor* colors,unsigned count) {
    const auto* bytes=reinterpret_cast<const std::uint8_t*>(colors);std::string hex;
    // 각 byte를 두 자리 hex로 기록한다.
    for (unsigned i=0;i<count*4;++i) { hex.push_back("0123456789abcdef"[bytes[i]>>4]);hex.push_back("0123456789abcdef"[bytes[i]&15]); }
    return hex.empty() ? "-" : hex;
}
// byte hex를 부분 팔레트의 메모리 배치로 읽는다. 색의 R/B 순서를 바꾸지 않는다.
void ReadColors(const std::string& hex,ScreenColor* colors,unsigned count) {
    auto* bytes=reinterpret_cast<std::uint8_t*>(colors);
    // 네 바이트 모두 입력 자료 그대로 복원한다.
    for (unsigned i=0;i<count*4;++i) bytes[i]=static_cast<std::uint8_t>(std::stoul(hex.substr(i*2,2),nullptr,16));
}
// 관찰 경계의 순서를 이어 붙인다. 빈 trace는 TSV의 '-' 표기를 사용한다.
void Append(std::string& trace,const std::string& event) { if (!trace.empty()) trace+='|';trace+=event; }
}

// 세 원본의 초기화/색 복사/간격과 10.78의 전체 진행·세 판본의 중단을 대조한다.
// CD/10.37 진행에는 초기화되지 않은 스택을 읽는 원본 버그가 있어 복원 목표 10.78과 같다고 취급하지 않는다.
TEST_CASE(PaletteFlash_MatchesThreeNativeBodies) {
    std::ifstream input(NETSTORM_PALETTEFLASH_FIXTURE);CHECK(input.good());std::string line;std::size_t cases=0;
    // 주석을 건너뛰고 각 독립 입력을 복원 모듈에 전달한다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::string edition,op,durationHex,wallHex,nowHex,paletteHex,initialHex,originalHex,currentHex,intervalHex,lastHex,expectedTrace;
        int full{},offset{},step{},remaining{},initialized{};unsigned start{},count{};std::uint32_t solid{};
        row>>edition>>op>>full>>offset>>durationHex>>start>>count>>wallHex>>nowHex>>step>>solid>>paletteHex
            >>initialHex>>originalHex>>currentHex>>intervalHex>>lastHex>>remaining>>initialized>>expectedTrace;
        CHECK(!row.fail());if (row.fail()) continue;
        PaletteFlashState state;state.start=start;state.count=count;state.offset=offset;state.duration=ReadDouble(durationHex);
        std::memcpy(&state.solid,&solid,4);
        if (op=="entry") { state.offset=ThunderOffset(offset);state.duration=ThunderDuration(full!=0); }
        std::array<ScreenColor,256> palette{};ReadColors(paletteHex,palette.data()+start,count);
        double wall=ReadDouble(wallHex);std::string trace;unsigned reads=0;
        PaletteFlash flash(state,{
            [&]() { return wall; },[&]() { return full!=0; },
            [&](unsigned first,unsigned size,ScreenColor* destination) { CHECK(first==start && size==count);std::copy_n(palette.data()+first,size,destination);++reads; },
            [&](unsigned first,unsigned size,ScreenColor* colors) { Append(trace,"apply:"+std::to_string(first)+":"+std::to_string(size)+":"+(size ? ColorHex(colors,size) : "")); },
            [&]() { Append(trace,"restore"); }
        });
        flash.Init();CHECK(ColorHex(state.current.data(),count)==initialHex);if (op!="entry") state.steps=step;
        CHECK(reads==2);CHECK(ColorHex(state.original.data(),count)==originalHex);
        CHECK(std::bit_cast<std::uint64_t>(state.interval)==std::bit_cast<std::uint64_t>(ReadDouble(intervalHex)));
        if (edition!="originals" && op!="cancel") { ++cases;continue; }
        if (op=="cancel") flash.Cancel();else { wall=ReadDouble(nowHex);if (!flash.Frame()) Append(trace,"kill"); }
        const auto before=netstorm::test::FailureCount();
        CHECK(reads==2);CHECK(ColorHex(state.original.data(),count)==originalHex);CHECK(ColorHex(state.current.data(),count)==currentHex);
        CHECK(std::bit_cast<std::uint64_t>(state.interval)==std::bit_cast<std::uint64_t>(ReadDouble(intervalHex)));
        CHECK(std::bit_cast<std::uint64_t>(state.last)==std::bit_cast<std::uint64_t>(ReadDouble(lastHex)));
        CHECK(state.steps==remaining && state.initialized==(initialized!=0));CHECK((trace.empty() ? "-" : trace)==expectedTrace);
        if (before!=netstorm::test::FailureCount()) std::fprintf(stderr,"PaletteFlash fixture: %s %s case %zu\n",edition.c_str(),op.c_str(),cases);
        ++cases;
    }
    CHECK(cases==768);
}

// 화면 모드는 매 프레임 읽고 수명은 생성 시 정한다. 브리핑 정지 중에도 실시간의 64단계를 끝낸다.
TEST_CASE(PaletteFlash_PausedGameAndModeChangesUseWallClock) {
    netstorm::o::GameClock clock(0);clock.Pause(0);double wall=0;bool full=false;int writes=0,restores=0;
    PaletteFlashState state;state.duration=ThunderDuration(true);
    PaletteFlash flash(state,{
        [&]() { return wall; },[&]() { return full; },
        [](unsigned,unsigned count,ScreenColor* colors) { std::fill_n(colors,count,ScreenColor{10,20,30,99}); },
        [&](unsigned,unsigned,ScreenColor*) { ++writes; },[&]() { ++restores; }
    });
    flash.Init();CHECK(flash.Frame() && writes==0);
    full=true;wall=clock.Capture(125).wall;CHECK(clock.IsPaused() && clock.Capture(125).game==0);
    CHECK(flash.Frame() && writes==1 && restores==0);
    wall=clock.Capture(510).wall;CHECK(!flash.Frame() && restores==1);
    flash.Cancel();CHECK(restores==1);
}

// 초기 RGB 부분 배열 대신 당시의 저장 팔레트를 복구한다. 별도 GDI 메모리 DC에서 실제 부분 적용/복구를 확인한다.
TEST_CASE(PaletteFlash_CancelRestoresLatestSavedScreenPalette) {
    // 창 없는 참조 DC를 소유한다. Screen이 먼저 사라지고 DC를 마지막에 해제한다.
    struct ReferenceDc { HDC value{CreateCompatibleDC(nullptr)};~ReferenceDc() { DeleteDC(value); } } dc;
    Screen screen(nullptr,dc.value,640,480);screen.Init();
    std::array<std::uint8_t,1024> bytes{};std::fill(bytes.begin(),bytes.end(),std::uint8_t{20});screen.LoadPalette(GamePalette(bytes));
    PaletteFlashState state;state.duration=0.5;double wall=0;int restores=0;
    PaletteFlash flash(state,{
        [&]() { return wall; },[]() { return true; },
        [&](unsigned start,unsigned count,ScreenColor* destination) { std::copy_n(screen.Palette().data()+start,count,destination); },
        [&](unsigned start,unsigned count,ScreenColor* colors) { screen.SetPalette(start,count,colors,true); },
        [&]() { ++restores;screen.SetPalette(0,256,nullptr,true); }
    });
    flash.Init();CHECK(flash.Frame());CHECK(screen.Palette()[228].red==84 && screen.Palette()[227].red==20 && screen.Palette()[246].red==20);
    // 효과 도중 새 저장 팔레트를 읽었으면 원본 null 팔레트 복구는 새 색을 사용한다.
    std::fill(bytes.begin(),bytes.end(),std::uint8_t{30});screen.LoadPalette(GamePalette(bytes));wall=0.001;CHECK(flash.Frame());
    flash.Cancel();flash.Cancel();CHECK(restores==1 && screen.Palette()[228].red==30 && screen.Palette()[227].red==30);
}

// 빈 필수 경계와 부분 배열 밖의 색 범위를 OS 자원 없이 거부한다.
TEST_CASE(PaletteFlash_RejectsIncompleteHooksAndInvalidRange) {
    PaletteFlashState state;bool incomplete=false;
    try { PaletteFlash flash(state,{}); } catch (const std::invalid_argument&) { incomplete=true; }
    CHECK(incomplete);state.start=255;state.count=2;bool range=false;
    try { PaletteFlash flash(state,{[]() { return 0.0; },[]() { return false; },[](unsigned,unsigned,ScreenColor*) {},[](unsigned,unsigned,ScreenColor*) {},[]() {}}); }
    catch (const std::out_of_range&) { range=true; }
    CHECK(range);
}
