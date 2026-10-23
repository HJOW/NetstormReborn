// 제품 전에 고정한 부모/catch/cleanup130관측을 같은 명시 장치로 비교한다.
#include "TestSupport.h"
#include "o/RawColorMapCache.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
// 마지막 빈 trace도 유지하는 원시 TSV 분리다.
std::vector<std::string> CacheSplit(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; // 아직 읽지 않은 원시 칸 시작이다.
    for (;;) { // 마지막 한 칸을 추가하고 끝내며 출력 기대를 해석하지 않는다.
        const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos?text.npos:end-start));if (end==text.npos) return result;start=end+1;
    }
}
// NUL 절단 없이 실제 전체 BYTE를 비교한다.
std::string CacheHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string text; // 한 BYTE 두 자리다.
    for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15]; } // 미작성 잔여도 생략하지 않는다.
    return text;
}
// 부모의 장치 경계에서만 발생시키는 명시 중단이다.
struct CacheStop {};
// 파일 열기 형과 다른 오류를 별도 전파해 fallback catch-all을 검출한다.
struct CacheOther {};
// 최초 Native와 같은 입력/장치 효과만 제공한다. 표 분기나 부모 출력 기대는 만들지 않는다.
struct CacheDevice {
    std::string entry,mutation,pending; // 선택 원문 진입점/하위 변경/미처리 장치 예외다.
    std::array<std::array<std::uint8_t,0x3800>,2> sources; // 현재 source가 바뀌어도 두 실제 입력 할당은 살아 있다.
    std::uint32_t current{}; // getter가 빌리는 source 선택이며 새 RNG/팔레트를 만들지 않는다.
    std::array<std::uint32_t,8> owners{84,249,184,140,150,112,156,222}; // 행 입력의 실제 lowBYTE다.
    std::array<std::uint8_t,2048> highlight,solid; // Native 두8행의 독립 실제 물리다.
    std::array<std::array<std::uint8_t,256>,9> maps; // Native 파일 순서의 실제9표다.
    std::array<std::uint8_t,24> reader,writer; // caller가 정의한 미정 scratch이며 장치가 same this로 사용한다.
    std::array<std::uint8_t,256> path{}; // 이름을 바꿔도 같은 할당을 반환한다.
    std::vector<std::uint8_t> input,output; // read의 부분 길이와 write의 실제 반환 길이를 구별한다.
    std::size_t position{},writeLimit{},cleanups{},dispatches{}; // 원문 호출의 실제 관측값만 센다.
    std::map<std::string,std::size_t> calls;std::vector<std::string> trace; // 호출 순서/번호는 장치가 직접 기록한다.
    bool directCleanup{}; // 독립 cleanup entry의 한 호출과 normal close를 구별한다.

