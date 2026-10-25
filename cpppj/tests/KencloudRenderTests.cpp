// 제품 전 고정 Native944관측의 전체 물리·clip·속도·반환·trace를 독립 재생한다.
#include "TestSupport.h"
#include "o/RawKencloudRender.h"
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 명시 하위 효과만 구현하며 부모 계산은 제품 몸체를 호출한다.
using namespace netstorm::o;
constexpr std::uint32_t kFirst=0x12001000,kSecond=0x12051000; // 독립 원문의 명시 생성/변경 주소다.
std::vector<std::string> Fields(const std::string& text,char delimiter) { std::vector<std::string> result;std::size_t at=0;for (;;) { const auto end=text.find(delimiter,at);result.push_back(text.substr(at,end==text.npos?text.npos:end-at));if (end==text.npos) return result;at=end+1; } } // 빈 마지막 trace도 버리지 않는다.
template<std::size_t Size> std::string Numbers(const std::array<std::uint32_t,Size>& values) { std::string result;for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result; } // 원시 unsigned DWORD의 십진 입력이다.
std::string Hash(const std::array<std::uint32_t,2054>& state) { std::uint64_t value=14695981039346656037ULL;for (const auto word:state) for (unsigned shift=0;shift<32;shift+=8) { value^=std::uint8_t(word>>shift);value*=1099511628211ULL; }std::ostringstream stream;stream<<std::hex<<std::setw(16)<<std::setfill('0')<<value;return stream.str(); } // 같은 little endian2054DWORD 전체를 식별한다.
struct Stop { std::string cause; }; // 명시 경계 중단을 정상 return과 구별한다.
struct Device {
    std::array<std::uint32_t,2054> state{};std::array<std::uint32_t,4> clip{0xfffffff7,0xfffffff9,900,700}; // 같은 packed 물리와 기존 clip다.
    std::uint32_t speed{},steps{},clips{},blits{};std::string mutation,trace; // 명시 현재 속도·경계 순번과 관측 문자열이다.
    void Event(const std::string& value) { if (!trace.empty()) trace+=';';trace+=value; } // 진입 순서만 기록한다.
    void Mutate(const std::string& label) { if (mutation==label+"-stop") throw Stop{label};if (mutation==label+"-speed2") speed=2;else if (mutation==label+"-speed0") speed=0;else if (mutation==label+"-cache22") state[515]=kSecond;else if (mutation==label+"-all0") state[2051]=0;else if (mutation==label+"-all1") state[2051]=1; } // 원문 도구와 같은 명시 하위 입력이다.
    KencloudRenderHooks Hooks() {
        KencloudRenderHooks hooks;hooks.build=[this] { Event("BUILD");state[515]=kFirst;Mutate("build"); };hooks.step=[this] { ++steps;Event("STEP");Mutate("step"+std::to_string(steps)); };
        hooks.clip=[this] { return clip; };hooks.setClip=[this](const auto& value) { ++clips;Event("CLIP:"+Numbers(value));clip=value;Mutate("clip"+std::to_string(clips)); };
        hooks.lock=[this](bool alternate) { Event("LOCK:"+std::to_string(alternate));Mutate("lock"); };hooks.unlock=[this](bool alternate) { Event("UNLOCK:"+std::to_string(alternate));Mutate("unlock"); };
        hooks.blit=[this](const auto& args) { ++blits;Event("BLIT:"+Numbers(args));Mutate("blit"+std::to_string(blits)); };return hooks; // 실제 잠금/복사 장치는 별도 실제 Screen 검사로 대조한다.
    }
};
}

// 양 x87 모드의944행을 그대로 읽고 부분 중단까지 같은 상태/순서를 대조한다.
TEST_CASE(KencloudRender_Original944Observations) {
    std::ifstream input(NETSTORM_KENCLOUD_RENDER_FIXTURE);CHECK(input.good());std::string line;std::size_t rows=0;
    while (std::getline(input,line)) { // 고정 fixture 행을 정규화하거나 중복 제거하지 않는다.
        if (line.empty() || line[0]=='#') continue;const auto fields=Fields(line,'\t'),args=Fields(fields.at(1),',');CHECK(fields.size()==8 && args.size()==13);Device device;
        for (std::size_t index=0;index<device.state.size();++index) device.state[index]=0x81000000U+static_cast<std::uint32_t>(index); // 같은 현재 물리 초기 입력이다.
        const auto number=[&](std::size_t index) { return static_cast<std::uint32_t>(std::stoll(args[index])); };const auto cache=number(1) ? kFirst : 0,pattern=number(3);device.speed=number(2);device.mutation=args[12];
        for (std::size_t index=0;index<512;++index) { device.state[index]=pattern==2 || (pattern==1 && (index==0 || index==8 || index==511));device.state[516+index]=pattern==2 || (pattern==1 && (index==1 || index==8 || index==511)); } // 같은 이전/현재 행이다.
        device.state[512]=cache+262144;device.state[513]=cache+261632;device.state[514]=cache+512;device.state[515]=cache;device.state[2051]=number(4);device.state[2052]=number(5);device.state[2053]=9;
        std::array<std::uint32_t,6> area{};for (std::size_t index=0;index<6;++index) area[index]=number(index+6); // 같은 인자 DWORD를 보존한다.
        const auto hooks=device.Hooks();const KencloudRenderGlobals globals{device.state,device.speed};std::uint32_t result{};std::string completion="return";
        try { if (args[0]=="update") UpdateKencloud(globals,hooks);else if (args[0]=="mark") MarkKencloudDirty(device.state);else if (args[0]=="clean") CleanKencloudDirty(device.state);else if (args[0]=="query") result=KencloudRowDirty(device.state,area[0],area[1]);else DrawKencloud(globals,area,hooks,args[0]=="alternate"); }catch (const Stop& stop) { completion=stop.cause; }
        const auto actual=fields[0]+'\t'+fields[1]+'\t'+completion+'\t'+std::to_string(result)+'\t'+Hash(device.state)+'\t'+Numbers(device.clip)+'\t'+std::to_string(device.speed)+'\t'+device.trace;
        if (actual!=line) std::cerr<<"구름 표시 최초 차이 "<<fields[1]<<"\n관측 "<<actual<<"\n원문 "<<line<<'\n';CHECK(actual==line);++rows;
    }
    CHECK(rows==944);
}

// 자연 생략은 하위를 요구하지 않으며 실제 순서의 물리 오류는 앞선 효과를 유지한다.
TEST_CASE(KencloudRender_NaturalSkipAndPhysicalPrefixes) {
    std::uint32_t speed{};DrawKencloud({{},speed},{0,0,65536,65536,0,0},{},true);std::array<std::uint32_t,516> cache{};cache[515]=1;UpdateKencloud({cache,speed},{});
    std::array<std::uint32_t,2052> shortState{};bool rejected=false;try { MarkKencloudDirty(shortState); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && shortState[2051]==1);
    Device device;device.state[515]=7;CHECK(KencloudRowDirty(device.state,0xffffffffU,0)==1);device.state[515]=0;CHECK(KencloudRowDirty(device.state,0xffffffffU,0)==0); // 행-1은 같은 cache DWORD의 인접 alias다.
}
