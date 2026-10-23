// 최초 PE prefix166관측을 사본의 새 순서 helper와 대조한다. 실제 GDI/장치 화면은 실행하지 않는다.
#include "TestSupport.h"
#include "client/ScreenPaletteApply.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::client;
constexpr std::uint32_t kSurface=0x12001000,kOther=0x12002000,kPixels=0x12005000; // 격리 원문과 동일한 명시 장치 토큰이다.
// 마지막 빈 칸도 보존하여 최초 형식을 유지한다.
std::vector<std::string> Fields(std::string_view line,char delimiter) {
    std::vector<std::string> result;std::size_t position=0; // 다음 소비 위치이다.
    // 끝 trace가 비어 있어도 제거하지 않는다.
    for (;;) { const auto end=line.find(delimiter,position);result.emplace_back(line.substr(position,end==line.npos?line.npos:end-position));if (end==line.npos) return result;position=end+1; }
}
// 물리와 정수 목록은 최초 fixture와 같은 형식으로 표시한다.
std::string Numbers(std::span<const std::uint32_t> values) { std::string result;for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result; }
// 콜백 중단을 부모의 정상 checkpoint와 구별한다.
struct Stop { std::string cause; }; // 원문 명시 경계 이름이다.
// 원문에서 명시 장치였던 Lock/Fill/Surface.Unlock/Update만 모델링한다. 순서는 사본 helper가 소유한다.
struct Device {
    std::uint32_t flags{},depth{},draw{kSurface},buffer{},pitch{},vfx{},loaded{77},width{8},height{4},clear{}; // 같은 실제 현재 필드의 격리 입력이다.
    std::array<std::uint32_t,4> clip{},view{0,0,8,4}; // current clip과 고정 viewport는 다르다.
    std::array<std::uint8_t,64> pixels{}; // prefix 하위의 명시 물리 입력이며 GDI 표면은 아니다.
    std::vector<std::string> trace;std::string mutation; // 현재 호출 순서와 하위 효과만 기록한다.
    // 매 관측을 독립 초기화하고 이전 관측의 버퍼/전역에 기대지 않는다.
    explicit Device(const std::vector<std::string>& fields) {
        if (fields.size()!=5) throw std::runtime_error("palette clear 입력칸");
        clear=static_cast<std::uint32_t>(std::stoul(fields[0]));flags=static_cast<std::uint32_t>(std::stoul(fields[1]));depth=static_cast<std::uint32_t>(std::stoul(fields[2]));
        clip=fields[3]=="1"?std::array<std::uint32_t,4>{1,1,7,3}:view;mutation=fields[4];
        if (depth!=0) { buffer=vfx=kPixels;pitch=8; }
        // NUL이 없는 전체 BYTE를 원문과 그대로 비교한다.
        for (std::size_t index=0;index<pixels.size();++index) pixels[index]=static_cast<std::uint8_t>(index);
    }
    // 원문 장치의 현재 값 변경/중단만 반영한다.
    void Mutate(std::string_view boundary) {
        if (mutation==std::string(boundary)+"-stop") throw Stop{std::string(boundary)};
        if (mutation=="lock-size"&&boundary=="lock") { width=6;height=3; }
        else if (mutation=="lock-flags0"&&boundary=="lock") flags=0;
        else if (mutation=="fill-draw"&&boundary=="fill") draw=kOther;
        else if (mutation=="fill-flags1"&&boundary=="fill") flags=0x81;
        else if (mutation=="unlock-buffer"&&boundary=="unlock") { buffer=vfx=kPixels+16;pitch=99; }
        else if (mutation=="update-clip"&&boundary=="update") clip={2,2,3,3};
        else if (mutation=="update-flags4"&&boundary=="update") flags=0x84;
    }
    // 테스트 명시 장치의 현재 raw필드만 반환한다.
    std::string Values() const { const std::array<std::uint32_t,13> fields{flags,depth,draw,buffer,pitch,vfx,loaded,width,height,clip[0],clip[1],clip[2],clip[3]};return Numbers(fields); }
    // 지우기 전후 모든 픽셀을 생략 없이 표시한다.
    std::string Raw() const { constexpr std::string_view digits="0123456789abcdef";std::string result;for (const auto value:pixels) { result+=digits[value>>4];result+=digits[value&15]; }return result; }
    // 관측 trace를 원문과 같은 순서로 합친다.
    std::string Trace() const { std::string result;for (const auto& value:trace) { if (!result.empty()) result+=';';result+=value; }return result; }
    // 실제 호출자와 같은 clear 관문을 새 helper에 맡기고 하위만 명시한다.
    std::string Run() {
        ScreenPaletteApplyHooks hooks; // 도달한 미연결 장치의 기본성공을 넣지 않는다.
        hooks.lock=[this] { const auto before=depth++;trace.push_back("LOCK:"+std::to_string(before));if (before==0) { buffer=vfx=kPixels;pitch=8; }Mutate("lock"); };
        hooks.fillBlack=[this] {
            const std::array<std::uint32_t,4> rect{0,0,width,height};trace.push_back("FILL:"+Numbers(rect)+":0:"+Numbers(clip)); // lock 뒤의 현재 크기와 clip이다.
            if ((flags&0x80)!=0&&buffer!=0) {
                const auto left=std::max(clip[0],std::min(rect[0],clip[2])),top=std::max(clip[1],std::min(rect[1],clip[3])); // 명시 하위의 현재 잘린 시작이다.
                const auto right=std::max(clip[0],std::min(rect[2],clip[2])),bottom=std::max(clip[1],std::min(rect[3],clip[3])); // 끝좌표를 포함하지 않는다.
                // FillRect 자체는 선택 prefix 밖의 기존 primitive이며 같은 명시 물리에만 쓴다.
                for (auto y=top;y<bottom;++y) for (auto x=left;x<right;++x) pixels.at(buffer-kPixels+y*pitch+x)=0;
            }
            Mutate("fill");
        };
        hooks.unlock=[this] { --depth;if (depth==0) { trace.push_back("UNLOCK:"+std::to_string(draw));Mutate("unlock");buffer=pitch=vfx=0; } };
        hooks.waitVerticalBlank=[this] { trace.push_back("WAIT:"+std::to_string(flags));if ((flags&1)!=0) throw Stop{"directdraw-vblank"}; };
        hooks.presentPreservingClip=[this] { trace.push_back("PRESERVE:"+Numbers(clip));const auto previous=clip;trace.push_back("UPDATE:"+Numbers(view));clip=view;Mutate("update");clip=previous; };
        hooks.flip=[this] { trace.push_back("FLIP:"+std::to_string(flags));if ((flags&4)!=0) throw Stop{"directdraw-flip"}; };
        try { ApplyScreenPaletteClear(clear,flags,loaded,hooks);trace.push_back("APPLY:0,256,0,1"); } catch (const Stop& stop) { return stop.cause; }
        return "checkpoint";
    }
};
}

