// 실제 파일을 쓰거나 게임/AI/통신을 시작하지 않는 시나리오 문서·설정 미리보기 검사다.
#include "client/ScenarioDocument.h"
#include "client/EditorScenarioModel.h"
#include "o/BaseFile.h"
#include "o/OriginalText.h" // EncodeConfigFile은 UTF-8 입력이므로 CP1252 시험 바이트를 먼저 UTF-8로 풀어 넣는다(S1 시험 교정, 1277).
#include "o/Config.h"
#include "o/ConfigInterface.h"
#include "TestSupport.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace { // 문서 입력과 기대값을 이 일반 검사 번역 단위에 한정한다.
using namespace netstorm::client; // 신규 순수 문서와 편집 모델이다.
using namespace netstorm::o; // 기존 Config/원본 읽기만 재사용한다.
// 임의 바이트 입력을 파일 I/O 없이 원문 문서에 전달한다.
ScenarioDocument Plain(std::string_view text) { return ScenarioDocument::FromBytes({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 실패 경계가 조용한 성공으로 처리되지 않는지 확인한다.
template<class Function> bool Rejects(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; }
// 동일한 격리 Config로 치환 결과를 읽으며 현재 게임 등록부에는 접근하지 않는다.
std::string Get(const ScenarioDocument& document,std::string_view key) {
    ConfigRegistry registry;Config mission(registry,"mission");mission.Assign(document.Text());return mission.Get("mission."+std::string(key)).value_or("");
}
}

// 암호화·CP1252·줄바꿈·BOM을 다시 쓰지 않고 입력 그대로 직렬화한다.
TEST_CASE(ScenarioDocument_UnchangedPreservesBytesAndOriginalEncoding) {
    const std::string cp="[Header]\r\ntitle=\"caf\xe9\" // raw\n[Repeat]\n$Text={mission.title}\r\n";
    const auto source=Plain(cp);CHECK(!source.Changed());CHECK(source.Encoding()==ScenarioDocumentEncoding::Windows1252);CHECK(source.Serialize()==std::vector<std::uint8_t>(cp.begin(),cp.end()));CHECK(Get(source,"title")=="café");
    const auto encrypted=EncodeConfigFile("mQdsT\r\n"+DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(cp.data()),cp.size()}));const auto original=ScenarioDocument::FromBytes(encrypted);CHECK(!original.Changed());CHECK(original.Encoding()==ScenarioDocumentEncoding::EncodedWindows1252);CHECK(original.Serialize()==encrypted);CHECK(original.Raw("title")==source.Raw("title"));
    const auto unicode=ScenarioDocument::FromUtf8("[Header]\nTitle=\"한글 😀\"\n");const auto bytes=unicode.Serialize();const auto reopened=ScenarioDocument::FromBytes(bytes);CHECK(reopened.Serialize()==bytes);CHECK(reopened.Encoding()==ScenarioDocumentEncoding::Utf8Bom);CHECK(Get(reopened,"title")=="한글 😀");
}

// 첫 실제 키의 값 구간만 바뀌며 반복·AI·브리핑·주석은 바이트 수준으로 동일한 본문에 남는다.
TEST_CASE(ScenarioDocument_ChangesFirstValueOnlyAndPreservesUnknownScript) {
    const std::string input="// unknown comment\r\n[Header]\r\n  MyStartMoney \t==  1000  // keep me\r\nmyStartMoney=999\r\nunknownKey={mission.ai2Tech|sunWalker}\r\nai2Tech=\"{someGlobal}\"\r\n[A.]\r\n<h1>Briefing</h1>\r\n$while={something}\r\n$while={again}\r\n[Repeat]\r\nmyStartMoney=123\r\n";
    auto document=Plain(input);CHECK(document.SetLiteral("myStartMoney","2000"));auto expected=input;expected.replace(expected.find("1000"),4,"\"2000\"");CHECK(document.Text()==expected);CHECK(document.Raw("myStartMoney")=="2000  ");CHECK(Get(document,"myStartMoney")=="2000");CHECK(document.Changed());
    const auto bytes=document.Serialize();CHECK(bytes.size()>=3 && bytes[0]==0xef && bytes[1]==0xbb && bytes[2]==0xbf);CHECK(ScenarioDocument::FromBytes(bytes).Text()==expected);CHECK(!document.SetLiteral("mystartmoney","2000"));
}

