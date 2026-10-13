// 실제 기본 방어false의 원본 반환과 현재 factory 가상 표/엄격한 다른 파생 처리를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawSquidDefense.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 PE가 반환한 기대값만 읽는다.
const std::vector<std::vector<std::string>>& DefenseRows() { static const auto rows=LoadFixture(NETSTORM_SQUIDDEFENSE_FIXTURE).rows;return rows; }
// 현재 판본의 실제 현재 가상 표/방어 진입 대응이다.
std::array<SquidDefenseEntry,5> DefenseEntries(OriginalEdition edition) { return edition==OriginalEdition::Patch1078 ? std::array<SquidDefenseEntry,5>{{{0x501dd0,0x43d9b0},{0x50b888,0x43d9b0},{0x50b9b8,0x43d9b0},{0x50f210,0x43d9b0},{0x509758,0x43d9b0}}} : std::array<SquidDefenseEntry,5>{{{0x5058d8,0x4aed90},{0x5014f8,0x4aed90},{0x501120,0x4aed90},{0x5003e0,0x4aed90},{0x503b30,0x4aed90}}}; }
// 한 판본의 원본 인자 무관 반환/전체 raw 불변을 대조한다.
void ReplayDefense(OriginalEdition edition) {
    SidPool pool(edition,kCapacity,false);std::size_t count=0;
    // 원본과 동일 유효 victim을 확보한다.
    for (std::uint16_t id=5;id<=127;++id) CHECK(pool.Allocate(2)==Sid{id});
    // 저장된 각각의 실제 정상 반환을 한 번씩 대조한다.
    for (const auto& row:DefenseRows()) {
        if ((row[0]=="originals")!=(edition==OriginalEdition::Patch1078)) continue;const Sid sid{static_cast<std::uint16_t>(Number(row[1]))};const auto slot=pool.Slot(sid);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()};
        // native가 보존한 padding 패턴을 그대로 입력한다.
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*11+sid.value);
        // 직접 기본 몸체는 raw를 읽지 않으며 current-vtable 분배는 별도 실제 factory 검사다.
        const auto before=Hex(raw);CHECK(static_cast<std::uint32_t>(RawSquidDefaultDefense(sid,Sid{static_cast<std::uint16_t>(Number(row[2]))}))==Number(row[3]));CHECK(Hex(raw)==before && Hex(raw)==row[4]);++count;
    }
    CHECK(count==24);
}
}
// 10.78의 실제 xor/RET4 반환을 대조한다.
TEST_CASE(SquidDefense_PatchNativeFalseBody) { ReplayDefense(OriginalEdition::Patch1078); }
// CD10.72의 독립 같은5바이트 몸체를 대조한다.
TEST_CASE(SquidDefense_CdNativeFalseBody) { ReplayDefense(OriginalEdition::Cd1072); }
// 실제 Factory가 만든 캠페인71/128/사제/anim 가상 표만 분배하고 다른 파생 callback을 보존한다.
TEST_CASE(SquidDefense_ActualFactoryTablesAndFallback) {
    // 현재 양판본 생성자 recipe와 원본 가상 표를 같은 실제 풀에서 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);const auto entries=DefenseEntries(edition);
        // 실제 생성자 주소는 보호 PE 타입 초기화에서 복원한 공통 표를 재사용한다.
        for (const auto type:{71U,128U,158U,160U}) types[type].constructorAddress=TypeConstructorAddress(edition,type);
        SquidFactory factory(pool,types);std::uint32_t fallbackCalls=0;DamageableHitHooks fallback;fallback.rejectDamage=[&](Sid,Sid) { ++fallbackCalls;return true; };auto hooks=MakeRawSquidDefenseHooks(pool,entries,std::move(fallback));Sid last;
        // 각 원본 타입은 현재 생성한 raw vtable에 따라 실제 기본 방어false를 소비한다.
        for (const auto type:{71U,128U,158U,160U}) { last=factory.Create(type,2);CHECK(!hooks.rejectDamage(last,Sid{65535}) && !fallbackCalls); }
        Put(pool.AllocatedBytes(last),0,0xdead1234);CHECK(hooks.rejectDamage(last,Sid{60}) && fallbackCalls==1);
    }
}
// 다른 파생·누락 표·범위 밖 표적을 기본false로 우회하지 않는다.
TEST_CASE(SquidDefense_MissingOtherDerivedRejected) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,false);CHECK(pool.Allocate(2)==Sid{5});const auto entries=DefenseEntries(pool.Edition());auto hooks=MakeRawSquidDefenseHooks(pool,entries,{});CHECK(Throws([&] { hooks.rejectDamage(Sid{5},Sid{60}); }));CHECK(Throws([&] { MakeRawSquidDefenseHooks(pool,{},{}); }));CHECK(Throws([&] { hooks.rejectDamage(Sid{},Sid{60}); }));
}
