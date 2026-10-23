// 원시Contents의최초PE668행을cursor/현재변환/TYPE/Create/재귀조기반환과대조한다.
#include "TestSupport.h"
#include "o/RawFortContentsLoad.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
namespace {
// 빈마지막필드도보존하는원시구분자분리다.
std::vector<std::string> ContentsFields(std::string_view source,char delimiter) { std::vector<std::string> fields;std::size_t start{};for (;;) { const auto end=source.find(delimiter,start);fields.emplace_back(source.substr(start,end==source.npos ? source.npos : end-start));if (end==source.npos) return fields;start=end+1; } } //제품기대를생성하지않는다.
// 현재네BYTE를정확한little endianDWORD로읽는다.
std::uint32_t ContentsRead(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; } //이웃필드를섞지않는다.
// 명시입력/하위효과의한DWORD만쓴다.
void ContentsPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //현재필드의전체비트를보존한다.
// PE에도공급한count/type/QA/QB/listFlags/중첩원시입력이다.
std::vector<std::uint8_t> ContentsProgram(std::uint32_t profile) {
    const std::array<std::vector<std::uint8_t>,10> inputs{{{1,1,0x11},{0},{3,2,254,3,128,255,1,4,85,52,128,16},{3,1,16,1,1,1,0},{2,5,16,2,1,0,2,7,1,4},{2,1,1,0},{2,5,1,2,1,1,0,1,4},{3,245,0,246,0,255,0},{},{1,3,128,255,0}}}; //변환결과나Create기대는포함하지않는다.
    if (profile!=8) return inputs.at(profile);std::vector<std::uint8_t> bytes{255};for (std::size_t index=0;index<255;++index) { bytes.push_back(1);bytes.push_back(0); }return bytes; //BYTE개수255의실제저장입력이다.
}
// callback정지와실제변환주소0물리실패를정상false와구별하는검사표식이다.
struct ContentsStop {};struct ContentsFault {};
// 네이티브가소유하지않는외부장치/원시입력만유지한다.
struct ContentsDevices {
    std::array<std::uint32_t,10> input{}; //fixture의앞10개원시DWORD다.
    std::uint32_t conversion{},checking{},depth{3},parentFlags{},typeCalls{},parentCalls{},creates{},assertions{},currentType{},lastConversionType{}; //원문뒤재조회할현재값과명시경계순번이다.
    bool zeroReturn{}; //해당Create호출에한해서만반환0을공급한다.
    std::string mutation,trace; //독립입력callback과실제호출관측이다.
    std::array<std::array<std::uint8_t,16>,2> streams{}; //두실제MemStream필드다.
    std::array<std::array<std::uint8_t,2048>,2> buffers{}; //논리end와다른실제물리입력이다.
    std::array<std::array<std::uint8_t,255>,2> tables{}; //현재토큰11/22가가리키는서로다른변환표다.
    std::array<FortContentsLoadType,256> types{}; //현재TYPE두필드만명시공급한다.
    // 입력열만으로네이티브와동일한외부저장소를준비한다.
    explicit ContentsDevices(const std::vector<std::string>& fields) {
        for (std::size_t index=0;index<input.size();++index) input[index]=std::stoul(fields[index]);mutation=fields[10];conversion=input[1];checking=input[2];parentFlags=input[8]; //출력열은여기서읽지않는다.
        types[2]={0x1000,0x104};types[3]={0x2000,0};types[4]={0x3000,0};types[5]={0x20000,0};
        for (std::size_t index=0;index<tables.size();++index) { for (std::size_t type=0;type<255;++type) tables[index][type]=static_cast<std::uint8_t>(type);tables[index][245]=index ? 0 : 2; } //독립원시변환입력이다.
        tables[1][0]=1;tables[1][1]=2;tables[1][2]=3;tables[1][3]=0;tables[1][4]=1;
        const auto program=ContentsProgram(input[0]); //앞으로소비할원시BYTE입력이다.
        for (std::size_t stream=0;stream<streams.size();++stream) { //각반환stream에다른물리버퍼를공급한다.
            ContentsPut(streams[stream],0,input[4] ? static_cast<std::uint32_t>(stream+1) : 0);ContentsPut(streams[stream],4,input[6]);ContentsPut(streams[stream],8,input[5]);ContentsPut(streams[stream],12,2048);buffers[stream].fill(stream ? 0xdd : 0xcc);
            for (const auto position:{std::size_t{0},std::size_t{512}}) std::copy(program.begin(),program.end(),buffers[stream].begin()+static_cast<std::ptrdiff_t>(position)); //callbackpos도독립원문을갖는다.
        }
    }
    // 실제하위진입의인자와순서를기록한다.
    void Record(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 명시외부효과만적용하며Contents의후속분기/반환을대신계산하지않는다.
    void Mutate(std::string_view boundary) {
        const auto separator=mutation.find('-');if (separator==std::string::npos || mutation.substr(0,separator)!=boundary) return;const auto action=mutation.substr(separator+1);auto& fields=streams[input[9]]; //그때실제선택한stream이다.
        if (action=="stop") throw ContentsStop{};
        if (action.starts_with("conversion")) conversion=std::stoul(action.substr(10));else if (action=="dataB") ContentsPut(fields,0,2);
        else if (action.starts_with("pos")) ContentsPut(fields,4,std::stoul(action.substr(3)));else if (action.starts_with("end")) ContentsPut(fields,8,std::stoul(action.substr(3),nullptr,16));
        else if (action=="flags0") types[currentType].flags1=0;else if (action=="flagsQB") types[currentType].flags1=0x2000;else if (action.starts_with("list")) types[currentType].contentListFlags=std::stoul(action.substr(4));
        else if (action=="parent4" || action=="flags4") parentFlags=4;else if (action=="checking0") checking=0;else if (action=="depth7") depth=7;else if (action=="return0") zeroReturn=true;else throw std::logic_error("Contents검사효과미해석");
    }
    // 실제직접읽기와동일한저장소를연결하고TYPE/Create/진단만명시외부응답으로둔다.
    FortContentsLoadHooks Hooks() {
        FortContentsLoadHooks hooks; //본문의stream/TYPE/flags계산은대체하지않는다.
        hooks.access=[this](auto token,const auto& operation) { if (token<1 || token>2) throw std::out_of_range("Contents검사stream물리토큰");MemStreamHooks memory;memory.buffer=[this](auto data)->std::span<std::uint8_t> { if (data<1 || data>2) throw std::out_of_range("Contents검사data물리토큰");return buffers[data-1]; };operation(streams[token-1],memory); };
        hooks.conversionBuffer=[this](auto token)->std::span<const std::uint8_t> {
            if (!token) { Record("F:"+std::to_string(lastConversionType)+":1");throw ContentsFault{}; }
            if (token!=11 && token!=22) throw std::out_of_range("Contents검사conversion물리토큰");const auto& fields=streams[input[9]];const auto data=ContentsRead(fields,0),position=ContentsRead(fields,4); //변환진단전처음인덱스만관측한다.
            if (position && position<=2048 && data>=1 && data<=2) lastConversionType=buffers[data-1][position-1];return tables[token==11 ? 0 : 1];
        };
        hooks.type=[this](auto type) { if (type>=types.size()) throw std::out_of_range("Contents검사TYPE물리범위");currentType=type;Record("T:"+std::to_string(type));Mutate("type"+std::to_string(++typeCalls));return types[type]; };
        hooks.parentListFlags=[this](auto parent) { Record("P:"+std::to_string(parent));Mutate("parent"+std::to_string(++parentCalls));return parentFlags; };
        hooks.create=[this](auto parent,auto type,auto quantity,auto flags,auto attach,auto quality) { Record("C:"+std::to_string(parent)+':'+std::to_string(type)+':'+std::to_string(quantity)+':'+std::to_string(flags)+':'+std::to_string(attach)+':'+std::to_string(quality));zeroReturn=false;Mutate("create"+std::to_string(++creates));return zeroReturn ? 0U : 20U+creates; };
        hooks.assertion=[this](auto line) { Record("A:"+std::to_string(static_cast<std::uint32_t>(line)));Mutate("assert"+std::to_string(++assertions)); };return hooks;
    }
};
// 도달한호스트물리/장치미연결예외만확인한다.
template<class Function> bool ContentsRejects(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; }
}

// 최초고정한모든직접cursor/Create/현재전역/재귀와실제정지·물리실패를비교한다.
TEST_CASE(FortContentsLoad_IndependentNativeRawReader) {
    std::ifstream fixture(NETSTORM_FORT_CONTENTS_LOAD_FIXTURE);CHECK(fixture.good());std::string line;std::size_t compared{}; //별도원문행수만집계한다.
    while (std::getline(fixture,line)) { //앞두열은입력이고뒤네열은제품전에독립고정한기대다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=ContentsFields(line,'\t');CHECK(row.size()==6);if (row.size()!=6) continue;ContentsDevices d(ContentsFields(row[1],','));auto hooks=d.Hooks();FortContentsLoadGlobals globals{d.conversion,[&d] { return d.checking; }};std::string status="normal";bool result{};
        try { result=LoadFortContents(10,d.input[9]+1,d.input[7],d.input[3],globals,hooks); }catch (const ContentsStop&) { status="stop"; }catch (const ContentsFault&) { status="fault"; }
        CHECK(status==row[2]);const auto state=ContentsFields(row[3],',');CHECK(state.size()==6);
        CHECK(static_cast<std::uint32_t>(result)==std::stoul(state[0]) && d.depth==std::stoul(state[1]) && d.conversion==std::stoul(state[2]) && d.checking==std::stoul(state[3]) && d.creates==std::stoul(state[4]) && d.parentFlags==std::stoul(state[5]));
        const auto streamState=ContentsFields(row[4],',');CHECK(streamState.size()==8);for (std::size_t stream=0;stream<2;++stream) for (std::size_t field=0;field<4;++field) CHECK(ContentsRead(d.streams[stream],field*4)==std::stoul(streamState[stream*4+field])); //모든현재stream필드를비교한다.
        CHECK((d.trace.empty() ? "-" : d.trace)==row[5]);++compared;
    }
    CHECK(compared==668);
}

// empty정상false/선택장치누락/논리끝과별개인물리거부/현재conversion을구별한다.
TEST_CASE(FortContentsLoad_SelectedDevicesAndPhysicalBounds) {
    ContentsDevices empty({"0","11","1","0","0","1","0","9","0","0","none"});auto hooks=empty.Hooks();hooks.type={};hooks.create={};hooks.conversionBuffer={};CHECK(!LoadFortContents(10,1,9,0,{empty.conversion,{}},hooks));CHECK(ContentsRead(empty.streams[0],4)==0);
    ContentsDevices missing({"0","11","0","1","1","1","0","9","0","0","none"});auto selected=missing.Hooks();selected.create={};CHECK(ContentsRejects([&] { static_cast<void>(LoadFortContents(10,1,9,1,{missing.conversion,{}},selected)); }));CHECK(ContentsRead(missing.streams[0],4)==3 && missing.creates==0); //Create누락도앞선count/type/listFlags소비뒤실패한다.
    ContentsDevices physical({"0","11","0","1","1","1","0","9","0","0","none"});auto bounded=physical.Hooks();ContentsPut(physical.streams[0],4,2048);CHECK(ContentsRejects([&] { static_cast<void>(LoadFortContents(10,1,9,1,{physical.conversion,{}},bounded)); }));CHECK(ContentsRead(physical.streams[0],4)==2048); //논리end1로읽기를생략하지않고물리끝에서만거부한다.
}
