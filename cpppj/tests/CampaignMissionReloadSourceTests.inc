// 실제 로컬 Login의 재로드도 미션 작업 source/현재 fortSpec을 사용하고 수명을 소유하는지 검증한다.
#include "client/EditorMapCatalogFiles.h"
namespace { //기존 Session/원문 로그인 helper를 빌리되 검사 입력/파일은 별도로 소유한다.
struct ReloadSourceScene483 { //보호 VFS와 다른 실제 작업 폴더/장부/Config를 함께 유지한다.
    const netstorm::test::dudespawn::DudeAssets& assets{netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078)}; //실제 TYPE와 원본 파일은 읽기 전용이다.
    std::filesystem::path root; //이 검사만 쓰는 임시 작업 폴더이며 기존 폴더를 삭제하지 않는다.
    std::shared_ptr<EditorFiles> files; //canonical/원본 보호를 적용하는 실제 writer다.
    std::shared_ptr<EditorCatalogWorkspace> workspace; //현재 작업 읽기/이관/삭제 부재를 소유한다.
    std::shared_ptr<std::vector<std::string>> reads{std::make_shared<std::vector<std::string>>()}; //각 실제 파일 읽기 경로를 독립 관측한다.
    ConfigInterface config; //Session reader보다 오래 사는 Client Config 경계다.
    std::vector<std::uint8_t> original,changed; //최초 적재와 로그인 직전 재작성할 서로 다른 전체 원본 Fort 입력이다.
    explicit ReloadSourceScene483(std::string_view label):root(std::filesystem::temp_directory_path()/("netstorm-login-source-"+std::string(label)+'-'+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))),
        files(std::make_shared<EditorFiles>(root,std::vector<std::filesystem::path>{assets.root,assets.root.parent_path()/"originalCD"})),
        workspace(std::make_shared<EditorCatalogWorkspace>(files,EditorCatalogSourceDevices{assets.root,[this](auto path) { return assets.files.TryReadArchiveOnly(path); }})),
        config([this](auto path) { return assets.files.TryRead(path); }) {
        config.Configuration().Assign("DataDir=\"d\"\nmissionSpec=\"d/{local.1}.english\"\nfortSpec=\"d/{local.1}.fort\"\n[END]\n");original=assets.files.Read("d/thewarbegins.fort");
        { MissionScript reference(config,"Tutorial2");CHECK(reference.Loaded());changed=assets.files.Read(reference.FortPath()); } //실제 loadFort가 고른 입력이며 동명 파일을 가정하지 않는다.
        CHECK(original!=changed);files->EnsureDirectory("maps");files->EnsureDirectory("other");files->EnsureDirectory("d");
        const std::string script="[Header]\nmissionType=\"Tutorial\"\nloadFort=\"WorkingReload483\"\n[A.]\nReload scenario\n[END]\n";
        files->Write("d/WorkingReload483.english",{reinterpret_cast<const std::uint8_t*>(script.data()),script.size()});files->Write("maps/WorkingReload483.fort",original);config.Set("fortSpec","maps/{local.1}.fort");
    }
    std::unique_ptr<MissionScript> Mission() { //source 자체가 namespace와 읽기 관측의 소유 복사를 보유한다.
        return std::make_unique<MissionScript>(config,"WorkingReload483",std::string_view{},[owner=workspace,log=reads](auto path) { log->emplace_back(path);return owner->Read(path); });
    }
    std::unique_ptr<RawCampaignSession> Session(MissionScript& mission,std::shared_ptr<CampaignTemplateStorage> templates,SessionServicesFixture& external) { //성공 상태를 입력하지 않고 실제 Login/Zone/패킷을 사용한다.
        auto services=external.Services();UseWorldTestLogin(services);services.templateStorage=std::move(templates);services.missionFortImage=std::make_shared<const std::vector<std::uint8_t>>(mission.ReadFortImage(mission.FortName()));services.missionFortRead=mission.FortReader();
        return std::make_unique<RawCampaignSession>(assets.game,assets.files,FortTemplate::Parse(*services.missionFortImage,assets.game.TypeTable()),mission.FortName(),std::move(services));
    }
};
void CheckReloadTerrain483(const FortTemplate& actual,const FortTemplate& expected) { //State/로그인 기록과 별개로 실제 원본 지형 섹션 전체 바이트를 대조한다.
    for (const auto name:FortTemplate::SectionNames()) { //로그인이 직접 갱신하는 State/Subscriber 외 섹션도 실제 설치한 파일과 같아야 한다.
        if (name=="State" || name=="Subscriber") continue;const auto first=actual.Section(name),second=expected.Section(name);CHECK(std::equal(first.begin(),first.end(),second.begin(),second.end()));
    }
}
}

