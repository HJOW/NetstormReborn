// 원문 CreateAt 전체 부모의 문자열·현재 소유자·HP·프레임·float32 순서를 독립 기대값으로 검사한다.
#include "TestSupport.h"
#include "o/RawCreateAt.h"
#include <array>
#include <bit>
#include <fstream>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace { //새 원문 관측 파일의 입력/호출/50BYTE를 읽는 독립 검사 도구다.
//탭으로 구별한 입력과 원문 결과를 빈 필드를 보존해 분리한다.
std::vector<std::string> CreateAtFields(std::string_view line) {
    std::vector<std::string> fields;std::size_t begin{};
    for (;;) { const auto end=line.find('\t',begin);fields.emplace_back(line.substr(begin,end==line.npos ? line.npos:end-begin));if (end==line.npos) return fields;begin=end+1; } //마지막 필드까지 입력 순서대로 읽는다.
}
//실제 BYTE 순서의 소문자16진 관측을 만든다.
std::string CreateAtHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef"; //기대값 파일이 사용하는 진법 글자다.
    std::string result;for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } //바이트를 호스트 정수로 재배열하지 않는다.
    return result;
}
//명시 외부 장치의 도달 순서만 기록한다.
void CreateAtTrace(std::string& trace,std::string effect) { if (!trace.empty()) trace+=';';trace+=effect; }
//원문과 같은 비정렬little endian DWORD를 작은 합성 객체에 저장한다.
void CreateAtWord(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) raw[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); //DWORD의 각BYTE를 순서대로 저장한다.
}
//Native입력JSON의 문자열과 다섯 정수만 읽는다. 정답 trace/raw는 해석하지 않는다.
struct CreateAtCase {
    std::string source; //NUL을 포함한 원래 문자열 입력이다.
    std::array<std::uint32_t,5> values{}; //Boss/HP/지면/owner효과/NULL의 독립 입력이다.
};
//ASCII JSON fixture의 quote/역슬래시/제어문자만 정확히 복원한다.
CreateAtCase CreateAtInput(std::string_view text) {
    if (!text.starts_with("[\"")) throw std::runtime_error("CreateAt Native 입력JSON 시작");CreateAtCase result;std::size_t pos=2;
    while (pos<text.size()) { //문자열 끝의 실제quote까지만 읽으며 escaped quote와NUL을 구별한다.
        const char byte=text[pos++];if (byte=='\"') break;if (byte!='\\') { result.source+=byte;continue; }
        const char escaped=text.at(pos++);
        if (escaped=='u') { const auto value=std::stoul(std::string(text.substr(pos,4)),nullptr,16);pos+=4;if (value>127) throw std::runtime_error("CreateAt ASCII JSON 입력 범위");result.source+=static_cast<char>(value); }
        else if (escaped=='n') result.source+='\n';else if (escaped=='r') result.source+='\r';else if (escaped=='t') result.source+='\t';else if (escaped=='b') result.source+='\b';else if (escaped=='f') result.source+='\f';else result.source+=escaped;
    }
    for (auto& value:result.values) { if (text.at(pos++)!=',') throw std::runtime_error("CreateAt Native 입력JSON 정수 구분");const auto end=text.find_first_of(",]",pos);value=static_cast<std::uint32_t>(std::stoul(std::string(text.substr(pos,end-pos))));pos=end; } //입력의 고정 다섯 숫자만 읽는다.
    if (text.substr(pos)!="]") throw std::runtime_error("CreateAt Native 입력JSON 끝");return result;
}
//기계어0049b060이 받는 것과 같은 독립 프레임 코드 입력이다.
netstorm::o::RiftTypeFrames CreateAtFrames(bool reverse) {
    std::vector<netstorm::o::FrameCode> codes;
    for (int index=0;index<16;++index) { const int side=reverse ? 15-index:index;for (int number=0;number<1+side%3;++number) codes.push_back({static_cast<std::uint8_t>('A'+side),'A',static_cast<std::uint8_t>(number),0}); } //방향 순서/개수는 명시 입력이며 첫프레임 기대값은 아니다.
    return netstorm::o::RiftTypeFrames(std::move(codes));
}
}

