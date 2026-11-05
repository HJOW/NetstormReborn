// 기존 checkpoint TU의 실제 Client/Editor/전송 조립 도우미를 빌려 새 SaveGoMain 연결만 검증한다.
#include "client/EditorSaveGoMain.h"
#include "client/ClientProductionSync.h"

// No는 실제 저장을 호출하지 않으며 Yes는 같은 전체 Fort/Notify 장치로 작업본 파일을 쓴다.
TEST_CASE(CampaignSaveGoMain_NoThenYesUsesActualFullFortWriter) {
    EditorInstallWireScene saveGoWire;std::shared_ptr<CampaignTemplateStorage> saveGoStorage; //실제 저장소와 로컬 양끝은 Session보다 오래 유지한다.
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        CHECK(CheckpointRejects<std::logic_error>([&] { session.SaveCurrentMap(); }));
        RunEditorWireInstallState(session,client,checkpoint,work);auto& owner=*client.LocalStateGlobals();session.CommitExistingEditorInstall(owner);
        const auto path=work.ResolvePath("d/Checkpoint385.fort");const auto before=ReadFileBytes(path);auto core=session.Core();bool bridge=false;
        for (std::uint32_t number=2;number<core.pool.Capacity();++number) { //현재 실제 풀의 원문 다리 대상을 찾아 저장 전 변경이 의미 있는지 확인한다.
            const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(number)});if (!(raw[11]&1U) && raw[10]==core.bridgeState.bridgeType) { bridge=true;break; }
        }
        CHECK(bridge);session.SetAllBridgeQuality(0);const auto random=client.RandomGlobals()->Game().State();const auto phase=owner.State().phase;std::uint32_t saves=0;
        auto& next=owner.State().next[0];next=77;
        CHECK(DispatchEditorSaveGoMain({kSaveGoMainCommand,1,0},next,[&] { ++saves;session.SaveCurrentMap(); }));
        CHECK(saves==0 && next==3 && ReadFileBytes(path)==before && client.RandomGlobals()->Game().State()==random && owner.State().phase==phase);
        next=78;CHECK(DispatchEditorSaveGoMain({kSaveGoMainCommand,0,1},next,[&] { CHECK(next==78);++saves;session.SaveCurrentMap();CHECK(next==78); }));
        const auto saved=ReadFileBytes(path);CHECK(saves==1 && next==3 && saved!=before && owner.State().phase==phase); //저장/다음 요청만 검증하며 아직 Native State3을 실행한 것으로 합산하지 않는다.
        const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto parsed=FortTemplate::Parse(saved,assets.game.TypeTable());
        CHECK(parsed.Section("TypeNames").size()>4 && !parsed.Section("Technology").empty() && parsed.Section("Territory").size()==120 && !parsed.Section("Deck").empty() && parsed.Section("Money").size()==4 && !parsed.Section("Badges").empty());
        for (const auto name:{"TypeNames","Technology","Territory","Deck","Money","Mission","Badges"}) { //이미지의 실제 섹션은 같은 현재 manager가 쓴 버퍼와 일치해야 한다.
            const auto actual=parsed.Section(name),expected=saveGoStorage->Template(session.TemplateOwner()).Section(name);CHECK(actual.size()==expected.size() && std::equal(actual.begin(),actual.end(),expected.begin()));
        }
        CHECK(&session.CursorDropSettings()==&client.GameplayGlobals()->cursor && session.Core().bookkeeping.depth==0); //이 명령은 현재 Session을 즉시 반납하지 않는다.
    },true,&saveGoWire,{},[&](Client&,RawCampaignSessionServices& services) { saveGoStorage=services.templateStorage; });
}

