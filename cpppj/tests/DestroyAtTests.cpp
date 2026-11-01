// 원문 문자열16·일반입력48·삭제부모804관측을 독립 기대값과 대조한다.
#include "TestSupport.h"
#include "client/DestroyAtInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include "o/RawDestroyAt.h"
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 원시 관측의 필드/바이트를 복원하는 검사 도구다.
// 탭 필드를 입력/관측 순서대로 분리한다.
std::vector<std::string> DestroyFields(std::string_view line) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=line.find('\t',begin);result.emplace_back(line.substr(begin,end==line.npos ? line.npos : end-begin));if (end==line.npos) return result;begin=end+1; } // 빈 마지막 열도 보존한다.
}
// DWORD 관측을 호스트 long 폭과 관계없이 읽는다.
std::uint32_t DestroyNumber(std::string_view text) { return static_cast<std::uint32_t>(std::stoul(std::string(text))); }
// 관측 문자열의 원래 BYTE를 복원한다.
std::string DestroyBytes(std::string_view text) {
    if (text=="-") return {};std::string result;
    for (std::size_t i=0;i<text.size();i+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(i,2)),nullptr,16)); // 두16진 문자를 한바이트로 읽는다.
    return result;
}
// 현재 BYTE를 원문 파일과 같은 소문자16진 표기로 기록한다.
std::string DestroyHex(std::span<const std::uint8_t> bytes) {
    static constexpr std::string_view digits="0123456789abcdef"; // 실제 관측 파일의 진법 글자다.
    std::string result;for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; } // 바이트 순서를 바꾸지 않는다.
    return result;
}
// 실제 장치 호출만 세미콜론으로 이어 기록한다.
void DestroyTrace(std::string& trace,std::string entry) { if (!trace.empty()) trace+=';';trace+=entry; }
// 입력의 네 물리 슬롯은 명시 가상 표와 타입/상태/소유자만 보유한다.
std::array<std::uint8_t,200> DestroyPool() {
    std::array<std::uint8_t,200> bytes{};
    for (std::size_t sid=1;sid<4;++sid) { // 원문 장치의 같은50BYTE raw 입력을 설치한다.
        constexpr std::uint32_t table=0x12001000; // 호스트 포인터가 아닌 원문 격리 장치 식별자다.
        for (std::size_t byte=0;byte<4;++byte) bytes[sid*50+byte]=static_cast<std::uint8_t>(table>>(byte*8)); // 실제 little endian DWORD를 보존한다.
        bytes[sid*50+10]=static_cast<std::uint8_t>(99+sid);bytes[sid*50+11]=static_cast<std::uint8_t>(sid);bytes[sid*50+34]=sid==2 ? 2:1;
    }
    return bytes;
}
}

// 토큰 구분자·따옴표·길이 제한·NUL·atol과 다섯 출력을 실제 원문과 대조한다.
TEST_CASE(DestroyAt_Native16LocationParsers) {
    std::ifstream file(NETSTORM_DESTROY_AT_PARSE_FIXTURE);if (!file) throw std::runtime_error("DestroyAt parser fixture 열기 실패");
    std::string line;std::getline(file,line);std::size_t count{};
    while (std::getline(file,line)) { // 문자열만 제품에 주고 뒤의 정수/호출은 독립 기대값으로 읽는다.
        const auto row=DestroyFields(line);CHECK(row.size()==7);if (row.size()!=7) continue;const auto text=DestroyBytes(row[0]);std::string trace;
        const auto result=netstorm::o::ParseDestroyAtLocation(text,[&](auto name) {
            DestroyTrace(trace,"type:"+(name.empty() ? std::string("-"):DestroyHex({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()})));
            if (name.empty()) return 0;std::string key(name);for (auto& byte:key) if (byte>='A' && byte<='Z') byte=static_cast<char>(byte+('a'-'A')); // 원문 명시 타입 조회 경계의 ASCII 이름만 해석한다.
            return key=="suncannon" ? 128:key=="buildmana" ? 134:-1;
        });
        CHECK(std::bit_cast<std::uint32_t>(result.x)==DestroyNumber(row[1]) && std::bit_cast<std::uint32_t>(result.y)==DestroyNumber(row[2]) && result.type==DestroyNumber(row[3]));
        CHECK(std::bit_cast<std::uint32_t>(result.radius)==DestroyNumber(row[4]) && result.owner==DestroyNumber(row[5]) && trace==row[6]);++count;
    }
    CHECK(count==16);
}

// 입력의 모드/뗌·전체 y 전달과 실제 parser/정수 큐 이름 연결을 확인한다.
TEST_CASE(DestroyAt_Native48InputAndScriptQueue) {
    using namespace netstorm::client;
    std::ifstream file(NETSTORM_DESTROY_AT_INPUT_FIXTURE);if (!file) throw std::runtime_error("DestroyAt 입력 fixture 열기 실패");
    std::string line;std::getline(file,line);std::size_t count{};
    while (std::getline(file,line)) { // 앞세열만 입력으로 주며 뒤관측은 직접 비교한다.
        const auto row=DestroyFields(line);CHECK(row.size()==6);if (row.size()!=6) continue;const auto code=DestroyNumber(row[0]),parameter=DestroyNumber(row[1]),mode=DestroyNumber(row[2]);std::string trace;
        CHECK(DispatchDestroyAtInput({code,305419896,std::bit_cast<std::int32_t>(parameter)},(mode&1)!=0,(mode&2)!=0,[&](auto token) { DestroyTrace(trace,"destroyAt:"+std::to_string(token)); }));
        CHECK((trace.empty() ? "-":trace)==row[3] && row[4]=="1" && row[5]=="0");++count;
    }
    CHECK(count==48);InputQueue queue;ScriptStringRing strings;QueueScriptCommand("dEsTrOyAt,(205x186)sunCannon/25#2",queue,strings,{GameMenuScriptCode,{}});
    const auto event=queue.Pop(false);CHECK(event.code==kDestroyAtCommand && IsGameMenuCommand(event.code) && event.x==event.y && queue.Empty());CHECK(strings.Resolve(std::bit_cast<std::uint32_t>(event.y))=="(205x186)sunCannon/25#2");CHECK(GameMenuScriptCode("DestroyAts")==0xffffffffU);
}