// Header 밖 첫 정의와 LF 전용 줄 이동도 기존 Config의 우선순위를 따른다.
TEST_CASE(ScenarioDocument_UsesActualFirstDefinitionAndHeaderInsertion) {
    auto outside=Plain("[A.]\nmyTech=all\n[Header]\nmyTech=sunWalker\n");CHECK(outside.SetLiteral("myTech","windBattery"));CHECK(outside.Text()=="[A.]\nmyTech=\"windBattery\"\n[Header]\nmyTech=sunWalker\n");
    auto document=Plain("[Header]\nunknown=1\n[Header]\nunchanged=2\n[A.]\nbody\n");CHECK(document.SetLiteral("myStartMoney","5000"));CHECK(document.Text()=="[Header]\nunknown=1\nmyStartMoney=\"5000\"\n[Header]\nunchanged=2\n[A.]\nbody\n");
    auto cr=Plain("ignored=1\rmyStartMoney=7\n[A.]\ntext");CHECK(!cr.Raw("myStartMoney"));CHECK(cr.SetLiteral("myStartMoney","9"));CHECK(Get(cr,"myStartMoney")=="9");CHECK(cr.Text().find("ignored=1\rmyStartMoney=7")==0);
}

// literal을 입력하면 quote·백틱·주석·매크로 글자가 대본 명령으로 바뀌지 않는다.
TEST_CASE(ScenarioDocument_LiteralEscapesRoundTripIncludingFinalBacktick) {
    auto document=Plain("[Header]\ntitle=old // comment\n[A.]\nbody\n");
    const std::array<std::string_view,5> values{{"한글/é 😀","quote \" // {mission.missing}","path`","two``","`\"{x}//"}};
    for (const auto value:values) { // 실제 Config 치환까지 확인하며 단순 serializer 자기 비교에 그치지 않는다.
        CHECK(document.SetLiteral("title",value));CHECK(Get(document,"title")==value);CHECK(document.Text().find("// comment\n[A.]\nbody\n")!=std::string::npos);const auto saved=document.Serialize();CHECK(Get(ScenarioDocument::FromBytes(saved),"title")==value);
    }
}

// 손상 입력을 기본 문서로 대체하지 않으며 관련 없는 기존 특수 대본은 보존한다.
TEST_CASE(ScenarioDocument_RejectsDamagedInputAndSelectedMalformedValue) {
    CHECK(Rejects([] { static_cast<void>(Plain(std::string("a=1\0hidden=2",12))); }));
    const std::array<std::string,5> invalid{{std::string("\xef\xbb\xbf\xc0\xaf",5),std::string("\xef\xbb\xbf\xed\xa0\x80",6),std::string("\xef\xbb\xbf\xf4\x90\x80\x80",7),std::string("\xef\xbb\xbf\xe2\x82",5),std::string("\0x\0",3)}};
    for (const auto& bytes:invalid) CHECK(Rejects([&] { static_cast<void>(Plain(bytes)); })); // 잘못된 UTF-8 또는 암호 서명을 명시 실패시킨다.
    auto document=Plain("[Header]\nmyTech=\"unfinished\nunknown={external}\n[A.]\n$while=...\n");const auto before=document.Serialize();CHECK(Rejects([&] { document.SetLiteral("myTech","all"); }));CHECK(document.Serialize()==before);CHECK(document.SetLiteral("myStartMoney","3"));CHECK(document.Text().find("myTech=\"unfinished\nunknown={external}")!=std::string::npos);
    CHECK(Rejects([&] { document.SetLiteral("x\ncommand","1"); }));CHECK(Rejects([&] { document.SetLiteral("title","x\n$command"); }));
}

