// 실제 원본 MovieGump의59개 입력/부분효과를 제품 판단과 대조한다. 창과 오디오 장치는 열지 않는다.
#include "TestSupport.h"
#include "client/MoviePlayback.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace { // 원시 BYTE/장치 순서를 보존하는 검사 전용 도우미다.
using namespace netstorm::client;
struct MovieStop {}; // 실제 장치 직전의 선택 중단 신호다.
// TSV와 trace를 마지막 빈 칸까지 손실 없이 분리한다.
std::vector<std::string> MovieSplit(std::string_view text,char delimiter) {
    std::vector<std::string> values;std::size_t start{}; // 현재 시작 위치와 순서 있는 실제 필드다.
    // 마지막 구분자 뒤의 빈 칸도 독립 필드로 남긴다.
    for (;;) { const auto end=text.find(delimiter,start);values.emplace_back(text.substr(start,end==text.npos ? end : end-start));if (end==text.npos) return values;start=end+1; }
}
// 실제 BGR0 배열을 원시 BYTE 순서의 소문자16진수로 바꾼다.
std::string MoviePaletteHex(const MoviePlaybackState& state) {
    constexpr char digits[]="0123456789abcdef";std::string value; // 최초 관측과 같은 nibble 표기다.
    // 색마다 네 BYTE 모두를 비교한다. 사용하지 않는 예약BYTE도 생략하지 않는다.
    for (const auto color:state.palette) for (const auto byte:{color.blue,color.green,color.red,color.reserved}) { value+=digits[byte>>4];value+=digits[byte&15]; }
    return value;
}
}

// 분기뿐 아니라 전체1024BYTE 팔레트·dirty 전역·callback 순서·중단 부분효과를 원본에 대조한다.
TEST_CASE(MoviePlayback_Native59InputPaletteIdleAndPartialEffects) {
    std::ifstream file(NETSTORM_MOVIE_PLAYBACK_FIXTURE);CHECK(file.good());std::string line;std::size_t count{}; // 최초 독립 PE 관측이다.
    // 각 원시 관측마다 별도 상태와 같은 명시 장치 입력을 설치한다.
    while (std::getline(file,line)) {
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto f=MovieSplit(line,'\t');CHECK(f.size()==17);if (f.size()!=17) return;
        MoviePlaybackState state;state.autoKill=std::stoul(f[2]);state.freeze=std::stoul(f[3]);state.palette.fill({0xa5,0xa5,0xa5,0xa5});state.image={10,20,0,0};
        std::uint32_t dirty=7,queries{},rectIndex{};std::string status="normal",result="-";std::vector<std::string> trace; // 실제 원문 현재값과 로그다.
        std::array<std::uint8_t,768> rgb{}; // 독립 입력 팔레트이며 제품 결과로 기대값을 만들지 않는다.
        // 원문 장치 입력과 같은 RGB 순열을 작성한다.
        for (std::size_t index=0;index<rgb.size();++index) rgb[index]=static_cast<std::uint8_t>(index*37+11);
        const auto event=[&](std::string value) { trace.push_back(value);if (f[10]==value) { status="stop";throw MovieStop{}; } }; // 실제 효과 직전에서만 중단한다.
        MoviePlaybackHooks h;
        h.atEnd=[&] { const auto value=std::stoul(f[queries++==0 ? 4 : 5]);event("atEnd:"+std::to_string(value));return value!=0; };
        h.wait=[&] { event("wait:"+f[6]);return f[6]!="0"; };h.paletteChanged=[&] { return f[7]!="0"; };h.palette=[&] { return std::span<const std::uint8_t,768>(rgb); };
        h.paletteLog=[&] { event("palette"); };h.invalidOption=[&](auto option) { event("invalid:"+std::to_string(option)); };
        h.doFrame=[&] { event("decode"); };h.origin=[&] { event("origin");return ScreenPoint{-3,7}; };
        h.nextRect=[&]() -> std::optional<ScreenRect> { const auto index=rectIndex++;event("rect:"+std::to_string(index));if (index>=std::stoul(f[8])) return {};return index==0 ? ScreenRect{1,2,4,6} : ScreenRect{-5,8,4,18}; };
        h.invalidate=[&](auto rect,auto flags) { CHECK(dirty==1 && flags==4);event("dirty:"+std::to_string(rect.left)+','+std::to_string(rect.top)+','+std::to_string(rect.right-rect.left)+','+std::to_string(rect.bottom-rect.top)); };
        h.nextFrame=[&] { event("next"); };h.end=[&] { event("end");if (f[9]!="0") state.autoKill=state.autoKill ? 0 : 1; };h.kill=[&] { event("kill"); };
        try {
            const auto arg=static_cast<std::uint32_t>(std::stoul(f[1]));
            if (f[0]=="input") result=std::to_string(HandleMoviePlayback(state,arg,h));else if (f[0]=="idle") IdleMoviePlayback(state,dirty,h);
            else if (f[0]=="palette") UpdateMoviePlaybackPalette(state,rgb,h);else SetMoviePlaybackOption(state,arg,1,h);
        }catch (const MovieStop&) {} // 중단 직전까지의 현재 상태를 그대로 비교한다.
        std::string events; // 원본 순서대로 연결하며 빈 로그는 명시 '-'다.
        // 중간 입력/장치를 빼거나 정렬하지 않는다.
        for (const auto& value:trace) { if (!events.empty()) events+=';';events+=value; }
        if (events.empty()) events="-";
        CHECK(status==f[11] && result==f[12]);CHECK(std::to_string(state.autoKill)+','+std::to_string(state.freeze)+','+std::to_string(state.palettePending)==f[13]);CHECK(std::to_string(dirty)==f[14]);CHECK(MoviePaletteHex(state)==f[15]);CHECK(events==f[16]);++count;
    }
    CHECK(count==59);
}

// 실행 장치가 누락된 경우 해당 경계에 도달하기 전에는 오류나 부작용을 만들지 않는다.
TEST_CASE(MoviePlayback_UnhandledInputRequiresNoDeviceAndOptionsNormalize) {
    MoviePlaybackState state;MoviePlaybackHooks hooks;CHECK(HandleMoviePlayback(state,65,hooks)==0);
    SetMoviePlaybackOption(state,1,0xffffffffU,hooks);SetMoviePlaybackOption(state,2,0,hooks);CHECK(state.autoKill==1 && state.freeze==0);
    SetMoviePlaybackOption(state,1,0,hooks);SetMoviePlaybackOption(state,2,9,hooks);CHECK(state.autoKill==0 && state.freeze==1);
}