// 실제 권한/점 정수화/필터 루프804관측과 장치 효과 뒤 늦은 읽기·전체raw를 비교한다.
TEST_CASE(DestroyAt_Native804ParentsKeepPointFilterAndNextOrder) {
    using namespace netstorm::o;
    std::ifstream file(NETSTORM_DESTROY_AT_PARENT_FIXTURE);if (!file) throw std::runtime_error("DestroyAt 부모 fixture 열기 실패");
    std::string line;std::getline(file,line);std::size_t count{};
    while (std::getline(file,line)) { // 앞8열은 입력이며 trace/pool은 원문 기대값이다.
        const auto row=DestroyFields(line);CHECK(row.size()==10);if (row.size()!=10) continue;
        const DestroyAtLocation location{std::stoi(row[0]),std::stoi(row[1]),DestroyNumber(row[2]),std::stoi(row[3]),DestroyNumber(row[4])};
        const auto bossMode=DestroyNumber(row[5]),profile=DestroyNumber(row[6]),mutation=DestroyNumber(row[7]);bool boss=bossMode==1;std::uint32_t capacity=4;std::size_t cursor{};auto pool=DestroyPool();std::string trace;
        const std::array<std::vector<std::uint32_t>,4> orders{{{1,2,3,0},{0},{4,1,0},{1,4,2,0}}}; // 명시 finder의 입력 순서이며 실제 필터 정답을 계산하지 않는다.
        const auto& order=orders.at(profile);DestroyAtHooks hooks;
        hooks.boss=[&] { return boss; };hooks.assertion=[&](auto where) { DestroyTrace(trace,"assert:"+std::to_string(where));if (bossMode==2) boss=true; };
        hooks.parse=[&](auto) { DestroyTrace(trace,"parse");return location; };
        hooks.begin=[&](auto area) { DestroyTrace(trace,"begin:"+std::to_string(std::bit_cast<std::uint32_t>(area.left))+':'+std::to_string(std::bit_cast<std::uint32_t>(area.top))+':'+std::to_string(std::bit_cast<std::uint32_t>(area.right))+':'+std::to_string(std::bit_cast<std::uint32_t>(area.bottom)));return order[0]; };
        hooks.next=[&] { const auto next=order.at(++cursor);DestroyTrace(trace,"next:"+std::to_string(next));return next; };hooks.capacity=[&] { return capacity; };
        hooks.slot=[&](auto sid) { return std::span<const std::uint8_t>(pool).subspan(sid*50,50); };
        hooks.genus=[&](auto sid) { const auto type=pool[sid*50+10];const auto flags=type==100 ? 0x100U:type==101 ? 0x2000U:0x80U;DestroyTrace(trace,"genus:"+std::to_string(sid)+':'+std::to_string(flags));if (mutation==2) pool[sid*50+34]=2;return flags; };
        hooks.destroy=[&](auto sid,auto flags) { DestroyTrace(trace,"delete:"+std::to_string(sid)+':'+std::to_string(flags));pool[sid*50+11]=1;if (mutation==1) { pool[2*50+10]=100;pool[2*50+34]=1; }if (mutation==3) capacity=2; };
        DestroyObjectsAt("explicit parser boundary",hooks);CHECK((trace.empty() ? "-":trace)==row[8] && DestroyHex(pool)==row[9]);++count;
    }
    CHECK(count==804);
}

// 실제로 도달하지 않은 하위는 요구하지 않으며 선택된 부모의 누락은 실패한다.
TEST_CASE(DestroyAt_OnlyReachedDevicesAreRequired) {
    using namespace netstorm::client;using namespace netstorm::o;
    CHECK(!DispatchDestroyAtInput({0x02000017U,0,0},true,true,{}));CHECK(DispatchDestroyAtInput({kDestroyAtCommand|InputCode::kRelease,0,0},true,true,{}));CHECK(DispatchDestroyAtInput({kDestroyAtCommand,0,0},false,false,{}));
    bool missing{};try { static_cast<void>(DispatchDestroyAtInput({kDestroyAtCommand,0,0},true,false,{})); }catch (const std::logic_error&) { missing=true; }CHECK(missing);
    DestroyAtHooks hooks;std::size_t diagnostics{};hooks.boss=[] { return false; };hooks.assertion=[&](auto line) { CHECK(line==839);++diagnostics; };DestroyObjectsAt("",hooks);CHECK(diagnostics==1);
    hooks.boss=[] { return true; };missing=false;try { DestroyObjectsAt("",hooks); }catch (const std::logic_error&) { missing=true; }CHECK(missing);
}
