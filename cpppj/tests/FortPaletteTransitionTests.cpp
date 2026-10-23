// 제품보다 먼저 고정한 원문 676관측의 반환/부분효과/주소/전체 물리를 독립 장치로 대조한다.
#include "TestSupport.h"
#include "o/RawFortPaletteTransition.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
constexpr std::uint32_t kPaletteName=0x5c8910; // 원문 고정 이름 주소를 trace에만 표현한다.
constexpr std::uint32_t kCloudName=0x56b7a0; // Kencloud 이름은 팔레트 이름과 별개 주소이다.
constexpr std::uint32_t kInput=0x12400000; // 격리 x86 장치의 두 512BYTE 입력 위치이다.
constexpr std::uint32_t kPath=kInput+0x1000; // 격리 PathSpec이 돌려주는 두 256BYTE 위치이다.
constexpr std::uint32_t kPool=0x12000000; // 실제 할당기 대신 명시한 cache 토큰 주소의 시작이다.
// 쉼표/탭/trace 분리는 마지막 빈 칸도 보존하여 fixture 형식 오류를 숨기지 않는다.
std::vector<std::string> SplitPalette(std::string_view text,char separator) {
    std::vector<std::string> result; // 원문 줄의 각 칸을 독립 소유한다.
    std::size_t start=0; // 아직 소비하지 않은 첫 위치이다.
    // 끝 빈 trace도 하나의 칸으로 남긴다.
    for (;;) {
        const auto end=text.find(separator,start); // 다음 구분자의 위치이다.
        result.emplace_back(text.substr(start,end==text.npos?text.size()-start:end-start));
        if (end==text.npos) return result;
        start=end+1;
    }
}
// 전체 물리와 CString trace를 같은 소문자 hex로 표시한다.
std::string PaletteHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef"; // 원문 fixture의 소문자 hex 자릿수이다.
    std::string result; // 각 BYTE당 두 글자를 추가한다.
    // NUL 뒤 BYTE도 raw에서는 생략하지 않는다.
    for (const auto value:bytes) { result+=digits[value>>4];result+=digits[value&15]; }
    return result;
}
// 명시 하위 callback 또는 물리 접근 중단만 나타내며 부모의 정상 반환과 구별한다.
struct PaletteStop { std::string cause; }; // fixture의 중단 원인을 그대로 보존한다.
// 동일한 실제 버퍼를 빌리고 하위 효과만 기록한다. 부모의 제어 흐름은 이 장치가 계산하지 않는다.
struct PaletteDevice {
    std::array<std::uint8_t,257> palette{},cloud{}; // 고정 이름 앞 1BYTE와 뒤 256BYTE 전체 물리이다.
    std::array<std::array<std::uint8_t,512>,2> inputs{}; // 입력 인자 변수 변경과 같은 입력 내용 변경을 구별한다.
    std::array<std::array<std::uint8_t,256>,2> paths{}; // PathSpec 반환 주소의 실제 물리이다.
    std::span<const std::uint8_t> argument{},captured{}; // stack의 현재 인자 변수와 parent가 캡처한 주소이다.
    std::uint32_t cache{},count{},renderer{},dirty{},built{}; // 원문이 직접 재조회하는 같은 현재 DWORD들이다.
    int pathIndex{},nameCase{}; // 현재 PathSpec 선택(-1은 NULL)과 독립 입력 번호이다.
    std::string kind,mutation; // 진입점과 명시 하위 변화만 선택한다.
    std::map<std::string,std::uint32_t> calls; // free/build/step/load/maps 횟수를 fixture와 비교한다.
    std::vector<std::string> trace; // 실제 순서대로 누적한 명시 하위 호출이다.
    // 논리 cache 입력을 격리 원문과 같은 주소 토큰으로 바꾼다.
    static std::uint32_t Token(std::uint32_t logical) { return logical==0?0:kPool+logical/11*0x1000; }
    // NUL까지 쓰되 주변 BYTE는 유지하여 alias/남은 물리를 검증한다.
    static void Put(std::span<std::uint8_t> target,std::string_view value) {
        if (value.size()+1>target.size()) throw std::out_of_range("검사 장치 입력 길이");
        std::copy(value.begin(),value.end(),target.begin());target[value.size()]=0;
    }
    // C++ 버퍼 주소를 격리 PE의 실제 주소와 대응시킨다.
    std::uint32_t Address(const std::uint8_t* pointer) const {
        if (pointer==nullptr) return 0;
        const std::array<std::pair<std::span<const std::uint8_t>,std::uint32_t>,6> regions{{
            {palette,kPaletteName-1},{cloud,kCloudName-1},{inputs[0],kInput},{inputs[1],kInput+0x400},{paths[0],kPath},{paths[1],kPath+0x200}}}; // 각 실제 소유 버퍼와 원문 토큰의 대응이다.
        // 서로 다른 배열의 포인터 대소 비교 없이 동일 물리의 BYTE 주소를 찾는다.
        for (const auto& [region,base]:regions) for (std::size_t index=0;index<region.size();++index) if (pointer==region.data()+index) return base+static_cast<std::uint32_t>(index);
        throw std::out_of_range("검사 장치 주소 대응 누락");
    }
    // 읽기 범위 안에 NUL이 없는 경우 실제 하위 접근 실패를 명시 중단으로 돌린다.
    static std::span<const std::uint8_t> CString(std::span<const std::uint8_t> bytes,std::string_view cause) {
        const auto found=std::find(bytes.begin(),bytes.end(),std::uint8_t{}); // 현재 내용에서 첫 NUL을 읽는다.
        if (bytes.empty()||found==bytes.end()) throw PaletteStop{std::string(cause)};
        return bytes.first(static_cast<std::size_t>(found-bytes.begin()));
    }
    // 하위 효과가 같은 captured source의 내용을 바꾸도록 실제 가변 물리를 찾는다.
    void ChangeSource() {
        const auto address=Address(captured.data()); // 입력 변수 변경과 달리 현재 캡처 주소를 사용한다.
        const std::array<std::pair<std::span<std::uint8_t>,std::uint32_t>,6> regions{{
            {palette,kPaletteName-1},{cloud,kCloudName-1},{inputs[0],kInput},{inputs[1],kInput+0x400},{paths[0],kPath},{paths[1],kPath+0x200}}}; // 동일 버퍼의 쓰기 뷰이다.
        // 명시 mutation의 현재 주소가 들어 있는 버퍼만 바꾼다.
        for (const auto& [region,base]:regions) if (address>=base&&address-base<region.size()) { Put(region.subspan(address-base),"ChangedByChild");return; }
        throw std::out_of_range("검사 source 쓰기 주소 누락");
    }
    // 부모 밖에서만 발생하도록 지정한 callback 변화/중단을 적용한다.
    void Mutate(const std::string& boundary) {
        if (mutation==boundary+"-argumentB") argument=inputs[1];
        else if (mutation==boundary+"-source") ChangeSource();
        else if (mutation==boundary+"-palette") Put(std::span(palette).subspan(1),"ChangedPalette");
        else if (mutation==boundary+"-cache11") cache=Token(11);
        else if (mutation==boundary+"-cache33") cache=Token(33);
        else if (mutation==boundary+"-cache0") cache=0;
        else if (mutation==boundary+"-count0") count=0;
        else if (mutation==boundary+"-count3") count=3;
        else if (mutation==boundary+"-renderer0") renderer=0;
        else if (mutation==boundary+"-renderer1") renderer=1;
        else if (mutation==boundary+"-dirty77") dirty=77;
        else if (mutation=="path-return1"&&boundary=="path") pathIndex=1;
        else if (mutation=="path-return0"&&boundary=="path") pathIndex=-1;
        else if (mutation==boundary+"-stop") throw PaletteStop{boundary};
    }
    // 각 관측을 새 장치로 초기화하고 NUL 뒤/별칭 영역까지 독립 입력을 넣는다.
    explicit PaletteDevice(const std::vector<std::string>& values) {
        if (values.size()!=9) throw std::runtime_error("palette fixture 입력 칸 수");
        kind=values[0];nameCase=std::stoi(values[1]);cache=Token(static_cast<std::uint32_t>(std::stoul(values[2])));
        count=static_cast<std::uint32_t>(std::stoul(values[3]));renderer=static_cast<std::uint32_t>(std::stoul(values[4]));dirty=static_cast<std::uint32_t>(std::stoul(values[5]));
        built=Token(static_cast<std::uint32_t>(std::stoul(values[6])));pathIndex=std::stoi(values[7]);mutation=values[8];
        palette.fill(0xa1);palette[0]='Z';cloud.fill(0xb2);cloud[0]='Y';
        Put(std::span(palette).subspan(1),nameCase==1||nameCase==2?"gifcloud":"OldPalette");Put(std::span(cloud).subspan(1),"LegacySort");
        // 각 입력의 NUL 뒤 바이트까지 서로 다른 초기값을 보존한다.
        for (std::size_t input=0;input<inputs.size();++input) for (std::size_t byte=0;byte<inputs[input].size();++byte) inputs[input][byte]=static_cast<std::uint8_t>(0xc0+input*7+byte);
        std::string name=nameCase==1?"GIFCLOUD":nameCase==3?"":"gifcloud"; // 일반 입력의 실제 C 문자열이다.
        if (nameCase==6) { name.clear();for (int part=0;part<14;++part) name+="LongPaletteName"; } // 210자 입력도 원문 길이대로 복사한다.
        Put(inputs[0],name);Put(inputs[1],"SecondPalette");if (nameCase==8) inputs[0].fill('Q');
        argument=inputs[0];
        if (nameCase==4) argument=std::span(palette).subspan(2);
        else if (nameCase==5) argument=std::span(cloud).subspan(2);
        else if (nameCase==7) argument=palette;
        else if (nameCase==9) argument=std::span(cloud).subspan(1);
        else if (nameCase==10) argument=cloud;
        captured=argument;
        // 실제 PathSpec 반환 둘의 주소와 내용도 서로 구별한다.
        for (std::size_t path=0;path<paths.size();++path) { for (std::size_t byte=0;byte<paths[path].size();++byte) paths[path][byte]=static_cast<std::uint8_t>(0xd0+path*5+byte);Put(paths[path],path==0?"Work/First.col":"Work/Second.col"); }
    }
    // 실제 DWORD/고정 버퍼의 같은 수명 참조를 새 부모에 전달한다.
    FortPaletteTransitionGlobals Globals() { return {std::span(palette).subspan(1),std::span(cloud).subspan(1),cache,count,renderer,dirty}; }
    // 제품 본체와 독립인 명시 CRT/계산/파일/Screen 장치이다. 실제 GUI나 파일은 열지 않는다.
    FortPaletteTransitionHooks Hooks() {
        FortPaletteTransitionHooks hooks; // 모든 도달 하위를 명시하며 미연결의 기본 성공은 없다.
        hooks.log=[this](std::uint32_t level,std::string_view text) { CHECK(level==29);CHECK(text=="init palette\n");trace.push_back("LOG:"+std::to_string(level)+":"+PaletteHex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}));Mutate("log"); };
        hooks.compare=[this](std::span<const std::uint8_t> first,std::span<const std::uint8_t> second) {
            captured=second;trace.push_back("COMPARE:"+std::to_string(Address(first.data()))+":"+std::to_string(Address(second.data())));
            const auto left=CString(first,"compare-physical"),right=CString(second,"compare-physical"); // callback 전에 비교 결과를 확정한다.
            const auto lower=[](std::uint8_t value) { return value>='A'&&value<='Z'?value+32:value; }; // 입력 전체가 ASCII인 명시 CRT 장치이다.
            const bool equal=left.size()==right.size()&&std::equal(left.begin(),left.end(),right.begin(),[&](auto a,auto b){return lower(a)==lower(b);}); // 원문 관측과 같은 0/비영 분기만 사용한다.
            Mutate("compare");return equal?0:1;
        };
        hooks.release=[this](std::uint32_t selected) { ++calls["free"];trace.push_back("FREE:"+std::to_string(selected));Mutate("free"); };
        hooks.buildCloud=[this] { ++calls["build"];trace.push_back("BUILD:"+std::to_string(cache)+":"+std::to_string(count));cache=built;Mutate("build"); };
        hooks.stepCloud=[this] { const auto index=++calls["step"];trace.push_back("STEP:"+std::to_string(index)+":"+std::to_string(cache)+":"+std::to_string(count));Mutate("step"+std::to_string(index)); };
        hooks.pathSpec=[this](std::string_view key,std::span<const std::uint8_t> source,std::uint32_t first,std::uint32_t second)->std::span<const std::uint8_t> {
            CHECK(key=="GamePalSpec");CHECK(first==0);CHECK(second==0);trace.push_back("PATH:"+std::to_string(Address(source.data()))+":"+std::to_string(first)+":"+std::to_string(second));trace.push_back("PATH-NAME:"+PaletteHex(CString(source,"path-physical")));Mutate("path");return pathIndex<0?std::span<const std::uint8_t>{}:std::span<const std::uint8_t>(paths.at(static_cast<std::size_t>(pathIndex)));
        };
        hooks.loadPalette=[this](std::span<const std::uint8_t> path,std::uint32_t clear) { ++calls["load"];CHECK(clear==1);trace.push_back("LOAD:"+std::to_string(Address(path.data()))+":"+std::to_string(clear));trace.push_back("LOAD-PATH:"+PaletteHex(CString(path,"load-physical")));Mutate("load"); };
        hooks.buildMaps=[this] { ++calls["maps"];trace.push_back("MAPS:"+std::to_string(renderer)+":"+std::to_string(dirty));Mutate("maps"); };
        return hooks;
    }
    // 네 선택 진입점만 실행하고 명시 중단/물리 예외를 정직하게 반환한다.
    std::string Run() {
        const auto globals=Globals();const auto hooks=Hooks(); // 함수 도중 같은 실제 필드를 계속 빌린다.
        try {
            if (kind=="transition") TransitionFortPalette(argument,globals,hooks);
            else if (kind=="cache") ReleaseFortPaletteCloudCache(globals,hooks);
            else if (kind=="sort") SetFortPaletteCloudName(argument,globals,hooks); //최초fixture의역사적진입식별자는API명칭정정과별개로보존한다.
            else if (kind=="dirty") MarkFortPaletteDirty(globals);
            else throw std::runtime_error("palette fixture 진입점");
        } catch (const PaletteStop& stopped) { return stopped.cause; }
        catch (const std::out_of_range& error) {
            if (std::string_view(error.what())!="팔레트 이름 출력 물리 범위") throw;
            return kind=="transition"&&nameCase==7?"palette-write":"sort-write"; //최초물리중단레이블은변경하지않는다.
        }
        return "1";
    }
    // 독립 원문과 같은 11개 현재 필드의 최종/중단 시점을 표시한다.
    std::string Values() { return std::to_string(cache)+","+std::to_string(count)+","+std::to_string(renderer)+","+std::to_string(dirty)+","+std::to_string(Address(argument.data()))+","+std::to_string(pathIndex)+","+std::to_string(calls["free"])+","+std::to_string(calls["build"])+","+std::to_string(calls["step"])+","+std::to_string(calls["load"])+","+std::to_string(calls["maps"]); }
    // 이름 앞 BYTE·NUL 뒤 잔여·입력 두 개·경로 두 개의 모든 물리를 비교한다.
    std::string Raw() const { return PaletteHex(palette)+":"+PaletteHex(cloud)+":"+PaletteHex(inputs[0])+":"+PaletteHex(inputs[1])+":"+PaletteHex(paths[0])+":"+PaletteHex(paths[1]); }
    // trace를 원문과 같은 순서로 한 줄에 결합한다.
    std::string Trace() const { std::string result;for (const auto& event:trace) { if (!result.empty()) result+=';';result+=event; }return result; }
};
// 프로젝트 검사와 독립 runner 모두에서 같은 최초 fixture를 연다.
std::ifstream PaletteFixture() {
#ifdef NETSTORM_FORT_PALETTE_TRANSITION_FIXTURE
    return std::ifstream(NETSTORM_FORT_PALETTE_TRANSITION_FIXTURE);
#else
    return std::ifstream(std::string(NETSTORM_SOURCE_DIR)+"/tests/fixtures/fort-palette-transition-x86.tsv");
#endif
}
}

