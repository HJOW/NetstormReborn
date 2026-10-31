// 논리 요새 이름과 실제 파일 경로가 로그인 지역 읽기/서버 최초 설치에서 분리되는지 실제 파일/Session으로 검사한다.
namespace { // 기존 작업본/원문 Fort helper의 기대 바이트를 재사용한다.
std::string ManagerName486(CampaignTemplateStorage& storage) { // 실제 manager+c의 NUL 문자열을 직접 읽어 Name 투영과 대조한다.
    const auto manager=Get(storage.Headers(),20);const auto bytes=storage.ManagerBytes(manager).subspan(12,256);const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});
    CHECK(end!=bytes.end());return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
}

TEST_CASE(CampaignMissionFortPath_ActualLoginKeepsLogicalSelectionAndSamePhysicalFile) { // manager의 절대 경로를 다시 논리 fortSpec 인자로 사용하지 않는다.
    for (const bool redirect:{false,true}) { //동일 파일 재작성과 현재 경로 변경을 실제 로컬 로그인으로 각각 확인한다.
        ReloadSourceScene483 scene(redirect ? "path-redirect-486" : "path-same-486");auto mission=scene.Mission();CHECK(mission->FileSource());
        SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);auto templates=std::make_shared<CampaignTemplateStorage>();services.templateStorage=templates;
        scene.BindSelection(services,*mission);
        services.missionFortRead=[](auto)->std::vector<std::uint8_t> { throw std::logic_error("파일 경로 reader를 지정한 실제 Session이 이전 byte reader를 선택함"); };
        const auto before=scene.reads->size();auto session=std::make_unique<RawCampaignSession>(scene.assets.game,scene.assets.files,FortTemplate{},mission->FortName(),std::move(services));services={};
        CHECK(!templates->OwnedImages() && !Get(templates->Headers(),20) && scene.reads->size()==before);const auto selected=redirect ? "other/WorkingReload483.fort" : "maps/WorkingReload483.fort";
        scene.files->Write(selected,scene.changed);if (redirect) scene.config.Set("fortSpec","other/{local.1}.fort");const auto expected=scene.workspace->Resolve(selected).string();
        std::weak_ptr<EditorCatalogWorkspace> weak=scene.workspace;mission.reset();scene.workspace.reset();CHECK(!weak.expired());session->LoadLocalConnection();
        CHECK(session->LocalConnectionPhase()==14 && session->PlayerKnown()==1 && scene.reads->size()==before+2 && (*scene.reads)[before]==selected && scene.reads->back()==selected);
        CHECK(templates->Name(1)==expected && ManagerName486(*templates)==expected);CheckReloadTerrain483(templates->Template(1),FortTemplate::Parse(scene.changed,scene.assets.game.TypeTable()));
        session.reset();CHECK(weak.expired()); //실제 Session의 선택기/읽기 장치를 모두 반납한 뒤 namespace도 반납된다.
    }
}

TEST_CASE(CampaignMissionFortPath_ConfigMutationDuringReadCannotRenameThatImage) { //I/O가 다음 fortSpec을 바꿔도 이번 이미지의 이름은 읽기에 사용한 경로다.
    ReloadSourceScene483 scene("stable-path-486");scene.files->Write("other/WorkingReload483.fort",scene.changed);bool changed{};
    auto mission=std::make_unique<MissionScript>(scene.config,"WorkingReload483",std::string_view{},[owner=scene.workspace,&scene,&changed](std::string_view path) {
        auto bytes=owner->Read(path);if (path.ends_with(".fort") && !changed) { changed=true;scene.config.Set("fortSpec","other/{local.1}.fort"); }return bytes;
    });
    CHECK(mission->Loaded() && mission->FileSource());auto reader=mission->FortFileReader([owner=scene.workspace](auto path) { return owner->Resolve(path).string(); });auto source=mission->FileSource();mission.reset();
    const auto first=reader("WorkingReload483");CHECK(changed && first.image==scene.original && first.fileName==scene.workspace->Resolve("maps/WorkingReload483.fort").string());
    const auto next=reader("WorkingReload483");CHECK(next.image==scene.changed && next.fileName==scene.workspace->Resolve("other/WorkingReload483.fort").string());
    bool mode{};auto replacement=CreateGuideMissionScript(scene.config,"WorkingReload483",mode,source);CHECK(replacement->Loaded() && replacement->FileSource());ReleaseGuideMissionScript(replacement,mode,scene.config);
    ConfigInterface defaults([&](auto path) { return scene.assets.files.TryRead(path); });defaults.Configuration().Assign("missionSpec=\"d/{local.1}.english\"\nfortSpec=\"d/{local.1}.fort\"\n[END]\n");
    MissionScript ordinary(defaults,"TheWarBegins");CHECK(ordinary.Loaded() && !ordinary.FileSource()); //기본 자산 경계는 Guide 교체에서 명시 작업 source로 오인하지 않는다.
}
