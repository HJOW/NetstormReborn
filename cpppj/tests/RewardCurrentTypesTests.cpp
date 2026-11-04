// 단위863: 현재 TYPE 대여와 독립 사본의 차이를 HP·비용·지면 조회에서 검사한다. 게임 틱이나 전투는 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawPriestState.h"
#include "o/RawPriestOwner.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
// 이 파일의 합성 입력 장면만 숨기는 검사 전용 영역이다.
namespace {
// 같은 풀의 보상·사제 상태를 만들고 생성 후 TYPE 필드 변경 입력을 공급하는 장면이다.
struct RewardCurrentScene {
    SidPool pool; // 실제 슬롯을 보관하는 판본별 풀이다.
    GameRandom random; // PATCH SP 저장소를 구성하며 조회 검사에서는 소비하지 않는다.
    ScrambledSpStore store; // PATCH 보상 구성에 필요한 SP 장치다.
    SquidPostPopState book; // 같은 풀의 장부다.
    SquidRewardState mode; // HP와 환불 모드 입력이다.
    std::vector<RiftTypeRecord> types; // 처리기 생성 뒤에도 주소를 유지하는 현재 TYPE 표다.
    std::vector<std::uint8_t> spots; // 이동 가능한 지면 비트 지도다.
    std::unique_ptr<SquidReward> reward; // 기본은 표 사본을 사용하는 보상 처리기다.
    std::unique_ptr<RawPriestState> priest; // 같은 보상 처리기의 HP를 읽는 사제 상태다.
    // TYPE158, HP150, 최대HP200과 좌표(20,21)를 명시 입력으로 설정한다.
    explicit RewardCurrentScene(OriginalEdition edition=OriginalEdition::Patch1078)
        :pool(edition,kCapacity,false),store(random,[] { return 1U; }),types(edition==OriginalEdition::Patch1078 ? 188 : 171),spots(65536,6) {
        // 조회 대상까지 실제 할당하며 특수 객체 생성이나 행동은 하지 않는다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
        types[158].maxHitPoints=200;types[158].flags1=0x10;types[158].flags2=0x200000;types[158].cost=80;
        types[164].maxHitPoints=400;types[164].flags1=0x10;
        reward=std::make_unique<SquidReward>(pool,types,book,mode,edition==OriginalEdition::Patch1078 ? &store : nullptr);
        priest=std::make_unique<RawPriestState>(pool,*reward,types,spots);
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,edition==OriginalEdition::Patch1078 ? kPatchPriestVtable : kCdPriestVtable);raw[10]=158;
        Put(raw,14,std::bit_cast<std::uint32_t>(20.0F));Put(raw,18,std::bit_cast<std::uint32_t>(21.0F));Put(raw,26,150,edition==OriginalEdition::Patch1078 ? 4 : 2);
    }
    // 두 처리기가 같은 현재 배열을 빌리게 한다. 독립 사본 검사는 호출하지 않는다.
    void Bind() { reward->BindCurrentTypes(types);priest->BindCurrentTypes(types); }
};
}
TEST_CASE(RewardCurrentTypes_MaximumReadsCurrentTableAndPatchCostKeepsLoadedCache) {
    RewardCurrentScene scene;scene.Bind();const auto before=Hex(scene.pool.Slot(kSource));const auto seed=scene.random.State();
    CHECK(scene.reward->TypeHitPoints(158)==200 && scene.reward->TypeCostUnits(158)==80);
    scene.types[158].maxHitPoints=600;scene.types[158].cost=120;
    CHECK(scene.reward->MaxHitPoints(kSource)==600 && scene.reward->TypeCostUnits(158)==80);
    scene.mode.priestQuarterHp=true;CHECK(scene.reward->MaxHitPoints(kSource)==150);
    CHECK(Hex(scene.pool.Slot(kSource))==before && scene.random.State()==seed && scene.reward->VirtualCountCalls()==0);
}
TEST_CASE(RewardCurrentTypes_CurrentHpFlagsAndDaisMaximumUseSameTable) {
    RewardCurrentScene scene;scene.Bind();CHECK(scene.reward->HasHitPoints(kSource));
    scene.types[158].flags1=0;CHECK(!scene.reward->HasHitPoints(kSource));
    scene.types[158].flags2=0x400000;CHECK(!scene.reward->HasHitPoints(kSource));
    scene.pool.AllocatedBytes(kSource)[40]=1;CHECK(scene.reward->HasHitPoints(kSource) && scene.reward->MaxHitPoints(kSource)==400);
    scene.types[164].maxHitPoints=900;CHECK(scene.reward->MaxHitPoints(kSource)==900);
}
TEST_CASE(RewardCurrentTypes_PriestGenusAndHalfHpBoundaryReadCurrentTable) {
    RewardCurrentScene scene;scene.Bind();CHECK(!scene.priest->GroundImmobile(kSource));
    scene.types[158].maxHitPoints=400;CHECK(scene.priest->GroundImmobile(kSource));
    scene.types[158].flags2=0;CHECK(!scene.priest->GroundImmobile(kSource));
    scene.types[158].flags2=0x200000;CHECK(scene.priest->GroundImmobile(kSource));
    scene.types[158].maxHitPoints=200;CHECK(!scene.priest->GroundImmobile(kSource));
}
TEST_CASE(RewardCurrentTypes_IndependentAndDetachedBindingsKeepConstructionSnapshot) {
    // PATCH와 CD의 기본 독립 구성 모두 기존 생성 시 사본을 유지한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        RewardCurrentScene scene(edition);scene.types[158].maxHitPoints=400;scene.types[158].cost=120;
        CHECK(scene.reward->MaxHitPoints(kSource)==200 && scene.reward->TypeCostUnits(158)==80 && !scene.priest->GroundImmobile(kSource));
        if (edition==OriginalEdition::Patch1078) {
            scene.Bind();CHECK(scene.reward->MaxHitPoints(kSource)==400 && scene.priest->GroundImmobile(kSource));
            scene.reward->BindCurrentTypes({});scene.priest->BindCurrentTypes({});
            CHECK(scene.reward->MaxHitPoints(kSource)==200 && !scene.priest->GroundImmobile(kSource));
        }
    }
}
TEST_CASE(RewardCurrentTypes_InvalidBindingsPreservePreviousBorrowAndBounds) {
    RewardCurrentScene patch;patch.Bind();patch.types[158].maxHitPoints=400;
    CHECK(Throws([&] { patch.reward->BindCurrentTypes(std::span<const RiftTypeRecord>(patch.types).first(187)); }));
    CHECK(Throws([&] { patch.priest->BindCurrentTypes(std::span<const RiftTypeRecord>(patch.types).first(187)); }));
    CHECK(patch.reward->MaxHitPoints(kSource)==400 && patch.priest->GroundImmobile(kSource));
    CHECK(Throws([&] { patch.reward->TypeHitPoints(188); }));CHECK(Throws([&] { patch.reward->TypeCostUnits(188); }));
    RewardCurrentScene cd(OriginalEdition::Cd1072);
    CHECK(Throws([&] { cd.reward->BindCurrentTypes(cd.types); }));CHECK(Throws([&] { cd.priest->BindCurrentTypes(cd.types); }));
    CHECK(cd.reward->MaxHitPoints(kSource)==200 && !cd.priest->GroundImmobile(kSource));
}