// 새 기본은 실제 Config 문법으로 읽히며 현재 맵 이름 fallback을 위해 loadFort를 만들지 않는다.
TEST_CASE(ScenarioDocument_DefaultScenarioContainsFiniteSafeStartingFields) {
    const auto document=CreateDefaultEditorScenario("새 맵 <A>&{literal}`");CHECK(Get(document,"missionType")=="Tutorial");CHECK(Get(document,"tutorialNumber")=="0");CHECK(Get(document,"myStartMoney")=="5000");CHECK(Get(document,"myTech")=="all");CHECK(Get(document,"title")=="새 맵 <A>&{literal}`");CHECK(!document.Raw("loadFort"));CHECK(!document.Raw("ai2Name"));CHECK(document.Text().find("$Button=Go!,DoNothing,0")!=std::string::npos);
    ConfigRegistry registry;Config config(registry);config.LoadBytes(document.Serialize());CHECK(config.Get("title")=="새 맵 <A>&{literal}`");const auto briefing=config.Section("A.");CHECK(briefing.has_value());if (briefing) CHECK(config.Substitute(*briefing).find("<h2>새 맵 &lt;A&gt;&amp;{literal}`</h2>")!=std::string::npos);CHECK(!config.Section("Succeeded"));
}

// 600BYTE 순수 설정 계산만 확인하며 AI 생성/RawPlayerParticipation/멀티 부모는 호출하지 않는다.
TEST_CASE(EditorScenarioModel_ViewsDefaultCommonOwnerEmptyAndExplicitZero) {
    EditorScenarioContext context;context.localOwner=3;context.startingSp=100;context.stuffRefreshRate=9;
    EditorScenarioModel model(Plain("[Header]\nmyStartMoney=4294967295tail\nmyTech=\"sunWalker;windBattery\"\naiStartMoney=8000\nai2StartMoney=\"\"\naiOff=1\nai2Off=0\naiTech=all\nai2Tech=\"\"\naiColor=red\nai2Color=none\naiAllyList=\"1;3\"\nai2AllyList=\"4\"\naiAbility=\"!BUILD_BRIDGES\"\nai2Ability=\"BUILD_BRIDGES\"\n"),context);
    const auto player=model.Player();CHECK(player.owner==3);CHECK(player.startingMoney==-1.0F);CHECK(player.technology.effective=="sunWalker;windBattery");
    const auto view=model.Ai(2);CHECK(view.fields.at("StartMoney").raw==std::optional<std::string>(""));CHECK(view.fields.at("StartMoney").effective=="8000");CHECK(view.fields.at("StartMoney").origin==ScenarioValueOrigin::CommonAi);CHECK(view.fields.at("Off").effective=="0");CHECK(view.fields.at("Off").origin==ScenarioValueOrigin::OwnerAi);CHECK(view.fields.at("Tech").effective=="all");CHECK(view.fields.at("Color").effective=="red");CHECK(view.fields.at("Color").origin==ScenarioValueOrigin::CommonAi);CHECK(view.fields.at("StuffRefreshRate").effective=="9.000000");CHECK(view.allyMask==((1U<<1)|(1U<<3)|(1U<<4)));CHECK(view.fields.at("AllyList").origin==ScenarioValueOrigin::CombinedAi);CHECK((view.abilityMask&(1U<<12))!=0);CHECK(!model.Document().Changed());
    CHECK(model.InheritAi(2,"Off"));CHECK(model.Ai(2).fields.at("Off").effective=="1");CHECK(Rejects([&] { static_cast<void>(model.Ai(9)); }));
    const EditorScenarioModel empty(Plain("[Header]\n"),context);const auto defaults=empty.Ai(2);CHECK(defaults.fields.at("StartMoney").effective=="4100");CHECK(defaults.fields.at("Tech").effective=="sunWalker;windBattery;sunCannon;sunBalloon;sunBlocker;sunArcher;windArcher");CHECK(defaults.abilityMask==0xffff);CHECK(defaults.allyMask==0);
}

