// 원문 분배/실제 Regexp512행의 전체 표·입력·현재 전역·외부 호출 순서를 대조한다.
#include "TestSupport.h"
#include "client/RawDotCommandDispatch.h"
#include "client/RawDotCommandRegister.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 관측 자료의 입력 구성과 원시 출력 직렬화만 수행하는 검사 부품이다.
using namespace netstorm::client;
constexpr std::uint32_t kNativeCallback=0x20000100; // 원문 관측의 격리 명령 장치 식별자다.
// 원시 DWORD 입력/장치 변경만 같은 little endian 네 바이트에 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // 네 실제 바이트 외의 입력은 보존한다.
}
// 최초 원문 관측과 독립적으로 지정했던 등록 표 입력을 구성한다.
std::array<std::uint8_t,kDotCommandStateBytes> Initial(unsigned scene) {
    std::array<std::uint8_t,kDotCommandStateBytes> result{};std::array<std::string,5> names{"say","clear","echo","front",std::string(31,'x')}; // 이름/함수/표시/count의 실제 입력이다.
    if (scene==3) names[1]="echo";
    for (std::size_t index=0;index<names.size();++index) { // 중복 이름과 실제31BYTE 이름을 원문 입력과 같은 위치에 쓴다.
        std::copy(names[index].begin(),names[index].end(),result.begin()+index*32);
        Put(result,0x800+index*4,kNativeCallback+static_cast<std::uint32_t>(index)*16);
        Put(result,0x900+index*4,0x531b30+static_cast<std::uint32_t>(index)*32);
    }
    Put(result,0xa04,scene==1 ? 0U : scene==2 ? 0xffffffffU : 5U);return result;
}
// 관측 TSV의 각 열을 빈 열까지 보존하며 읽는다.
std::vector<std::string> Fields(std::string_view text) {
    std::vector<std::string> result;std::size_t start{}; // 각 TAB 사이의 실제 열 위치다.
    for (;;) { const auto end=text.find('\t',start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1; } // 입력 순서대로 열을 복사한다.
}
// 고정 관측의 hex를 유니코드 변환 없이 원시 입력 바이트로 되돌린다.
std::vector<std::uint8_t> Bytes(std::string_view text) {
    std::vector<std::uint8_t> result;if (text=="-") return result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16))); // 두 자리마다 한 바이트를 복원한다.
    return result;
}
// 전체 물리/문자열을 원문 관측과 같은 소문자 hex로 직렬화한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // BYTE 인코딩용 고정 문자표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // 미작성 인접 바이트도 누락하지 않는다.
    return result;
}
// 오류에 전달된 현재 이름 바이트만 위와 같은 hex로 표현한다.
std::string Hex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 실제 NUL 앞의 장치 인자만 복사해 호출 순서를 기록한다.
std::string Argument(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::runtime_error("명령 장치 인자 NUL 없음");
    return Hex(bytes.first(static_cast<std::size_t>(end-bytes.begin())));
}
// 같은 원문 가변 입력의 NUL과 보호 꼬리를 구성한다.
std::vector<std::uint8_t> Input(std::string_view text) {
    std::vector<std::uint8_t> result(text.begin(),text.end());result.push_back(0);result.insert(result.end(),32,0xd7);return result;
}
}

