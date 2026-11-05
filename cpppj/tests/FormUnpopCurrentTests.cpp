// Form 공통 부모의 최초 PE 관측과 현재 World 종속 분리만 검사한다. 게임/MP는 실행하지 않는다.
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawSquidContainedUnpop.h"
#include "o/SquidFactory.h"
#include "o/SquidForms.h"

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // fixture와 같은 합성 바이트만 공급하며 기대값은 계산하지 않는다.
constexpr std::uint32_t kFormInputVtable=0x14000000; // 명시 가상 장치의 두 VT 토큰 시작값이다.
std::span<std::uint8_t> FormInputRaw(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 검사에서만 free/SID0의 물리 입력도 구성한다.
BridgeConnectState FormWorldLinks() { BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;return links; } // World 설치에 필요한 정상 TYPE 입력이다.
void ContentAttachmentInput(SidPool& pool,Sid form,Sid parent) { // 정렬 부착 자체를 시험하지 않고 이미 부착된 단일 ContentForm의 명시 입력만 놓는다.
    auto raw=FormInputRaw(pool,form);raw[11]=8;Put(raw,4,0,2);Put(raw,14,0,2);Put(raw,16,parent.value,2);Put(FormInputRaw(pool,parent),6,form.value,2);
}
}

TEST_CASE(FormUnpopCurrent_IndependentPePrefixFixture) { // 두 PE의 실제 진단/가상 선택/void와 두 슬롯 전체를 최초 관측에 대조한다.
    std::ifstream input(NETSTORM_FORM_UNPOP_CURRENT_FIXTURE);CHECK(input.good());if (!input) throw std::runtime_error("Form Unpop 최초 관측 파일 없음");
    std::string line;std::array<std::size_t,2> counts{};
    while (std::getline(input,line)) { // 각 행은 이전 결과와 독립된 합성 물리 입력으로 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');CHECK(row.size()==4);if (row.size()!=4) throw std::runtime_error("Form Unpop 관측 열 수 오류");const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");++counts[patch ? 0 : 1];const auto fields=Split(row[1],',');CHECK(fields.size()==5);
        const Sid sid{static_cast<std::uint16_t>(Number(fields[0]))};const auto number=Number(fields[1]),state=Number(fields[2]),parentState=Number(fields[3]),profile=Number(fields[4]);SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768);
        for (std::uint16_t index=0;index<16;++index) { // Python 장치와 동일한 입력 패턴만 놓는다.
            auto raw=FormInputRaw(pool,Sid{index});for (std::size_t offset=0;offset<raw.size();++offset) raw[offset]=static_cast<std::uint8_t>((index*raw.size()+offset)*37+19); // 각 물리 슬롯의 초기 payload다.
        }
        auto raw=FormInputRaw(pool,sid);Put(raw,0,kFormInputVtable);raw[10]=static_cast<std::uint8_t>(number);raw[11]=static_cast<std::uint8_t>(state);FormInputRaw(pool,Sid{9})[11]=static_cast<std::uint8_t>(parentState);std::string events;
        const auto append=[&](std::string event) { if (!events.empty()) events+=';';events+=event; }; // 관측된 원문 호출 순서만 직렬화한다.
        FormUnpopHooks hooks;
        hooks.assertion=[&](auto expression,auto file,auto sourceLine) { append("D"+std::string(expression)+"|"+std::string(file)+"|"+std::to_string(sourceLine));if (profile==1) { raw[10]=6;raw[11]=8;Put(raw,0,kFormInputVtable+0x100); } }; // Native와 같은 명시 정상 복귀/외부 변경 입력이다.
        hooks.parent=[&](Sid current,std::uint32_t table) { CHECK(current==sid);append("P"+std::to_string(table));if (profile==2) FormInputRaw(pool,Sid{9})[11]^=1;return Sid{9}; }; // getter 뒤 바뀐 현재 부모 free를 본문이 다시 읽어야 한다.
        hooks.detach=[&](Sid current) { CHECK(current==sid);append("U"+std::to_string(pool.Slot(current)[11])); }; // 실제 종속 자식으로 주장하지 않는 관측 장치다.
        SquidForms(pool,{}, {},std::move(hooks)).Unpop(sid);CHECK(events==row[2]);CHECK(Hex(pool.Slot(sid))+";"+Hex(pool.Slot(Sid{9}))==row[3]);
    }
    CHECK(counts[0]==864 && counts[1]==864);
}