// 미지 능력/미지 키/동적 값은 원문에 남고 외부 resolver는 staged 문서를 받는다.
TEST_CASE(EditorScenarioModel_PreservesLegacyAndUsesExplicitStagedResolver) {
    auto input=Plain("[Header]\nmyStartMoney={external.money|23}\nmyAllyList=2\naiAbility=UNKNOWN_LEGACY\nai2Tech={external.tech|all}\n[A.]\n$while={external.repeat}\n");const auto original=input.Serialize();EditorScenarioModel isolated(input);CHECK(isolated.Field("myStartMoney").dynamic);CHECK(isolated.Player().startingMoney==23.0F);CHECK(!isolated.Ai(2).diagnostics.empty());CHECK(isolated.Document().Serialize()==original);
    EditorScenarioContext context;context.resolve=[](const ScenarioDocument& document,std::string_view key) { // 외부 문맥도 staged 첫 값의 읽기 규칙을 명시 적용한다.
        if (key=="myStartMoney" && document.Raw(key)==std::optional<std::string>("{external.money|23}")) return std::string("7000");return Get(document,key);
    };
    EditorScenarioModel model(input,context);CHECK(model.Player().startingMoney==7000.0F);CHECK(model.Set("myStartMoney","9000"));CHECK(model.Player().startingMoney==9000.0F);CHECK(model.Document().Text().find("myAllyList=2\naiAbility=UNKNOWN_LEGACY\nai2Tech={external.tech|all}")!=std::string::npos);
}

// 실제 지원 키와 새 입력만 검증하고 unknown legacy를 덮어쓰거나 추측 타입을 만들지 않는다.
TEST_CASE(EditorScenarioModel_ValidatesOnlyRequestedSupportedChanges) {
    EditorScenarioContext context;context.knownTechnology=[](std::string_view name) { return name=="sunWalker" || name=="windBattery"; }; // 현재 타입 표를 빌릴 위치다.
    EditorScenarioModel model(Plain("[Header]\nunknown={x}\nmyTech=OLD_UNKNOWN\n"),context);
    CHECK(model.Set("myStartMoney","+2147483647"));CHECK(model.Set("myTech","sunWalker;windBattery"));CHECK(model.Set("techAllowed","deny;all;allow;sunWalker"));CHECK(model.Set("ai2Name","Enemy"));CHECK(model.Set("ai2Color","blue"));CHECK(model.Set("ai2TimeBetweenMoves","1.5"));CHECK(model.Set("ai2Ability","!BUILD_BRIDGES"));
    const auto valid=model.Document().Serialize();CHECK(Rejects([&] { model.Set("myStartMoney","2147483648"); }));CHECK(Rejects([&] { model.Set("myStartMoney","+-1"); }));CHECK(Rejects([&] { model.Set("myTech","notARealType"); }));CHECK(Rejects([&] { model.Set("myTech","all;;sunWalker"); }));CHECK(Rejects([&] { model.Set("ai2Name",std::string(30,'n')); }));CHECK(Rejects([&] { model.Set("ai2Color","cyan"); }));CHECK(Rejects([&] { model.Set("ai2TimeBetweenMoves","inf"); }));CHECK(Rejects([&] { model.Set("ai2AllyList","9"); }));CHECK(Rejects([&] { model.Set("ai2Ability","GUESS_ABILITY"); }));CHECK(Rejects([&] { model.Set("unknown","1"); }));CHECK(Rejects([&] { model.Set("myAllyList","2"); }));CHECK(model.Document().Serialize()==valid);
}

// 전체 원문 전수를 주장하지 않는 선정32 입력의 왕복 identity이며 원본은 읽기만 한다.
TEST_CASE(ScenarioDocument_Selected32OriginalScriptsRoundTripIdentity) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");
    const std::array<std::string_view,32> names{{"TEST01","Duel","BC3Acid_Bar","Tutorial1","Tutorial2","Tutorial3","Tutorial4","Tutorial5","Tutorial6","TheWarBegins","SaveTheIsland","MasterOfWhirligigs","BreakingThrough","EnemyTerritory","FinalConfrontation","FragileFortune","RunForIt","ThunderingPower","DissolvedAlliance","GuardMyBack","DemoStormRages","BC1Menu","BC2Bridging","BC4Bow_Rush","BC5Walkers","BC8Mows","BC9Sun_Bars","BCSSun_Bars","BCFury","GuideBattle","GuideFW1","TpVsetup"}};
    for (const auto name:names) { // loose/TAFF를 같은 기존 VFS로 읽되 실제 게임/대본 명령은 실행하지 않는다.
        const auto bytes=files.Read("d/"+std::string(name)+".english");const auto document=ScenarioDocument::FromBytes(bytes);CHECK(document.Serialize()==bytes);CHECK(!document.Changed());CHECK(document.Text().find('[')!=std::string::npos);EditorScenarioModel model(document);CHECK(model.Document().Serialize()==bytes);
    }
}