    // 실제 double 비트와 나머지56BYTE를 명시하며 원문 생성자의 출력을 계산하지 않는다.
    static void Source(std::span<std::uint8_t> bytes,std::uint32_t profile,std::uint32_t alternate) {
        for (std::size_t byte=0;byte<bytes.size();++byte) bytes[byte]=static_cast<std::uint8_t>(0x37+byte*13+alternate*29); // Raw source의 미사용 필드도 명시 입력이다.
        const std::array<double,8> values{0.8,0.800000011920929,0.80000002,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),-1.0,1.0}; // Native 경계 입력과 같은 값이다.
        for (std::size_t index=0;index<256;++index) { // 매 기록+8만 명도 입력으로 덮는다.
            const auto light=profile?values[(index+alternate)%values.size()]:static_cast<double>(index)/255.0;const auto bits=std::bit_cast<std::uint64_t>(light);
            for (std::size_t byte=0;byte<8;++byte) bytes[index*56+8+byte]=static_cast<std::uint8_t>(bits>>(byte*8)); // 원문 little endian double이다.
        }
    }
    // 선택 path의 같은 물리만 바꾸며 뒤 파일 호출이 현재 NUL 이름을 보게 한다.
    void Name(std::string_view text) { std::copy(text.begin(),text.end(),path.begin());path[text.size()]=0; }
    // 하위 File이 읽는 현재 C 문자열을 관측하며 부모에 새 이름 복사본을 공급하지 않는다.
    static std::string NameHex(std::span<const std::uint8_t> bytes) {
        const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("ColorCache 장치 경로 NUL 물리");return CacheHex(bytes.first(static_cast<std::size_t>(end-bytes.begin())));
    }
    // 원시 입력만 실제 source/표/두 stack 객체에 채운다.
    explicit CacheDevice(const std::vector<std::string>& fields) {
        if (fields.size()!=6) throw std::runtime_error("ColorCache 입력 칸 수");entry=fields[0];mutation=fields[5];const auto profile=static_cast<std::uint32_t>(std::stoul(fields[1])),initial=static_cast<std::uint32_t>(std::stoul(fields[2]));writeLimit=std::stoul(fields[4]);
        Source(sources[0],profile,0);Source(sources[1],profile,1);Name("!color.dat");reader.fill(initial?0xa5:0x3c);writer=reader;
        for (std::size_t byte=0;byte<2048;++byte) { highlight[byte]=static_cast<std::uint8_t>(initial*31+byte*11);solid[byte]=static_cast<std::uint8_t>(initial*31+7+byte*11); } // 출력 잔여는 원시 초기 입력이다.
        for (std::size_t index=0;index<9;++index) for (std::size_t byte=0;byte<256;++byte) maps[index][byte]=static_cast<std::uint8_t>(initial*13+index*19+byte*5); // 짧은 read가 덮지 않는 BYTE다.
        input.resize(std::stoul(fields[3]));for (std::size_t byte=0;byte<input.size();++byte) input[byte]=static_cast<std::uint8_t>(19+byte*7); // 파일 파서가 아닌 명시 파일 원문이다.
    }
    // 장치의 실제 호출 인자를 순서대로 기록한다.
    std::string Event(std::string kind,std::string text) { trace.push_back(std::move(text));return kind+std::to_string(++calls[kind]); }
    // 선택된 단 하나의 하위 효과에서 현재 전역/경로를 바꾸거나 명시 예외를 던진다.
    void Mutate(const std::string& boundary) {
        const auto dash=mutation.find('-');if (dash==mutation.npos) return;const auto trigger=mutation.substr(0,dash),action=mutation.substr(dash+1);auto kind=boundary;while (!kind.empty() && kind.back()>='0' && kind.back()<='9') kind.pop_back(); // 정확 번호 또는 종류만 선택한다.
        if (boundary!=trigger && kind!=trigger) return;
        if (action=="sourceB") current=1;
        else if (action=="owner7") owners[0]=0x12345607;
        else if (action=="pathChanged") Name("changed-color.dat");
        else if (action=="stop") { pending=action;throw CacheStop{}; }
        else if (action=="other") { pending=action;throw CacheOther{}; }
        else if (action=="unable") { pending=action;throw ColorCacheUnableToOpen{}; }
        else throw std::logic_error("ColorCache 미해석 변경");
    }
    // 같은 실제 배열/현재 source getter만 빌린다.
    ColorMapCacheGlobals Globals() {
        return {[this] { return std::span<const std::uint8_t>(sources[current]); },owners,highlight,solid,{maps[0],maps[1],maps[2],maps[3],maps[4],maps[5],maps[6],maps[7],maps[8]}};
    }
    // 같은 source/파일/표 주소만 식별하며 내용으로 파일 역할을 추측하지 않는다.
    char Role(std::span<std::uint8_t,24> file) const {
        if (file.data()==reader.data()) return 'R';if (file.data()==writer.data()) return 'W';throw std::logic_error("ColorCache 미등록 File 물리");
    }
    // 24BYTE 객체의 명시 생성 효과이며 부모가 반환값을 사용했다고 가장하지 않는다.
    void Open(std::span<std::uint8_t,24> file,std::span<const std::uint8_t> name,std::uint32_t mode,std::uint32_t flag) {
        const auto role=Role(file);CHECK(mode==(role=='R'?0U:1U) && flag==1);const auto kind=std::string("open")+role;const auto boundary=Event(kind,"O:"+std::string(1,role)+":"+CacheHex(file)+":"+NameHex(name));
        for (std::size_t byte=0;byte<24;++byte) file[byte]=static_cast<std::uint8_t>(0x10+(role=='W')*0x30+byte); // ctor 부분 출력도 예외 전에 실제 물리에 쓴다.
        Mutate(boundary);
    }
    // 필수 장치 경계만 만든다. 계산/TYPE표/File 전체 구현 완료를 주장하지 않는다.
    ColorMapCacheHooks Hooks() {
        ColorMapCacheHooks hooks;
        hooks.path=[this] { const auto boundary=Event("path","P");Mutate(boundary);return std::span<const std::uint8_t>(path); };
        hooks.owners=[this] { const auto boundary=Event("owners","T");Mutate(boundary); };
        hooks.buildMaps=[this] {
            if (pending=="unable" && entry=="parent") { ++dispatches;pending.clear(); } //제품 catch에서 도달한 정확 marker만 별도 분류한다.
            for (std::size_t index=0;index<9;++index) for (std::size_t byte=0;byte<256;++byte) maps[index][byte]=static_cast<std::uint8_t>(0x80+index*7+byte*3); // Native의 같은 명시 계산 장치 입력이다.
            const auto boundary=Event("maps","M");Mutate(boundary);
        };
        hooks.openRead=hooks.openWrite=[this](auto file,auto name,auto mode,auto flag) { Open(file,name,mode,flag); };
        hooks.read=[this](auto file,auto target,auto limit) {
            CHECK(Role(file)=='R' && limit==0xffffffffU);const auto selected=std::find_if(maps.begin(),maps.end(),[&](const auto& map) { return map.data()==target.data(); });if (selected==maps.end()) throw std::logic_error("ColorCache read 표 주소");
            const auto count=std::min<std::size_t>(256,input.size()-position);std::copy_n(input.begin()+static_cast<std::ptrdiff_t>(position),count,target.begin());position+=count;const auto boundary=Event("read","R:"+std::to_string(selected-maps.begin())+":"+std::to_string(count));Mutate(boundary);return static_cast<std::uint32_t>(count);
        };
        hooks.write=[this](auto file,auto target,auto limit) {
            CHECK(Role(file)=='W' && limit==0xffffffffU);const auto selected=std::find_if(maps.begin(),maps.end(),[&](const auto& map) { return map.data()==target.data(); });if (selected==maps.end()) throw std::logic_error("ColorCache write 표 주소");
            const auto count=std::min<std::size_t>(256,writeLimit);output.insert(output.end(),target.begin(),target.begin()+static_cast<std::ptrdiff_t>(count));const auto boundary=Event("write","W:"+std::to_string(selected-maps.begin())+":"+std::to_string(count));Mutate(boundary);return static_cast<std::uint32_t>(count);
        };
        hooks.close=[this](auto file) {
            if (!pending.empty() || directCleanup) { ++cleanups;directCleanup=false; } //전파 중 실제 close와 독립 funclet entry만 센다.
            const auto role=Role(file);const auto boundary=Event(std::string("close")+role,"D:"+std::string(1,role)+":"+CacheHex(file));
            for (std::size_t byte=0;byte<24;++byte) file[byte]=static_cast<std::uint8_t>(0xd0+(role=='W')*7+byte); // 같은 실제 객체의 close 효과다.
            Mutate(boundary);
        };return hooks;
    }
    // 실제 장치 호출 순서를 정렬/종류별 묶음 없이 반환한다.
    std::string Trace() const { std::string text;for (const auto& item:trace) { if (!text.empty()) text+=';';text+=item; }return text.empty()?"-":text; }
};
// 처음 다른 칸만 진단하고 실제 전체 물리와 원시 기대를 그대로 비교한다.
void CacheCheck(std::size_t row,std::string_view column,const std::string& actual,const std::string& expected) {
    if (actual!=expected) std::fprintf(stderr,"ColorCache row=%zu column=%.*s actual(%zu)=%.160s expected(%zu)=%.160s\n",row,static_cast<int>(column.size()),column.data(),actual.size(),actual.c_str(),expected.size(),expected.c_str());CHECK(actual==expected);
}
}

