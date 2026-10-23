// 제품 전에 고정한 원문 부모116관측의 현재 슬롯·부분 효과·호출 인자를 대조한다.
#include "TestSupport.h"
#include "o/RawColorSortStartup.h"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
constexpr std::array<std::uint32_t,3> kBlocks{0x12001000,0x12006000,0x1200c000}; // 원문 명시 하위 세 할당 토큰이다.
// 마지막 빈 칸까지 보존하는 원시 fixture 분리다.
std::vector<std::string> Split(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t start=0; // 다음 원시 칸의 시작이다.
    for (;;) { const auto end=text.find(delimiter,start);result.emplace_back(text.substr(start,end==text.npos?text.npos:end-start));if (end==text.npos) return result;start=end+1; } // 순서/중복을 바꾸지 않는다.
}
// 같은 실제 BYTE 순서의 소문자 hex다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 고정 자릿수와 출력이다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // NUL 뒤 부분 물리도 포함한다.
    return result;
}
// 중단 위치는 정상 반환과 별도 결과다.
struct Stop { std::string cause; }; // 실제 하위 callback 예외다.
// 부모 기대를 계산하지 않고 명시 하위 생성·할당·원시 물리만 제공한다.
struct Device {
    std::uint32_t primary{},search{},gradient{},count{0xffffffffU},loaded{},profile{}; // 원문 현재 전역의 같은 실제 DWORD다.
    std::array<std::array<std::uint8_t,4>,3> blocks; // 부모 직접 vtable store와 하위 부분 효과를 관측한다.
    std::array<std::uint8_t,8> source{'o','r','i','g','i','n','a','l'}; // 생성자 진입 때 읽는 같은 원시 입력이다.
    std::string kind,mutation;std::map<std::string,std::uint32_t> calls;std::vector<std::string> events; // 진입점·하위효과·순서다.
    // 논리 입력의 포인터만 실제 격리 토큰에 대응한다.
    static std::uint32_t Token(std::uint32_t logical) { return logical==0?0:kBlocks.at(logical/11-1); }
    // 원문 출력과 무관한 입력을 같은 현재 상태로 초기화한다.
    explicit Device(const std::vector<std::string>& input) {
        CHECK(input.size()==7);kind=input[0];loaded=std::stoul(input[1]);primary=Token(std::stoul(input[2]));search=Token(std::stoul(input[3]));gradient=Token(std::stoul(input[4]));profile=std::stoul(input[5]);mutation=input[6];
        for (auto& block:blocks) block.fill(0xa5); // 미작성 물리도 독립 입력으로 보존한다.
    }
    // 매 호출에 같은 현재 전역 참조만 빌린다.
    ColorSortStartupGlobals Globals() { return {primary,search,gradient,count,loaded}; }
    // 실제 인자를 기록한 뒤 하위 종류의 번호를 반환한다.
    std::string Event(const std::string& name,std::string text) { events.push_back(std::move(text));return name+std::to_string(++calls[name]); }
    // 명시 하위 복귀 직전에만 전역을 바꾸거나 중단한다.
    void Change(const std::string& boundary) {
        if (mutation==boundary+"-stop") throw Stop{boundary};
        if (mutation==boundary+"-sort22") primary=Token(22);
        else if (mutation==boundary+"-sort33"||mutation==boundary+"-current33") primary=Token(33);
        else if (mutation==boundary+"-search0") search=0;
        else if (mutation==boundary+"-search33") search=Token(33);
        else if (mutation==boundary+"-count91") count=91;
        else if (mutation==boundary+"-count7") count=7;
        else if (mutation==boundary+"-source") source={'c','h','a','n','g','e','d',0};
    }
    // 토큰의 같은 실제 prefix만 조회한다. 해제/복제 여부를 부모가 추측하지 않는다.
    std::span<std::uint8_t> Memory(std::uint32_t token) {
        for (std::size_t index=0;index<kBlocks.size();++index) if (token==kBlocks[index]) return blocks[index]; // 동일 주소의 저장소다.
        throw std::out_of_range("ColorSort 검사 미등록 토큰");
    }
    // 하위 생성이 쓰는 marker는 실제 생성 알고리즘을 대신한 명시 장치 입력이다.
    void Marker(std::uint32_t token,std::string_view text) { const auto bytes=Memory(token);std::copy_n(text.begin(),4,bytes.begin()); }
    // x86 stack 인자의 little endian DWORD/QWORD를 그대로 작성한다.
    static void Append(std::vector<std::uint8_t>& bytes,std::uint64_t value,std::size_t size) { for (std::size_t index=0;index<size;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(index*8))); } // 원시 double 비트도 손실 없이 기록한다.
    // 부모가 호출한 하위 경계만 관측하며 현재 slot/분기/직접store는 제품이 실행한다.
    ColorSortStartupHooks Hooks() {
        ColorSortStartupHooks hooks; // 미연결에 대한 기본 성공을 만들지 않는다.
        hooks.assertion=[this](auto line) { CHECK(line==451);Event("assert","A:451");Change("assert"); };
        hooks.release=[this](auto token) { const auto boundary=Event("free","F:"+std::to_string(token));Change(boundary); };
        hooks.allocate=[this](std::uint32_t amount) {
            const std::size_t index=amount==0x3800?0:amount==0x5008?1:amount==0x5020?2:3; // 실제 요청 폭만 허용한다.
            CHECK(index<3);const auto token=(profile&(1U<<index))!=0?0U:kBlocks.at(index);const auto boundary=Event("alloc","N:"+std::to_string(amount)+":"+std::to_string(token));Change(boundary);return token;
        };
        hooks.construct=[this](auto target,auto logical) { CHECK(logical==0x12400000U);Event("sort","C:"+std::to_string(target)+":"+std::to_string(logical)+":"+Hex(source));Marker(target,"SORT");Change("sort");return mutation=="sort-return0"?0U:target; };
        hooks.constructSearch=[this](auto target) { Event("search","S:"+std::to_string(target)+":"+std::to_string(primary));Marker(target,"BASE");Change("search"); };
        hooks.memory=[this](auto token) { return Memory(token); };
        hooks.sort=[this](auto target,auto size,auto width,auto comparator) { Event("qsort","Q:"+std::to_string(target)+","+std::to_string(size)+","+std::to_string(width)+","+std::to_string(comparator));Change("qsort"); };
        hooks.constructGradient=[this](auto target,std::array<double,3> from,std::array<double,3> to,auto number,double width) {
            std::vector<std::uint8_t> raw; // 같은60BYTE 실제인자표현이다.
            for (const auto value:from) Append(raw,std::bit_cast<std::uint64_t>(value),8); // 순방향 RGB 세 double이다.
            for (const auto value:to) Append(raw,std::bit_cast<std::uint64_t>(value),8); // 두번째 RGB 세 double이다.
            Append(raw,number,4);Append(raw,std::bit_cast<std::uint64_t>(width),8);
            Event("gradient","G:"+std::to_string(target)+":"+std::to_string(primary)+":"+Hex(raw));Marker(target,"GRAD");Change("gradient");return mutation=="gradient-return0"?0U:target;
        };
        return hooks;
    }
    // 선택 진입점의 정상/명시 중단을 그대로 반환한다.
    std::string Run() { const auto hooks=Hooks();try { if (kind=="startup") InitializeColorSort(0x12400000U,Globals(),hooks);else InitializeColorSortGradient(Globals(),hooks); } catch (const Stop& stop) { return stop.cause; }return "return"; }
    // 현재 슬롯을 원문 fixture 순서로 표현한다.
    std::string Values() const { return std::to_string(primary)+","+std::to_string(search)+","+std::to_string(gradient)+","+std::to_string(count)+","+std::to_string(loaded); }
    // 실제 prefix와 source의 부분 쓰기를 모두 비교한다.
    std::string Raw() const { return Hex(blocks[0])+":"+Hex(blocks[1])+":"+Hex(blocks[2])+":"+Hex(source); }
    // 기록한 호출을 순서대로 연결한다.
    std::string Trace() const { std::string result;for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }return result; }
};
}