// 최초 원문512행/독립1024관측을 현재 제품의 전체 출력과 한 번씩 비교한다.
TEST_CASE(DotCommandDispatch_IndependentNative512) {
    std::ifstream file(NETSTORM_DOT_COMMAND_DISPATCH_FIXTURE);if (!file) throw std::runtime_error("점 명령 분배 원문 관측 없음");std::string line;std::getline(file,line);std::size_t count{};
    while (std::getline(file,line)) { // 기대 출력으로 장치를 결정하지 않고 최초 입력 열만 사용한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto row=Fields(line);if (row.size()!=9) throw std::runtime_error("점 명령 분배 관측 열수 오류");
        const auto scene=static_cast<unsigned>(std::stoul(row[1]));const auto mode=std::stoul(row[2]);auto table=Initial(scene);auto input=Bytes(row[0]);input.push_back(0);input.insert(input.end(),32,0xd7);
        DotCommandDispatchState state;DotCommandDispatchHooks hooks;std::size_t calls{};std::vector<std::string> trace; // 현재 명령표/검색/장치 입력의 독립 실행 상태다.
        hooks.invoke=[&](auto token,auto argument) {
            ++calls;trace.push_back("C:"+std::to_string(token-kNativeCallback)+":"+Argument(argument));
            if (calls==1) {
                if (mode==3) { constexpr std::string_view name="changed";std::copy(name.begin(),name.end(),table.begin()+32);table[32+name.size()]=0;Put(table,0x808,kNativeCallback+48); }
                else if (mode==4) { state.fallbackIndex=1;constexpr std::string_view name="new-default";std::copy(name.begin(),name.end(),table.begin()+32);table[32+name.size()]=0; }
                else if (mode==5 && argument[0]) argument[0]='Z';
                else if (mode==6 && state.separatorOffset!=-1) state.separatorOffset=2;
            }
            return mode==2 ? 0x80000001U : mode==1 || mode==3 || mode==4 || mode==7 ? 0x1234U : 0U;
        };
        hooks.error=[&](auto name,auto code) { std::ostringstream output;output<<"E:"<<Hex(name)<<":"<<std::hex<<code;trace.push_back(output.str());if (mode==7 && state.separatorOffset!=-1) state.separatorOffset=2; };
        const auto result=DispatchDotCommand(table,state,input,hooks);
        CHECK(std::to_string(result)==row[3]);CHECK(Hex(table)==row[4]);CHECK(std::to_string(state.fallbackIndex)==row[5]);CHECK(std::to_string(state.separatorOffset)==row[6]);CHECK(Hex(input)==row[7]);
        std::string joined;
        for (const auto& event:trace) { if (!joined.empty()) joined+=';';joined+=event; } // 실제 호출 순서를 원문과 같은 한 열로 연결한다.
        CHECK((joined.empty() ? "-" : joined)==row[8]);++count;
    }
    CHECK(count==512);
}

// 빈 입력은 검색/표/장치를 읽지 않고, 실제 도달한 장치 누락만 명시적으로 거부한다.
TEST_CASE(DotCommandDispatch_LazyDevicesAndPhysicalBounds) {
    std::array<std::uint8_t,1> empty{};DotCommandDispatchState state{123,77};CHECK(DispatchDotCommand({},state,empty,{})==1);CHECK(state.fallbackIndex==123 && state.separatorOffset==77);
    auto table=Initial(0);auto input=Input(".clear");state={};bool rejected{};
    try { static_cast<void>(DispatchDotCommand(table,state,input,{})); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    DotCommandDispatchHooks hooks;hooks.invoke=[](auto,auto) { return 1U; };rejected=false;
    try { static_cast<void>(DispatchDotCommand(table,state,input,hooks)); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    hooks.invoke=[](auto,auto) { return 0U; };CHECK(DispatchDotCommand(table,state,input,hooks)==1);
    std::array<std::uint8_t,2> unterminated{'h','i'};rejected=false;
    try { static_cast<void>(DispatchDotCommand(table,state,unterminated,hooks)); } catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
}

// 같은 상태의 실제 중첩 호출은 바깥 명령의 다음 위치에도 현재 Regexp 캡처를 사용한다.
TEST_CASE(DotCommandDispatch_ReentrantSharedCapture) {
    auto table=Initial(0);auto outer=Input(".echo abc;tail");auto inner=Input(".clear;");DotCommandDispatchState state;DotCommandDispatchHooks hooks;std::vector<std::string> trace;
    hooks.invoke=[&](auto token,auto argument) { trace.push_back(std::to_string(token-kNativeCallback)+":"+Argument(argument));if (token==kNativeCallback+32) CHECK(DispatchDotCommand(table,state,inner,hooks)==1);return 0U; };
    CHECK(DispatchDotCommand(table,state,outer,hooks)==1);
    CHECK((trace==std::vector<std::string>{"32:20616263","16:","0:6162633b7461696c"}));CHECK(state.separatorOffset==-1);
}
