// 실제 사제 +38/공통 Halt의 반환·전체 raw·명시 공간/낙하/운반 호출을 독립 PE에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPriestHalt.h"
#include <cmath>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kHaltNativeVtable=0x11018000; // 원본 실행기의 합성 가상 효과 표 기록이며 호스트 코드 포인터가 아니다.
// 원본 사건 기록의 순서와 구분자를 유지한다.
std::string HaltEffects(const std::vector<std::string>& events) {
    std::string value;
    // 입력 경계에서 실제 호출된 사건만 이어 붙인다.
    for (const auto& event:events) { if (!value.empty()) value+=';';value+=event; }
    return value.empty() ? "-" : value;
}
}

// 같은 풀의 기존 실제 HP/지면/일반 finder를 호출하고 상위 기대값은 Native만 사용한다.
TEST_CASE(rawPriestHalt_native_ground_half_hp_common_finder_and_return_codes) {
    const auto fixture=LoadFixture(NETSTORM_PRIESTHALT_FIXTURE);std::size_t checked=0;
    // 각 원본 입력은 독립 상태와 실제 raw client 슬롯을 사용한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==5);std::map<std::string,std::string> input;
        // 결과를 재계산하지 않고 기록된 JSON 입력의 정수/문자열만 읽는다.
        for (const auto& field:Split(std::string_view(row[1]).substr(1,row[1].size()-2),',')) { const auto colon=field.find(':');input.emplace(field.substr(1,colon-2),field.substr(colon+1)); }
        const auto number=[&](const char* key) { return Number(input.at(key)); };const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,24000,true);SquidHash hash;std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<std::uint8_t> spots(65536);std::vector<std::string> events;
        // 기록된 SID50/60까지 실제 client free list에서 동일 번호를 확보한다.
        for (std::uint16_t sid=5;sid<=60;++sid) { CHECK(pool.Allocate(2)==Sid{sid});auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        // raw 필드/발자국/버킷만 준비하고 검색 및 Halt 분기 결과는 계산하지 않는다.
        const auto insert=[&](Sid sid,std::uint32_t type,std::uint32_t xb,std::uint32_t yb) { auto raw=pool.AllocatedBytes(sid);Put(raw,0,kHaltNativeVtable);raw[10]=static_cast<std::uint8_t>(type);Put(raw,14,xb);Put(raw,18,yb);types[type].footX=types[type].footY=1;auto& head=hash.Bucket(1,std::bit_cast<float>(xb),std::bit_cast<float>(yb));Put(raw,4,head,2);head=sid.value; };
        insert(Sid{50},158,number("x"),number("y"));const auto neighbor=number("neighbor");if (neighbor) insert(Sid{60},neighbor,std::bit_cast<std::uint32_t>(20.2F),std::bit_cast<std::uint32_t>(21.3F));Put(pool.AllocatedBytes(Sid{50}),26,number("hp"),patch ? 4 : 2);types[158].maxHitPoints=200;types[158].flags2=number("genus");
        const auto x=std::bit_cast<float>(number("x")),y=std::bit_cast<float>(number("y"));spots[static_cast<std::size_t>(static_cast<int>(static_cast<double>(y)+0.9999F)*256+static_cast<int>(static_cast<double>(x)+0.9999F))]=static_cast<std::uint8_t>(number("ground"));GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidPostPopState book;SquidRewardState rewardState;SquidReward reward(pool,types,book,rewardState,patch ? &store : nullptr);RawPriestState priest(pool,reward,types,spots);
        RawCarrierCheck check(pool,types,spots,[&](Sid sid) { CHECK(sid==Sid{50});events.push_back("F:50:"+std::to_string(number("fall")));return number("fall")!=0; });PriestHaltState state;state.battle=number("battle")!=0;state.boss=number("boss")!=0;PriestHaltHooks hooks;
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { CHECK(sid==Sid{50} && flags==0);events.push_back("U:50:0");if (number("change")) { auto raw=pool.AllocatedBytes(sid);Put(raw,14,std::bit_cast<std::uint32_t>(30.75F));Put(raw,18,std::bit_cast<std::uint32_t>(31.9F)); } };
        hooks.pop=[&](Sid sid,float px,float py,std::uint32_t flags) { CHECK(sid==Sid{50} && flags==0);events.push_back("P:50:"+std::to_string(std::bit_cast<std::uint32_t>(px))+":"+std::to_string(std::bit_cast<std::uint32_t>(py))+":0");auto raw=pool.AllocatedBytes(sid);Put(raw,14,std::bit_cast<std::uint32_t>(px));Put(raw,18,std::bit_cast<std::uint32_t>(py)); };
        hooks.carrierPickup=[&](Sid sid) { CHECK(sid==Sid{60});events.push_back("C:60:"+std::to_string(number("reply")));return number("reply"); };
        hooks.nuggetTransfer=[&](Sid sid) { CHECK(patch && sid==Sid{60});events.push_back("N:60:"+std::to_string(number("reply")));return number("reply"); };RawPriestHalt halt(pool,hash,types,spots,check,priest,reward,state,std::move(hooks));
        const auto result=input.at("kind")=="\"Halt\"" ? halt.Halt(Sid{50}) : halt.CommonHalt(Sid{50});CHECK(result==Number(row[2]));CHECK(HaltEffects(events)==row[3]);CHECK(Hex(pool.Slot(Sid{50}))==row[4]);++checked;
    }
    CHECK(checked==864);
}
