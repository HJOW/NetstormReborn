// 미션 script/추가 설정/실제 Fort 설치가 같은 작업 namespace와 수명을 사용하는지 확인한다.
#include "DudeSpawnSupport.h"
#include "client/Mission.h"
#include "client/EditorFiles.h"
#include "client/EditorMapCatalogFiles.h"
#include "o/Player.h"
#include <chrono>

using namespace netstorm::client;
using namespace netstorm::o;
namespace { //실제 보호 입력은 읽기 전용이며 새 파일은 검사 전용 임시 작업 폴더에만 쓴다.
struct MissionFileScene { //실제 VFS/Config와 canonical 작업 장치/장부를 함께 준비한다.
    const netstorm::test::dudespawn::DudeAssets& assets; //보호 TYPE/TAFF와 원본 파일 reader를 빌린다.
    std::filesystem::path root; //검사마다 다른 NTFS 임시 작업 폴더다. 기존 폴더를 삭제하지 않는다.
    std::shared_ptr<EditorFiles> files; //canonical/원본 보호 검사를 사용하는 실제 작업 파일 장치다.
    std::shared_ptr<EditorCatalogWorkspace> workspace; //한번 이관/삭제 부활 방지와 현재 파일 읽기를 소유한다.
    ConfigInterface config; //일반 reader는 원본 VFS로 유지하며 특정 미션의 source만 별도로 넘긴다.
    explicit MissionFileScene(std::string_view label):assets(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078)),
        root(std::filesystem::temp_directory_path()/("netstorm-mission-source-"+std::string(label)+'-'+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))),
        files(std::make_shared<EditorFiles>(root,std::vector<std::filesystem::path>{assets.root,assets.root.parent_path()/"originalCD"})),
        workspace(std::make_shared<EditorCatalogWorkspace>(files,EditorCatalogSourceDevices{assets.root,[this](auto path) { return assets.files.TryReadArchiveOnly(path); }})),
        config([this](auto path) { return assets.files.TryRead(path); }) {
        files->EnsureDirectory("d");config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}/{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}/{local.1}.fort\"\n[END]\n");
    }
    void Text(std::string_view path,std::string_view text) { //사용자가 별도 텍스트 편집기로 작성할 언어 script와 같은 실제 파일을 만든다.
        files->Write(path,{reinterpret_cast<const std::uint8_t*>(text.data()),text.size()});
    }
    ConfigInterface::FileReader Reader() const { //이 callback의 소유 복사가 workspace를 Editor 외부까지 유지한다.
        return [owner=workspace](auto path) { return owner->Read(path); };
    }
};
template<class Function> std::string FileFailure(Function function) { //명시 I/O 실패와 성공한 빈 파일을 구별해 진단을 확인한다.
    try { function(); }catch (const std::exception& error) { return error.what(); }return {};
}
}

TEST_CASE(MissionFileSource_ActualWorkspaceScenarioAliasFortAndPlayerHeader) { //새 사용자 script와 편집한 기존 Fort가 실제 parser/Player 입력까지 전달된다.
    MissionFileScene scene("scenario");const auto original=scene.assets.files.Read("d/TEST01.fort");std::vector<std::uint8_t> changed;
    { MissionScript reference(scene.config,"Tutorial2");CHECK(reference.Loaded());changed=scene.assets.files.Read(reference.FortPath()); } //보호 Tutorial2의 실제 loadFort를 입력으로 쓰며 존재하지 않는 동명 Fort를 가정하지 않는다.
    CHECK(original!=changed);
    scene.files->Write("d/TEST01.fort",changed);
    scene.Text("d/Fresh482.english","[Header]\nmissionType=\"Tutorial\"\nloadFort=\"TEST01\"\nmyStartMoney=12345\nmyTech=\"all\"\nmyAllyList=\"2\"\nai2Name=\"Custom Wind\"\nai2StartMoney=987\nai2Ability=\"!BUILD_DAIS\"\n[A.]\n<h2>User scenario</h2>\n[END]\n");
    bool mode=false;auto mission=CreateGuideMissionScript(scene.config,"Fresh482",mode,scene.Reader());
    CHECK(mission->Loaded() && mode && scene.config.GetInt("global.inMission")==1);CHECK(mission->MissionType()=="Tutorial" && mission->FortName()=="TEST01");
    const auto section=mission->Section("A.");CHECK(section && section->find("User scenario")!=std::string::npos);const auto image=mission->ReadFortImage(mission->FortName());CHECK(image==changed);
    const auto fort=FortTemplate::Parse(image,scene.assets.game.TypeTable());const auto players=MissionPlayers::Load([&](auto key) { return mission->Get(key); },fort,scene.assets.game.TypeTable(),6500);
    CHECK(players.players[1].stormPower==12345 && players.players[2].stormPower==987);CHECK(players.players[2].active && players.players[2].name=="Custom Wind");
    CHECK(players.players[2].ability=="!BUILD_DAIS" && players.Allied(1,2) && !players.players[1].knowledge.empty());
    mission->Set("loadFort","NotTheCapturedRequest");CHECK(mission->ReadFortImage("TEST01")==changed); //이미 선택한 Init 요청을 이후 loadFort 변경으로 바꾸지 않는다.
    CHECK(scene.config.Reader()("d/TEST01.fort")==std::optional(original));CHECK(scene.assets.files.Read("d/TEST01.fort")==original); //전역 reader와 보호 원본은 그대로다.
    ReleaseGuideMissionScript(mission,mode,scene.config);CHECK(!mission && !mode && !scene.config.GetInt("global.inMission"));
}

