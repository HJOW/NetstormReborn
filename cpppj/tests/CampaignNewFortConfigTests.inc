// 실제 새 자료 초기화가 설정 장치의 NUL 포함 계약과 같은 원본 handle을 전달하는지 검사한다.
TEST_CASE(CampaignNewFort_ConfigReceivesTerminatedOriginalHandle) {
    std::vector<std::vector<std::uint8_t>> values; // 두 원문 Configure 위치에 전달한 실제 물리 바이트다.
    WithNewFortSession([&](auto& session,Client& client,auto checkpoint,EditorFiles& work,CampaignTemplateStorage& storage,auto&) {
        checkpoint.file.title=[] { throw CheckpointStop("new-config-before-world"); };bool stopped{};
        try { session.InstallExistingEditorCheckpoint(checkpoint,client.ReadOnlyForts(),work); }catch (const CheckpointStop& error) { stopped=std::string_view(error.what())=="new-config-before-world"; }
        CHECK(stopped && values.size()==2 && session.CurrentHandle()=="Unnamed" && storage.ReadSubscriberName(1)=="Unnamed");
        const std::vector<std::uint8_t> expected{'U','n','n','a','m','e','d',0}; // 원본 Unnamed의 좁은 문자열과 실제 끝 BYTE다.
        // 원문 두 설정 위치가 같은 내용/종료를 넘기고 물리 handle의 미정 꼬리는 넘기지 않는지 확인한다.
        for (const auto& value:values) CHECK(value==expected);
        const auto bytes=ReadFileBytes(work.ResolvePath("d/Fresh472.fort"));const auto parsed=FortTemplate::Parse(bytes,netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game.TypeTable());CHECK(parsed.sections==storage.Template(1).sections);
    },[&](auto& services) {
        // 실제 CampaignClientServices와 같은 NUL 필수 경계다. 종료 누락을 문자열 길이로 대신 수락하지 않는다.
        services.install.setConfig=[&](std::string_view key,std::span<const std::uint8_t> bytes) {
            const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::invalid_argument("신규 Fort 설정 장치 NUL 누락");CHECK(key=="fort.mySubHandle");values.emplace_back(bytes.begin(),bytes.end());
        };
    });
}
