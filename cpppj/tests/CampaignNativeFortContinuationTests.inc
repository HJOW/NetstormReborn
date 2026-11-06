// State3 제목 뒤의 외부 재선택 입력으로 실제 Session 설치를 잇는다. 자연 UI 도달을 합성하지 않는다.
#include "client/NativeMainMenuState.h"

// 실제 비편집 파일/월드/Deck은 설치하고 다음 미복원 사제 부모의 명시 중단과 앞선 효과를 확인한다.
TEST_CASE(CampaignNativeFortContinuation_ActualNonEditorWorldReachesPriestBoundary) {
    for (const auto target:{1U,2U}) { // 원문 공통 Fort의 두 비멀티 선택을 각각 독립 Client에서 확인한다.
        WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
            EditorInstallStateContext scene(client,3);std::uint32_t prefix{},hud{},titles{},continuations{},outcome{},first{1};
            std::array<char,256> background{};std::array<std::uint8_t,256> current{},pending{}; // Native의 첫 BYTE 쓰기와 파일 이름 장치 입력은 별도 명시 물리다.
            const auto previous=scene.owner.State().phase,old=scene.owner.State().old;
            NativeMainMenuStateHooks native;native.configInt=[](auto key) { CHECK(key=="intro");return 1; };
            native.prefix=[&] { ++prefix; };native.hudPrefix=[&] { ++hud; };native.title=[&] { ++titles;scene.owner.State().current[0]=target; };
            native.changedState=[&]()->LocalMissionParentResult { ++continuations;session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);throw std::logic_error("미복원 사제 부모를 넘어선 잘못된 성공"); };
            LocalMissionStateHooks stateHooks;stateHooks.logState=[](auto value) { CHECK(value==3); };stateHooks.parentState=[&](auto selected) {
                CHECK(selected==3);return InstallNativeMainMenuState({scene.owner.State(),scene.source,outcome,first,scene.battle,client.GameplayGlobals()->world.editor,background,current,pending,{},[] { return 0U; }},native);
            };
            RawLocalMissionState state(OriginalEdition::Patch1078,scene.owner.State(),scene.Globals(),std::move(stateHooks));bool stopped{};
            try { state.Run(); }catch (const std::logic_error& error) { stopped=std::string_view(error.what())=="캠페인 비미션 Technology/Priest 후처리 전체 미복원"; }
            CHECK(stopped && prefix==1 && hud==1 && titles==1 && continuations==1 && first==1);
            CHECK(scene.owner.State().phase==previous && scene.owner.State().old==old && scene.owner.State().current[0]==target && scene.owner.State().next[0]==0);
            CHECK(!client.GameplayGlobals()->world.editor && !client.GameplayGlobals()->world.missionPost && !session.ExistingEditorCheckpointReady());
            CHECK(session.IslandCount()>0 && session.CurrentTechnologyParent()!=0 && session.Core().bookkeeping.depth==0);
            CHECK(CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(scene.owner); }) && CheckpointRejects<std::logic_error>([&] { session.Cursor(); }));
            CHECK(!ReadFileBytes(work.ResolvePath("d/Checkpoint385.fort")).empty());
        },true,nullptr,{},[](Client& client,RawCampaignSessionServices& services) {
            services.adoptMainMenuFort=true;client.LocalStateGlobals()->State().current[0]=2;
            client.GameplayGlobals()->world.editor=0;client.GameplayGlobals()->world.missionPost=false;client.GameplayGlobals()->FullDeck()=0; // State3의 cleanup/prefix(false) 뒤에 Editor의 네 쓰기를 추가하지 않는다.
        });
    }
}

// 실제 파일 설치 뒤 Guide 요청은 World/tail/공통 phase 확정과 입력 공개를 모두 건너뛴다.
TEST_CASE(CampaignNativeFortContinuation_ActualGuideEarlyReturnKeepsPriorPhase) {
    WithEditorInstallSession([](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        EditorInstallStateContext scene(client,3);const auto previous=scene.owner.State().phase,old=scene.owner.State().old;
        std::uint32_t prefix{},hud{},requested{},outcome{},first{1};std::array<char,256> background{};std::array<std::uint8_t,256> current{},pending{};
        auto effects=checkpoint;MissionFortLoadRequest request;std::uint32_t port{6799};const std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};
        effects.fort.requestMission=[&] {
            ++requested;GuideFortRequestHooks guide;guide.subscriber=[&](auto id,auto name) { session.ChangeFortSubscriber(id,std::span(reinterpret_cast<const std::uint8_t*>(name.data()),name.size()+1)); };
            guide.handle=[&] { return session.CurrentHandle(); };guide.duplicate=[&](auto& packet) { return CaptureMissionFortZone(request,packet); };
            WriteCurrentGuideFortRequest({scene.owner.State().next,scene.owner.ZonePending(),address,port},guide);
        };
        NativeMainMenuStateHooks native;native.configInt=[](auto key) { CHECK(key=="intro");return 1; };native.prefix=[&] { ++prefix; };native.hudPrefix=[&] { ++hud; };
        native.title=[&] { scene.owner.State().current[0]=2; };native.changedState=[&] { CHECK(!session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work));return LocalMissionParentResult::ReturnWithoutCommit; };
        LocalMissionStateHooks hooks;hooks.logState=[](auto value) { CHECK(value==3); };hooks.parentState=[&](auto selected) {
            CHECK(selected==3);return InstallNativeMainMenuState({scene.owner.State(),scene.source,outcome,first,scene.battle,client.GameplayGlobals()->world.editor,background,current,pending,{},[] { return 0U; }},native);
        };
        RawLocalMissionState state(OriginalEdition::Patch1078,scene.owner.State(),scene.Globals(),std::move(hooks));state.Run();
        CHECK(prefix==1 && hud==1 && requested==1 && first==1 && scene.owner.State().phase==previous && scene.owner.State().old==old);
        CHECK(scene.owner.State().current[0]==2 && scene.owner.State().next[0]==7 && scene.owner.ZonePending()==1 && request.zone.size()==86 && session.CurrentHandle()=="Champion");
        CHECK(!session.ExistingEditorCheckpointReady() && CheckpointRejects<std::logic_error>([&] { session.CommitExistingEditorInstall(scene.owner); }));
        CHECK(CheckpointRejects<std::logic_error>([&] { session.Cursor(); }));
    },true,nullptr,[](RawSurfaceStorage& surface) { surface.links.mission=true; },[](Client& client,RawCampaignSessionServices& services) { services.adoptMainMenuFort=true;client.LocalStateGlobals()->State().current[0]=2; client.GameplayGlobals()->world.editor=0;client.GameplayGlobals()->world.missionPost=false; });
}