TEST_CASE(FormUnpopCurrent_WorldTakeUsesCurrentSessionDetachAndReturnsToDefault) { // 실제 Factory의 현재 VT와 약한 Session 대여를 통해 같은 종속 분리에 도달한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 양판본 실제 World 공장을 쓰되 게임/통신은 시작하지 않는다.
        const bool patch=edition==OriginalEdition::Patch1078;auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);types[7].constructorAddress=TypeConstructorAddress(edition,3);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());RawSurfaceWorld world(edition,types,frames,shapes,FormWorldLinks(),83);auto core=world.Core();
        const auto parent=core.factory.Create(74),form=core.factory.CreateContained();ContentAttachmentInput(core.pool,form,parent);Put(FormInputRaw(core.pool,form),18,0x12345678);FormInputRaw(core.pool,form)[10]=7;
        std::uint32_t contentType=7,gumpType=3,event=0x02000039;std::size_t notices{},commands{};SquidContainedUnpopHooks hooks;
        hooks.parent=[&](Sid sid) { return CommonContainedParent(core.pool,sid); }; // 현재 Form VT의 부모 WORD를 사용한다.
        hooks.previous=[&](Sid sid) { return CommonContainedPrevious(core.pool,sid); }; // TYPE7을 고정 Form 검사로 거부하지 않는다.
        hooks.setPrevious=[&](Sid sid,Sid previous) { CommonContainedSetPrevious(core.pool,sid,previous); }; // 현재 이웃의 VT로 분배한다.
        hooks.parentNotice=[&](Sid owner,Sid source,std::uint32_t reason) { CHECK(owner==parent && source==form && reason==2 && core.pool.Slot(form)[11]==4);++notices;world.Storage()->types[7].constructorAddress=TypeConstructorAddress(edition,1); }; // 실제 분리 알림 뒤 변경된 현재 생성자를 Take가 선택해야 한다.
        hooks.command=[&](auto,auto,auto) { ++commands; }; // Form 해제는 자신의 UI finder를 호출하지 않는다.
        auto session=std::make_shared<RawSquidContainedUnpop>(core.pool,SquidContainedUnpopState{contentType,gumpType,event,{}, {}, {}},std::move(hooks));world.BindContainedUnpop(session);const auto free=core.pool.FreeCount();
        CHECK(core.factory.Take(7,form)==form);CHECK(notices==1 && commands==0 && Get(core.pool.Slot(form),0)==SquidForms(core.pool).Vtable(1));CHECK(core.pool.Slot(form)[10]==7 && core.pool.Slot(form)[11]==4 && Get(core.pool.Slot(form),18)==0x12345678 && Get(core.pool.Slot(parent),6,2)==0 && core.pool.FreeCount()==free);
        world.BindContainedUnpop({});session.reset();auto raw=FormInputRaw(core.pool,form);raw[11]=8;Put(raw,4,0,2);Put(raw,14,0,2);Put(raw,16,parent.value,2);Put(FormInputRaw(core.pool,parent),6,form.value,2);core.factory.UnpopForm(form);CHECK(notices==1 && commands==0 && raw[11]==4 && Get(core.pool.Slot(parent),6,2)==0); // 반납한 Session 알림을 다시 호출하지 않고 World 기본 분리로 돌아간다.
    }
}

TEST_CASE(FormUnpopCurrent_MissingNoticeKeepsCompletedLinkWrites) { // 선택된 부모 알림 부재를 선행 검사로 당겨 원문의 부분 변경을 없애지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 같은 물리 링크와 payload를 두 판본에서 확인한다.
        SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SquidFactory factory(pool,types);const auto parent=factory.Create(74),form=factory.CreateContained();SquidForms forms(pool);ContentAttachmentInput(pool,form,parent);Put(FormInputRaw(pool,form),18,0xabcdef01);const auto free=pool.FreeCount();CHECK(Throws([&] { forms.Unpop(form); }));CHECK(pool.Slot(form)[11]==4 && Get(pool.Slot(parent),6,2)==0 && Get(pool.Slot(form),18)==0xabcdef01 && pool.FreeCount()==free);
    }
}

TEST_CASE(FormUnpopCurrent_ActualGetterUsesRepairedVtableAfterDiagnostic) { // 잘못된 TYPE 진단 복귀가 남긴 현재 VT와 상태를 실제 공통 getter/분리가 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 정상 복귀/부모 dead 및 free 분기를 실제 물리 풀에 연결한다.
        SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SquidFactory factory(pool,types);const auto parent=factory.Create(74),form=factory.CreateGumpForm(0);SquidForms(pool).Attach(form,parent,0x10);auto raw=FormInputRaw(pool,form);raw[10]=70;Put(raw,0,0xdeadbeef);std::size_t diagnostics{};FormUnpopHooks hooks;
        hooks.assertion=[&](auto expression,auto,auto line) { CHECK(expression=="isForm()" && line==(edition==OriginalEdition::Patch1078 ? 0xa59U : 0xa10U));++diagnostics;raw[10]=69;Put(raw,0,SquidForms(pool).Vtable(3)); }; // 고정 타입69의 Vtable 조회를 넣으면 이 정상 가상 진입을 잃는다.
        SquidForms(pool,{}, {},hooks).Unpop(form);CHECK(diagnostics==1 && raw[11]==4 && Get(pool.Slot(parent),6,2)==0);
        for (const auto parentState:{1U,2U,3U}) { // 부모 free는 분리 자체를 생략하고 dead는 분리 안에서 링크를 보존한다.
            raw[11]=8;Put(FormInputRaw(pool,parent),6,form.value,2);FormInputRaw(pool,parent)[11]=static_cast<std::uint8_t>(parentState);SquidForms(pool).Unpop(form);CHECK(raw[11]==12 && Get(pool.Slot(parent),6,2)==form.value);
        }
    }
}
