// 창/게임/통신 없이 실제 보존형 문서/모델/파일 정책/패널 동작을 대조한다.
#include "TestSupport.h"
#include "client/EditorScenarioController.h"
#include "client/DisplayTextInput.h"
#include "client/RawGameMenus.h" // 원문 기본 메뉴와 명시 Scenario 확장 유무를 확인한다.
#include <map>
#include <memory>
#include <set>
#include <stdexcept>

using namespace netstorm::client;
namespace { // 디스크 대신 명시 입력/실제 저장 바이트를 소유한다.
// 실제 파일 장치의 성공/부재/삭제/쓰기 실패를 독립적으로 제공한다.
struct ScenarioMemory {
    std::map<std::string,std::vector<std::uint8_t>> work,archive; // 사용자/원본 TAFF 내용을 구별하며 archive에는 쓰지 않는다.
    std::set<std::string> deleted; // 완료 이관 뒤 삭제된 경로다.
    std::size_t writes{}; // 실제 성공한 사용자 쓰기 횟수다.
    bool failWrite{},korean{}; // 명시 실패와 현재 요청 언어다.
    // 실제 정책과 같은 장치를 제공한다. 반환값으로 기대 결과를 만들지 않는다.
    EditorScenarioFileHooks Hooks() {
        EditorScenarioFileHooks hooks;
        hooks.read=[this](auto path)->std::optional<std::vector<std::uint8_t>> { const std::string key(path);if (const auto at=work.find(key);at!=work.end()) return at->second;if (deleted.contains(key)) return std::nullopt;if (const auto at=archive.find(key);at!=archive.end()) return at->second;return std::nullopt; };
        hooks.deleted=[this](auto path) { return deleted.contains(std::string(path)); };
        hooks.write=[this](auto path,auto bytes) { if (failWrite) throw std::runtime_error("명시 atomic 실패");work[std::string(path)]={bytes.begin(),bytes.end()};++writes; };
        hooks.paths=[this](auto map) { return EditorScenarioPaths{"D/"+std::string(map)+(korean ? ".korean" : ".english"),"D/"+std::string(map)+".english"}; };return hooks;
    }
    // 명시 원문 대본을 BOM UTF-8 파일로 제공한다.
    void Put(std::string name,std::string text,bool source=false) { (source ? archive : work)[std::move(name)]=ScenarioDocument::FromUtf8(std::move(text)).Serialize(); }
};
// 새 UI가 실제 기술 표에 이름을 물어보는 경계를 제공한다.
EditorScenarioContext Context() { EditorScenarioContext context;context.knownTechnology=[](auto name) { return name=="Priest" || name=="Temple"; };return context; }
}

// 신규 부재만 기본 생성하고 기존 원문/TAFF/사용자 파일을 자동 변환하지 않는다.
TEST_CASE(EditorScenarioIntegration_DefaultEnglishAndTombstone) {
    ScenarioMemory memory;EditorScenarioFiles files(memory.Hooks());CHECK(files.EnsureEnglish("NEW01"));CHECK(memory.writes==1);const auto saved=memory.work.at("D/NEW01.english");CHECK(saved[0]==0xef);
    auto opened=files.Open("NEW01");CHECK(opened.document.Raw("myStartMoney")=="5000");CHECK(opened.document.Raw("myTech")=="all");CHECK(opened.document.Section("A.").has_value());CHECK(files.EnsureEnglish("NEW01"));CHECK(memory.writes==1 && memory.work.at("D/NEW01.english")==saved);
    memory.Put("D/OLD.english","[Header]\nmyStartMoney=123\n[A.]\nold\n",true);const auto source=memory.archive;CHECK(files.EnsureEnglish("OLD"));CHECK(memory.writes==1 && memory.archive==source);
    memory.deleted.insert("D/OLD.english");CHECK(!files.EnsureEnglish("OLD"));CHECK(memory.writes==1 && memory.archive==source);auto deleted=files.Open("OLD");CHECK(deleted.deleted && !deleted.baseline);
    bool refused{};try { files.Save(deleted,deleted.document,false); }catch (const std::runtime_error&) { refused=true; }CHECK(refused);files.Save(deleted,deleted.document,true);CHECK(memory.writes==2 && !deleted.deleted);
}