// 명시 CoreData 섹션 입력의 10BYTE/seed/무결성 세 블록을 실제 설치 도중 읽고 사제 경계까지 확인한다.
TEST_CASE(CampaignNativeFortContinuation_ActualCoreReadSeedAndLocalIntegrity) {
    for (const bool corrupt:{false,true}) { // 정상 인코딩과 첫 랭크 비트 불일치를 별도 Session에서 확인한다.
        std::shared_ptr<CampaignTemplateStorage> storage;
        WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
            auto selected=checkpoint;std::uint32_t titles{};client.RandomGlobals()->FortCoreSeed()=0xa55a0000U;
            selected.file.title=[&] {
                if (++titles!=1) return; // 실제 LoadFile의 첫 제목 갱신 뒤 선택한 같은 manager에 검사 입력을 쓴다.
                std::vector<std::uint8_t> bytes{3,0,5,0,2,0x71,0x72,0x73,0x74,0x75,0x34,0x12};
                for (const std::uint32_t expected:{2U,3U,5U}) { // 원문의 랭크/승리/패배 세 블록 순서다.
                    for (std::uint32_t bit=0;bit<32;++bit) { // 각 블록의8 BYTE·8 WORD·16 DWORD를 명시 비트로 인코딩한다.
                        const auto value=(expected&(1U<<bit)) ^ (corrupt && expected==2 && bit==0 ? 1U : 0U),width=bit<8 ? 1U : bit<16 ? 2U : 4U;
                        for (std::uint32_t byte=0;byte<width;++byte) bytes.push_back(static_cast<std::uint8_t>(value>>(byte*8))); // 선택 폭의 little endian BYTE만 기록한다.
                    }
                }
                storage->WriteSection(storage->CreateSection(1,"CoreData"),bytes);
            };
            bool stopped{};try { session.InstallExistingEditorCheckpoint(selected,client.ReadOnlyForts(),work); }catch (const std::logic_error& error) { stopped=std::string_view(error.what())=="캠페인 비미션 Technology/Priest 후처리 전체 미복원"; }
            CHECK(stopped && titles==2 && session.IslandCount()>0 && !session.ExistingEditorCheckpointReady());
            const auto core=client.CoreDataGlobals()->Current();CHECK(core.size()==10 && core[0]==3 && core[1]==0 && core[2]==5 && core[3]==0 && core[4]==(corrupt ? 1 : 2) && core[5]==0x71 && core[9]==0x75);
            CHECK(client.CoreDataGlobals()->Active()==1 && client.RandomGlobals()->FortCoreSeed()==0xa55a1234U);
            CHECK(client.GameplayGlobals()->PatchDiagnostic()==(corrupt ? 0U : 6U) && session.Core().bookkeeping.depth==0);
            CHECK(!client.GameplayGlobals()->world.editor && !client.GameplayGlobals()->world.missionPost);
        },true,nullptr,{},[&](Client& client,RawCampaignSessionServices& services) {
            services.adoptMainMenuFort=true;storage=services.templateStorage;client.LocalStateGlobals()->State().current[0]=2;
            client.GameplayGlobals()->world.editor=0;client.GameplayGlobals()->world.missionPost=false;client.GameplayGlobals()->FullDeck()=0;
        });
    }
}

// 새 대여 경로는 기존 Editor 모드 검사와 다른 current/미준비 World의 거부를 유지한다.
TEST_CASE(CampaignNativeFortContinuation_RejectsWrongModeOriginAndUnpreparedBorrow) {
    for (std::uint32_t choice=0;choice<4;++choice) { // Editor1을 일반 Fort로 대여, 일반 Fort를 Editor로 대여, current3, 미준비의 네 잘못된 입력이다.
        bool rejected{};
        try { WithEditorInstallSession([](auto&,auto&,const auto&,auto&) { throw CheckpointStop("잘못된 모드가 실제 Session 본문에 도달"); },choice!=3,nullptr,{},[choice](Client& client,RawCampaignSessionServices& services) {
            services.adoptMainMenuFort=choice!=1;client.LocalStateGlobals()->State().current[0]=choice==2 ? 3 : 2;
            if (choice!=0) { client.GameplayGlobals()->world.editor=0;client.GameplayGlobals()->world.missionPost=false; }
        }); }catch (const std::logic_error& error) {
            const std::string_view message(error.what());rejected=choice<2 ? message=="준비된 Editor의 실제 prefix 모드/Player 불일치" : message=="State3 continuation의 같은 준비된 비멀티 Fort/State1·2 계약";
        }
        CHECK(rejected);
    }
}
