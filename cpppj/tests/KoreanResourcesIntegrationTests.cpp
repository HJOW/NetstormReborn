// 클론한국어43본문/표1개와실제Config/미션/HTML경계를창·게임·MP없이대조한다.
#include "TestSupport.h"
#include "client/KoreanResources.h"
#include "client/ConfigFileRead.h"
#include "client/DisplayTextInput.h"
#include "client/Mission.h"
#include "client/HtmlTextParser.h"
#include "client/HtmlWindowFactory.h"
#include "client/HtmlConditionEffect.h" // 실제 Help와 같은 condition 효과를 검사 파서에 연결한다(K1).
#include "client/LanguageOptions.h"
#include "client/ProductPresentation.h" // 11.01 패치 크레딧 한국어 항목 시험(K4).
#include "client/QuickHelpHtml.h"
#include "client/TellFileMenus.h"
#include "o/BaseFile.h"
#include "o/OriginalText.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <stdexcept>

namespace { // 제품경계에넣을실제파일과명시표시효과만준비하며원문기대값은수정하지않는다.
// 제품에적용된검사소스기준으로저장소를찾는다. pending자체에서는빌드하지않는다.
std::filesystem::path KoreanRepo() { return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path(); }
// 각검사는고정된별도작업폴더만쓰며원본/번역자산에는쓰지않는다.
std::filesystem::path KoreanWork(std::string_view label) {
    const auto directory=KoreanRepo()/"cpppj/build/test-data"/("korean-integration-"+std::string(label)); // 검사전용작업경로다.
    std::filesystem::create_directories(directory/"d");return directory;
}
// 검사입력파일만바이트그대로기록한다. BOM/1252제어BYTE도자동변환하지않는다.
void KoreanWrite(const std::filesystem::path& path,std::string_view text) {
    std::filesystem::create_directories(path.parent_path());std::ofstream stream(path,std::ios::binary|std::ios::trunc);stream.write(text.data(),static_cast<std::streamsize>(text.size()));if (!stream) throw std::runtime_error("한국어독립입력파일쓰기실패");
}
// 기존문자모델의표시BYTE를검사에서만편하게다룬다.
std::string KoreanDisplay(std::string_view text) { const auto bytes=netstorm::o::EncodeDisplayText(text);return {bytes.begin(),bytes.end()}; }
// 원문BYTE와UTF8표시문장의경계를명시적으로반환한다.
std::string KoreanDecode(std::string_view text) { return netstorm::o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 누락과빈파일을구별하여실제읽기결과를문자열로확인한다.
std::string KoreanBytes(const std::optional<std::vector<std::uint8_t>>& bytes) { return bytes ? std::string(bytes->begin(),bytes->end()):"<missing>"; }
// 실제ClientConfig와같은NativeFILE본문입구를명시적인root/resolver로설치한다.
netstorm::o::ConfigFileLoadHooks KoreanConfigHooks(const netstorm::o::BaseFileSystem& files,const std::vector<std::filesystem::path>& roots) {
    return netstorm::client::MakeConfigFileReadHooks(files,[&files,roots](auto name) { return netstorm::client::FindBuiltinKoreanFile(files,roots,name); });
}
// 현재본문을실제전용Help/링크/그림파서로읽는다. TYPE0이므로동적정보pane은요구하지않는다.
std::string KoreanHelpBody(std::string_view source,std::string_view topic,bool extended=true) {
    using namespace netstorm::client;
    HtmlTextParserState state;HtmlTextParserGlobals globals;globals.extendedText=extended;std::uint32_t info{};HtmlLinkTextState links;links.base=0x13000000;links.enabled=true; // 같은8192BYTE링크모델과클론표시관문이다.
    netstorm::o::ConfigInterface conditionConfig([](std::string_view)->std::optional<std::vector<std::uint8_t>> { return std::nullopt; }); /* K1 컴파일 교정: 실제 Help처럼 condition 효과를 Config에 연결한다(빈 Config라 조건 키는 부재). */ HtmlTextParserHooks base;BindHtmlConditionConfig(base,conditionConfig);BindHtmlPictureText(base);BindHtmlLinkText(base,links);QuickHelpHtml parser(state,globals,info,0,topic,false,{}, {},std::move(base),&links);
    return parser.Parse(source);
}
}

TEST_CASE(KoreanIntegration_All43BuiltinBodiesResolveWithOriginalBomBytes) { // 실제번들목록과허용목록의누락/변조를공통입구에서찾는다.
    using namespace netstorm::client;const auto repo=KoreanRepo(),work=KoreanWork("all-bodies");netstorm::o::BaseFileSystem files(work);std::size_t count{}; // 원본game/쓰기경로와분리된빈VFS다.
    for (const auto& entry:std::filesystem::directory_iterator(repo/"assets/lang/korean/d")) { // 실제적용43파일을각각같은resolver로찾는다.
        if (!entry.is_regular_file() || entry.path().extension()!=".korean") continue;const auto name="d/"+entry.path().filename().string();const auto selected=FindBuiltinKoreanFile(files,{repo},name);CHECK(selected.has_value());if (!selected) continue;
        const auto actual=ReadLocalizedGameFile(files,{repo},name);CHECK(actual && *actual==netstorm::o::ReadFileBytes(entry.path()));if (actual) CHECK(actual->size()>=3 && (*actual)[0]==0xef && (*actual)[1]==0xbb && (*actual)[2]==0xbf);++count;
    }
    CHECK(count==43);
}

TEST_CASE(KoreanIntegration_PortableNamespaceAndExactNativeFileTakePriority) { // EXE옆언어번들은읽기전용게임폴더에본문을복사하지않아도동작한다.
    using namespace netstorm::client;const auto work=KoreanWork("portable"),portable=work/"portable";netstorm::o::BaseFileSystem files(work); // 경로우선권만확인하는독립입력이다.
    KoreanWrite(work/"d/help.english","ENGLISH\xe9");KoreanWrite(portable/"lang/korean/d/help.korean","\xef\xbb\xbf한국어 도움말");std::filesystem::remove(work/"d/help.korean");
    CHECK(files.ProbeExact("d/help.korean")==0 && files.Probe("d/help.korean")!=0);const auto found=FindBuiltinKoreanFile(files,{portable},"\\D\\HELP.KOREAN");CHECK(found && *found==std::filesystem::absolute(portable/"lang/korean/d/help.korean").lexically_normal());
    CHECK(KoreanBytes(ReadLocalizedGameFile(files,{portable},"d/help.korean"))=="\xef\xbb\xbf한국어 도움말");CHECK(KoreanBytes(ReadLocalizedGameFile(files,{portable},"d/help.english"))=="ENGLISH\xe9"); // 영어원시BYTE는그대로다.
    KoreanWrite(work/"d/help.korean","\xef\xbb\xbf사용자 도움말");CHECK(!FindBuiltinKoreanFile(files,{portable},"d/help.korean"));CHECK(KoreanBytes(ReadLocalizedGameFile(files,{portable},"d/help.korean"))=="\xef\xbb\xbf사용자 도움말");
}

TEST_CASE(KoreanIntegration_UnknownAbsoluteAndOtherLanguagesNeverSelectBuiltin) { // 사용자CustomMaps/절대/다른언어이름을공식번들로오인하지않는다.
    using namespace netstorm::client;const auto repo=KoreanRepo(),work=KoreanWork("filters");netstorm::o::BaseFileSystem files(work); // 검사고유경로다.
    for (const std::string name:{"d/custom.korean","d/help.english","d/help.german","help.korean","d/../help.korean","d/sub/help.korean","//server/d/help.korean","C:/CustomMaps/d/help.korean"}) CHECK(!FindBuiltinKoreanFile(files,{repo},name)); // 공식43본문과정확한논리디렉터리만허용한다.
    KoreanWrite(work/"d/custom.english","CUSTOM-EN");CHECK(KoreanBytes(ReadLocalizedGameFile(files,{repo},"d/custom.korean"))=="CUSTOM-EN"); // 미등록커스텀본문의영어대체는기존VFS가소유한다.
    KoreanWrite(work/"CustomMaps/d/help.korean","USER-ABS");const auto absolute=netstorm::platform::PathToUtf8(std::filesystem::absolute(work/"CustomMaps/d/help.korean"));CHECK(!FindBuiltinKoreanFile(files,{repo},absolute));CHECK(KoreanBytes(ReadLocalizedGameFile(files,{repo},absolute))=="USER-ABS");
}

TEST_CASE(KoreanIntegration_GuideFinderUsesKoreanLogicalNamesAndKeepsEnglishOrder) { // 영어fallback행위치와같은공식한국어행을연결해실제제목로드가한국어로간다.
    using namespace netstorm::client;const auto repo=KoreanRepo(),work=KoreanWork("finder");netstorm::o::BaseFileSystem files(work);KoreanWrite(work/"d/offical1.english","title=EN"); // 기존첫열거행이다.
    const auto english=files.MatchFinder("d\\offical*.english");CHECK(MatchLocalizedGameFiles(files,{repo},"d\\offical*.english")==english); // 영어목록순서/이름은동일하다.
    const auto korean=MatchLocalizedGameFiles(files,{repo},"d\\offical*.korean");CHECK(korean.size()==8 && netstorm::o::AsciiLower(korean.front()).ends_with("offical1.korean"));std::set<std::string> unique; // 중복stem은하나만남아야한다.
    for (const auto& name:korean) { CHECK(netstorm::o::AsciiLower(name).ends_with(".korean"));CHECK(unique.insert(netstorm::o::AsciiLower(name)).second);CHECK(FindBuiltinKoreanFile(files,{repo},name)); }
}

TEST_CASE(KoreanIntegration_NativeConfigAndMissionDisplayKeepAiState) { // 같은실제FILE부모의BOM해독과언어전환표시만갱신되는지확인한다.
    using namespace netstorm::client;const auto repo=KoreanRepo(),game=repo/"assets/game-data";netstorm::o::BaseFileSystem files(game);files.RegisterArchives();const std::vector<std::filesystem::path> roots{repo}; // 보호자료는읽기만한다.
    netstorm::o::ConfigInterface config([&](auto name) { return ReadLocalizedGameFile(files,roots,name); },files.FileNameOwner(),MakeConfigFileSearchHooks([&] { return game; }),KoreanConfigHooks(files,roots));
    config.Configuration().Append("missionSpec=\"d/{local.1}.{currentLanguage}\"\nlanguageSpec=\"d/config.{local.1}\"\ncurrentLanguage=english\n");config.LoadLanguage("korean");CHECK(config.Language().Get("factory")=="워크샾");
    MissionScript mission(config,"tutorial2");CHECK(mission.Get("myStartMoney")=="10000");mission.Set("myStartMoney","31415");const auto englishTitle=mission.Get("title");config.Set("currentLanguage","korean");
    CHECK(mission.Get("title")=="비밀 워크샾" && mission.Get("title")!=englishTitle);CHECK(mission.Get("myStartMoney")=="31415");const auto body=mission.Section("A.");CHECK(body && body->find("건물, 1부")!=body->npos);config.Set("currentLanguage","english");CHECK(mission.Get("title")==englishTitle && mission.Get("myStartMoney")=="31415");
}

TEST_CASE(KoreanIntegration_ExplicitCustomMissionSourceKeepsItsNamespace) { // 공식미션과이름이같아도명시CustomMaps reader를번들로우회하지않는다.
    using namespace netstorm::client;netstorm::o::ConfigInterface config([](std::string_view)->std::optional<std::vector<std::uint8_t>> { return std::nullopt; });config.Configuration().Append("missionSpec=\"d/{local.1}.{currentLanguage}\"\ncurrentLanguage=english\n");std::vector<std::string> reads; /* K1 컴파일 교정: 빈 읽기 장치를 명시한다. */ // 명시source만있고공식자산reader는없다.
    MissionScript mission(config,"tutorial2",{},[&](std::string_view name)->std::optional<std::vector<std::uint8_t>> { reads.emplace_back(name);const std::string text=name.ends_with(".korean") ? "\xef\xbb\xbf[Header]\ntitle=사용자 제목\nmyStartMoney=7\n[A.]\n사용자 안내\n":"[Header]\ntitle=CUSTOM TITLE\nmyStartMoney=7\n[A.]\nCUSTOM BODY\n";return std::vector<std::uint8_t>(text.begin(),text.end()); });
    mission.Set("myStartMoney","19");config.Set("currentLanguage","korean");CHECK(mission.Get("title")=="사용자 제목" && mission.Get("myStartMoney")=="19");const auto body=mission.Section("A.");CHECK(body && body->find("사용자 안내")!=std::string::npos);CHECK(!reads.empty() && reads.back()=="d/tutorial2.korean");
}

TEST_CASE(KoreanIntegration_HelpBomIsTheOnlyDisplayDecodeTrigger) { // 원문1252/NUL/제어BYTE를잘못UTF8로재해석하지않는다.
    using namespace netstorm::client;const std::string english("<h2>caf\xe9</h2>\0tail",19);const auto before=english; // 원문BYTE는한글표시토큰으로확장하지않는다.
    CHECK(DecodeQuickHelpDisplaySource(english)==before);const std::string utf8="\xef\xbb\xbf<h2>한국어 도움말</h2>";const auto display=DecodeQuickHelpDisplaySource(utf8);CHECK(!display.starts_with("\xef\xbb\xbf") && HasExtendedDisplayText(display));CHECK(KoreanDecode(display)=="<h2>한국어 도움말</h2>");
    CHECK(DecodeQuickHelpDisplaySource("한국어") == "한국어"); // BOM없는임의UTF8는새로추측해독하지않는다.
}

TEST_CASE(KoreanIntegration_HtmlExtendedFlagPreservesWholeTokensAndLegacyBytes) { // 기존938의제어BYTE삭제를실제평문파서에서검출한다.
    using namespace netstorm::client;const auto text=KoreanDisplay("한국어 본문");HtmlTextParserState state;HtmlTextParserGlobals globals; // 기본false는원문제어BYTE관문이다.
    CHECK(!HasExtendedDisplayText(HtmlTextParser(state,globals).Parse(text)));globals.extendedText=true;const auto result=HtmlTextParser(state,globals).Parse(text);CHECK(KoreanDecode(result).find("한국어 본문")!=std::string::npos);
    const std::string malformed("\x1f\x01",2);CHECK(HtmlTextParser(state,globals).Parse(malformed)=="\n");globals.extendedText=false;CHECK(HtmlTextParser(state,globals).Parse("ASCII unchanged")=="ASCII unchanged\n");
}

TEST_CASE(KoreanIntegration_HtmlFactoryRecognizesPureKoreanOnlyThroughCloneHook) { // 한글만있는본문도빈창xxx/삭제로처리하지않는다.
    using namespace netstorm::client;HtmlWindowGumpState shared;HtmlWindowGumpNode node{};node.serial=61;node.baseWidth=200;node.baseHeight=100;HtmlTextParserGlobals globals;globals.extendedText=true;std::string shown;bool killed{}; // 게임/Gump창대신명시부모만제공한다.
    HtmlWindowFactoryHooks hooks;hooks.create=[](auto,auto) { return 1U; };hooks.node=[&](auto) { return &node; };hooks.substitute=[](auto value,auto) { return std::string(value); };hooks.parse=[&](auto& state,auto,auto value,auto capacity) { return HtmlTextParser(state,globals).Parse(value,static_cast<std::int32_t>(capacity)); };hooks.metadata=[](auto,auto) { return 1U; };hooks.wallSeconds=[] { return 1.0; };hooks.fallback=[] { return std::string("OK"); };hooks.body=[&](auto,auto value) { shown=value; };hooks.attach=[](auto,auto,auto,auto,auto) {};hooks.modal=[](auto,auto) {};hooks.kill=[&](auto) { killed=true; };hooks.read=[](auto address) { return address==0x5c78f4U ? 600U:800U; };
    CHECK(HtmlWindowFactory(shared,hooks).Show(KoreanDisplay("본문"),0,0,0)==0 && killed);killed=false;hooks.localizedContent=[&](auto value) { return globals.extendedText && HasExtendedDisplayText(value); };
    CHECK(HtmlWindowFactory(shared,hooks).Show(KoreanDisplay("본문"),0,0,0)==61 && !killed);CHECK(KoreanDecode(shown).find("본문")!=std::string::npos);
}

TEST_CASE(KoreanIntegration_RealHelpAll128TopicsAndLargestBodyKeepKoreanTail) { // 준비파일추산이아닌실제전체Help/링크/HTML출력을8192BYTE경계에서검사한다.
    using namespace netstorm::client;const auto bytes=netstorm::o::ReadFileBytes(KoreanRepo()/"assets/lang/korean/d/help.korean");const std::string raw(bytes.begin(),bytes.end()),display=DecodeQuickHelpDisplaySource(raw);const std::regex anchor("<a name=\"([^\"]*)\">");std::set<std::string> topics; // 실제본문의중복을보존하고주제별전체순회한다.
    for (std::sregex_iterator entry(raw.begin(),raw.end(),anchor),end;entry!=end;++entry) topics.insert(netstorm::o::AsciiLower((*entry)[1].str()));
    CHECK(topics.size()==128);
    for (const auto& topic:topics) { const auto body=KoreanHelpBody(display,topic);CHECK(body.size()<8192);for (std::size_t at=0;at<body.size();++at) { if (static_cast<std::uint8_t>(body[at])!=netstorm::o::kExtendedTextLead) continue;CHECK(netstorm::o::DecodeExtendedSequence({reinterpret_cast<const std::uint8_t*>(body.data()),body.size()},at));at+=netstorm::o::kExtendedTextLength-1; } } // 마지막한글도중간BYTE에서잘리지않아야한다.
    const auto large=KoreanHelpBody(display,"battleOptionsHelp");CHECK(HasExtendedDisplayText(large));CHECK(KoreanDecode(large).find("멀티플레이어 빠른 시작으로 돌아가기")!=std::string::npos); // 8086BYTE원자료의끝안내까지남아야한다.
}

TEST_CASE(KoreanIntegration_TellFileMenuInvalidationReloadsLocalizedTitles) { // 기존등록Config가언어전환후영어제목을계속노출하지않는다.
    using namespace netstorm::client;const auto repo=KoreanRepo(),game=repo/"assets/game-data";netstorm::o::BaseFileSystem files(game);files.RegisterArchives(); // 파일목록의namespace는현재선택언어만바뀐다.
    netstorm::o::ConfigInterface config([&](auto name) { return ReadLocalizedGameFile(files,{repo},name); },files.FileNameOwner(),MakeConfigFileSearchHooks([&] { return game; }),KoreanConfigHooks(files,{repo}));TellFileMenus menus(config,[&](auto pattern) { return MatchLocalizedGameFiles(files,{repo},pattern); });ScriptStringRing strings;std::vector<std::string> labels;
    menus.Append(KoreanDisplay("d/offical*.english"),1,"0",true,strings,[&](const auto& item) { if (!item.separator) labels.push_back(KoreanDecode(item.label)); });CHECK(!labels.empty());const auto english=labels;auto* registered=config.Registry().FindByName("offical1");CHECK(registered);config.Set("currentLanguage","korean");menus.InvalidateLanguageFiles();CHECK(config.Registry().FindByName("offical1")==registered);if (registered) CHECK(registered->Section("Overview"));labels.clear();
    menus.Append(KoreanDisplay("d/offical*.korean"),1,"0",true,strings,[&](const auto& item) { if (!item.separator) labels.push_back(KoreanDecode(item.label)); });CHECK(!labels.empty() && labels!=english);CHECK(std::any_of(labels.begin(),labels.end(),[](const auto& value) { return value.find("초기 미션")!=value.npos || value.find("사제")!=value.npos; }));
}

TEST_CASE(KoreanIntegration_EnglishHelpBytesAndOutputStayIdentical) { // 실제클론표시허용이기존영문파일선택/출력을바꾸지않는다.
    using namespace netstorm::client;const auto repo=KoreanRepo();netstorm::o::BaseFileSystem files(repo/"assets/game-data");files.RegisterArchives();const auto raw=files.Read("d/help.english");const std::string english(raw.begin(),raw.end()); // 원본자료는읽기만한다.
    const auto localized=ReadLocalizedGameFile(files,{repo},"d/help.english");CHECK(localized && *localized==raw);CHECK(DecodeQuickHelpDisplaySource(english)==english);const std::regex anchor("<a name=\"([^\"]*)\">");std::set<std::string> topics;
    for (std::sregex_iterator entry(english.begin(),english.end(),anchor),end;entry!=end;++entry) topics.insert(netstorm::o::AsciiLower((*entry)[1].str())); // 실제원문주제의중복/별칭은전체파서입력에남긴다.
    CHECK(topics.size()==128);
    for (const auto& topic:topics) CHECK(KoreanHelpBody(english,topic,false)==KoreanHelpBody(english,topic,true)); // defaultfalse오라클과클론true를동일한영문입력으로대조한다.
}

// 11.01 패치 크레딧의 세 번째 항목과 본문이 한국어 모드에서만 한국어로 나오고 영어는 기존 표기를 유지하는지 검사한다(K4 잔여 교정).
TEST_CASE(KoreanIntegration_ProductCreditsEntryAndBodyFollowLanguage) {
    using namespace netstorm::client;
    const std::string list="<h2>{H2}NetStorm Credits</h2>\n$Menu=NetStorm 10.72 Patch Credits,Tell,CreditsNew\n";
    const auto english=ProductTellBody("Credits",list,0U);
    CHECK(english.find("$Menu=Netstorm 11.01 Patch Credits,Tell,Patch1101.Credits")!=std::string::npos);
    const auto korean=ProductTellBody("credits",list,kKoreanLanguageNumber);
    CHECK(korean.find("$Menu=NetStorm 11.01 패치 크레딧,Tell,Patch1101.Credits")!=std::string::npos && korean.find("Netstorm 11.01 Patch Credits")==std::string::npos);
    CHECK(ProductTellBody("Credits",english,0U)==english); // 이미 들어 있으면 두 번 덧붙이지 않는다.
    const auto body=ProductTellBody("Patch1101.Credits","original",kKoreanLanguageNumber);
    CHECK(body.find("<h2>{H2}NetStorm 11.01 패치 크레딧</h2>")!=std::string::npos && body.find("HJOW")!=std::string::npos && body.find("AI")!=std::string::npos);
    CHECK(ProductTellBody("Patch1101.Credits","original",0U)=="original"); // 영어 본문은 등록된 영어 Config 본문을 그대로 쓴다.
    CHECK(ProductTellBody("Other","x",kKoreanLanguageNumber)=="x");
}