// typed dispatch는 명시 분류 장치이고 부모/catch/cleanup 명령만 Native인130행을 재생한다.
TEST_CASE(ColorMapCache_ReplaysIndependentParentCatchCleanup130) {
#ifdef NETSTORM_COLOR_MAP_CACHE_FIXTURE
    const std::filesystem::path path=NETSTORM_COLOR_MAP_CACHE_FIXTURE; //공용 등록 때 root가 선택할 고정 경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/colorsort-cache-parent-x86.tsv"; //격리 검사도 같은 최초 자료를 읽는다.
#endif
    std::ifstream input(path);if (!input) throw std::runtime_error("ColorCache 최초 fixture 열기");std::string line;std::size_t rows{}; // 실제 비교한 관측만 센다.
    while (std::getline(input,line)) { // 두 정밀도 자료를 제품의 같은 명시 입력으로 대조한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto expected=CacheSplit(line,'\t');if (expected.size()!=13) throw std::runtime_error("ColorCache TSV 칸 수");CacheDevice device(CacheSplit(expected[1],','));auto globals=device.Globals();auto hooks=device.Hooks();std::string completion="return";
        try {
            if (device.entry=="prefix") { static_cast<void>(PrepareColorMapCache(globals,hooks));completion="prefix"; }
            else if (device.entry=="parent") InitializeColorMapCache(globals,hooks,device.reader,device.writer);
            else if (device.entry=="catch") RegenerateColorMapCache(device.path,globals,hooks,device.writer);
            else { device.directCleanup=true;CloseColorMapCacheFile(device.entry=="cleanupR"?std::span<std::uint8_t,24>(device.reader):std::span<std::uint8_t,24>(device.writer),hooks); }
        }catch (const CacheStop&) { completion="stop"; }catch (const CacheOther&) { completion="other"; }catch (const ColorCacheUnableToOpen&) { completion="unable"; }
        ++rows;CacheCheck(rows,"completion",completion,expected[2]);CacheCheck(rows,"cleanup",std::to_string(device.cleanups),expected[3]);CacheCheck(rows,"dispatch",std::to_string(device.dispatches),expected[4]);CacheCheck(rows,"current",std::to_string(device.current),expected[5]);
        std::array<std::uint8_t,32> ownerBytes;for (std::size_t owner=0;owner<8;++owner) for (std::size_t byte=0;byte<4;++byte) ownerBytes[owner*4+byte]=static_cast<std::uint8_t>(device.owners[owner]>>(byte*8)); //원문 DWORD endian만 표기한다.
        CacheCheck(rows,"ownerDwords",CacheHex(ownerBytes),expected[6]);CacheCheck(rows,"ownerTables",CacheHex(device.highlight)+CacheHex(device.solid),expected[7]);std::string maps;for (const auto& map:device.maps) maps+=CacheHex(map); //실제 파일 순서의 전체 표다.
        CacheCheck(rows,"maps",maps,expected[8]);CacheCheck(rows,"files",CacheHex(device.reader)+CacheHex(device.writer),expected[9]);CacheCheck(rows,"written",device.output.empty()?"-":CacheHex(device.output),expected[10]);CacheCheck(rows,"trace",device.Trace(),expected[11]);CHECK(std::stoul(expected[12])>0);
    }
    CHECK(rows==130);
}

