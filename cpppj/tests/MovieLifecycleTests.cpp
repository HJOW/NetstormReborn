// 원본 MovieGump setup/end/draw의59사례를 전체 팔레트·좌표·장치 순서로 비교한다.
#include "TestSupport.h"
#include "client/MovieLifecycle.h"
#include <fstream>
#include <string>
#include <vector>

namespace { // 장치를 실행하지 않는 원문 관측 재생 도우미다.
using namespace netstorm::client;
struct LifecycleStop {}; // 원본과 같은 장치 진입 직전 중단 신호다.
// 마지막 빈 칸까지 보존하는 원시 TSV 분리기다.
std::vector<std::string> LifeSplit(std::string_view text) {
    std::vector<std::string> values;std::size_t start{}; // 순서 있는 칸과 다음 시작 위치다.
    // 탭마다 다음 칸을 만들고 마지막 행 끝도 하나의 칸으로 읽는다.
    for (;;) { const auto end=text.find('\t',start);values.emplace_back(text.substr(start,end==text.npos ? end : end-start));if (end==text.npos) return values;start=end+1; }
}
// RGBQUAD의 네 BYTE 모두를 최초 관측과 같은 문자열로 변환한다.
std::string LifePalette(std::span<const ScreenColor,256> colors) {
    constexpr char digits[]="0123456789abcdef";std::string result; // 원문 BYTE 순서/소문자 nibble이다.
    // 모든 색에서 예약BYTE까지 순서대로 비교한다.
    for (auto color:colors) for (auto byte:{color.blue,color.green,color.red,color.reserved}) { result+=digits[byte>>4];result+=digits[byte&15]; }
    return result;
}
// 현재 사각형을 원문 표기의 네 signed 십진수로 직렬화한다.
std::string LifeRect(ScreenRect rect) { return std::to_string(rect.left)+','+std::to_string(rect.top)+','+std::to_string(rect.right)+','+std::to_string(rect.bottom); }
}

