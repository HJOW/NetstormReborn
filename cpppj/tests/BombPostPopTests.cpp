// 게임/시전 Frame 없이 원문 접두의 예약 인자와 같은 가상 분배/깊이를 비교한다.
#include "RawSceneSupport.h"
#include "o/RawBombPostPop.h"
#include "o/RawCarrierSteal.h"
#include "o/SquidPostPop.h"
#include <filesystem>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
TEST_CASE(BombPostPop_Native432PrefixesAndCommonDepth) { // 원문 두 정밀도가 같은432행을 실제 분배로 재생한다.
    SidPool pool(OriginalEdition::Patch1078,24000);std::vector<RiftTypeRecord> types(188);SquidPostPopState state;state.suppressed=true;SquidPostPop base(pool,types,state);GameRandom random; // 억제된 공통 장부는 깊이만 줄이며 다른 효과를 합성하지 않는다.
    const auto fixture=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-postpop-x86.tsv").string().c_str());std::size_t count=0; // Native행 누락을 확인한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==7);if (row.size()!=7) return;const auto kind=Number(row[0]),flags=Number(row[1]);const bool found=Number(row[2])!=0,allocated=Number(row[3])!=0;random.SetState(Number(row[4]));std::vector<std::string> events; // 원문과 같은 입력/콜백 호출열이다.
        BombPostPopHooks hooks;hooks.hasRegular=[&](Sid parent,std::uint32_t event) { events.push_back("find:"+std::to_string(parent.value)+":"+std::to_string(event));return found; };hooks.reserveRegular=[&] { events.push_back("alloc:40");return allocated; };hooks.addRegular=[&](Sid parent,std::uint32_t event,float delay) { events.push_back("regular:"+std::to_string(event)+":"+std::to_string(parent.value)+":"+std::to_string(std::bit_cast<std::uint32_t>(delay))); };
        RawBombPostPop post(pool,random,std::move(hooks));base.SetBombPostPop(&post);const auto slot=pool.Slot(kSource);auto raw=std::span(const_cast<std::uint8_t*>(slot.data()),slot.size());Put(raw,0,std::array{0x502c38U,0x502cf0U,0x502db0U}.at(kind));raw[10]=0;raw[11]=9;state.depth=1;const auto saved=std::vector<std::uint8_t>(raw.begin(),raw.end()); // 명시 상태는 사전 Validate가 거부하지만 실제 PATCH는 원문처럼 접두에 진입한다.
        CHECK(base.HandlesDerived(kSource));base.PostPopVirtual(kSource,flags);CHECK(state.depth==0 && std::equal(saved.begin(),saved.end(),raw.begin()));events.push_back("base:50:"+std::to_string(flags));std::string joined;for (const auto& event:events) { if (!joined.empty()) joined+='|';joined+=event; } // 호출 순서와 공통 깊이의 단일 감소를 함께 대조한다.
        CHECK(joined==row[5] && random.State()==Number(row[6]));base.SetBombPostPop(nullptr);++count;
    }
    CHECK(count==432);
}
TEST_CASE(BombPostPop_LazySelectedDevicesAndSavedVtable) { // 하위 실패 앞의 난수/할당 효과와 현재 VT 변경 경계를 확인한다.
    SidPool pool(OriginalEdition::Patch1078,24000);const auto slot=pool.Slot(kSource);auto raw=std::span(const_cast<std::uint8_t*>(slot.data()),slot.size());Put(raw,0,0x502c38U);GameRandom random(55);RawBombPostPop empty(pool,random,{});empty.Prefix(kSource,0);CHECK(random.State()==55 && Throws([&] { empty.Prefix(kSource,1); }));
    BombPostPopHooks hooks;hooks.hasRegular=[](Sid,std::uint32_t) { return false; };hooks.reserveRegular=[] { return false; };RawBombPostPop failed(pool,random,hooks);failed.Prefix(kSource,1);CHECK(random.State()==55); // 할당 실패에서 생성자/난수 장치는 요구하지 않는다.
    hooks.reserveRegular=[] { return true; };Put(raw,0,0x502cf0U);RawBombPostPop missingAdd(pool,random,hooks);CHECK(Throws([&] { missingAdd.Prefix(kSource,1); }) && random.State()!=55); // 뒤 장치 실패여도 원문 앞선 난수 소비는 남는다.
    std::uint32_t payload=0;hooks.hasRegular=[raw](Sid,std::uint32_t) { Put(raw,0,0x502db0U);return false; };hooks.addRegular=[&](Sid,std::uint32_t,float delay) { payload=std::bit_cast<std::uint32_t>(delay); };Put(raw,0,0x502c38U);random.SetState(55);RawBombPostPop selected(pool,random,std::move(hooks));selected.Prefix(kSource,1);CHECK(payload==0x3d4ccccdU && random.State()==55 && Get(raw,0)==0x502db0U); // 이미 선택한 그라비테이션 몸체는 조회 중 VT가 바뀌어도 난수를 소비하지 않는다.
}
TEST_CASE(BombPostPop_Reserved187UsesSameCarrierPostPopAddress) { // 포획/수신/주기 없이 두 VT의 같은+20 분배만 검사한다.
    SidPool pool(OriginalEdition::Patch1078,24000);SquidHash hash;std::vector<RiftTypeRecord> types(188);std::array<std::uint32_t,81> allies{};bool boss=true;std::uint32_t editor=0,owner=1,carrierType=156,priestType=158,counter=0;FormPacketHeader header;RawCarrierSteal steal(pool,hash,types,boss,editor,owner,allies,carrierType,priestType,header,counter,{}); // 다른 크랩 동작에는 진입하지 않는다.
    const auto slot=pool.Slot(kSource);auto raw=std::span(const_cast<std::uint8_t*>(slot.data()),slot.size());for (const auto table:{0x503f20U,0x503ff0U}) { Put(raw,0,table);CHECK(steal.Handles(kSource)); }Put(raw,0,0x503d80U);CHECK(!steal.Handles(kSource));
}