TEST_CASE(CampaignMissionReloadSource_ActualLoginRereadsWorkingFortAndCurrentSpec) { //최초 원문 사본 대신 로그인 시점의 작업본을 실제 월드에 설치한다.
    for (const bool redirect:{false,true}) { //같은 경로의 재작성과 현재 Config 경로 변경을 각각 실제 로그인으로 소비한다.
        ReloadSourceScene483 scene(redirect ? "redirect" : "same");auto mission=scene.Mission();auto templates=std::make_shared<CampaignTemplateStorage>();SessionServicesFixture external;
        auto session=scene.Session(*mission,templates,external);const auto before=scene.reads->size();CHECK(templates->OwnedImages()==1);
        const auto selected=redirect ? "other/WorkingReload483.fort" : "maps/WorkingReload483.fort";scene.files->Write(selected,scene.changed);if (redirect) scene.config.Set("fortSpec","other/{local.1}.fort");
        session->LoadLocalConnection();CHECK(session->LocalConnectionPhase()==14 && session->PlayerKnown()==1 && session->IslandCount()>0);CHECK(scene.reads->size()==before+1 && scene.reads->back()==selected);
        CheckReloadTerrain483(templates->Template(1),FortTemplate::Parse(scene.changed,scene.assets.game.TypeTable()));CHECK(scene.assets.files.Read("d/thewarbegins.fort")==scene.original);
        CHECK(!scene.assets.files.TryRead("d/WorkingReload483.fort")); //기존 고정 d/ reader로는 열 수 없는 실제 사용자 이름이다.
    }
}

TEST_CASE(CampaignMissionReloadSource_SessionOwnsReaderAfterScriptAndEditorOwnersRelease) { //Guide/Editor 외부 소유자가 사라져도 현재 Session의 재로드 장치는 같은 namespace다.
    ReloadSourceScene483 scene("owner");std::weak_ptr<EditorCatalogWorkspace> weak=scene.workspace;SessionServicesFixture external;auto templates=std::make_shared<CampaignTemplateStorage>();
    auto mission=scene.Mission();auto session=scene.Session(*mission,templates,external);scene.files->Write("maps/WorkingReload483.fort",scene.changed);mission.reset();scene.workspace.reset();CHECK(!weak.expired());
    session->LoadLocalConnection();CHECK(session->LocalConnectionPhase()==14 && session->PlayerKnown()==1);CheckReloadTerrain483(templates->Template(1),FortTemplate::Parse(scene.changed,scene.assets.game.TypeTable()));
    session.reset();CHECK(weak.expired()); //실제 Session이 소유한 reader 반납 뒤 파일 장부 잠금도 함께 소멸한다.
}

TEST_CASE(CampaignMissionReloadSource_SelectedMissingEmptyAndThrowPreservePriorManager) { //명시 source 실패를 실제 원본 fallback/기존 Template 교체로 바꾸지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort");
    for (std::uint32_t mode=0;mode<3;++mode) { //부재/빈 실제 파일/외부 I/O 예외는 모두 선택 source에서 발생한다.
        ConfigInterface config([&](auto path) { return assets.files.TryRead(path); });config.Configuration().Assign("missionSpec=\"d/{local.1}.english\"\nfortSpec=\"d/{local.1}.fort\"\n[END]\n");std::size_t calls{};
        MissionScript mission(config,"thewarbegins",{},[&](std::string_view path)->std::optional<std::vector<std::uint8_t>> { if (path.ends_with(".english")) return assets.files.TryRead(path);++calls;if (mode==0) return std::nullopt;if (mode==1) return std::vector<std::uint8_t>{};throw std::runtime_error("selected reload failure"); });
        CHECK(mission.Loaded());SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);auto templates=std::make_shared<CampaignTemplateStorage>();services.templateStorage=templates;services.missionFortImage=std::make_shared<const std::vector<std::uint8_t>>(original);services.missionFortRead=mission.FortReader();
        RawCampaignSession session(assets.game,assets.files,FortTemplate::Parse(original,assets.game.TypeTable()),"thewarbegins",std::move(services));const auto manager=Get(templates->Headers(),20);bool failed=false;
        try { session.LoadLocalConnection(); }catch (const std::runtime_error&) { failed=true; }
        CHECK(failed && calls==1 && session.LocalConnectionPhase()!=14 && templates->OwnedImages()==1 && Get(templates->Headers(),20)==manager);CheckReloadTerrain483(templates->Template(1),FortTemplate::Parse(original,assets.game.TypeTable()));
    }
}