// 실제 ftol/중앙 배치·시작/종료 복원·clip/버퍼/팔레트 관문과 선택 중단 부분효과를 대조한다.
TEST_CASE(MovieLifecycle_Native59BeginEndDrawAndCurrentOptions) {
    std::ifstream file(NETSTORM_MOVIE_LIFECYCLE_FIXTURE);CHECK(file.good());std::string line;std::uint32_t count{}; // 독립 PE가 먼저 작성한 최초59관측이다.
    // 각 관측은 별도 실제 상태/전체 팔레트와 같은 외부 장치 입력을 사용한다.
    while (std::getline(file,line)) {
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto f=LifeSplit(line);CHECK(f.size()==19);if (f.size()!=19) return;
        MovieLifecycleState state;state.playback.freeze=std::stoul(f[5]);state.playback.palettePending=std::stoul(f[6]);state.playback.image={11,22,33,44};state.playback.palette.fill({0x5a,0x5a,0x5a,0x5a});state.savedPalette.fill({0xa5,0xa5,0xa5,0xa5});
        const auto width=std::stof(f[1]),height=std::stof(f[2]);const ScreenPoint size{std::stoi(f[3]),std::stoi(f[4])};const ScreenRect bounds{-5,7,static_cast<std::int32_t>(width)-5,static_cast<std::int32_t>(height)+7};
        constexpr std::array<ScreenRect,4> clips{{{0,0,1024,768},{12,30,20,40},{-1000,-1000,2000,2000},{0,0,0,0}}}; // 내부/외부/빈 exclusive clip이다.
        std::string status="normal",applied="-";std::vector<std::string> trace; // 실제 callback 순서와 부분효과다.
        const auto event=[&](std::string text) { trace.push_back(text);if (f[11]==text) { status="stop";throw LifecycleStop{}; } }; // 선택 장치의 효과 직전에만 멈춘다.
        MovieLifecycleHooks h;h.isOpen=[] { return true; };h.hasBuffer=[&] { return f[7]!="0"; };h.soundEnabled=[&] { return f[8]!="0"; };h.assertion=[&](auto value) { event("assert:"+std::to_string(value)); };
        h.videoSize=[&] { return size; };h.windowSize=[&] { return std::array<float,2>{width,height}; };h.clip=[&] { return clips.at(std::stoul(f[9])); };
        h.capturePalette=[&](std::span<ScreenColor,256> palette) { event("capture");
            // 독립 원본 장치와 같은1024BYTE 순열을 색 배열의 모든 성분에 설치한다.
            for (std::size_t index=0;index<palette.size();++index) { const auto byte=[&](auto channel) { return static_cast<std::uint8_t>((index*4+channel)*19+2); };palette[index]={byte(0),byte(1),byte(2),byte(3)}; }
        };
        h.shutdownSound=[&] { event("shutdown"); };h.muteSound=[&] { event("mute"); };h.bindBuffer=[&] { event("bind"); };h.unmuteSound=[&] { event("unmute"); };h.refreshCursor=[&] { event("refresh"); };
        h.bounds=[&] { event("bounds");return bounds; };h.clear=[&](auto rect,auto color) { event("clear:"+LifeRect(rect)+','+std::to_string(color)); };h.fill=[&](auto rect,auto color) { event("fill:"+LifeRect(rect)+','+std::to_string(color)); };
        h.applyPalette=[&](auto palette) { event("apply");applied=LifePalette(palette); };h.restoreSound=[&](auto quality) { event("sound:"+std::to_string(quality)); };h.black=[] { return 9; };
        h.idle=[&](bool enabled) { event("idle:"+std::to_string(enabled));if (enabled && f[10]!="0") state.playback.freeze=state.playback.freeze ? 0 : 1; };
        h.redraw=[&](bool erase) { event("redraw:"+std::to_string(erase));if (f[10]!="0") state.playback.freeze=state.playback.freeze ? 0 : 1; };
        h.pauseUpdates=[&](bool enabled) { event("updates:"+std::to_string(enabled)); };h.pauseGame=[&](bool enabled) { event("pause:"+std::to_string(enabled)); };h.showCursor=[&](bool enabled) { event("cursor:"+std::to_string(enabled)); };
        h.blit=[&](auto position,auto dims) { event("blit:"+std::to_string(position.x)+','+std::to_string(position.y)+','+std::to_string(dims.x)+','+std::to_string(dims.y)); };
        try { if (f[0]=="begin") BeginMovieLifecycle(state,h);else if (f[0]=="end") EndMovieLifecycle(state,h);else DrawMovieLifecycle(state,h); }catch (const LifecycleStop&) {} // 앞서 발생한 실제 BYTE 쓰기는 되돌리지 않는다.
        std::string events; // 원문 장치 진입을 빠짐없이 순서대로 비교한다.
        // 빈 장치 순서와 실제 '-' 값을 구분하기 위해 끝에서만 표시를 정한다.
        for (const auto& value:trace) { if (!events.empty()) events+=';';events+=value; }if (events.empty()) events="-";
        CHECK(status==f[12]);CHECK(LifeRect(state.playback.image)==f[13]);CHECK(std::to_string(state.playback.freeze)==f[14] && std::to_string(state.playback.palettePending)==f[15]);CHECK(LifePalette(state.savedPalette)==f[16]);CHECK(applied==f[17]);CHECK(events==f[18]);++count;
    }
    CHECK(count==59);
    // 실제 Screen::SetPalette가0/255번 입력을 수정할 때 복사본이 아닌 같은 MovieGump 배열에 남아야 한다.
    MovieLifecycleState live;live.savedPalette.fill({1,2,3,4});live.playback.palette.fill({5,6,7,8});live.playback.palettePending=1;live.playback.image={0,0,1,1};
    MovieLifecycleHooks effects;effects.bounds=[] { return ScreenRect{0,0,1,1}; };effects.clip=effects.bounds;effects.isOpen=[] { return true; };effects.hasBuffer=[] { return false; };effects.soundEnabled=[] { return false; };
    effects.clear=[](auto,auto) {};effects.unmuteSound=[] {};effects.idle=[](auto) {};effects.redraw=[](auto) {};effects.refreshCursor=[] {}; // 이 검사에서 선택되는 장치만 명시한다.
    bool saved=true; // 종료의 저장 배열과 표시의 영상 배열을 같은 callback에서 구별한다.
    effects.applyPalette=[&](std::span<ScreenColor,256> palette) { CHECK(palette.data()==(saved ? live.savedPalette.data() : live.playback.palette.data()));palette[0]={};palette[255]={255,255,255,0}; };
    EndMovieLifecycle(live,effects);CHECK(live.savedPalette[0].red==0 && live.savedPalette[255].red==255 && live.savedPalette[1].red==3);
    saved=false;DrawMovieLifecycle(live,effects);CHECK(live.playback.palette[0].red==0 && live.playback.palette[255].red==255 && live.playback.palette[1].red==7 && live.playback.palettePending==0);
}