TEST_CASE(MissionFileSource_CompletedMissingNeverResurrectsOriginalScriptOrFort) { //작업본 삭제가 정상 원본 fallback으로 바뀌지 않으며 현재 재작성은 다시 읽는다.
    MissionFileScene scene("missing");const auto original=scene.assets.files.Read("d/TEST01.fort");
    CHECK(scene.workspace->Read("d/TEST01.english").has_value());CHECK(scene.workspace->Read("d/TEST01.fort")==std::optional(original));
    {
        MissionScript mission(scene.config,"TEST01",{},scene.Reader());CHECK(mission.Loaded());scene.files->Delete("d/TEST01.fort");
        CHECK(FileFailure([&] { static_cast<void>(mission.ReadFortImage("TEST01")); })=="Game file not found: d/TEST01.fort");
        CHECK(scene.assets.files.TryRead("d/TEST01.fort").has_value());scene.files->Write("d/TEST01.fort",original);CHECK(mission.ReadFortImage("TEST01")==original);
    }
    scene.files->Delete("d/TEST01.english");MissionScript missing(scene.config,"TEST01",{},scene.Reader());CHECK(!missing.Loaded());
    CHECK(scene.config.Reader()("d/TEST01.english").has_value()); //명시 미션 source의 부재를 전역 자산 reader로 우회하지 않는다.
}

TEST_CASE(MissionFileSource_OwnsWorkspaceUntilActualGuideRelease) { //편집 외부 소유자가 사라진 뒤에도 같은 미션이 실제 파일 장치/잠금을 보유한다.
    MissionFileScene scene("lifetime");std::weak_ptr<EditorCatalogWorkspace> borrowed=scene.workspace;bool mode=false;
    auto mission=CreateGuideMissionScript(scene.config,"TEST01",mode,scene.Reader());CHECK(mission->Loaded());scene.workspace.reset();CHECK(!borrowed.expired());
    CHECK(mission->ReadFortImage("TEST01")==scene.assets.files.Read("d/TEST01.fort"));
    bool notified=false;scene.config.onChanged=[&] { if (!scene.config.GetInt("global.inMission")) { CHECK(borrowed.expired() && !mode);notified=true; } }; //실제 Script/reader 반납 뒤 Native0/Config0 통지가 이어진다.
    ReleaseGuideMissionScript(mission,mode,scene.config);CHECK(!mission && borrowed.expired() && notified);scene.config.onChanged={};
}

TEST_CASE(MissionFileSource_CurrentLanguageAndNormalMissionReaderStayIndependent) { //언어별 사용자 파일은 원래 missionSpec 치환을 쓰고 일반 캠페인 읽기는 원본으로 남는다.
    MissionFileScene scene("language");scene.Text("d/TEST01.korean","[Header]\nmissionType=\"Tutorial\"\ntitle=\"KoreanScenario\"\nmyStartMoney=22\n[A.]\nKorean scenario briefing\n[END]\n");
    scene.config.Set("currentLanguage","korean");
    { MissionScript mission(scene.config,"TEST01",{},scene.Reader());CHECK(mission.Loaded() && mission.ScriptPath()=="d/TEST01.korean");CHECK(mission.Get("title")==std::optional<std::string>("KoreanScenario")); }
    scene.config.Set("currentLanguage","english");
    { MissionScript normal(scene.config,"TEST01");CHECK(normal.Loaded() && normal.Get("myStartMoney")==std::optional<std::string>("50000"));CHECK(normal.ReadFortImage("TEST01")==scene.assets.files.Read("d/TEST01.fort")); }
    { MissionScript working(scene.config,"TEST01",{},scene.Reader());CHECK(working.Loaded() && working.ScriptPath()=="d/TEST01.english"); }
}

TEST_CASE(MissionFileSource_ExplicitReaderMissingEmptyAndThrowRemainDistinct) { //명시 source를 선택하면 추가 설정 파일도 같은 장치에 전달하고 실패를 성공으로 바꾸지 않는다.
    ConfigInterface config([](auto)->std::optional<std::vector<std::uint8_t>> { throw std::logic_error("전역 reader 우회 호출"); });
    config.Configuration().Assign("missionSpec=\"d/{local.1}.english\"\nfortSpec=\"d/{local.1}.fort\"\n[END]\n");
    std::vector<std::string> reads;const auto source=[&](std::string_view path)->std::optional<std::vector<std::uint8_t>> {
        reads.emplace_back(path);if (path=="@d/extra.english") { const std::string text="[Header]\nextraOnly=17\n[END]\n";return std::vector<std::uint8_t>(text.begin(),text.end()); }
        if (path=="d/Only.english") { const std::string text="[Header]\nmissionType=\"Tutorial\"\n[END]\n";return std::vector<std::uint8_t>(text.begin(),text.end()); }
        if (path=="d/empty.fort") return std::vector<std::uint8_t>{};if (path=="d/throw.fort") throw std::runtime_error("explicit I/O failure");return std::nullopt;
    }; //경로 입력으로만 명시 결과를 공급하며 기존 Config parser는 그대로 실행한다.
    { MissionScript mission(config,"Only","@d/extra.english",source);CHECK(mission.Loaded());CHECK(reads.size()>=2 && reads.front()=="@d/extra.english" && reads.back()=="d/Only.english");
      CHECK(mission.ReadFortImage("empty").empty());CHECK(FileFailure([&] { static_cast<void>(mission.ReadFortImage("missing")); })=="Game file not found: d/missing.fort");
      CHECK(FileFailure([&] { static_cast<void>(mission.ReadFortImage("throw")); })=="explicit I/O failure"); }
    bool mode=false;CHECK(!FileFailure([&] { static_cast<void>(CreateGuideMissionScript(config,"Only",mode,[](auto)->std::optional<std::vector<std::uint8_t>> { throw std::runtime_error("reader failure"); })); }).empty());
    CHECK(!mode && !config.GetInt("global.inMission"));
}