//실제 parser·권한·지면·HP·메타데이터 부모580관측과 전체50BYTE를 비교한다.
TEST_CASE(CreateAt_Native580ParentsPreserveCurrentStateAndFloatBits) {
    using namespace netstorm::o;
    std::ifstream file(NETSTORM_CREATE_AT_PARENT_FIXTURE);if (!file) throw std::runtime_error("CreateAt Native 부모 fixture 열기 실패");
    std::string line;std::getline(file,line);std::size_t count{};
    while (std::getline(file,line)) { //첫열만 제품 입력으로 쓰며 나머지는 최초 원문 관측이다.
        const auto row=CreateAtFields(line);CHECK(row.size()==4);if (row.size()!=4) continue;const auto input=CreateAtInput(row[0]);const auto bossMode=input.values[0],hasHp=input.values[1],terrain=input.values[2],mutation=input.values[3],nullObject=input.values[4];
        bool boss=bossMode==1;std::array<std::uint8_t,50> raw;raw.fill(0xcd);std::array<std::uint8_t,50> surface{};surface[34]=3;std::uint32_t maximum{};std::string trace,outcome="ok";const auto forward=CreateAtFrames(false),reverse=CreateAtFrames(true);
        CreateAtHooks hooks;hooks.boss=[&] { return boss; };hooks.assertion=[&](auto where) { CreateAtTrace(trace,"assert:"+std::to_string(where));if (bossMode==2) boss=true; };
        hooks.parse=[&](auto source) { return ParseDestroyAtLocation(source,[&](auto name) { CreateAtTrace(trace,"type:"+(name.empty() ? std::string("-"):CreateAtHex({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()})));std::string key(name);for (auto& byte:key) if (byte>='A' && byte<='Z') byte=static_cast<char>(byte+'a'-'A');return key.empty() ? 0:key=="provider" ? 76:key=="suncannon" ? 131:key=="special2" ? 129:key=="special3" ? 140:key=="special4" ? 142:-1; }); }; //실제 기존parser와 명시 타입조회만 사용한다.
        hooks.create=[&](auto type,auto flags) { CreateAtTrace(trace,"create:"+std::to_string(type)+':'+std::to_string(flags));if (nullObject) return 0U;raw.fill(0);CreateAtWord(raw,0,0x12001000);raw[10]=static_cast<std::uint8_t>(type);raw[11]=4;CreateAtWord(raw,36,0xdeadbeef);maximum=type==76 ? 203U:503U;return 1U; };
        hooks.surface=[&](auto x,auto y) { CreateAtTrace(trace,"surface:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y)));return terrain && x==20 && y==21 ? 2U:0U; };hooks.slot=[&](auto sid) { CHECK(sid==2);return std::span<const std::uint8_t>(surface); };
        hooks.owner=[&](auto sid,auto owner) { CHECK(sid==1);CreateAtTrace(trace,"owner:"+std::to_string(owner));raw[34]=static_cast<std::uint8_t>(owner);if (mutation==1) { raw[10]=76;maximum=203; }if (mutation==2) maximum=701; };
        hooks.hasHitPoints=[&](auto sid) { CHECK(sid==1);CreateAtTrace(trace,"hasHp");return hasHp!=0; };hooks.maximumHitPoints=[&](auto sid) { CHECK(sid==1);CreateAtTrace(trace,"maxHp");return static_cast<std::int32_t>(maximum); };hooks.hitPoints=[&](auto sid,auto value) { CHECK(sid==1);CreateAtTrace(trace,"hp:"+std::to_string(value));CreateAtWord(raw,26,std::bit_cast<std::uint32_t>(value)); };
        hooks.directional=[&](auto type) { CreateAtTrace(trace,"directional:"+std::to_string(type));return type==129 || type==131 || type==140 || type==142; };hooks.frameField=[&](auto sid,auto offset) { CHECK(sid==1);CreateAtTrace(trace,"frameField:"+std::to_string(offset));return CreateAtFrameField(raw[10]==76 ? reverse:forward,offset); };hooks.frame=[&](auto sid,auto value) { CHECK(sid==1);CreateAtWord(raw,36,value); };
        hooks.pop=[&](auto sid,float x,float y,auto flags) { CHECK(sid==1);const auto xb=std::bit_cast<std::uint32_t>(x),yb=std::bit_cast<std::uint32_t>(y);CreateAtTrace(trace,"pop:"+std::to_string(xb)+':'+std::to_string(yb)+':'+std::to_string(flags));CreateAtWord(raw,14,xb);CreateAtWord(raw,18,yb); };
        try { CreateObjectAt(input.source,hooks); }catch (const std::bad_alloc&) { outcome="null";CreateAtTrace(trace,"NULL"); }
        CHECK(outcome==row[1] && (trace.empty() ? "-":trace)==row[2] && CreateAtHex(raw)==row[3]);++count;
    }
    CHECK(count==580);
}

//선택되지 않은 하위를 요구하지 않고 firstNUL/로컬버퍼 범위와 부분 효과를 구별한다.
TEST_CASE(CreateAt_ReachedParentFailuresKeepEarlierEffects) {
    using namespace netstorm::o;
    CreateAtHooks hooks;hooks.boss=[] { return false; };std::size_t diagnostics{};hooks.assertion=[&](auto line) { CHECK(line==803);++diagnostics; };CreateObjectAt("",hooks);CHECK(diagnostics==1);
    hooks.boss=[] { return true; };hooks.parse=[](auto text) { CHECK(text=="x");return DestroyAtLocation{0,1,76,0,1}; };CreateObjectAt(std::string("x\0ignored",9),hooks);
    bool longInput{};try { CreateObjectAt(std::string(256,'a'),hooks); }catch (const std::length_error&) { longInput=true; }CHECK(longInput);
    hooks.parse=[](auto) { return DestroyAtLocation{20,21,131,44,1}; };std::vector<std::string> effects;
    std::uint32_t expectedType=131; //각 독립 입력이 요청한 실제 TYPE을 별도로 확인한다.
    hooks.create=[&](auto type,auto flags) { CHECK(type==expectedType && !flags);effects.emplace_back("create");return 1U; };hooks.owner=[&](auto,auto owner) { CHECK(owner==1);effects.emplace_back("owner"); };hooks.hasHitPoints=[](auto) { return true; };hooks.maximumHitPoints=[](auto) { return 503; };hooks.hitPoints=[&](auto,auto value) { CHECK(value==503);effects.emplace_back("hp"); };hooks.directional=[](auto) { return true; };hooks.frameField=[&](auto,auto offset) { CHECK(offset==0x1b4U);return CreateAtFrameField(CreateAtFrames(false),offset); };hooks.frame=[&](auto,auto) { effects.emplace_back("frame"); };hooks.pop=[&](auto,float,float,auto) { effects.emplace_back("pop"); };
    bool unsupported{};try { CreateObjectAt("explicit parser input",hooks); }catch (const std::out_of_range&) { unsupported=true; }CHECK(unsupported && effects==std::vector<std::string>({"create","owner","hp"}));
    expectedType=76;hooks.parse=[](auto) { return DestroyAtLocation{20,21,76,0,1}; };hooks.owner={};bool missing{};try { CreateObjectAt("explicit parser input",hooks); }catch (const std::logic_error&) { missing=true; }CHECK(missing && effects.back()=="create");
}

//TYPE+15c가L방향 첫프레임에서 개수/종류 필드까지 넘어가는 물리표를 보존한다.
TEST_CASE(CreateAt_FrameFieldRetainsFirstCountsKindsAndRejectsPointers) {
    using namespace netstorm::o;const auto frames=CreateAtFrames(true);CHECK(CreateAtFrameField(frames,0x15c)==static_cast<std::uint32_t>(frames.Run('L').first));CHECK(CreateAtFrameField(frames,0x12c)==16);CHECK(CreateAtFrameField(frames,0x1ac)==static_cast<std::uint32_t>(frames.Run('P').count));
    bool pointer{};try { static_cast<void>(CreateAtFrameField(frames,0x124)); }catch (const std::out_of_range&) { pointer=true; }CHECK(pointer);
}
