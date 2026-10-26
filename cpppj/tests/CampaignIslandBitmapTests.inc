// 기존 실제 Client/Editor 검사 TU의 장치를 빌리며 기존 검사 본문은 변경하지 않는다.
// 실제 기존 지도와 한 후보의 Bitmap은 공유 게임 난수/Pool/섬/저장 자료를 변경하지 않는다.
TEST_CASE(CampaignIslandBitmap_ActualEditorMapChoiceAndSharedOwners) {
    EditorInstallWireScene wire;std::shared_ptr<RawSurfaceStorage> surface;std::uint32_t greenCalls{};
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        ChunkMap preview(14,14);CHECK(CheckpointRejects<std::logic_error>([&] { static_cast<void>(session.BuildEditorIslandMapImage(preview)); }));
        RunEditorWireInstallState(session,client,checkpoint,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());
        const auto core=session.Core();const std::vector<std::uint8_t> pool(core.pool.Bytes().begin(),core.pool.Bytes().end());const auto islands=surface->islands.records;const auto seed=client.RandomGlobals()->Game().State();
        const auto image=session.BuildEditorIslandMapImage(preview);CHECK(image && image->width==224 && image->height==224 && image->pixels.size()==50176 && greenCalls==1);
        CHECK(image && std::any_of(image->pixels.begin(),image->pixels.end(),[](auto value) { return value!=0; }));
        const std::array<std::uint8_t,6> record{0,1,0,37,0,0};const auto first=session.BuildEditorIslandChoiceImage(record),again=session.BuildEditorIslandChoiceImage(record);
        CHECK(first.width==224 && first.height==224 && first.pixels==again.pixels && std::any_of(first.pixels.begin(),first.pixels.end(),[](auto value) { return value!=0; }));
        CHECK(greenCalls==1 && client.GameplayGlobals()->TerrainBitmaps().classified['x']==43 && client.RandomGlobals()->Game().State()==seed);
        CHECK(surface->islands.records==islands && std::equal(pool.begin(),pool.end(),core.pool.Bytes().begin()));
    },true,&wire,{},[&](Client&,RawCampaignSessionServices& services) { surface=services.surfaceStorage;services.editorIslandGreen=[&] { ++greenCalls;return 43U; }; });
}

// 신규 Fort의 빈 지도도 실제 Bitmap0과 비어 있지 않은 후보를 같은 장기 분류로 생성한다.
TEST_CASE(CampaignIslandBitmap_NewFortEmptyMapAndRealCandidate) {
    std::uint32_t greenCalls{};
    WithNewFortSession([&](auto& session,Client& client,const auto& checkpoint,EditorFiles& work,auto&,auto&) {
        EditorInstallStateContext scene(client);InputQueue input;std::uint32_t calls{};const auto tail=EditorInstallTail(session,calls);LocalMissionStateHooks hooks;hooks.logState=[](auto) {};
        hooks.parentState=[&](auto selected) { CHECK(selected==1);session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work);session.WithExistingEditorInstallUi(scene.owner,input,tail,[] {});return LocalMissionParentResult::Commit; };
        RawLocalMissionState state(OriginalEdition::Patch1078,scene.owner.State(),scene.Globals(),std::move(hooks));state.Run();session.CommitExistingEditorInstall(scene.owner);
        ChunkMap map(14,14);const auto blank=session.BuildEditorIslandMapImage(map);CHECK(blank && blank->pixels.size()==50176 && std::all_of(blank->pixels.begin(),blank->pixels.end(),[](auto value) { return value==0; }));
        std::array<std::uint8_t,126> choices{};CHECK(session.SeedEditorIslandChoices(choices));const auto seed=client.RandomGlobals()->Game().State();
        const auto candidate=session.BuildEditorIslandChoiceImage(std::span<const std::uint8_t,6>(choices.data(),6));CHECK(candidate.width==224 && candidate.height==224 && std::any_of(candidate.pixels.begin(),candidate.pixels.end(),[](auto value) { return value!=0; }));
        CHECK(greenCalls==1 && session.IslandCount()==0 && client.RandomGlobals()->Game().State()==seed && session.Sp(1)==194162.0F);
    },[&](auto& services) { services.editorIslandGreen=[&] { ++greenCalls;return 43U; }; });
}
