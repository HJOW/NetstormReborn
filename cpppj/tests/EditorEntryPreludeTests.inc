// 실제 단독 Session/로컬 큐와 Native State1 앞단의 정리 전 직접 쓰기/통지를 창 없이 확인한다.
#include "client/NativeMainMenuState.h"
#include "client/RawBattleLeave.h"

// 이전phase/현재 battle의 세 조합에서 출처 선택과52h 예약·지연 수신을 구별한다.
TEST_CASE(EditorEntryPrelude_ActualLocalLeaveBeforeCleanupAndPreservedState) {
    for (const unsigned input:{0U,1U,2U}) { // 낮은 이전phase, Battle14, battle0/phase14를 각각 새 단독 World에서 확인한다.
        MissionAsyncScene scene;scene.world->StartLocalConnection();CHECK(scene.world->Session().PollLocalConnectionState());scene.world->Session().PumpLocalConnection();CHECK(scene.world->CompleteLocalConnection());auto& session=scene.world->Session();auto state=scene.state->State();state.phase[0]=input==0 ? 4U : 14U;scene.surface->ownerMode.battle=input==1;scene.state->SourceType()=99;state.next=state.phase;state.next[0]=1;const auto previous=state.phase;MissionParentTime time;std::vector<std::uint32_t> phases;std::array<char,80> background;background.fill('#');auto expected=background;expected[0]=0;std::size_t notified{},cleanup{};
        const auto before=session.LocalConnectionPackets().size();const auto packetCount=[&] { const auto packets=session.LocalConnectionPackets();return std::count(packets.begin(),packets.end(),std::uint8_t{0x52}); };const auto before52=packetCount();const auto playerAt=std::size_t(session.LocalOwner())*180+80;const auto flags=session.PlayerRecords()[playerAt];
        auto parent=MakeMissionParent(scene,time,phases,{},[&](auto selected) { CHECK(selected==1);PrepareNativeEditorEntry(state,scene.state->SourceType(),background,scene.surface->ownerMode.battle,[&] { ++notified;CHECK(state.phase==previous && background==expected && scene.state->SourceType()==99 && cleanup==0);SendBattleLeaveForState(session);CHECK(session.LocalConnectionPackets().size()==before); });++cleanup;return LocalMissionParentResult::Commit; }); // cleanup 자체는 명시 외부 경계이며 완전한 Editor 재설치를 성공으로 합성하지 않는다.
        parent->Poll();CHECK(state.phase[0]==1 && state.old==previous && notified==(input==1 ? 1U : 0U) && cleanup==1 && phases==std::vector<std::uint32_t>{1} && background==expected && scene.state->SourceType()==(input==0 ? 0U : 99U));CHECK(packetCount()==before52); // 통지는 정리 전에 예약하지만 같은 호출 안에서 수신/flush를 강제하지 않는다.
        if (input==1) { session.PumpLocalConnection();CHECK(packetCount()==before52+1 && session.PlayerRecords()[playerAt]==static_cast<std::uint8_t>(flags|4U)); } // 실제52h 수신기는 현재 sender의 참여 기록에만 비트2를 추가한다.
    }
}

// 필요한 이전 Battle 통지 장치의 누락/실패는 cleanup과State1 확정 전에 앞선 쓰기만 보존한다.
TEST_CASE(EditorEntryPrelude_LeaveFailureKeepsUncommittedNativeFields) {
    for (const bool missing:{true,false}) { // 필수 장치 누락과 실제 통지 callback 실패를 구별한다.
        MissionAsyncScene scene;scene.world->StartLocalConnection();CHECK(scene.world->Session().PollLocalConnectionState());scene.world->Session().PumpLocalConnection();CHECK(scene.world->CompleteLocalConnection());auto state=scene.state->State();state.next=state.phase;state.next[0]=1;scene.state->SourceType()=99;scene.surface->ownerMode.battle=true;const auto previous=state.phase,older=state.old;MissionParentTime time;std::vector<std::uint32_t> phases;std::array<char,80> background;background.fill('@');auto expected=background;expected[0]=0;std::size_t cleanup{},notified{};std::function<void()> leave;if (!missing) leave=[&] { ++notified;throw std::logic_error("집중 검사 State1 통지 실패"); };
        auto parent=MakeMissionParent(scene,time,phases,{},[&](auto) { PrepareNativeEditorEntry(state,scene.state->SourceType(),background,scene.surface->ownerMode.battle,leave);++cleanup;return LocalMissionParentResult::Commit; });bool rejected{};try { parent->Poll(); }catch (const std::logic_error& error) { rejected=std::string(error.what()).find(missing ? "Fort 진입의 이전 Battle14 실제 통지 미연결" : "집중 검사 State1 통지 실패")!=std::string::npos; }
        CHECK(rejected && cleanup==0 && notified==(missing ? 0U : 1U) && state.phase==previous && state.old==older && state.current[0]==1 && state.next[0]==0 && background==expected && scene.state->SourceType()==99);CHECK(scene.gameplay->Borrowed() && scene.surface->Borrowed()); // 실패한 앞단이 실제 cleanup/설치/상태 확정을 수행했다고 표시하지 않는다.
    }
}
