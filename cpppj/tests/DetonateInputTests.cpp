// 실제 일반 입력768조건을 원문과 대조하고0반환/누락 하위/뗌을 구별한다.
#include "RawSceneSupport.h"
#include "client/DetonateInput.h"
#include "o/RawObjectConversion.h"
#include "o/RawObjectCommand.h"

using namespace netstorm::test::rawscene;
using namespace netstorm::client;
// 원문이 호출한16VT의 실제 Spell→Object→Form은 상태/요청을 바꾸지 않고0을 반환한다.
TEST_CASE(DetonateInput_ReplaysNative768FullInputAndActualParents) {
    SidPool pool(OriginalEdition::Patch1078,24000,true);ObjectCommandState state;RawObjectCommand base(pool,state,{});std::uint32_t buriedType=112;std::size_t count{}; // 같은 물리 슬롯과 실제 부모 몸체다.
    for (const auto& row:LoadFixture(NETSTORM_DETONATE_INPUT_FIXTURE).rows) { // 기대값은 독립 원문 네 필드만 읽는다.
        CHECK(row.size()==4);const auto fields=Split(row[0],',');CHECK(fields.size()==4);const auto table=Number(fields[0]),code=Number(fields[1]),parameter=Number(fields[2]);
        auto raw=pool.Slot(Sid{50});std::span<std::uint8_t> input{const_cast<std::uint8_t*>(raw.data()),raw.size()};std::fill(input.begin(),input.end(),std::uint8_t{});Put(input,0,table);input[10]=114;input[34]=1;Put(input,14,std::bit_cast<std::uint32_t>(112.F));Put(input,18,std::bit_cast<std::uint32_t>(118.F));
        ObjectConversionHooks hooks;hooks.base=[&](Sid sid,auto command) { return base.Command(sid,command); };RawObjectConversion object(pool,buriedType,std::move(hooks));std::string trace;
        CHECK(DispatchDetonateInput({code,std::bit_cast<std::int32_t>(parameter),std::bit_cast<std::int32_t>(parameter)},[&](Sid sid,auto command) {
            trace="object:"+std::to_string(sid.value)+':'+std::to_string(command);const auto result=object.Command(sid,command,ObjectConversionBody(table));trace+=";return:"+std::to_string(result);
        }));
        CHECK((trace.empty() ? "-" : trace)==row[1]);CHECK(Hex(pool.Slot(Sid{50}))==row[2]);++count;
    }
    CHECK(count==768);
}
// 호출되지 않는 하위는 요구하지 않으며 알 수 없는 명령은 원래 부모에게 돌려준다.
TEST_CASE(DetonateInput_UnknownReleaseZeroAndMissingReachedObject) {
    CHECK(!DispatchDetonateInput({0x02000017,0,50},{}));CHECK(DispatchDetonateInput({kDetonateBombCommand|InputCode::kRelease,0,50},{}));
    CHECK(DispatchDetonateInput({kDetonateBombCommand,0,0},{}));CHECK(DispatchDetonateInput({kDetonateBombCommand,0,65536},{}));
    CHECK(Throws([&] { static_cast<void>(DispatchDetonateInput({kDetonateBombCommand,0,50},{})); }));
    std::uint32_t called{};CHECK(DispatchDetonateInput({kDetonateBombCommand,999,65586},[&](Sid sid,auto code) { CHECK(sid.value==50 && code==kDetonateBombCommand);++called; }));CHECK(called==1);
}
