// 미션 제한의 장기 DWORD와 Start 출력 주소를 검사한다. 근거: docs/exe/cpp-guide-release-20261101.md.
// 원문004c2b20의 순차 store와004c3210이 World 없이 접근하는 전역 수명을 명시 입력으로 검사하며 새 Native fixture는 만들지 않는다.
#include "TestSupport.h"
#include "client/ClientGameplayGlobals.h"
#include "o/RawMissionStart.h"
#include <limits>
#include <string>

using namespace netstorm::client;
using namespace netstorm::o;
namespace { // 실제 Start의 외부 세계 효과를 선택하지 않는 최소 장치만 제공한다.
// 설정 조회 사이의 직접 store와 SP 갱신 뒤 현재값 보존을 관찰하는 Start 장치다.
MissionStartHooks RestrictionStartHooks(MissionStartGlobals& globals) {
    MissionStartHooks hooks;
    hooks.get=[&globals](std::string_view key) {
        if (key=="denySalvage") return std::string("-17");
        if (key=="denyAscend") { CHECK(globals.DenySalvage()==-17);return std::string("2147483647"); }
        if (key=="allowAnyCapture") CHECK(globals.DenyAscend()==std::numeric_limits<std::int32_t>::max());
        return std::string{};
    };
    hooks.sectionNames=[](std::string_view) { return std::string{}; };hooks.set=[](std::string_view,std::string_view) {}; // 빈 tellAt 목록만 준비하며 실제 Config 출력을 성공 사본으로 되돌리지 않는다.
    hooks.refreshSp=[&globals] { CHECK(globals.DenySalvage()==-17 && globals.DenyAscend()==std::numeric_limits<std::int32_t>::max());globals.DenySalvage()=-29;globals.DenyAscend()=37; };
    hooks.clearTechnology=[](std::uint32_t) {};hooks.addTechnology=[](std::uint32_t,std::string_view) {}; // 기술이 없는 명시 입력의 효과 경계다.
    hooks.hasPriest=[](std::uint32_t) { return false; }; // 사제/AI를 만들거나 공격하지 않는 입력이다.
    hooks.refreshCommandGump=[] {};hooks.resetCamera=[] {};hooks.gameSeconds=[] { return 1.0; }; // Start 꼬리의 선택된 필수 장치다.
    return hooks;
}
}

// 새 대여·준비 대여·반납은 원문 Start/소멸의 명시 store를 대신 수행하지 않아야 한다.
TEST_CASE(MissionRestrictionGlobals_ClientLifetimePreservesDwordsAcrossBorrowing) {
    ClientGameplayGlobals owner;const auto& current=owner;auto* salvage=&owner.DenySalvage();auto* ascend=&owner.DenyAscend();
    CHECK(*salvage==0 && *ascend==0 && salvage!=ascend);CHECK(&current.DenySalvage()==salvage && &current.DenyAscend()==ascend);
    owner.DenySalvage()=-7;owner.DenyAscend()=std::numeric_limits<std::int32_t>::min();
    { auto use=owner.AcquirePrepared();CHECK(owner.Borrowed() && owner.DenySalvage()==-7 && owner.DenyAscend()==std::numeric_limits<std::int32_t>::min()); }
    CHECK(!owner.Borrowed() && *salvage==-7 && *ascend==std::numeric_limits<std::int32_t>::min());
    { auto use=owner.Acquire();CHECK(owner.Borrowed() && &owner.DenySalvage()==salvage && &owner.DenyAscend()==ascend);CHECK(*salvage==-7 && *ascend==std::numeric_limits<std::int32_t>::min());owner.DenySalvage()=41;owner.DenyAscend()=-43; }
    CHECK(!owner.Borrowed() && current.DenySalvage()==41 && current.DenyAscend()==-43);
}

// 실제 Start는 연결된 장기 owner에만 쓰고 효과가 바꾼 현재값을 보존하며 독립 fixture fallback도 유지한다.
TEST_CASE(MissionRestrictionGlobals_StartWritesSharedDwordsAtOriginalBoundaries) {
    ClientGameplayGlobals owner;owner.DenySalvage()=5;owner.DenyAscend()=-9;MissionStartGlobals globals;globals.denySalvage=11;globals.denyAscend=13;
    globals.sharedDenySalvage=&owner.DenySalvage();globals.sharedDenyAscend=&owner.DenyAscend();const auto& current=globals;
    CHECK(&current.DenySalvage()==&owner.DenySalvage() && &current.DenyAscend()==&owner.DenyAscend());
    MissionFrameGateState gate;MissionStartState state;RawMissionStart start(OriginalEdition::Patch1078,gate,state,globals,RestrictionStartHooks(globals));start.Start();
    CHECK(gate.started==1 && owner.DenySalvage()==-29 && owner.DenyAscend()==37);CHECK(globals.denySalvage==11 && globals.denyAscend==13);
    owner.DenySalvage()=-47;owner.DenyAscend()=53;CHECK(current.DenySalvage()==-47 && current.DenyAscend()==53); // World 밖의 명시 writer도 같은 현재 참조에 즉시 드러난다.
    MissionStartGlobals independent;const auto& fallback=independent;CHECK(&fallback.DenySalvage()==&independent.denySalvage && &fallback.DenyAscend()==&independent.denyAscend);
    MissionFrameGateState fallbackGate;MissionStartState fallbackState;RawMissionStart fallbackStart(OriginalEdition::Patch1078,fallbackGate,fallbackState,independent,RestrictionStartHooks(independent));fallbackStart.Start();
    CHECK(fallbackGate.started==1 && independent.denySalvage==-29 && independent.denyAscend==37);CHECK(owner.DenySalvage()==-47 && owner.DenyAscend()==53);
}
