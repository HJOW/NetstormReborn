// 삭제와 이동이 같은 현재 TYPE를 읽고 독립 명시 사본은 원래 값을 보유하는지 확인한다.
#include "RawSceneSupport.h"
#include "client/RawPriestWorldBindings.h"
#include "o/RawGameAuxState.h"
using namespace netstorm::o;
using namespace netstorm::client;

// 현재 장기 TYPE를 바꾼 뒤 실제 이동 선택의 우선순위와 새 view의 주소 정체성을 관측한다.
TEST_CASE(CurrentDamageableRoles_MovementUsesSameCurrentOwner) {
    RawGameAuxState owner;owner.RainWalkerType()=99;owner.BulfType()=100;owner.TempleProductType()=96;owner.EmptyCarrierType()=97;
    RawMovementProcessTypes first(&owner.PriestType(),&owner);std::vector<RiftTypeRecord> types(188);types[99].flags2=TypeFlag2::kBalloon;
    CHECK(SelectMovementProcessType(OriginalEdition::Patch1078,types,first,99)==first.walkerProcess);CHECK(SelectMovementProcessType(OriginalEdition::Patch1078,types,first,100)==first.bulfProcess);
    owner.RainWalkerType()=101;owner.BulfType()=102;owner.WindFlyerType()=103;owner.RainFlyerType()=104;owner.ResetForGeneralWorld();RawMovementProcessTypes second(&owner.PriestType(),&owner);
    CHECK(SelectMovementProcessType(OriginalEdition::Patch1078,types,first,99)==first.balloonProcess);CHECK(SelectMovementProcessType(OriginalEdition::Patch1078,types,second,101)==second.walkerProcess);CHECK(SelectMovementProcessType(OriginalEdition::Patch1078,types,second,102)==second.bulfProcess);
    CHECK(&first.sunWalker==&owner.TempleProductType() && &first.empty==&owner.EmptyCarrierType() && &first.rainWalker==&second.rainWalker && &first.bulf==&owner.BulfType());CHECK(owner.WindFlyerType()==103 && owner.RainFlyerType()==104);
}

// 명시 사본은 독립 값이며 대입은 목적지의 실제 주소를 유지한 채 값을 쓴다.
TEST_CASE(CurrentDamageableRoles_ExplicitCopyStaysIndependent) {
    RawGameAuxState owner;owner.RainWalkerType()=99;RawMovementProcessTypes borrowed(&owner.PriestType(),&owner);RawMovementProcessTypes copied(borrowed);std::vector<RiftTypeRecord> types(188);types[99].flags2=TypeFlag2::kBalloon;owner.RainWalkerType()=100;
    CHECK(SelectMovementProcessType(OriginalEdition::Patch1078,types,copied,99)==copied.walkerProcess);CHECK(SelectMovementProcessType(OriginalEdition::Patch1078,types,borrowed,99)==borrowed.balloonProcess);CHECK(&copied.rainWalker!=&owner.RainWalkerType());
    RawMovementProcessTypes independent;independent.rainWalker=101;borrowed=independent;CHECK(&borrowed.rainWalker==&owner.RainWalkerType() && owner.RainWalkerType()==101 && copied.rainWalker==99);
}
