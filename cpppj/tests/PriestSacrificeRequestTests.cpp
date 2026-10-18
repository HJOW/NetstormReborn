// 제품 전 동결한128입력으로 wrapper의 호출 순서/현재 owner 재독/반환을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPriestSacrificeRequest.h"
using namespace netstorm::test::rawscene;
// 큰 Dais/내용물/음악은 명시 입력 경계다. 실제 Session 연결은 별도 검사로 확인한다.
TEST_CASE(PriestSacrificeRequest_Native128WrapperOrder) {
    SidPool pool(OriginalEdition::Patch1078,24000,true); // 원본과 같은50바이트 물리 입력 풀이다.
    std::uint32_t kind{},local{};std::size_t count{}; // 원문 입력인 현재 종류/로컬 번호와 실제 대조 개수다.
    for (const auto& row:LoadFixture(NETSTORM_PRIEST_SACRIFICE_REQUEST_FIXTURE).rows) { // 기대값을 계산하지 않고 독립 원문을 순서대로 읽는다.
        CHECK(row.size()==6);if (row.size()!=6) continue;
        const auto values=Split(row[0],',');CHECK(values.size()==6);if (values.size()!=6) continue; // 현재 명시 입력6개다.
        const auto op=Number(values[0]),owner=Number(values[1]),allowed=Number(values[3]),mutation=Number(values[4]); // 원문 DWORD 비트를 보존한다.
        local=Number(values[2]);kind=Number(values[5]);const auto view=pool.Slot(Sid{50});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(view.data()),view.size()};raw[34]=static_cast<std::uint8_t>(owner);std::string trace; // callback이 바꾸는 현재 입력과 관찰 순서다.
        const auto event=[&trace](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; // 실제 요청의 전체 인자를 원문 형식으로 기록한다.
        PriestSacrificeRequestHooks hooks; // 각각의 하위 경계는 판단 결과를 주입하지 않는다.
        hooks.allowed=[&](Sid dais,Sid priest) { event("A:"+std::to_string(dais.value)+":"+std::to_string(priest.value));return allowed!=0; };
        hooks.remove=[&](Sid priest,std::uint32_t type,std::uint32_t index,std::uint32_t remove) { event("R:"+std::to_string(priest.value)+":"+std::to_string(type)+":"+std::to_string(index)+":"+std::to_string(remove));if (mutation==1) raw[34]=2;if (mutation==2) kind=77; };
        hooks.command=[&](Sid dais,Sid priest,std::uint32_t current) { event("C:"+std::to_string(dais.value)+":"+std::to_string(priest.value)+":"+std::to_string(current));if (mutation==3) raw[34]=2;if (mutation==4) local=2; };
        hooks.music=[&](std::string_view name) { event("M:"+std::string(name)); };
        RawPriestSacrificeRequest request(pool,kind,local,std::move(hooks));const bool result=op==0 ? request.Allowed(Sid{60},Sid{50}) : (request.Begin(Sid{50},Sid{60}),false); // 시작은 void이며 반환0은 원문 직렬화 표기다.
        CHECK(trace==row[1]);CHECK(std::to_string(result)==row[2]);CHECK(std::to_string(raw[34])==row[3]);CHECK(std::to_string(local)==row[4]);CHECK(std::to_string(kind)==row[5]);++count;
    }
    CHECK(count==128);
}
