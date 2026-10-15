// 실제 SP 저장소의 안정 수명 조회와 기존61개 원본 관찰을 Player 통지에 연결해 검사한다.
#include "ConstructionSupport.h"
#include "o/RawSpStoreNoticeBindings.h"
#include <bit>
#include <type_traits>

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
// 아직 생성하지 않은 값/빈 조회기를 정상 현재 전역처럼 공급하지 못하는지 관찰한다.
template<class Function> bool NoticeThrows(Function&& function) {
    try { function();return false; } catch (const std::exception&) { return true; }
}
}

// 원본 Initialize는 실제/추적 offset을 함께 쓰며 Rotate는 실제 offset만 전진시킨다.
TEST_CASE(SpStoreNotice_ReadsStableOriginalFields) {
    std::int32_t tracking=-123;GameRandom random;ScrambledSpStore store(random,[] { return 100U; },0x80000001U);
    store.BindTrackingOffset(tracking);const auto reads=BindSpStoreNotice(store);
    const auto* offsetAddress=&store.OffsetCurrent();const auto* poolAddress=&store.FirstPoolCurrent();
    const auto* permutationAddress=&store.Permutation();CHECK(&store.TrackingOffsetCurrent()==&tracking);
    CHECK(!store.FirstPoolCurrent());CHECK(reads.firstSpPool.Read()==0);CHECK(reads.cursorB.Read()==std::bit_cast<std::uint32_t>(tracking));
    store.Initialize();CHECK(store.OffsetCurrent()==0 && tracking==0);CHECK(reads.cursorA.Read()==0 && reads.cursorB.Read()==0);
    CHECK(store.FirstPoolCurrent()==store.Pools().data());CHECK(reads.firstSpPool.Read()==1);
    // 항등 순열도 실제 배열로 유지하며 각 SP 동작은 이 같은 배열을 읽는다.
    for (std::size_t index=0;index<store.Permutation().size();++index) CHECK(store.Permutation()[index]==static_cast<std::int16_t>(index));
    store.Set(1,0x80000001U);store.Rotate();CHECK(reads.cursorA.Read()==3 && reads.cursorB.Read()==0);CHECK(store.Get(1)==0x80000001U);
    tracking=3;CHECK(reads.cursorB.Read()==3);store.Initialize();CHECK(reads.cursorA.Read()==0 && reads.cursorB.Read()==0);
    CHECK(&store.OffsetCurrent()==offsetAddress && &store.FirstPoolCurrent()==poolAddress && &store.Permutation()==permutationAddress);
    CHECK(store.FirstPoolCurrent()==store.Pools().data());CHECK(reads.firstPermutation.Read()==store.Permutation().front());
}

// 기존61개 실제 원본 출력의 actual/tracking 값을 사본 없이 각 현재 조회에서 재확인한다.
TEST_CASE(SpStoreNotice_ReplaysExistingNativeOffsets) {
    std::ifstream input(NETSTORM_SPROTATE_FIXTURE);if (!input) throw std::runtime_error("SP 현재 상태 원본 fixture 없음");
    std::string line;std::uint32_t count=0;
    // 입력과 명령만 C++에 공급하고 두 offset 기대값은 원본 fixture에서 읽는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=4) throw std::runtime_error("SP 현재 상태 원본 fixture 열 수 오류");++count;
        const auto seed=Number(row[0]),fill=Number(row[1]);std::int32_t tracking=-999;GameRandom random;
        ScrambledSpStore store(random,[seed] { return seed; },fill);store.BindTrackingOffset(tracking);const auto reads=BindSpStoreNotice(store);
        const auto actions=Split(row[2],';'),snapshots=Split(row[3],'|');CHECK(actions.size()==snapshots.size());
        // 재초기화/저장/회전의 매 원본 관찰 끝점에서 같은 주소의 현재 값을 읽는다.
        for (std::size_t index=0;index<actions.size();++index) {
            const auto action=Split(actions[index],':');
            if (action[0]=="I") store.Initialize();else if (action[0]=="S") store.Set(Number(action[1]),Number(action[2]));
            else {
                // 9600 포함/초과 경계도 원본처럼 각 회전을 실제 호출한다.
                for (std::uint32_t step=0;step<Number(action[1]);++step) store.Rotate();
            }
            const auto halves=Split(snapshots[index],':'),values=Split(halves[0],',');CHECK(values.size()==12);
            CHECK(reads.cursorA.Read()==Number(values[0]));CHECK(reads.cursorB.Read()==Number(values[1]));
            CHECK(&store.TrackingOffsetCurrent()==&tracking && store.FirstPoolCurrent()==store.Pools().data());
            CHECK(reads.firstSpPool.Read()==1 && reads.firstPermutation.Read()==0);
        }
    }
    CHECK(count==61);
}

