//연소 세 앞단의 현재 SID/슬롯 고정·진단 복귀를 최초 원문 관측과 비교한다.
#include "TestSupport.h"
#include "RawSceneSupport.h"
#include "o/RawBurningVictim.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <string>

using namespace netstorm::o;
namespace { //기대 파일은 읽기 전용이며 새 게임이나 실제 효과를 실행하지 않는다.
std::string VictimTextHex(std::string_view text) { return netstorm::test::rawscene::Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } //조건/파일의 원문 바이트다.
template<class Function> bool FailedVictim(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; } //선택 장치 누락/진단 중단을 빈 반환과 구별한다.
}
TEST_CASE(BurningVictimParent_Native504CurrentSidAndSelectedSlot) { //세 부모의 전체1008관측에 대응하는 고유504행을 모두 실행한다.
    using namespace netstorm::test::rawscene;std::ifstream input(NETSTORM_BURNING_VICTIM_PARENT_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) { //최초 관측에서 현재 입력만 읽고 제품 결과로 기대값을 쓰지 않는다.
        const auto row=Split(line,'\t');CHECK(row.size()==6);if (row.size()!=6) return;const auto p=Split(row[0],',');CHECK(p.size()==8);if (p.size()!=8) return;
        const auto entry=static_cast<BurningVictimEntry>(Number(p[0]));Sid current{static_cast<std::uint16_t>(Number(p[1]))};auto checking=Number(p[2]);const auto mask=Number(p[3]),vtable=Number(p[4]),hp=Number(p[5]),resume=Number(p[6]),mutate=Number(p[7]);std::uint32_t calls{};std::string events,outcome="empty",selected="-";BurningVictimHooks hooks;
        const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; //진단 앞뒤 실제 효과 순서만 기록한다.
        hooks.robust=[&](Sid sid,bool live) { CHECK(live);const bool result=((mask>>std::min(calls,1U))&1U)!=0;event("R:"+std::to_string(sid.value)+":"+std::to_string(result));++calls;if (mutate==1 && calls==1) { current=Sid{1};checking=0; }return result; };
        hooks.vtable=[&](Sid sid) { event("V:"+std::to_string(sid.value)+":"+std::to_string(vtable));return vtable!=0; };
        hooks.hasHitPoints=[&](Sid sid) { event("H:"+std::to_string(sid.value)+":"+std::to_string(hp));if (mutate==2) { current=Sid{1};checking=0; }return hp!=0; };
        hooks.assertion=[&](auto condition,auto source,auto sourceLine) { event("D:"+VictimTextHex(condition)+":"+VictimTextHex(source)+":"+std::to_string(sourceLine));if (!resume) throw std::runtime_error("assertion"); };
        try { if (const auto victim=RunBurningVictimParent(current,entry,RawCheckingFlag{checking},hooks)) { outcome="body";selected=std::to_string(victim->value); } }catch (const std::runtime_error& failure) { outcome=failure.what(); }
        const bool same=outcome==row[1] && std::to_string(checking)==row[2] && std::to_string(current.value)==row[3] && selected==row[4] && events==row[5];CHECK(same);if (!same) { std::fprintf(stderr,"연소 앞단 입력%s 상태=%s 현재=%u 선택=%s 사건=%s\n",row[0].c_str(),outcome.c_str(),current.value,selected.c_str(),events.c_str());return; }++rows;
    }
    CHECK(rows==504);
}
TEST_CASE(BurningVictimParent_SelectedHooksAndDwordLifetime) { //미선택 가상 표/HP/진단 없이 조기 반환하고 현재 owner 상위 비트를 보존한다.
    Sid current{50};std::int32_t checking{};RawCheckingFlag view{checking};BurningVictimHooks hooks;std::uint32_t calls{};hooks.robust=[&](Sid sid,bool live) { CHECK(sid==current && live);++calls;return false; };
    CHECK(!RunBurningVictimParent(current,BurningVictimEntry::Capture,view,hooks) && calls==1);checking=std::bit_cast<std::int32_t>(0x80000000U);CHECK(FailedVictim([&] { RunBurningVictimParent(current,BurningVictimEntry::Sources,view,hooks); }) && calls==2 && view.Read()==0x80000000U);
    hooks.assertion=[&](auto condition,auto source,auto line) { CHECK(condition=="isSidRobust(burningSid)" && source==".\\Burning.cpp" && line==0x11f);checking=0;current=Sid{0}; };CHECK(!RunBurningVictimParent(current,BurningVictimEntry::Frame,view,hooks) && calls==4 && current==Sid{0} && !view); //진단 복귀 뒤에도 두 번째 실제 조회를 생략하지 않는다.
}
