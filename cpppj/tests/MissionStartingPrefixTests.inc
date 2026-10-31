// 실제 Client의 현재 요청/기본 배경 분모와 같은 원문 지역 값의 조건부 복원을 창 없이 확인한다.
#include "client/ClientHtmlWindowGlobals.h"

// 정리 전/후의+84 조합과 분모 변경/동일값을 같은 장기 Client의 네 요청에서 구별한다.
TEST_CASE(MissionStartingPrefix_ActualClientDefaultDenominatorAndCurrentRequest) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));auto& local=*client.LocalStateGlobals();auto state=local.State();auto& camera=client.GameplayGlobals()->Camera();
    for (std::uint32_t flags=0;flags<4;++flags) { // 이전+84와 복원 순간의 현재+84를 각각 독립 입력으로 바꾼다.
        MissionFortLoadRequest request;const std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};const std::uint32_t port=6799;MissionFortRequestHooks hooks;hooks.selectFort=[&](auto name) { request.fort=name; };hooks.duplicate=[&](auto& packet) { return CaptureMissionFortZone(request,packet); };WriteMissionFortRequest("thewarbegins",{state.next,local.ZonePending(),address,port},hooks);request.next=state.next;request.sequence=local.ZonePending();
        state.phase.fill(0x10203040U);state.phase[0]=4;state.old.fill(0x50607080U);const auto previous=state.phase,older=state.old;state.next[33]=flags&1U;state.next[2]=0xfedcba98U+flags;state.next[35]=0x89abcdefU;auto active=state.next;auto next=active;next[0]=0; // 각 반복의 실제 인자를 다르게 만들어 직전 요청의 stale 값과 구별한다.
        local.GameTicks()=0xfedcba98U;local.SourceType()=0x87654321U;const auto pending=local.ZonePending();auto& title=client.HtmlWindowGlobals()->text.background;title[0]='X';title[1]='Y';camera.Denominator().defaultValue=23;camera.Denominator().current=(flags&1U) ? 23 : 37;client.KencloudGlobals()->WorldRedraw()=19;
        camera.Snapshot().pixels={11,13};camera.RendererView().pixels={17,19};camera.Target().pixels={29,31};const auto snapshot=std::bit_cast<std::array<std::uint32_t,16>>(camera.Snapshot()),view=std::bit_cast<std::array<std::uint32_t,16>>(camera.RendererView());
        PrepareCampaignMissionStarting(client,request);CHECK(request.starting && request.starting->zonePending==pending && request.starting->gameTicks==((flags&1U) ? 0xfedcba98U : 0U) && request.starting->sourceType==((flags&1U) ? 0x87654321U : 0U));CHECK(state.current==active && state.next==next && state.phase==previous && state.old==older);CHECK(request.next[2]!=state.current[2]);
        CHECK(camera.Denominator().current==23 && camera.Denominator().defaultValue==23 && client.KencloudGlobals()->WorldRedraw()==((flags&1U) ? 19U : 1U));CHECK((std::bit_cast<std::array<std::uint32_t,16>>(camera.Snapshot())==snapshot && std::bit_cast<std::array<std::uint32_t,16>>(camera.RendererView())==view));CHECK((camera.Target().pixels==std::array<std::int32_t,2>{29,31}) && title[0]==0 && title[1]=='Y');
        local.ZonePending()=0;local.GameTicks()=41;local.SourceType()=43;state.current[33]=(flags>>1)&1U;netstorm::o::RestoreLocalMissionStarting(state.current,*request.starting,local.GameTicks(),local.SourceType()); // 정리/명령 효과가 바꾼 현재 표식을 다시 읽는다.
        CHECK(local.GameTicks()==((flags&2U) ? request.starting->gameTicks : 41U) && local.SourceType()==((flags&2U) ? request.starting->sourceType : 43U) && request.starting->zonePending==pending);
    }
}

// 실제 앞단이 소비한 현재 블록을 독립 Init 캡처로 덮어쓰지 않고7/8 전체 확정에 넘긴다.
TEST_CASE(MissionStartingPrefix_ActualLoginRetainsPreparedCurrentBlock) {
    MissionAsyncScene scene;auto state=scene.state->State();state.current=scene.request->next;state.next=state.current;state.next[0]=0;state.current[2]=0x12345678U;state.current[35]=0x90abcdefU;scene.state->ZonePending()=7;std::uint8_t title=0x58;scene.request->starting=CaptureLocalMissionStarting(state.current,scene.state->ZonePending(),title,scene.state->GameTicks(),scene.state->SourceType());scene.state->ZonePending()=0;state.current[2]=0x87654321U; // 실제 정리 뒤 바뀐 요청과 이전 캡처를 구별한다.
    auto seven=state.current;seven[32]=0;auto eight=state.current;eight[0]=8;eight[1]=20;scene.world->StartLocalConnection();CHECK(state.old==seven && state.phase==eight && scene.request->next[2]!=state.phase[2] && scene.request->starting->zonePending==7 && scene.state->ZonePending()==0 && scene.request->consumed && scene.packets==0);
    CHECK(scene.world->Session().PollLocalConnectionState());scene.world->Session().PumpLocalConnection();CHECK(scene.world->CompleteLocalConnection() && scene.world->Session().LocalConnectionReady());
}
