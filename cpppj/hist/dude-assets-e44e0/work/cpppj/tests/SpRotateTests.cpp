// 실제 10.78 PE의 SP 회전 결과와 전체 DWORD/논리 SP/난수/bit 위치를 대조한다.
#include "TestSupport.h"
#include "o/SpStore.h"
#include "o/RawPriestRegen.h"
#include "o/SquidFactory.h"
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 구분자로 분리하며 첫/마지막 빈 항목도 보존한다.
std::vector<std::string> Split(std::string_view value,char separator) {
    std::vector<std::string> result;std::size_t start=0;
    // 원본 fixture의 모든 항목을 저장 순서 그대로 읽는다.
    for (;;) {
        const auto end=value.find(separator,start);
        result.emplace_back(value.substr(start,end==std::string_view::npos ? end : end-start));
        if (end==std::string_view::npos) return result;
        start=end+1;
    }
}
// 십진수 DWORD 관찰을 호스트 signed 값으로 바꾸지 않고 읽는다.
std::uint32_t Number(const std::string& value) { return static_cast<std::uint32_t>(std::stoull(value)); }
// little endian hex의 바이트 하나를 읽는다. 잘못된 입력은 예외로 거부한다.
std::uint8_t Byte(std::string_view value) { return static_cast<std::uint8_t>(std::stoul(std::string(value),nullptr,16)); }
// 한 명령 뒤의 전체 288 DWORD/논리 값/offset/난수 관찰을 비교한다.
void Observe(ScrambledSpStore& store,const GameRandom& random,std::string_view snapshot) {
    const auto halves=Split(snapshot,':');CHECK(halves.size()==2);
    const auto numbers=Split(halves.at(0),',');CHECK(numbers.size()==12);
    CHECK(store.Offset()==Number(numbers.at(0)));CHECK(random.State()==Number(numbers.at(2)));
    // 네이티브 init_copy는 Priest의 별도 expected offset이다. 이 저장소의 offset과 구별해서 자료만 보존한다.
    CHECK(store.Initialized());
    // 읽기 몸체도 회전된 물리 자료를 다시 조합하여 실제 SP를 유지해야 한다.
    for (std::uint32_t owner=0;owner<ScrambledSpStore::kOwners;++owner) CHECK(store.Get(owner)==Number(numbers.at(owner+3)));
    const auto& hex=halves.at(1);CHECK(hex.size()==ScrambledSpStore::kBits*ScrambledSpStore::kOwners*8);
    // 전체 raw DWORD는 native의 물리 배열/소유자 순서와 같다. 잡음도 생략하지 않는다.
    for (std::size_t i=0;i<store.Pools().size();++i) {
        std::uint32_t expected=0;
        // x86 little endian의 네 바이트를 합친다.
        for (std::size_t byte=0;byte<4;++byte) expected|=static_cast<std::uint32_t>(Byte(std::string_view(hex).substr(i*8+byte*2,2)))<<(byte*8);
        CHECK(store.Pools()[i]==expected);
    }
}
// 한 fixture의 초기 시드·힙 채움·각 실제 명령을 실행한다.
void Replay(const std::vector<std::string>& row) {
    CHECK(row.size()==4);const auto seed=Number(row.at(0)),fill=Number(row.at(1));
    GameRandom random;ScrambledSpStore store(random,[seed] { return seed; },fill);
    const auto actions=Split(row.at(2),';'),snapshots=Split(row.at(3),'|');CHECK(actions.size()==snapshots.size());
    // native와 같은 명령 순서로 초기화/저장/반복 회전을 실행하고 각 끝점에서 대조한다.
    for (std::size_t index=0;index<actions.size();++index) {
        const auto parts=Split(actions[index],':');
        if (parts[0]=="I") store.Initialize();
        else if (parts[0]=="S") store.Set(Number(parts.at(1)),Number(parts.at(2)));
        else {
            // 경계 입력은 수천 회의 실제 호출로 9600 포함/초과를 도달한다.
            for (std::uint32_t step=0;step<Number(parts.at(1));++step) store.Rotate();
        }
        Observe(store,random,snapshots.at(index));
    }
}
}

// 원본 실행 없이 고유 fixture의 모든 정상/반복/재초기화/9600 경계 입력을 대조한다.
TEST_CASE(SpRotate_ReplaysActualPatch) {
    std::ifstream input(NETSTORM_SPROTATE_FIXTURE);if (!input) throw std::runtime_error("SP 회전 fixture 없음");
    std::string line;std::size_t count=0;
    // 주석/빈 줄은 독립 관찰로 세지 않는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty() || line.front()=='#') continue;
        Replay(Split(line,'\t'));++count;
    }
    CHECK(count==61);
}

