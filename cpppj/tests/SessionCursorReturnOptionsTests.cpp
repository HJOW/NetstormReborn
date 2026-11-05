// 원본 반환 몸체가 실제 설정 getter를 생성 전후와 할당 뒤에 읽는 순서를 검사한다.
#include "FormTypeInputs.h" // 원문 공통 Create가 읽는 내장 Form 생성자 주소를 입력에 제공한다.
#include "ConstructionSupport.h"
#include "client/RawCursorReturnPiece.h"

using namespace netstorm::test::construction;
using namespace netstorm::client;
namespace { // 현재 옵션을 바꿀 수 있는 실제 contained 생성/부착과 타이머 경계다.
constexpr Sid kParent{3}; // 외부 부착 경계가 사용할 명시 원본 부모 SID다.
struct ReturnOptionsScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; // 실제 PATCH의 타입6 슬롯을 소유한다.
    std::vector<RiftTypeRecord> types=netstorm::test::MakeFormTypeInputs(188);SquidFactory factory{pool,types}; // 같은 실제 생성기를 사용한다.
    ContainedCreateState containedState;CursorReturnPieceState state;GameRandom random{17}; // 독립 state는 의도적으로 잘못된 초기 Slow를 유지한다.
    std::uint32_t timers{},fort{},flagReads{},modeReads{},rateReads{},allocations{};std::uint8_t rate{2}; // 실제 getter 입력과 호출 수다.
    Sid born{};float duration{};std::uint32_t timerCount{}; // 실제 생성 슬롯과 타이머 요청을 관측한다.
    std::function<void()> attached,allocated; // 생성/할당 효과가 현재 옵션을 바꾸는 명시 검사 장치다.
    RawContainedCreate contained{pool,containedState,ContainedHooks()}; // 계산/flags/품질 쓰기는 실제 몸체다.
    ReturnOptionsScene() { state.type=128;state.keepPieces=1;state.lifetimeIndex=0; } // 서비스 조회를 state 초기값으로 대체하면 검사가 실패해야 한다.
    ContainedCreateHooks ContainedHooks() {
        ContainedCreateHooks hooks;hooks.notifySurface=[](Sid) { CHECK(false); };
        hooks.attach=[this](Sid sid,Sid parent,std::uint32_t flags) { CHECK(parent==kParent && flags==0x10);born=sid;if (attached) attached(); };
        return MakeContainedCreateHooks(pool,factory,std::move(hooks));
    } // 실제 생성 직후의 부착만 외부 경계로 둔다.
    CursorReturnPieceHooks Hooks() {
        CursorReturnPieceHooks hooks;hooks.productTimers=[this] { ++flagReads;return timers; };
        hooks.fortMode=[this] { ++modeReads;return fort; };hooks.unitRate=[this] { ++rateReads;return rate; };
        hooks.allocateTimer=[this](auto bytes) { CHECK(bytes==40);++allocations;if (allocated) allocated();return true; };
        hooks.createTimer=[this](Sid sid,auto event,float value) { CHECK(sid==born && event==0);duration=value;++timerCount; };return hooks;
    } // 원문에서 선택하는 시점마다 현재 저장소를 조회한다.
    void Return() { RawCursorReturnPiece(contained,state,random,Hooks()).Return(8,kParent.value,3); } // 현재 같은 몸체/생성기에 반환을 요청한다.
};
}
TEST_CASE(ReturnOptions_CurrentFastAndChangedRate) {
    ReturnOptionsScene scene;scene.Return();CHECK(scene.duration==1 && scene.timerCount==1 && scene.flagReads==2 && scene.modeReads==1 && scene.rateReads==1);
    CHECK(Get(scene.pool.Slot(scene.born),20,2)==0x108 && scene.pool.Slot(scene.born)[30]==2);
    scene.rate=1;scene.Return();CHECK(scene.duration==5 && scene.timerCount==2);scene.rate=0;scene.Return();CHECK(scene.duration==10 && scene.timerCount==3);
} // 실제 Fast/Medium/Slow를 같은 살아 있는 반환 몸체의 입력으로 확인한다.
TEST_CASE(ReturnOptions_RequeryAfterCreateAndAllocation) {
    ReturnOptionsScene scene;scene.attached=[&] { scene.timers=1; };scene.Return();
    CHECK(scene.flagReads==2 && !scene.allocations && !scene.modeReads && !scene.rateReads && Get(scene.pool.Slot(scene.born),20,2)==0x108);
    scene.timers=0;scene.attached={};scene.rate=0;scene.allocated=[&] { scene.rate=2; };scene.Return();CHECK(scene.duration==1 && scene.rateReads==1);
} // 생성 뒤 재조회와 할당 뒤 BYTE2 조회를 앞당기지 않는다.
TEST_CASE(ReturnOptions_FortSkipsRateAndEnabledSkipsTimer) {
    ReturnOptionsScene scene;scene.fort=1;scene.rate=255;scene.Return();CHECK(Bits(scene.duration)==0x38d1b717U && !scene.rateReads);
    scene.timers=1;scene.Return();CHECK(scene.timerCount==1 && scene.allocations==1 && scene.flagReads==4 && Get(scene.pool.Slot(scene.born),20,2)==8);
} // 요새의 짧은 예약과 별도 재고 타이머 경로에서 불필요한 조회/예약을 하지 않는다.
