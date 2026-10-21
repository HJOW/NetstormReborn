// 지상 전용 필터/실제 해시·거리·Depend 조회의 경계를 검사한다. 작성·컴파일만 한다.
#include "o/RawGunGroundTarget.h"
#include "o/SquidFactory.h"
#include "RawSceneSupport.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 서버 슬롯/네 단계 해시/기존 Gun과 현재 진단 전역을 함께 준비한다.
struct GroundScene {
    SidPool pool{OriginalEdition::Patch1078,32768,false};SquidHash hash; // 같은 실제 후보/종속 풀과 일반 해시다.
    std::vector<RiftTypeRecord> types{188};std::vector<GunTargetType> targetTypes{188};std::vector<SquidDisplayShape> shapes{188}; // 현재 TYPE/사거리/확장 높이/SHP 입력이다.
    GunTargetState state;RawGunTarget common{pool,hash,types,targetTypes,shapes,state}; // 실제 공통 HP·서버번호·동맹 필터다.
    std::uint32_t excluded{72},depend{1};bool checking{},metamap{},boss{true}; // 실제 전역 위치에 대응하는 현재 입력이다.
    std::vector<std::uint32_t> diagnostics;std::function<void(std::uint32_t)> duringDiagnostic; // 선택된 원본 진단과 그 효과다.
    // 서버 후보는 실제 Take로 확보하고 client5/6은 명시 종속 체인 입력으로 쓴다.
    GroundScene() {
        // 모든 물리 타입의 기본 발자국만 입력한다.
        for (auto& type:types) type.footX=type.footY=1;
        types[90].group=3;targetTypes[90].range=10;types[80].flags1=0x10;types[81].flags1=0x10;types[72].group=3;
        CHECK(pool.Allocate(2)==Sid{5} && pool.Allocate(2)==Sid{6});SquidFactory factory(pool,types);
        // 원본의 서버 대역에만 실제 Take하며 결과 SID를 사후에 덮어쓰지 않는다.
        for (Sid sid:{Sid{15000},Sid{15001},Sid{15002}}) CHECK(factory.Take(80,sid)==sid);
        state.allyMasks[1]=1U<<1;
    }
    // 명시 raw 객체를 실제 버킷 머리에 넣고 next를 원본 순서로 저장한다.
    void Object(Sid sid,float x,float y,std::uint8_t type=80) { auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;raw[34]=2;Put(raw,26,1000);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));const auto scale=SquidHash::kScales[1];auto& head=hash.Cell(1,static_cast<int>(x)/scale,static_cast<int>(y)/scale);Put(raw,4,head,2);head=sid.value; }
    // 실제 현재 Depend Form과 next를 명시 입력한다.
    void Child(Sid sid,std::uint8_t type,Sid next={}) { auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;Put(raw,4,next.value,2); }
    // 호출 중 바뀐 실제 전역도 같은 참조로 읽는다.
    RawGunGroundTarget Actions() { return RawGunGroundTarget(pool,hash,types,targetTypes,common,state,{excluded,depend,checking,metamap,boss,[this](std::string_view text,auto line) { CHECK(text==(line==0x158 ? "inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy" : "shootMyFriends || me->isAlly(me->getPlayerId())"));diagnostics.push_back(line);if (duringDiagnostic) duringDiagnostic(line); }}); }
    // 명시 Pop좌표/실제1×1중심/방향0/프레임0의 원본 문맥이다.
    GunTargetQuery Query() const { return {50.5F,50.5F,50,50,90,0,1,0}; }
};
}

