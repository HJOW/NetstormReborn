// 원본 기계어 관측13입력과 실제 부모의 부분 실패를 검사한다. 큰 게임 하위는 명시 장치다.
#include "TestSupport.h"
#include "o/RawFortPriestPreparation.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
using namespace netstorm::o;
namespace { // Native 관측의 입력과 원시 trace만 다룬다.
void PutPriestFixture(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::uint32_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } // 명시 입력의 실제 DWORD만 쓴다.
}
// 부모/두 helper/안정 제거/테마 TYPE를 실제 PE로 실행한 고정13행을 대조한다.
TEST_CASE(FortPriestPreparation_Native13ActualParentAndListHelpers) {
    std::ifstream file(NETSTORM_FORT_PRIEST_PREPARATION_FIXTURE);CHECK(file.good());std::string line;std::uint32_t rows{};
    while (std::getline(file,line)) { // 주석 외 모든 최초 관측 행을 한 번씩 독립 물리에서 재현한다.
        if (line.empty() || line[0]=='#') continue;std::istringstream fields(line);std::string label,expected,post;std::getline(fields,label,'\t');std::getline(fields,expected,'\t');std::getline(fields,post);++rows;
        SidPool pool(OriginalEdition::Patch1078,30000);PriestPostPopState priests;bool mission=label=="mission";std::uint32_t editor=label=="editor",subscriber{5},templateOwner{1},playerOwner{7},finds{};
        std::array<std::uint8_t,28> state{};state[17]=label=="state17";std::vector<std::string> events;
        if (label=="duplicates") priests.priests.entries={1,1,2};else if (label=="full-capacity") { for (std::uint32_t value=1;value<=22;++value) priests.priests.entries.push_back(value); } // 고정20칸 출력은 더 큰 원문 목록도 끝까지 조회한다.
        else if (label!="populate") priests.priests.entries={1,2,3};priests.priests.count=static_cast<std::uint32_t>(priests.priests.entries.size());
        auto bytes=std::span(const_cast<std::uint8_t*>(pool.Bytes().data()),pool.Bytes().size()); // 검사 소유의 실제 비const Pool에 raw 입력만 설치한다. 게임 생성/Pop을 실행한 것으로 세지 않는다.
        for (std::uint32_t sid=1;sid<=22;++sid) { auto raw=bytes.subspan(sid*50,50);PutPriestFixture(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(sid)));PutPriestFixture(raw,18,std::bit_cast<std::uint32_t>(1.0F));raw[12]=(label=="marked" && sid==2) || (label=="duplicates" && sid==1) ? 0x80 : 0; } // Native 입력과 같은 좌표/표시 비트다.
        FortPriestPreparationHooks hooks;hooks.baseType=[](auto index) { return index==0 ? 133U : index==1 ? 71U : 128U; };hooks.themeType=[](auto theme) { CHECK(theme==2);return 77U; };
        hooks.technology=[&] { events.push_back("tech");return 4U; };hooks.contains=[&](auto parent,auto type) { CHECK(parent==4);events.push_back("find:"+std::to_string(type));++finds;if (label=="post-change" && finds==1) mission=true;return finds%2==0 ? type : 0U; };
        hooks.createTechnology=[&](auto parent,auto type) { CHECK(parent==4);events.push_back("create:"+std::to_string(type)); };hooks.templatePresent=[](auto owner) { CHECK(owner==1);return true; };hooks.state=[&](auto owner) { CHECK(owner==1);return std::span<std::uint8_t>(state); };
        hooks.territory=[&](auto owner,auto output) { CHECK(owner==1);events.push_back("territory");std::fill(output.begin(),output.end(),std::uint8_t{});if (label!="no-record") output[1]=1;if (label=="two-regions") output[7]=1; };
        hooks.island=[&](auto owner,auto index) { CHECK(owner==7);events.push_back("island:"+std::to_string(index));return label=="missing-island" ? 127 : static_cast<int>(index)+1; };
        hooks.region=[&](float x,float y) { CHECK(y==1.0F);if (label=="shorten-list") priests.priests.count=1;return label=="two-regions" && x==3.0F ? 2 : 1; };
        hooks.setOwner=[&](auto sid,auto owner) { events.push_back("owner:"+std::to_string(sid)+":"+std::to_string(owner));bytes[sid*50+34]=static_cast<std::uint8_t>(owner); };
        hooks.populate=[&](auto region) { events.push_back("populate:"+std::to_string(region)); };hooks.tellPriests=[&](auto count) { events.push_back("tell:"+std::to_string(count)); };
        RawFortPriestPreparation(pool,priests,{mission,editor,subscriber,templateOwner,playerOwner},std::move(hooks)).Run();
        std::string actual;for (const auto& event:events) { if (!actual.empty()) actual+=';';actual+=event; } // 원문 callback trace의 순서와 인자를 모두 대조한다.
        CHECK(actual==expected && std::to_string(mission ? 1 : 0)==post);
    }
    CHECK(rows==13);
}
// 선택된 기술 생성의 예외는 앞선 효과를 보존하고 뒤 장치 누락을 미리 요구하지 않는다.
TEST_CASE(FortPriestPreparation_SelectedFailureKeepsEarlierTechnologyEffects) {
    SidPool pool(OriginalEdition::Patch1078,30000);PriestPostPopState priests;bool mission{};std::uint32_t editor{1},subscriber{},owner{1},reads{},created{};
    FortPriestPreparationHooks hooks;hooks.baseType=[&](auto index) { ++reads;return 100U+index; };hooks.technology=[] { return 1U; };hooks.contains=[](auto,auto) { return 0U; };
    hooks.createTechnology=[&](auto,auto type) { ++created;if (type==101) throw std::runtime_error("선택한 두 번째 기술 생성 중단"); };
    bool stopped{};try { RawFortPriestPreparation(pool,priests,{mission,editor,subscriber,owner,owner},std::move(hooks)).Run(); }catch (const std::runtime_error& error) { stopped=std::string_view(error.what())=="선택한 두 번째 기술 생성 중단"; }
    CHECK(stopped && reads==2 && created==2 && !mission && editor==1);
}