// 실제 PE prefix/inline unlock/clip 복구의 최초166관측 모두를 비교한다.
TEST_CASE(ScreenPaletteApply_FirstPePrefixFixture) {
    std::ifstream input(NETSTORM_SCREEN_PALETTE_CLEAR_FIXTURE);CHECK(input.good());if (!input) return; // 자료 누락을 통과시키지 않는다.
    std::string line;std::size_t compared=0,checkpoint=0; // 중단과 정상 checkpoint 수를 따로 센다.
    // 두 정밀도도 별개 장치로 다시 초기화한다.
    while (std::getline(input,line)) {
        if (line.empty()||line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto fields=Fields(line,'\t');CHECK(fields.size()==6);if (fields.size()!=6) return; // 최초 고정 형식이다.
        Device device(Fields(fields[1],','));const auto cause=device.Run(); // 제품 사본 helper를 실행한 결과다.
        const bool equal=cause==fields[2]&&device.Values()==fields[3]&&device.Raw()==fields[4]&&device.Trace()==fields[5]; // 원인/필드/전체물리/trace를 생략하지 않는다.
        if (!equal) std::fprintf(stderr,"clear row %zu %s cause=%s expected=%s\n%s\n%s\n",compared,fields[1].c_str(),cause.c_str(),fields[2].c_str(),device.Values().c_str(),device.Trace().c_str());
        CHECK(equal);++compared;if (cause=="checkpoint") ++checkpoint;
    }
    CHECK(compared==166);CHECK(checkpoint==122);
}
// clear0/flags0은loaded만쓰며,도달한필수하위의누락은부분효과와함께예외를낸다.
TEST_CASE(ScreenPaletteApply_NaturalSkipAndRequiredDevices) {
    std::uint32_t flags=0,loaded=77;ScreenPaletteApplyHooks devices; // 미연결 하위에 기본성공을 넣지 않은 입력이다.
    ApplyScreenPaletteClear(1,flags,loaded,devices);CHECK(loaded==1);
    flags=0x8a;loaded=77;ApplyScreenPaletteClear(0,flags,loaded,devices);CHECK(loaded==1);
    bool rejected=false;loaded=77;try { ApplyScreenPaletteClear(1,flags,loaded,devices); } catch (const std::bad_function_call&) { rejected=true; } // 누락장치도loaded쓰기뒤에도달한다.
    CHECK(rejected);CHECK(loaded==1);
}
