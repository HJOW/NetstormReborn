// 전용 플라이어 Pre의 WORD/현재 callback/flags/네 슬롯 전체를 실제 원문과 비교한다.
#include "RawSceneSupport.h"
#include "o/RawFlyerPreDestroy.h"

using namespace netstorm::test::rawscene;
namespace {
// 원문이 읽는 부모/두 대상/높은 WORD 슬롯이다.
constexpr std::array<std::uint16_t,4> kIds{50,60,61,32767};
// 독립 원문 hex만 입력 바이트로 복원한다.
std::vector<std::uint8_t> Decode(std::string_view text) {
    std::vector<std::uint8_t> bytes;
    // 두16진 문자마다 바이트 하나만 추가한다.
    for (std::size_t index=0;index<text.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return bytes;
}
}
// 대상 주소를 DWORD로 읽지 않고 현재 WORD만 가상 삭제한 뒤 같은 flags로 하위를 호출한다.
TEST_CASE(FlyerPreDestroy_NativeWordAndReentry) {
    SidPool pool(OriginalEdition::Patch1078,32768,false);std::size_t count=0;
    // 다음 독립 입력은 이전 행의 dead/free도 재설치하므로 검사 전용 raw 접근을 사용한다.
    const auto raw=[&](std::uint16_t sid) { const auto view=pool.Slot(Sid{sid});return std::span<std::uint8_t>(const_cast<std::uint8_t*>(view.data()),view.size()); };
    // 모든 원문 입력의 현재 상태/요청 순서를 비교한다.
    for (const auto& row:LoadFixture(NETSTORM_FLYER_PREDESTROY_FIXTURE).rows) {
        if (row.size()!=5) throw std::runtime_error("플라이어 Pre fixture 열 수 오류");const auto child=static_cast<std::uint16_t>(Number(row[0]));const auto flags=Number(row[1]),mutation=Number(row[2]);
        // 실제 TYPE/상태가+8 WORD 위에 있으므로 넓게 읽으면 다른 대상이 된다.
        for (const auto sid:kIds) {
            auto bytes=raw(sid);std::fill(bytes.begin(),bytes.end(),std::uint8_t{});Put(bytes,0,0x12400000);bytes[10]=144;bytes[11]=2;Put(bytes,14,std::bit_cast<std::uint32_t>(2.125F));Put(bytes,18,std::bit_cast<std::uint32_t>(3.25F));if (sid==50) Put(bytes,8,child,2);
        }
        std::string events;const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // callback 요청의 순서다.
        RawFlyerPreDestroy logic(pool,{
            [&](Sid sid,std::uint32_t value) {
                event("D:"+std::to_string(sid.value)+":"+std::to_string(value));CHECK(sid.value==child && value==0);
                if (mutation==1) { Put(raw(50),8,61,2);raw(50)[40]=9; }
                if (mutation==2) { Put(raw(50),8,0,2);raw(sid.value)[11]=1; }
            },
            [&](Sid sid,std::uint32_t value) { event("P:"+std::to_string(sid.value)+":"+std::to_string(value));if (mutation==3) Put(raw(50),14,std::bit_cast<std::uint32_t>(9.75F)); }
        });logic.PreDestroy(Sid{50},flags);CHECK(events==row[3]);std::vector<std::uint8_t> slots;
        // 네 슬롯200BYTE의 미변경 필드도 포함한다.
        for (const auto sid:kIds) { const auto bytes=pool.Slot(Sid{sid});slots.insert(slots.end(),bytes.begin(),bytes.end()); }
        CHECK(slots==Decode(row[4]));++count;
    }
    CHECK(count==96);
}