// cachehit은 미선택 재생성 장치를 요구하지 않으며 정확 marker에서만 그 누락이 드러난다.
TEST_CASE(ColorMapCache_RequiresOnlySelectedFileAndMapDevices) {
    CacheDevice hit({"parent","0","0","2304","256","none"});auto globals=hit.Globals();auto hooks=hit.Hooks();hooks.buildMaps={};hooks.openWrite={};hooks.write={};InitializeColorMapCache(globals,hooks,hit.reader,hit.writer);CHECK(hit.calls["maps"]==0 && hit.calls["closeR"]==1);
    CacheDevice missing({"parent","0","0","2304","256","openR-unable"});auto selected=missing.Globals();auto devices=missing.Hooks();devices.buildMaps={};bool rejected{};try { InitializeColorMapCache(selected,devices,missing.reader,missing.writer); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && !missing.calls["closeR"] && !missing.calls["openW"]);
}

// 미지원 raw source 물리는 첫 solid/identity 쓰기 뒤 거부한다. 이를 Native NULL 성공으로 세지 않는다.
TEST_CASE(ColorMapCache_PhysicalSourceFailurePreservesFirstPrefixStores) {
    CacheDevice device({"prefix","0","1","2304","256","none"});auto globals=device.Globals();auto hooks=device.Hooks();globals.primary=[] { return std::span<const std::uint8_t>{}; };const auto oldSecond=device.highlight[1];bool rejected{};
    try { static_cast<void>(PrepareColorMapCache(globals,hooks)); }catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && device.solid[0]==84 && device.highlight[0]==0 && device.highlight[1]==oldSecond && !device.calls["owners"]);
}
