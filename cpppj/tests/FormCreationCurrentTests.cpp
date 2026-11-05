// 실제 Form 전용 생성의 현재 TYPE/반환 VT와 contained 현재 번호만 검사한다. 게임/수신/통신은 실행하지 않는다.
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "o/GumpFormHost.h"
#include "o/SquidForms.h"
#include "o/SquidUnpop.h"
#include "o/RawContainedCreate.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;

TEST_CASE(FormCreationCurrent_FacadesUseCurrentConstructorsAndReturnedVtable) { // 두 판본의6/3이 고정VT를 쓰지 않고 현재 공통 생성자를 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 같은 풀/현재 표를 사용하고 기대VT는 기존 검증된 원문 생성자 표에서 읽는다.
        SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SquidFactory factory(pool,types);factory.BindDiagnostics({types,{},{}});SquidForms forms(pool);
        const auto first=factory.CreateContained(2);CHECK(Get(pool.Slot(first),0)==forms.Vtable(6) && pool.Slot(first)[10]==6);
        types[6].constructorAddress=TypeConstructorAddress(edition,1);const auto second=factory.CreateContained(2);CHECK(Get(pool.Slot(second),0)==forms.Vtable(1) && pool.Slot(second)[10]==6 && pool.Slot(second)[11]==4);
        types[3].constructorAddress=TypeConstructorAddress(edition,6);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);GumpFormHost gumps(pool,factory,types,destroy);
        const auto third=gumps.Create(2);CHECK(Get(pool.Slot(third),0)==forms.Vtable(6) && pool.Slot(third)[10]==3 && Get(pool.Slot(third),26)==0);
    }
}

TEST_CASE(FormCreationCurrent_NullConstructorUsesRealBaseAndSelectedDiagnostics) { // 명시 주소0을 고정 Form VT로 대체하지 않고 실제 자산 초기화에 도달한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두 판본의 진단과 HP 저장 폭을 함께 확인한다.
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);types[3].constructorAddress=0;types[3].zOrder=0x123;types[3].flags1=16;types[3].maxHitPoints=100;SquidFactory factory(pool,types);std::vector<std::uint32_t> lines;
        factory.BindDiagnostics({types,[&](auto expression,auto file,auto line) { CHECK(expression=="!isForm()" && file==(patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp"));lines.push_back(line); },{}});
        const auto free=pool.FreeCount();const auto sid=factory.CreateGumpForm(2);const auto raw=pool.Slot(sid);CHECK(lines==std::vector<std::uint32_t>{patch ? 0x7d4U : 0x7a6U});CHECK(Get(raw,0)==(patch ? 0x501dd0U : 0x5058d8U) && raw[10]==3 && raw[patch ? 35 : 33]==0x23 && Get(raw,26,patch ? 4 : 2)==100 && pool.FreeCount()==free-1);
    }
}

TEST_CASE(FormCreationCurrent_ContainedAdapterUsesBorrowedTypeAndPostCreateBridge) { // 현재 전체 번호와 자식 정상 복귀 뒤 현재 bridge 번호를 읽는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 실제 공통 생성은 같은 Form ctor/빈+28을 사용하며 부착 효과만 명시 장치다.
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);types[7].constructorAddress=TypeConstructorAddress(edition,6);types[8].constructorAddress=TypeConstructorAddress(edition,3);SquidFactory factory(pool,types);factory.BindDiagnostics({types,{},{}});
        std::uint32_t containedType=7,bridgeType=82,notices{},attachments{};ContainedCreateState state;state.sharedContainedType=&containedType;state.sharedBridgeType=&bridgeType;const auto copied=state;const Sid parent{50};ContainedCreateHooks hooks;
        hooks.notifySurface=[&](Sid sid) { CHECK(pool.Slot(sid)[10]==containedType && Get(pool.Slot(sid),18,2)==0x1234 && Get(pool.Slot(sid),12,2)==0x5678);++notices; };
        hooks.attach=[&](Sid sid,Sid actualParent,std::uint32_t flags) { CHECK(actualParent==parent && flags==0x10 && Get(pool.Slot(sid),20,2)==0x2345 && pool.Slot(sid)[patch ? 30 : 26]==0x67);++attachments; };
        hooks=MakeContainedCreateHooks(pool,factory,std::move(hooks));const auto create=hooks.create;hooks.create=[&](auto type,auto flags) { const auto sid=create(type,flags);bridgeType=type;return sid; }; // 원문 자식의 명시 정상 복귀 변경이며 부모가 값을 미리 저장하면 실패한다.
        RawContainedCreate body(pool,state,std::move(hooks));const auto first=body.Create(parent,0x1234,0x5678,0x12345,0x10,0x167);CHECK(pool.Slot(first)[10]==7 && copied.ContainedType()==7 && notices==1 && attachments==1);
        containedType=8;const auto second=body.Create(parent,0x1234,0x5678,0x12345,0x10,0x167);CHECK(pool.Slot(second)[10]==8 && copied.ContainedType()==8 && notices==2 && attachments==2);
    }
}