// 실제 헤더 첫 DWORD0은 선택된 Battle/파일 장치까지 가지 않으며 Battle 중단 뒤 next3을 만들지 않는다.
TEST_CASE(CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop) {
    EditorInstallWireScene saveGoWire;std::shared_ptr<CampaignTemplateStorage> saveGoStorage; //원문 header와 실제 파일 장치를 같은 Session에서 빌린다.
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        RunEditorWireInstallState(session,client,checkpoint,work);auto& owner=*client.LocalStateGlobals();session.CommitExistingEditorInstall(owner);
        const auto path=work.ResolvePath("d/Checkpoint385.fort");const auto original=ReadFileBytes(path);auto core=session.Core();auto header=saveGoStorage->Headers().subspan(session.TemplateOwner()*20U,4);const auto manager=EditorWireDword(header,0);
        std::fill(header.begin(),header.end(),std::uint8_t{});core.ownerMode.battle=true;std::uint32_t next=87;
        CHECK(DispatchEditorSaveGoMain({kSaveGoMainCommand,0,1},next,[&] { session.SaveCurrentMap(); }));CHECK(next==3 && ReadFileBytes(path)==original);
        for (std::size_t byte=0;byte<4;++byte) header[byte]=static_cast<std::uint8_t>(manager>>(byte*8)); //정상 실제 header를 원래 값으로 돌려 다음 선택 경계를 따로 검사한다.
        next=88;bool selected=false;try { static_cast<void>(DispatchEditorSaveGoMain({kSaveGoMainCommand,0,1},next,[&] { session.SaveCurrentMap(); })); }catch (const std::logic_error& error) { selected=std::string_view(error.what()).find("004bf880")!=std::string_view::npos; }
        CHECK(selected && next==88 && ReadFileBytes(path)==original);core.ownerMode.battle=false;
    },true,&saveGoWire,{},[&](Client&,RawCampaignSessionServices& services) { saveGoStorage=services.templateStorage; });
}

// 같은 Client readOnly 값은 실제 전체 snapshot 뒤 파일쓰기만 자연 생략하며 상태 예약은 계속한다.
TEST_CASE(CampaignSaveGoMain_ReadOnlyStillSnapshotsBeforeNextState) {
    EditorInstallWireScene saveGoWire;std::shared_ptr<CampaignTemplateStorage> saveGoStorage; //현재 manager와 파일을 별도 사본으로 대체하지 않는다.
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        RunEditorWireInstallState(session,client,checkpoint,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());
        const auto path=work.ResolvePath("d/Checkpoint385.fort");const auto original=ReadFileBytes(path);client.Configuration().SetInt("readOnlyForts",1);CHECK(client.ReadOnlyForts()==1);
        session.SetAllBridgeQuality(0);const auto random=client.RandomGlobals()->Game().State();std::uint32_t next=89;
        CHECK(DispatchEditorSaveGoMain({kSaveGoMainCommand,0,1},next,[&] { session.SaveCurrentMap(); }));
        CHECK(next==3 && ReadFileBytes(path)==original && client.RandomGlobals()->Game().State()!=random && !saveGoStorage->Template(session.TemplateOwner()).Section("Badges").empty());
    },true,&saveGoWire,{},[&](Client&,RawCampaignSessionServices& services) { saveGoStorage=services.templateStorage; });
}

// World 없는 실제 Getter가 만든 같은 블록을 준비 Session이 재할당/초기화 없이 빌린다.
TEST_CASE(CampaignNoWorldProduction_AdoptsActualClientDeckBeforePrefix) {
    std::shared_ptr<RawProductionDeckStorage> memory;std::uint32_t token{};std::uint8_t* address{}; //Client보다 오래 보관한 물리 조회로 실제 Session 정리도 확인한다.
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto&,EditorFiles&) {
        CHECK(token && client.PlayerRecords()->ProductionDeckMemory()==memory);const auto raw=session.Production().Deck().Bytes(token);CHECK(raw.data()==address && raw[0]==0x5a && EditorWireDword(session.PlayerRecords(),180+0x68)==token);
        CHECK(CheckpointRejects<std::logic_error>([&] { SynchronizeProductionControlContents(session.ProductionContext()); }));CHECK(CheckpointRejects<std::logic_error>([&] { session.SynchronizeProductionContents(); })); //실제 블록이 있어도 설치 전 일반 입력의 기존 한정 Scope는 열지 않는다.
    },true,nullptr,{},[&](Client& client,RawCampaignSessionServices& services) {
        const std::array<std::uint8_t,87> options{};SynchronizeProductionControlContents({services.surfaceStorage,client.GameplayGlobals(),client.PlayerRecords(),options,client.SpGlobals()});memory=client.PlayerRecords()->ProductionDeckMemory();token=EditorWireDword(client.PlayerRecords()->Records(),180+0x68);auto raw=memory->Bytes(token);raw[0]=0x5a;address=raw.data(); //실제44f760과43df80을 Session 생성 전에 실행한다.
    });
    CHECK(CheckpointRejects<std::out_of_range>([&] { static_cast<void>(memory->Bytes(token)); })); //Session 소멸의 실제 링크 부모는 Client가 소유한 같은 블록을 반납한다.
}