// 현재 언어 override가 실효 편집 파일이며 없는 언어만 같은 English로 돌아간다.
TEST_CASE(EditorScenarioIntegration_LanguageOverrideAndMissionReload) {
    ScenarioMemory memory;memory.korean=true;memory.Put("D/MAP.english","[Header]\nmyStartMoney=10\n[A.]\nEnglish\n");auto files=std::make_shared<EditorScenarioFiles>(memory.Hooks());
    CHECK(files->ReadForMission("MAP","D/MAP.korean")==memory.work.at("D/MAP.english"));CHECK(files->Open("MAP").path=="D/MAP.english");CHECK(!files->ReadForMission("MAP","D/MAP.fort"));
    memory.Put("D/MAP.korean","[Header]\nmyStartMoney=20\n[A.]\n한국어\n");auto opened=files->Open("MAP");CHECK(opened.path=="D/MAP.korean");auto changed=opened.document;changed.SetLiteral("myStartMoney","300");files->Save(opened,changed,true);
    const auto reloaded=files->ReadForMission("MAP","D/MAP.korean");CHECK(reloaded.has_value());CHECK(ScenarioDocument::FromBytes(*reloaded).Raw("myStartMoney")=="300");CHECK(ScenarioDocument::FromBytes(memory.work.at("D/MAP.english")).Raw("myStartMoney")=="10");
}

// atomic 실패와 외부 변경은 baseline/보류 문서/이전 파일을 그대로 보존한다.
TEST_CASE(EditorScenarioIntegration_WriteFailureAndConcurrentChange) {
    ScenarioMemory memory;memory.Put("D/MAP.english","[Header]\nmyStartMoney=7\n[A.]\nkeep\n");EditorScenarioFiles files(memory.Hooks());auto opened=files.Open("MAP");auto changed=opened.document;changed.SetLiteral("myStartMoney","9");const auto baseline=opened.baseline;
    memory.failWrite=true;bool failed{};try { files.Save(opened,changed,true); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && opened.baseline==baseline && changed.Changed());memory.failWrite=false;
    memory.Put("D/MAP.english","[Header]\nmyStartMoney=99\n");failed=false;try { files.Save(opened,changed,true); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && opened.baseline==baseline && memory.writes==0);
}

// Save as는 현재/English 두 파일을 보존 복사하고 충돌은 첫 쓰기 전에 거부한다.
TEST_CASE(EditorScenarioIntegration_SaveAsCopiesWithoutChangingScripts) {
    ScenarioMemory memory;memory.korean=true;memory.Put("D/OLD.english","[Header]\nloadFort=OTHER\nunknown=keep\n[A.]\nold\n");memory.Put("D/OLD.korean","[Header]\ntitle=기존\n[A.]\n이전\n");EditorScenarioFiles files(memory.Hooks());
    files.CopyAs("OLD","NEW");CHECK(memory.work.at("D/NEW.english")==memory.work.at("D/OLD.english"));CHECK(memory.work.at("D/NEW.korean")==memory.work.at("D/OLD.korean"));CHECK(memory.writes==2);
    files.CopyAs("OLD","NEW");CHECK(memory.writes==2);memory.Put("D/OTHER.korean","conflict");bool conflict{};try { files.CopyAs("OLD","OTHER"); }catch (const std::runtime_error&) { conflict=true; }CHECK(conflict && memory.writes==2 && !memory.work.contains("D/OTHER.english"));
}

// 실제 Config 사본은 간접 missionSpec/local 인자를 해석하면서 현재 Registry/언어를 바꾸지 않는다.
TEST_CASE(EditorScenarioIntegration_ActualConfigSnapshotAndStagedValues) {
    netstorm::o::ConfigInterface config(netstorm::o::ConfigInterface::FileReader{});config.Configuration().Assign("DataDir=D\ncurrentLanguage=korean\nwhere={DataDir}/{local.1}.{currentLanguage}\nmissionSpec={where}\nbase=42\n");const auto text=config.Configuration().Text();const auto count=config.Registry().Objects().size();
    const auto paths=ResolveEditorScenarioPaths(config,"MAP");CHECK(paths.requested=="D/MAP.korean" && paths.english=="D/MAP.english");CHECK(config.Configuration().Text()==text && config.Registry().Objects().size()==count);
    auto document=ScenarioDocument::FromUtf8("[Header]\nmyStartMoney={base}\n");CHECK(ResolveEditorScenarioField(config,document,"myStartMoney")=="42");document.SetLiteral("myStartMoney","500");CHECK(ResolveEditorScenarioField(config,document,"myStartMoney")=="500");CHECK(config.Configuration().Text()==text && config.Registry().Objects().size()==count);
}

// 새 AI 이름은 한글4BYTE/29BYTE 상한이며 실제 기존 settings 소비자의 실효값을 읽는다.
TEST_CASE(EditorScenarioIntegration_KoreanAiNameAndActualSettings) {
    EditorScenarioModel model(CreateDefaultEditorScenario("MAP"),Context());model.Set("ai2Name","가나다라마바사");model.Set("ai2StartMoney","6500");model.Set("ai2Tech","Priest;Temple");model.Set("ai2AllyList","1;3");model.Set("ai2Off","0");const auto ai=model.Ai(2);
    CHECK(ai.fields.at("Name").effective=="가나다라마바사");CHECK(ai.fields.at("StartMoney").effective=="6500");CHECK(ai.fields.at("Tech").effective=="Priest;Temple");CHECK(ai.allyMask==10);
    bool rejected{};try { model.Set("ai2Name","가나다라마바사아"); }catch (const std::length_error&) { rejected=true; }CHECK(rejected);CHECK(model.Ai(2).fields.at("Name").effective=="가나다라마바사");
}