// 실제 Player 접두가 초기화/현재 SP/현재 추적 불일치를 같은 저장소에서 읽는지 확인한다.
TEST_CASE(SpStoreNotice_PlayerReadsCurrentStorage) {
    SidPool pool(OriginalEdition::Patch1078,32768);auto raw=InputRaw(pool,Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});
    GameRandom random;std::int32_t tracking=0;ScrambledSpStore store(random,[] { return 100U; });store.BindTrackingOffset(tracking);
    const auto reads=BindSpStoreNotice(store);std::uint32_t problem=99,fallback=7,checks=0,local=1,gumpType=3,event=42,moneyCalls=0;bool battle=false;
    PlayerContentNoticeHooks hooks;
    hooks.money=[&](std::uint32_t owner) { ++moneyCalls;return std::bit_cast<float>(store.Get(owner)); };
    hooks.gump=[](std::uint32_t)->std::uint32_t { throw std::logic_error("빈 부모는 Gump를 조회하지 않는다"); };
    hooks.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("빈 부모는 가상 Gump를 처리하지 않는다"); };
    const auto battleRead=PlayerContentNoticeRead<std::uint32_t>::Bind([&] { return battle ? 1U : 0U; });
    RawPlayerContentNotice notice(pool,{reads.cursorA,reads.cursorB,problem,fallback,checks,battleRead,local,
        reads.firstSpPool,reads.firstPermutation,gumpType,event},std::move(hooks));
    notice.Notify(Sid{50},6,1);CHECK(problem==7 && !store.Initialized() && moneyCalls==0);
    // 같은 통지의 실제 SP조회가 lazy 초기화한 뒤 첫 배열/순열도 즉시 현재 상태를 읽어야 한다.
    checks=1;battle=true;problem=99;notice.Notify(Sid{50},6,1);CHECK(problem==99 && store.Initialized() && moneyCalls==1 && checks==0);
    store.Set(1,std::bit_cast<std::uint32_t>(7000.5F));checks=1;notice.Notify(Sid{50},6,1);CHECK(problem==7 && checks==0 && moneyCalls==2);
    problem=99;fallback=8;store.Rotate();notice.Notify(Sid{50},6,2);CHECK(problem==8);
    tracking=3;problem=99;notice.Notify(Sid{50},6,2);CHECK(problem==99 && store.Get(1)==std::bit_cast<std::uint32_t>(7000.5F));
    battle=false;checks=1;notice.Notify(Sid{50},6,2);CHECK(problem==99 && checks==0 && moneyCalls==2);
}

// 외부 추적 상태가 없는 저장소/늦은 연결/빈 현재 조회기를 허용하지 않는다.
TEST_CASE(SpStoreNotice_RejectsUnownedAndLateTracking) {
    static_assert(!std::is_constructible_v<PlayerContentNoticeRead<std::uint32_t>,std::uint32_t&&>);
    static_assert(!std::is_constructible_v<PlayerContentNoticeRead<std::uint32_t>,const std::uint32_t&&>);
    GameRandom random;ScrambledSpStore missing(random,[] { return 1U; });std::int32_t tracking=0,other=0;
    CHECK(NoticeThrows([&] { static_cast<void>(BindSpStoreNotice(missing)); }));
    missing.Initialize();CHECK(NoticeThrows([&] { missing.BindTrackingOffset(tracking); }));
    ScrambledSpStore store(random,[] { return 1U; });store.BindTrackingOffset(tracking);
    CHECK(NoticeThrows([&] { store.BindTrackingOffset(other); }));
    CHECK(NoticeThrows([] { static_cast<void>(PlayerContentNoticeRead<std::uint32_t>::Bind({})); }));
}
