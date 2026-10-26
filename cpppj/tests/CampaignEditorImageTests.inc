// 실제 기존 Editor의 같은 Session에서 bitmap→crop Image 수명과 공유 상태를 검증한다.
TEST_CASE(CampaignEditorImage_ActualMapAndCroppedChoicesUseSameSession) {
    EditorInstallWireScene wire;std::shared_ptr<RawSurfaceStorage> surface;
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        ChunkMap preview(14,14);CHECK(CheckpointRejects<std::logic_error>([&] { static_cast<void>(session.BuildEditorIslandMapShape(preview)); }));
        RunEditorWireInstallState(session,client,checkpoint,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());
        const auto core=session.Core();const std::vector<std::uint8_t> pool(core.pool.Bytes().begin(),core.pool.Bytes().end());const auto seed=client.RandomGlobals()->Game().State();const auto islands=surface->islands.records;
        const auto map=session.BuildEditorIslandMapShape(preview);CHECK(map && map->Width()==224 && map->Height()==224 && map->Bits().empty() && map->Shape().size()>40);
        const std::array<std::uint8_t,6> record{0,1,0,37,0,0};auto choice=session.BuildEditorIslandChoiceShape(record);
        CHECK(choice.Width()>0 && choice.Width()<224 && choice.Height()>0 && choice.Height()<224 && choice.Bits().empty() && choice.Shape().size()>40);
        RawImage copy;copy.CopyFrom(choice);const std::vector<std::uint8_t> bytes(copy.Shape().begin(),copy.Shape().end());choice.Destroy();choice.Destroy();
        CHECK(std::equal(copy.Shape().begin(),copy.Shape().end(),bytes.begin(),bytes.end()) && copy.Width()>0 && copy.Height()>0);
        CHECK(surface->islands.records==islands && client.RandomGlobals()->Game().State()==seed && std::equal(pool.begin(),pool.end(),core.pool.Bytes().begin()));
    },true,&wire,{},[&](Client&,RawCampaignSessionServices& services) { surface=services.surfaceStorage;services.editorIslandGreen=[] { return 43U; }; });
}