// 패널 필드 ID가 실효 문서에 반영되고 저장/새 열기에서 동일하게 읽힌다.
TEST_CASE(EditorScenarioIntegration_ControllerFieldsBriefingAndReload) {
    ScenarioMemory memory;auto files=std::make_shared<EditorScenarioFiles>(memory.Hooks());EditorScenarioController controller(files,"MAP",Context());CHECK(controller.Dirty());const auto page=controller.Page(0,[](auto owner) { return owner==2; });CHECK(page.size()<40);CHECK(controller.Edit(4).key=="myStartMoney");
    controller.Apply(4,"9876");controller.Apply(5,"Priest;Temple");controller.Apply(2*32,"한글 AI");controller.Apply(2*32+5,"1;3");controller.Apply(0x1000,"<h2>한글 안내</h2>\n$Button=Go!,DoNothing,0\n");controller.Save(true);CHECK(!controller.Dirty());
    const auto opened=files->Open("MAP");EditorScenarioModel model(opened.document,Context());CHECK(model.Player().startingMoney==9876.0F);CHECK(model.Ai(2).fields.at("Name").effective=="한글 AI");CHECK(model.Ai(2).allyMask==10);CHECK(model.Document().Section("A.")->find("한글 안내")!=std::string::npos);
    CHECK(controller.Edit(0x1000).multiline && controller.Edit(0x1000).maximum==1000);
}

// 브리핑 편집은 다른 섹션/미지 대본을 보존하고 섹션 삽입·길이 초과는 거부한다.
TEST_CASE(EditorScenarioIntegration_BriefingPreservesUnselectedScript) {
    auto document=ScenarioDocument::FromUtf8("[Header]\nunknown={script}\n[A.]\nold\n[Won.]\nDoNotChange\n[END]\n");document.SetSection("A.","새 안내\n$Button=Go!,DoNothing,0\n");CHECK(document.Text().find("[Won.]\nDoNotChange\n[END]")!=std::string::npos);CHECK(document.Raw("unknown")=="{script}");
    const auto saved=document.Serialize();bool rejected{};try { document.SetSection("A.","[Won.]\nnew"); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected && document.Serialize()==saved);
    auto missing=ScenarioDocument::FromUtf8("[Header]\nunknown=keep\n[END]\n");missing.SetSection("A.","brief\n");CHECK(missing.Section("A.")=="brief\n");CHECK(missing.Text().ends_with("[END]\n"));
    ScenarioMemory memory;EditorScenarioController controller(std::make_shared<EditorScenarioFiles>(memory.Hooks()),"MAP",Context());rejected=false;try { controller.Apply(0x1000,std::string(1000,'x')); }catch (const std::length_error&) { rejected=true; }CHECK(rejected);
}

// 기본 훅은 원문 Edit행 그대로이고 실제 클론에서만 Scenario 일반 명령이 추가된다.
// 메뉴 명령 관문이 허용하는 Scenario 확장 명령 범위가 정확한지 검사한다. 원문 명령(0x0300e0ff, 0x0300e106)은 거부하고 여섯 명령은 모두 허용해야 한다. S1 실제 확인에서 Scenario 행 클릭이 미연결 명령으로 거부되던 결함의 회귀 검사다.
TEST_CASE(EditorScenarioIntegration_CommandGateAcceptsOnlyExtensionRange) {
    CHECK(!IsEditorScenarioCommand(0x0300e0ffU));
    for (auto code=kEditorScenarioOpen;code<=kEditorScenarioInherit;++code) CHECK(IsEditorScenarioCommand(code)); // 열기부터 상속까지 연속 여섯 명령이다.
    CHECK(!IsEditorScenarioCommand(0x0300e106U));
}
TEST_CASE(EditorScenarioIntegration_EditMenuExtensionIsExplicit) {
    GameMenusHooks hooks;std::vector<std::pair<std::string,std::uint32_t>> rows;std::uint32_t next{};
    hooks.vertical=[] {};hooks.global=[](auto address) { return address; };hooks.translate=[](auto value) { return std::string(value); };hooks.take=[&] { return ++next; };hooks.submenu=[](auto,auto,auto,auto) {};
    hooks.item=[&](auto label,auto command,const auto&,auto,auto) { rows.emplace_back(label,command); };
    RawGameMenus(hooks).Edit();CHECK(rows.size()==5);CHECK(rows[0].first=="Save as...");rows.clear();hooks.scenarioCommand=kEditorScenarioOpen;RawGameMenus(hooks).Edit();CHECK(rows.size()==6 && rows[2].first=="Scenario..." && rows[2].second==kEditorScenarioOpen);
}