// 실제 전체 맵 snapshot 뒤 파일 공유 거부가 나도 다음 상태를 예약하지 않고, 잠금 해제 후 같은 Session으로 다시 저장한다.
TEST_CASE(CampaignSaveGoMain_ActualFileSharingFailureKeepsStateAndAllowsRetry) {
#ifdef _WIN32
    EditorInstallWireScene saveGoWire;std::shared_ptr<CampaignTemplateStorage> saveGoStorage; // 편집용 로컬 장치와 실제 manager를 Session보다 오래 보관한다.
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        RunEditorWireInstallState(session,client,checkpoint,work);auto& owner=*client.LocalStateGlobals();session.CommitExistingEditorInstall(owner);
        const auto path=work.ResolvePath("d/Checkpoint385.fort");const auto original=ReadFileBytes(path);const auto phase=owner.State().phase; // 파일과 상태 블록을 서로 다른 독립 기준으로 보관한다.
        session.SetAllBridgeQuality(0);const auto random=client.RandomGlobals()->Game().State();auto& next=owner.State().next[0];next=91;
        struct SaveLock { // 검사에서 연 읽기 핸들만 반납하며 원본/사용자 핸들을 다루지 않는다.
            HANDLE value{INVALID_HANDLE_VALUE}; // 실제 CRT 쓰기를 막고 읽기만 공유하는 검사 소유 핸들이다.
            void Close() { if (value!=INVALID_HANDLE_VALUE) { CHECK(CloseHandle(value)!=0);value=INVALID_HANDLE_VALUE; } } // 재시도 전 잠금을 풀고 중복 닫기를 막는다.
            ~SaveLock() { Close(); } // 예외가 나도 같은 핸들 하나만 반납한다.
        } lock{CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)};
        CHECK(lock.value!=INVALID_HANDLE_VALUE);if (lock.value==INVALID_HANDLE_VALUE) return;
        bool rejected{};try { static_cast<void>(DispatchEditorSaveGoMain({kSaveGoMainCommand,0,1},next,[&] { session.SaveCurrentMap(); })); }
        catch (const std::runtime_error& error) { rejected=std::string_view(error.what())=="편집Template작업본파일열기실패"; } // 실제 두 CRT 열기의 공유 거부만 기대한다.
        CHECK(rejected && next==91 && owner.State().phase==phase && ReadFileBytes(path)==original);
        CHECK(client.RandomGlobals()->Game().State()!=random && !saveGoStorage->Template(session.TemplateOwner()).Section("Badges").empty()); // 실패 전 끝난 실제 snapshot 효과를 되돌리지 않는다.
        lock.Close();CHECK(DispatchEditorSaveGoMain({kSaveGoMainCommand,0,1},next,[&] { session.SaveCurrentMap(); }));
        const auto saved=ReadFileBytes(path);CHECK(next==3 && owner.State().phase==phase && saved!=original && session.Core().bookkeeping.depth==0);
        const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto restored=FortTemplate::Parse(saved,assets.game.TypeTable());
        for (const auto name:{"TypeNames","Technology","Territory","Deck","Money","Mission","Badges"}) { // 성공한 실제 파일의 현재 섹션이 같은 manager와 일치하는지 재파싱한다.
            const auto actual=restored.Section(name),expected=saveGoStorage->Template(session.TemplateOwner()).Section(name);CHECK(actual.size()==expected.size() && std::equal(actual.begin(),actual.end(),expected.begin()));
        }
    },true,&saveGoWire,{},[&](Client&,RawCampaignSessionServices& services) { saveGoStorage=services.templateStorage; });
#endif
}
