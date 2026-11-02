// 원문447340의96입력과 실패 경계 뒤 실제 선행 쓰기를 검사한다.
#include "RawSceneSupport.h"
#include "client/CursorObjectPickup.h"

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

TEST_CASE(CursorObjectPickup_ReplaysNative96LateTypeWordAndFlags) { //기대값은 독립 CPU fixture에서만 읽는다.
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_CURSOR_OBJECT_PICKUP_FIXTURE).rows) { //원문 TYPE/WORD/flags/Unpop 변경/생성0의 모든 행을 재생한다.
        CHECK(row.size()==7);SidPool pool(OriginalEdition::Patch1078,kCapacity);Sid sid{};
        while (sid.value<50) sid=pool.Allocate(2); //원문과 같은 물리SID50을 준비한다.
        auto raw=pool.AllocatedBytes(sid);raw[10]=static_cast<std::uint8_t>(Number(row[1]));Put(raw,12,Number(row[2]),2);std::string trace;
        const auto append=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; //효과의 순서/인자 폭만 기록한다.
        CursorObjectPickupHooks hooks;hooks.unpop=[&](Sid current,auto flags) { append("U,"+std::to_string(current.value)+','+std::to_string(flags));if (Number(row[3])) { auto changed=pool.AllocatedBytes(current);changed[10]=120;Put(changed,12,0x1234,2); } };
        hooks.create=[&](Sid current,auto type,auto argument,auto flags,auto attach,auto quality) { append("F,"+std::to_string(current.value)+','+std::to_string(type)+','+std::to_string(argument)+','+std::to_string(flags)+','+std::to_string(attach)+','+std::to_string(quality));return Sid{static_cast<std::uint16_t>(Number(row[4]))}; };
        hooks.take=[&](Sid form,auto flags) { append("T,"+std::to_string(form.value)+','+std::to_string(flags)); };
        PickupObjectForCursor(pool,50,Number(row[0]),hooks);CHECK(trace==row[5]);CHECK(std::to_string(pool.Slot(sid)[10])+','+std::to_string(Get(pool.Slot(sid),12,2))==row[6]);++count;
    }
    CHECK(count==96);
}
TEST_CASE(CursorObjectPickup_PreservesUnpopAndFormOnReachedFailure) { //누락한 하위에서 앞선 효과를 되돌리거나 무동작 성공을 만들지 않는다.
    SidPool pool(OriginalEdition::Patch1078,kCapacity);const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);raw[10]=82;Put(raw,12,7,2);std::uint32_t unpopped{},created{};
    CursorObjectPickupHooks hooks;hooks.unpop=[&](Sid,auto) { ++unpopped; };CHECK(Throws([&] { PickupObjectForCursor(pool,sid.value,0,hooks); }));CHECK(unpopped==1 && created==0);
    hooks.create=[&](Sid,auto,auto,auto,auto,auto) { ++created;return Sid{}; };CHECK(Throws([&] { PickupObjectForCursor(pool,sid.value,0,hooks); }));CHECK(unpopped==2 && created==1);
    CHECK(Throws([&] { PickupObjectForCursor(pool,0xffffffffU,0,hooks); }));CHECK(unpopped==2 && created==1);
}
