// 단위864: 보호막 Regular의 도달 장치·현재 TYPE·진단 정상 복귀와 콜백 중 처리기 반납을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawForcefieldLifecycle.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
// 합성 입력과 실제 처리기의 수명을 이 파일 안에 제한한다.
namespace {
// 검사마다 같은 풀/타입/해시와 물리 좌표 슬롯을 만들며 게임 틱은 실행하지 않는다.
struct ForceRegularScene {
    SidPool pool; // 대상까지 할당된 실제 PATCH/CD 슬롯 풀이다.
    SquidHash hash; // 독립 finder용 해시이며 기본 입력은 비어 있다.
    std::vector<RiftTypeRecord> types; // 판본별 TYPE의 물리 표다.
    ForcefieldRegularState state; // 진단/현재 사제 TYPE 입력이다.
    // 기본 슬롯은 보호막 VT와 raw 좌표만 가지며 공간에 Pop하지 않는다.
    explicit ForceRegularScene(OriginalEdition edition=OriginalEdition::Patch1078)
        :pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {
        // 지정된 대상 번호까지 실제 슬롯을 확보한다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,edition==OriginalEdition::Patch1078 ? kPatchForcefieldVtable : kCdForcefieldVtable);raw[10]=167;
        Put(raw,14,std::bit_cast<std::uint32_t>(20.75F));Put(raw,18,std::bit_cast<std::uint32_t>(21.9F));
    }
};
}
TEST_CASE(ForcefieldRegularCurrent_UnknownEventSkipsRawAndUnselectedDevices) {
    ForceRegularScene scene;RawForcefieldRegular body(scene.pool,scene.hash,scene.types,scene.state,{});
    CHECK(body.Handle(Sid{65535},99,0xffffffffU,std::numeric_limits<float>::quiet_NaN())==0);
    CHECK(Throws([&] { body.Handle(kSource,1,0,0); }));
}
TEST_CASE(ForcefieldRegularCurrent_AdvanceOnlyAndCallbackCanReleaseHandler) {
    ForceRegularScene scene;std::unique_ptr<RawForcefieldRegular> body;std::uint32_t calls{}; // 콜백이 처리기 소유자를 반납한다.
    ForcefieldRegularHooks hooks;hooks.advance=[&](Sid sid,std::int32_t delta,std::uint32_t flags) { CHECK(sid==Sid{65535} && delta==1 && flags==0);++calls;body.reset(); };
    body=std::make_unique<RawForcefieldRegular>(scene.pool,scene.hash,scene.types,scene.state,std::move(hooks));
    CHECK(body->Handle(Sid{65535},1,0,0)==std::bit_cast<float>(0x3da3d70aU));CHECK(!body && calls==1);
}
TEST_CASE(ForcefieldRegularCurrent_CurrentDwordAndRawFloatsReachTypedFinder) {
    ForceRegularScene scene;std::uint32_t type=158,calls{};scene.state.sharedPriestType=&type; // 독립 사본 뒤에도 바뀌는 장기 DWORD다.
    auto raw=scene.pool.AllocatedBytes(kSource);raw[10]=74;raw[11]=9;Put(raw,0,0xdeadbeefU);
    const auto before=Hex(raw);ForcefieldRegularHooks hooks;
    hooks.findTypeAt=[&](float x,float y,std::uint32_t current) { CHECK(std::bit_cast<std::uint32_t>(x)==std::bit_cast<std::uint32_t>(20.75F) && std::bit_cast<std::uint32_t>(y)==std::bit_cast<std::uint32_t>(21.9F));CHECK(current==type);++calls;return Sid{60}; };
    RawForcefieldRegular body(scene.pool,scene.hash,scene.types,scene.state,std::move(hooks));
    CHECK(body.Handle(kSource,2,0,0)==std::bit_cast<float>(0x3c23d70aU));type=0x1009eU;
    CHECK(body.Handle(kSource,2,0,0)==std::bit_cast<float>(0x3c23d70aU));CHECK(calls==2 && Hex(raw)==before);
}
TEST_CASE(ForcefieldRegularCurrent_MissingDestroyPreservesCompletedFindEffect) {
    ForceRegularScene scene;std::uint32_t calls{};ForcefieldRegularHooks hooks;
    hooks.findTypeAt=[&](float,float,std::uint32_t) { ++calls;Put(scene.pool.AllocatedBytes(kSource),26,1234);return Sid{}; };
    RawForcefieldRegular body(scene.pool,scene.hash,scene.types,scene.state,std::move(hooks));
    CHECK(Throws([&] { body.Handle(kSource,2,0,0); }));CHECK(calls==1 && Get(scene.pool.Slot(kSource),26)==1234);
}
TEST_CASE(ForcefieldRegularCurrent_DestroyCallbackCanReleaseHandler) {
    ForceRegularScene scene;std::unique_ptr<RawForcefieldRegular> body;std::uint32_t calls{};ForcefieldRegularHooks hooks;
    hooks.findTypeAt=[](float,float,std::uint32_t) { return Sid{}; };
    hooks.destroy=[&](Sid sid,std::uint32_t flags) { CHECK(sid==kSource && flags==0);++calls;body.reset(); };
    body=std::make_unique<RawForcefieldRegular>(scene.pool,scene.hash,scene.types,scene.state,std::move(hooks));
    CHECK(body->Handle(kSource,2,0,0)==-1.0F);CHECK(!body && calls==1);
}
TEST_CASE(ForcefieldRegularCurrent_DiagnosticReadsCurrentCheckingAndReturnsAfterRelease) {
    ForceRegularScene scene;std::uint32_t checking{},calls{};scene.state.debug.Bind(checking);std::unique_ptr<RawForcefieldRegular> body;ForcefieldRegularHooks hooks;
    hooks.assertion=[&](auto expression,auto file,std::uint32_t line) { CHECK(expression=="0" && file==".\\Dais.cpp" && line==0x479);++calls;body.reset(); };
    body=std::make_unique<RawForcefieldRegular>(scene.pool,scene.hash,scene.types,scene.state,std::move(hooks));
    CHECK(body->Handle(Sid{65535},99,0,0)==0 && calls==0);checking=0x80000000U;
    CHECK(body->Handle(Sid{65535},99,0,0)==0);CHECK(!body && calls==1);
}
TEST_CASE(ForcefieldRegularCurrent_MissingSelectedDiagnosticFailsWithoutRawMutation) {
    ForceRegularScene scene;scene.state.debug=true;RawForcefieldRegular body(scene.pool,scene.hash,scene.types,scene.state,{});const auto before=Hex(scene.pool.Slot(kSource));
    CHECK(Throws([&] { body.Handle(kSource,99,0,0); }));CHECK(Hex(scene.pool.Slot(kSource))==before);
}
TEST_CASE(ForcefieldRegularCurrent_CdKeepsPreflightAndNoDiagnosticForUnknownEvent) {
    ForceRegularScene scene(OriginalEdition::Cd1072);CHECK(Throws([&] { RawForcefieldRegular missing(scene.pool,scene.hash,scene.types,scene.state,{}); }));
    ForcefieldRegularHooks hooks;hooks.advance=[](Sid,std::int32_t,std::uint32_t) { CHECK(false); };hooks.destroy=[](Sid,std::uint32_t) { CHECK(false); };hooks.assertion=[](auto,auto,std::uint32_t) { CHECK(false); };scene.state.debug=true;
    RawForcefieldRegular body(scene.pool,scene.hash,scene.types,scene.state,std::move(hooks));CHECK(body.Handle(kSource,99,0,0)==0);
    scene.pool.AllocatedBytes(kSource)[10]=74;CHECK(Throws([&] { body.Handle(kSource,99,0,0); }));
}