// 회전은 값을 유지하면서 원본의 signed 상한/포함 경계를 사용하고 추가 난수를 소비하지 않는다.
TEST_CASE(SpRotate_PreservesAllOwnersAndWrapBoundary) {
    // 시드100은 최초 offset0을 만든다. 값은 SP float만으로 제한하지 않고 전체 DWORD를 보존한다.
    GameRandom random;ScrambledSpStore store(random,[] { return 100U; },0x80000001U);store.Initialize();CHECK(store.Offset()==0);
    // 각 소유자에 부호 비트/낮은 비트를 포함한 다른 값을 쓴다.
    for (std::uint32_t owner=0;owner<9;++owner) store.Set(owner,0x80000001U+owner*0x01010101U);
    const auto rng=random.State();
    // 9600과 같을 때에는 감기지 않는다.
    for (std::uint32_t step=0;step<3200;++step) store.Rotate();
    CHECK(store.Offset()==9600);CHECK(random.State()==rng);store.Rotate();CHECK(store.Offset()==3);
    // 실제 Get 비트가 전체 회전/offset 감김 뒤에도 일치한다.
    for (std::uint32_t owner=0;owner<9;++owner) CHECK(store.Get(owner)==0x80000001U+owner*0x01010101U);
    CHECK(random.State()==rng);
}

// 미연결 재시드는 lazy 회전도 첫 저장소 변화 전에 거부한다.
TEST_CASE(SpRotate_LazyInitializationRequiresReseed) {
    GameRandom random(7);ScrambledSpStore store(random,{},0xffffffffU);bool failed=false;
    try { store.Rotate(); } catch (const std::logic_error&) { failed=true; }
    CHECK(failed);CHECK(!store.Initialized());CHECK(store.Offset()==0);CHECK(random.State()==7);
}

// fullHP 사제도 실제 회복 이벤트에서 SP 회전을 실행하며 별도 관찰 offset과 같은 자료를 읽어야 한다.
TEST_CASE(SpRotate_FullHpPriestUsesTheCurrentStoreOffset) {
    // 테스트의 실제 사제 타입/풀/좌표/HP는 기존 생성자를 통해 준비한다.
    constexpr std::uint32_t priestType=158;SidPool pool(OriginalEdition::Patch1078,32768,true);
    std::vector<RiftTypeRecord> types(188);auto& type=types[priestType];type.constructorAddress=TypeConstructorAddress(pool.Edition(),priestType);
    type.flags1=0x69012;type.flags2=0x210000;type.maxHitPoints=100;
    SquidFactory factory(pool,types);const auto sid=factory.Create(priestType);auto raw=pool.AllocatedBytes(sid);raw[11]=0;raw[34]=1;
    // 정렬되지 않은 HP DWORD의 네 바이트만 쓰고 생성자 필드/주변 바이트를 보존한다.
    for (std::size_t byte=0;byte<4;++byte) raw[26+byte]=static_cast<std::uint8_t>(100U>>(byte*8));
    GameRandom random;ScrambledSpStore store(random,[] { return 100U; });store.Initialize();store.Set(1,0x44960000U);
    SquidPostPopState books;SquidRewardState rewardState;SquidReward hp(pool,types,books,rewardState,&store);
    std::vector<std::uint8_t> spots(65536,6);RawPriestState priest(pool,hp,types,spots);PriestHitPointMode mode;PriestRegenState state;
    state.patch.sequence=std::bit_cast<std::int32_t>(store.Offset());state.patch.observedSequence=state.patch.sequence;
    state.patch.gate=1;state.patch.changedSample=99;std::uint32_t audits=0,unexpected=0;
    RawPriestRegen regen(pool,hp,priest,mode,{[&](Sid,std::uint32_t) { ++unexpected; },[&](Sid,std::uint32_t) { ++unexpected; }},random,state,
        {[&](Sid) { ++unexpected; },[&](Sid) { ++unexpected; },[](std::uint32_t) { return 0U; },
         [](Sid,std::uint32_t) { return false; },[&](Sid) { ++unexpected; },
         [&] { store.Rotate();state.patch.observedSequence=std::bit_cast<std::int32_t>(store.Offset());++audits; },[] { return 0.0; }});
    // 모든 이벤트는 이미 가득 찬 HP를 유지하면서 현재 offset/expected 추적값을 같이 전진시킨다.
    for (std::uint32_t count=0;count<33;++count) {
        CHECK(regen.Handle(sid,kPriestRegenEvent,count,8.25F)==8.25F);CHECK(priest.CurrentHitPoints(sid)==100);
        CHECK(state.patch.observedSequence==state.patch.sequence);CHECK(store.Get(1)==0x44960000U);
    }
    CHECK(audits==33 && unexpected==0);CHECK(store.Offset()==99);CHECK(state.patch.changedSample==99);
}