// 정상100/부분중단16의 모든 현재 전역과 물리/trace를 최초 원문 자료에 대조한다.
TEST_CASE(ColorSortStartup_OriginalParents116) {
#ifdef NETSTORM_COLORSORT_STARTUP_FIXTURE
    const std::filesystem::path path=NETSTORM_COLORSORT_STARTUP_FIXTURE; // root 공용 등록의 최초 원문 경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/colorsort-startup-x86.tsv"; // 격리 빌드도 같은 자료를 사용한다.
#endif
    std::ifstream input(path);CHECK(input.good());std::size_t rows=0,normal=0;std::string line; // 원문 관측 수를 별도로 확인한다.
    while (std::getline(input,line)) { // 두control의 전체116행을 생략하지 않는다.
        if (line.empty()||line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto values=Split(line,'\t');CHECK(values.size()==6);Device device(Split(values.at(1),','));const auto status=device.Run(); // 부모기대를 재계산하지 않는다.
        const bool same=status==values[2]&&device.Values()==values[3]&&device.Raw()==values[4]&&device.Trace()==values[5];
        if (!same) std::fprintf(stderr,"ColorSortStartup row=%zu status=%s values=%s raw=%s trace=%s\n",rows,status.c_str(),device.Values().c_str(),device.Raw().c_str(),device.Trace().c_str());
        CHECK(same);++rows;if (status=="return") ++normal;
    }
    CHECK(rows==116&&normal==100);
}
// NULL 할당의 자연 분기는 다른 하위를 요구하지 않으며 선택된 누락 진단은 이전 상태를 보존한다.
TEST_CASE(ColorSortStartup_RequiredSelectedDevices) {
    std::uint32_t primary=0,search=0,gradient=0,count=19,loaded=1,allocations=0; // 각 실제 슬롯의 독립 현재입력이다.
    ColorSortStartupHooks hooks;hooks.allocate=[&](auto) { ++allocations;return 0U; }; // 3개실제NULL할당만 명시한다.
    InitializeColorSort(7,{primary,search,gradient,count,loaded},hooks);CHECK(allocations==3&&primary==0&&search==0&&gradient==0&&count==10);
    loaded=0;primary=9;bool rejected=false; // 진단 하위가 미연결이면 free 앞에서 중단한다.
    try { InitializeColorSort(7,{primary,search,gradient,count,loaded},hooks); } catch (const std::bad_function_call&) { rejected=true; }
    CHECK(rejected&&primary==9&&allocations==3);
}
