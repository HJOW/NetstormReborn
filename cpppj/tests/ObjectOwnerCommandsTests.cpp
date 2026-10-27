// 실제 SetOwner 원문256입력과 선택된 외부 경계의 누락/해제 정책을 검사한다.
#include "RawSceneSupport.h"
#include "client/ObjectOwnerCommands.h"
#include <bit>

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // Native 효과 문자열만 조합하는 검사 장치다.
void AppendOwnerTrace(std::string& trace,std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 효과 순서와 인자 폭을 보존한다.
}
// 가상74 뒤 모드/선택이 바뀌어도 원문의 보관 SID와 현재 owner 조회가 일치해야 한다.
TEST_CASE(ObjectOwnerCommand_ReplaysNative256LateModeAndHeldSid) {
    std::size_t count{}; // 실제 소비한 독립 fixture 행 수다.
    for (const auto& row:LoadFixture(NETSTORM_OBJECT_OWNER_COMMAND_FIXTURE).rows) { // 기대 상태/효과는 원문 출력에서만 읽는다.
        CHECK(row.size()==8);SidPool pool(OriginalEdition::Patch1078,kCapacity);Sid slot{}; // 실제 풀의 고정 예측 구간도 범위 안에 둔다.
        while (slot.value<50) slot=pool.Allocate(2); // 원문 this와 같은 클라이언트 물리 SID50을 준비한다.
        auto raw=pool.AllocatedBytes(slot);raw[34]=2;
        const auto before=std::vector<std::uint8_t>(pool.Bytes().begin(),pool.Bytes().end());
        auto editor=Number(row[0]),mission=Number(row[1]),selected=Number(row[2]),revision=Number(row[5]);const auto mutation=Number(row[4]);std::string trace;
        ObjectOwnerCommandHooks hooks;hooks.editor=[&] { return editor; };hooks.missionPost=[&] { return mission; };hooks.selected=[&] { return selected; };
        hooks.owner=[&](auto sid,auto owner) {
            AppendOwnerTrace(trace,"O,"+std::to_string(sid)+','+std::to_string(owner));pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(sid)})[34]=static_cast<std::uint8_t>(owner);
            if (mutation&1) editor=1-editor;
            if (mutation&2) { selected=51;pool.AllocatedBytes(slot)[34]=6; }
        };
        hooks.follow=[&](auto sid) { FollowEditorObjectOwner(pool,sid,revision,[&](auto owner,auto templateOwner) { AppendOwnerTrace(trace,"C,"+std::to_string(owner)+','+std::to_string(templateOwner)); }); };
        CHECK(DispatchObjectOwner({kSetObjectOwnerCommand,0,std::bit_cast<std::int32_t>(Number(row[3]))},revision,hooks));
        CHECK((trace.empty() ? "-" : trace)==row[6]);
        CHECK(std::to_string(revision)+','+std::to_string(editor)+','+std::to_string(selected)+','+std::to_string(pool.Slot(slot)[34])==row[7]);
        auto after=std::vector<std::uint8_t>(pool.Bytes().begin(),pool.Bytes().end());after[50*50+34]=before[50*50+34];CHECK(after==before);++count;
    }
    CHECK(count==256);
}
// 미선택 경계는 요구하지 않고 선택된 누락 뒤에는 앞선 효과를 되돌리지 않는다.
TEST_CASE(ObjectOwnerCommand_ReleaseUnknownAndReachedMissingEffects) {
    std::uint32_t revision=7;ObjectOwnerCommandHooks hooks; // 해제/다른 명령에는 하위를 전혀 요구하지 않는다.
    CHECK(DispatchObjectOwner({kSetObjectOwnerCommand|InputCode::kRelease,0,1},revision,hooks));CHECK(revision==7);
    CHECK(!DispatchObjectOwner({0x02000095,0,1},revision,hooks));
    hooks.editor=[] { return 0U; };hooks.missionPost=[] { return 0U; };CHECK(DispatchObjectOwner({kSetObjectOwnerCommand,0,1},revision,hooks));CHECK(revision==7);
    hooks.editor=[] { return 1U; };hooks.selected=[] { return 0U; };CHECK(DispatchObjectOwner({kSetObjectOwnerCommand,0,1},revision,hooks));CHECK(revision==8);
    hooks.selected=[] { return 50U; };bool failed=false; // 선택한 가상74 누락은 revision 전에 실패한다.
    try { static_cast<void>(DispatchObjectOwner({kSetObjectOwnerCommand,0,1},revision,hooks)); }catch (const std::logic_error&) { failed=true; }CHECK(failed && revision==8);
    bool changed=false;hooks.owner=[&](auto,auto) { changed=true; };failed=false;
    try { static_cast<void>(DispatchObjectOwner({kSetObjectOwnerCommand,0,1},revision,hooks)); }catch (const std::logic_error&) { failed=true; }CHECK(failed && changed && revision==8);
    SidPool pool(OriginalEdition::Patch1078,kCapacity);const auto slot=pool.Allocate();auto raw=pool.AllocatedBytes(slot);raw[34]=0;
    FollowEditorObjectOwner(pool,slot.value,revision,{});CHECK(revision==8);raw[34]=2;failed=false;
    try { FollowEditorObjectOwner(pool,slot.value,revision,{}); }catch (const std::logic_error&) { failed=true; }CHECK(failed && revision==8);
}
