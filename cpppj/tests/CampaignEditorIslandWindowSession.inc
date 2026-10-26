// 기존 실제 Editor 설치 helper를 새 공통 UI 검사에서 빌리며 소유권은 이 동기 범위에 유지한다.
void WithEditorIslandWindowSession480(bool fresh,const std::function<void(RawCampaignSession&,Client&)>& body) {
    std::vector<Sid> soundRequests; // 실제 객체 교체의 외부 음향 요청만 관측하며 오디오 장치를 만들지 않는다.
    if (fresh) {
        WithNewFortSession([&](auto& session,Client& client,const auto& checkpoint,EditorFiles& work,auto&,auto&) {
            EditorInstallStateContext scene(client);InputQueue input;std::uint32_t calls{};const auto tail=EditorInstallTail(session,calls);LocalMissionStateHooks hooks;hooks.logState=[](auto) {};
            hooks.parentState=[&](auto selected) { CHECK(selected==1);session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);session.WithExistingEditorInstallUi(scene.owner,input,tail,[] {});return LocalMissionParentResult::Commit; };
            RawLocalMissionState state(OriginalEdition::Patch1078,scene.owner.State(),scene.Globals(),std::move(hooks));state.Run();session.CommitExistingEditorInstall(scene.owner);body(session,client);
        },[&](RawCampaignSessionServices& services) { services.editorIslandGreen=[] { return 43U; };services.unitLost=[&](Sid sid) { soundRequests.push_back(sid); }; });
        return;
    }
    EditorInstallWireScene wire; // 실제 기존 Editor의 공통 패킷 큐다.
    WithEditorInstallSession([&](auto& session,Client& client,const auto& checkpoint,EditorFiles& work) { session.SetSp(1,0.0F);RunEditorWireInstallState(session,client,checkpoint,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());body(session,client); },true,&wire,{},[&](Client&,RawCampaignSessionServices& services) { services.editorIslandGreen=[] { return 43U; };services.unitLost=[&](Sid sid) { soundRequests.push_back(sid); }; });
}