// 네 원문 본문의 모든 최초 관측을 callback/물리 중단까지 포함하여 대조한다.
TEST_CASE(FortPaletteTransition_OriginalX86Fixture) {
    auto input=PaletteFixture();CHECK(input.good());if (!input) return; // fixture 누락을 성공으로 처리하지 않는다.
    std::string line;std::size_t observations=0,stops=0; // 전체 줄 수와 부분 중단 수를 별개로 확인한다.
    // 두 FPCW 관측 모두 독립 초기 상태에서 실행한다.
    while (std::getline(input,line)) {
        if (line.empty()||line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto fields=SplitPalette(line,'\t');CHECK(fields.size()==6);if (fields.size()!=6) return; // 최초 형식 6개 칸을 강제한다.
        PaletteDevice device(SplitPalette(fields[1],','));const auto cause=device.Run(); // 제품을 실행한 실제 부분 결과이다.
        const bool equal=cause==fields[2]&&device.Values()==fields[3]&&device.Raw()==fields[4]&&device.Trace()==fields[5]; // 반환·필드·raw·trace를 생략 없이 비교한다.
        if (!equal) std::fprintf(stderr,"palette row %zu input=%s cause=%s expected=%s\nvalues=%s\ntrace=%s\n",observations,fields[1].c_str(),cause.c_str(),fields[2].c_str(),device.Values().c_str(),device.Trace().c_str());
        CHECK(equal);++observations;if (cause!="1") ++stops;
    }
    CHECK(observations==676);CHECK(stops==34);
}
// 자연 생략은 미연결 하위를 부르지 않으며 도달한 필수 Screen 경계의 누락은 예외와 이전 효과를 보존한다.
TEST_CASE(FortPaletteTransition_RequiredBoundariesAndNaturalReturn) {
    PaletteDevice same(SplitPalette("transition,1,11,3,1,42,22,0,none",','));auto globals=same.Globals();auto hooks=same.Hooks(); // 대소문자만 다른 같은 이름 입력이다.
    hooks.release={};hooks.buildCloud={};hooks.stepCloud={};hooks.pathSpec={};hooks.loadPalette={};hooks.buildMaps={};
    TransitionFortPalette(same.argument,globals,hooks);CHECK(same.cache==PaletteDevice::Token(11));CHECK(same.dirty==42);CHECK(same.trace.size()==2);
    PaletteDevice missing(SplitPalette("transition,0,11,0,1,42,22,0,none",','));auto missingGlobals=missing.Globals();auto missingHooks=missing.Hooks();missingHooks.loadPalette={}; // 실제 arg2=1 미연결을 성공으로 가장하지 않는다.
    bool rejected=false;try { TransitionFortPalette(missing.argument,missingGlobals,missingHooks); } catch (const std::bad_function_call&) { rejected=true; } // 도달한 필수 장치 누락은 원래 부분 효과를 남긴다.
    CHECK(rejected);CHECK(missing.cache==PaletteDevice::Token(22));CHECK(missing.dirty==42);CHECK(missing.calls["free"]==1);CHECK(missing.calls["build"]==1);CHECK(missing.calls["maps"]==0);
    CHECK(PaletteHex(PaletteDevice::CString(std::span(missing.palette).subspan(1),"test"))=="676966636c6f7564");
}