// 비행/벌룬은 공통 필터 전에 제외하고 일반 지상 표적은 실제 공통 필터로 간다.
TEST_CASE(GunGroundTarget_AirborneAndCommonTargetFilters) {
    GroundScene scene;scene.Object(Sid{15000},52,50);auto actions=scene.Actions();CHECK(&actions.Pool()==&scene.pool && actions.MayTarget(scene.Query(),Sid{15000}));scene.types[80].flags2=0x100000;CHECK(!actions.MayTarget(scene.Query(),Sid{15000}));scene.types[80].flags2=0x20000;CHECK(!actions.MayTarget(scene.Query(),Sid{15000}));scene.types[80].flags2=0;scene.pool.AllocatedBytes(Sid{15000})[34]=1;CHECK(!actions.MayTarget(scene.Query(),Sid{15000}));scene.state.friendlyFire=true;CHECK(actions.MayTarget(scene.Query(),Sid{15000}));
}
// Depend는 자산70이상을 건너뛰고 현재 전역 TYPE의 첫 Form을 찾는다.
TEST_CASE(GunGroundTarget_WalkerUsesActualDependChainAndCurrentType) {
    GroundScene scene;scene.Object(Sid{15000},52,50);scene.types[80].flags2=0x10000;scene.Child(Sid{5},80,Sid{6});scene.Child(Sid{6},1);Put(scene.pool.AllocatedBytes(Sid{15000}),6,5,2);CHECK(!scene.Actions().MayTarget(scene.Query(),Sid{15000}));scene.depend=2;CHECK(scene.Actions().MayTarget(scene.Query(),Sid{15000}));scene.depend=80;CHECK(scene.Actions().MayTarget(scene.Query(),Sid{15000}));scene.depend=1;scene.pool.AllocatedBytes(Sid{6})[11]=3;CHECK(!scene.Actions().MayTarget(scene.Query(),Sid{15000}));
}
// 제외 TYPE72는 손상된 Depend 체인도 조회하기 전에 반환한다.
TEST_CASE(GunGroundTarget_ExcludedSourcePrecedesDependencyLookup) {
    GroundScene scene;scene.Object(Sid{15000},52,50);scene.types[80].flags2=0x10000;Put(scene.pool.AllocatedBytes(Sid{15000}),6,65535,2);auto query=scene.Query();query.type=72;CHECK(!scene.Actions().MayTarget(query,Sid{15000}));query.type=90;CHECK(Throws([&] { scene.Actions().MayTarget(query,Sid{15000}); }));
}
// 원본 dead 부모 진단 뒤 head를 다시 읽고 metamap/boss에서는 진단만 생략한다.
TEST_CASE(GunGroundTarget_DependencyDiagnosticAndCurrentHead) {
    GroundScene scene;scene.Object(Sid{15000},52,50);scene.types[80].flags2=0x10000;scene.checking=true;scene.boss=false;scene.pool.AllocatedBytes(Sid{15000})[11]=2;scene.Child(Sid{5},1);scene.duringDiagnostic=[&](auto line) { CHECK(line==0x158);Put(scene.pool.AllocatedBytes(Sid{15000}),6,5,2); };CHECK(!scene.Actions().MayTarget(scene.Query(),Sid{15000}) && scene.diagnostics==std::vector<std::uint32_t>{0x158});Put(scene.pool.AllocatedBytes(Sid{15000}),6,0,2);scene.diagnostics.clear();scene.metamap=true;CHECK(scene.Actions().MayTarget(scene.Query(),Sid{15000}) && scene.diagnostics.empty());scene.metamap=false;scene.boss=true;CHECK(scene.Actions().MayTarget(scene.Query(),Sid{15000}) && scene.diagnostics.empty());
}
// 첫 일치 이후 체인은 읽지 않고 일치 없이 순환한 경우만 명시 실패한다.
TEST_CASE(GunGroundTarget_DependencyFirstMatchAndCycleBoundary) {
    GroundScene scene;scene.Object(Sid{15000},52,50);scene.types[80].flags2=0x10000;scene.Child(Sid{5},1,Sid{5});Put(scene.pool.AllocatedBytes(Sid{15000}),6,5,2);CHECK(!scene.Actions().MayTarget(scene.Query(),Sid{15000}));scene.depend=2;CHECK(Throws([&] { scene.Actions().MayTarget(scene.Query(),Sid{15000}); }));
}
// 실제 거리 제곱이 range²와 같으면 제외하고 조금 큰 사거리에서만 선택한다.
TEST_CASE(GunGroundTarget_ActualHashStrictSquaredRange) {
    GroundScene scene;scene.Object(Sid{15000},52,50);scene.targetTypes[90].range=2;CHECK(!scene.Actions().Find(50.5F,50.5F,90,1).value);scene.targetTypes[90].range=2.01F;CHECK(scene.Actions().Find(50.5F,50.5F,90,1)==Sid{15000});
}
// 같은 해시 체인의 등거리 후보는 처음 반환한 SID를 유지한다.
TEST_CASE(GunGroundTarget_ActualHashKeepsFirstEqualCandidate) {
    GroundScene scene;scene.Object(Sid{15000},52,50);scene.Object(Sid{15001},52,50);CHECK(scene.Actions().Find(50.5F,50.5F,90,1)==Sid{15001});
}
// 현재 Player 동맹은 첫 후보 준비 뒤와 각 공통 필터에서 새로 읽는다.
TEST_CASE(GunGroundTarget_CurrentAllianceAndDiagnosticContinuation) {
    GroundScene scene;scene.Object(Sid{15000},52,50);scene.Object(Sid{15001},55,50);std::size_t calls{};scene.state.allyMaskRead=[&](auto owner) { CHECK(owner==1);return ++calls==2 ? (1U<<1)|(1U<<2) : 1U<<1; };CHECK(scene.Actions().Find(50.5F,50.5F,90,1)==Sid{15001} && calls==3);scene.state.allyMaskRead={};scene.state.allyMasks[1]=0;scene.duringDiagnostic=[&](auto line) { CHECK(line==0x2fc);scene.state.allyMasks[1]=1U<<1; };CHECK(scene.Actions().Find(50.5F,50.5F,90,1).value && scene.diagnostics==std::vector<std::uint32_t>{0x2fc});
}
// 원본0.99999 보정은 단순ceil과 다르며 검색 문맥은 실제 발자국 절반을 뺀다.
TEST_CASE(GunGroundTarget_SnapBiasAndAsymmetricSourceFootprint) {
    GroundScene scene;scene.Object(Sid{15000},50.4F,50.4F);scene.targetTypes[90].range=0.9F;CHECK(scene.Actions().Find(50.500004F,50.5F,90,1)==Sid{15000});GroundScene wide;wide.types[90].footX=2;wide.types[90].footY=4;wide.Object(Sid{15000},52,50);wide.targetTypes[90].range=2.01F;CHECK(wide.Actions().Find(51,52,90,1)==Sid{15000});
}
// 이 별도 검색은 일반 Gun의 사정선/최종 표적 그림 출력을 호출하지 않는다.
TEST_CASE(GunGroundTarget_NoRangeDrawAndRejectedWalkerDoesNotWin) {
    GroundScene scene;scene.Object(Sid{15000},52,50);scene.Object(Sid{15001},55,50,81);scene.types[80].flags2=0x10000;scene.Child(Sid{5},1);Put(scene.pool.AllocatedBytes(Sid{15000}),6,5,2);scene.state.showRanges=true;CHECK(scene.Actions().Find(50.5F,50.5F,90,1)==Sid{15001});
}
