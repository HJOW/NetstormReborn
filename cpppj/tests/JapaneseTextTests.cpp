// 11차 일본어: 히라가나/가타카나 정적 구간, 한자 동적 배정, 일본어 번역표(assets/lang/japanese/xlat.japanese)의 계약을 게임/GUI 없이 검사한다.
#include "TestSupport.h"
#include "client/KoreanResources.h"
#include "client/LanguageOptions.h"
#include "client/ProductPresentation.h"
#include "client/ConfigFileRead.h" // 일본어 번들도 실제 Native Config FILE/BOM 경계로 읽는다.
#include "client/StyleTextBuffer.h" // 일본어무공백본문을실제표시줄버퍼로레이아웃한다.
#include "o/BaseFile.h" // 사용자 정확 파일과 공식 번들의 우선권을 같은 VFS로 검사한다.
#include "o/OriginalText.h"
#include "o/Xlat.h"
#include <algorithm>
#include <filesystem>
#include <fstream> // 공식 자료와 분리된 검사 경로에만 명시 입력을 쓴다.
#include <set>
#include <string>
#include <vector>

namespace { // 저장소 루트를 이 검사 파일 위치에서 찾는다.
std::filesystem::path JapaneseRepo() { return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path(); }
std::span<const std::uint8_t> JapaneseView(const std::vector<std::uint8_t>& bytes) { return {bytes.data(), bytes.size()}; }
}

// 새 공식 일본어 번들의9파일/BOM과 기존 영어·한국어/사용자 경로의 우선권을 검사한다.
TEST_CASE(JapaneseText_BuiltinConfigCampaignsAndExactFilePriority) {
    using namespace netstorm::client;
    const auto repo=JapaneseRepo(),work=repo/"cpppj/build/test-data/japanese-builtin-1334"; // 이 검사만 쓰는 입력 경로다.
    std::filesystem::create_directories(work/"d");std::filesystem::remove(work/"d/config.japanese");
    netstorm::o::BaseFileSystem files(work);
    for (const auto stem:{"config","offical1","offical2","offical2sep","offical3","offical4","offical5","offical5sep","offical6"}) { // 공식43stem 중 이번에 번역한9본문을 각자 읽는다.
        const auto logical=std::string("d/")+stem+".japanese";const auto selected=FindBuiltinKoreanFile(files,{repo},logical);CHECK(selected);
        const auto actual=ReadLocalizedGameFile(files,{repo},logical);CHECK(actual && *actual==netstorm::o::ReadFileBytes(repo/"assets/lang/japanese"/logical));
        if (actual) CHECK(actual->size()>=3 && (*actual)[0]==0xef && (*actual)[1]==0xbb && (*actual)[2]==0xbf);
    }
    for (const auto invalid:{"d/custom.japanese","d/../config.japanese","d/sub/config.japanese","//server/d/config.japanese","C:/CustomMaps/d/config.japanese","d/config.german"}) CHECK(!FindBuiltinKoreanFile(files,{repo},invalid)); // 사용자/다른 언어/경로 우회를 번들로 바꾸지 않는다.
    netstorm::o::ConfigInterface config([&](auto name) { return ReadLocalizedGameFile(files,{repo},name); },files.FileNameOwner(),MakeConfigFileSearchHooks([&] { return work; }),MakeConfigFileReadHooks(files,[&](auto name) { return FindBuiltinKoreanFile(files,{repo},name); }));
    config.Configuration().Append("languageSpec=\"d/config.{local.1}\"\n");config.LoadLanguage("japanese");CHECK(config.Language().Get("factory")=="ワークショップ" && config.Language().Get("helpWord")=="バージョン情報");
    const auto campaigns=MatchLocalizedGameFiles(files,{repo},"d/offical*.japanese");CHECK(campaigns.size()==8);
    for (const auto& name:campaigns) CHECK(name.ends_with(".japanese") && FindBuiltinKoreanFile(files,{repo},name)); // Finder도 같은 공식8파일만 일본어 논리 이름으로 돌려준다.
    const std::string exact="\xef\xbb\xbf" "factory=\"USER EXACT\"\n"; // 사용자가 설치한 같은 논리 이름의 명시 파일이다.
    { std::ofstream file(work/"d/config.japanese",std::ios::binary|std::ios::trunc);file.write(exact.data(),exact.size());CHECK(file.good()); }
    CHECK(!FindBuiltinKoreanFile(files,{repo},"d/config.japanese"));const auto actual=ReadLocalizedGameFile(files,{repo},"d/config.japanese");CHECK(actual && std::string(actual->begin(),actual->end())==exact);
    std::filesystem::remove(work/"d/config.japanese"); // 다음 재실행도 같은 초기 우선권을 검사한다.
}

// 실제조각복사/표시줄에서일본어잘림·4BYTE절단·닫는구두점의줄첫머리이동을검사한다.
TEST_CASE(JapaneseText_LongUnspacedParagraphWrapsAndKeepsLegacyWords) {
    using namespace netstorm::client;
    const auto layout=[](std::string_view input,std::int32_t width) {
        StyleTextState fragments;StyleTextBufferState state; // 이실행이소유하는원문/표시조각과줄저장소다.
        const auto measure=[](std::span<const char> text,std::int32_t length) { // 글리프당10픽셀인독립검사폰트로폭을측정한다.
            const auto end=std::find(text.begin(),text.end(),'\0');const auto count=length<0 ? static_cast<std::size_t>(end-text.begin()) : static_cast<std::size_t>(length);
            const auto decoded=netstorm::o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),count});
            return std::array<std::int32_t,2>{static_cast<std::int32_t>(std::count_if(decoded.begin(),decoded.end(),[](unsigned char byte) { return (byte&0xc0)!=0x80; }))*10,10};
        };
        StyleTextHooks hooks;hooks.textMetrics=[](const auto&) { return std::array<std::int32_t,3>{10,10,0}; };hooks.textMeasure=[&](auto bytes,const auto&,auto& w,auto& h) { const auto size=measure(bytes,-1);w=size[0];h=size[1]; };
        StyleText style(fragments,std::move(hooks));StyleTextBufferHooks bufferHooks;bufferHooks.measureText=[&](auto bytes,const auto&,auto length) { return measure(bytes,length); };
        StyleTextBuffer buffer(state,fragments,style,8,std::move(bufferHooks));auto encoded=netstorm::o::EncodeDisplayText(std::string(input)+'\n');encoded.push_back(0);buffer.AppendText({reinterpret_cast<const char*>(encoded.data()),encoded.size()},{});buffer.Layout(width);
        std::vector<std::string> lines; // 다음포인터순서대로실제표시문자열을합쳐누락/중복을확인한다.
        for (auto line=state.sentinel.next;line;line=state.softLines.at(line).next) {
            std::string text;
            for (auto token=state.softLines.at(line).head;token;token=fragments.fragments.at(token).next) { const auto& raw=fragments.fragments.at(token).text;const auto end=std::find(raw.begin(),raw.end(),'\0');text+=netstorm::o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(raw.data()),static_cast<std::size_t>(end-raw.begin())}); } // 실제출력처럼첫NUL까지만조각마다독립해독해원본의여분NUL복사와토큰절단을구별한다.
            lines.push_back(std::move(text));
        }
        buffer.Destroy();return lines;
    };
    const std::string japanese="日本語の文章を「正しく」折り返します。文字を失いません。";const auto lines=layout(japanese,61);
    std::string joined;for (const auto& line:lines) { CHECK(!line.empty() && !line.starts_with("。") && !line.starts_with("」") && !line.ends_with("「"));CHECK(std::none_of(line.begin(),line.end(),[](unsigned char byte) { return byte<0x20; }));joined+=line; } // 폭조건과별도로행과문자순서/구두점보존을검증한다.
    CHECK(lines.size()>1 && joined==japanese);
    CHECK((layout("one two",51)==std::vector<std::string>{"one ","two"})); // 기존영문공백포함분할계약이다.
    CHECK(layout("한국어문장",31).front()=="한국어문장"); // 일본어없는한국어긴단어의기존경로를유지한다.
}

// 히라가나·가타카나·반각 가타카나·전각 문장 부호는 한국어 구간 뒤의 정적 구간이라 4바이트 확장으로 왕복하고 한글 인덱스는 그대로다.
TEST_CASE(JapaneseText_KanaStaticRangesRoundTripAndKeepHangulIndices) {
    using namespace netstorm::o;
    for (const std::uint32_t cp : {0x3041U, 0x3042U, 0x309fU, 0x30a1U, 0x30feU, 0xff71U, 0x3001U, 0x3002U}) { // あ ぁ ゟ ァ ヾ ｱ 、 。
        const auto sequence = EncodeExtendedCodePoint(cp);
        CHECK(sequence.has_value());
        if (sequence) CHECK(DecodeExtendedSequence(std::span<const std::uint8_t>(sequence->data(), sequence->size()), 0) == cp);
    }
    const auto first = EncodeExtendedCodePoint(0xac00);
    CHECK(first && (*first)[1] == 1 && (*first)[2] == 1 && (*first)[3] == 1); // '가'는 인덱스0이며 6차 한국어 부호와 같다.
}

// CJK 통합 한자는 처음 쓰일 때 남은 인덱스를 배정하고 같은 한자는 같은 시퀀스를 다시 받으며 모두 한 문자로 되돌아온다.
TEST_CASE(JapaneseText_IdeographsAreAssignedOnFirstUseAndRoundTrip) {
    using namespace netstorm::o;
    const std::string text = "日本語のメインメニュー 開始";
    const auto bytes = EncodeDisplayText(text);
    CHECK(DecodeDisplayText(JapaneseView(bytes)) == text);
    CHECK(EncodeDisplayText(text) == bytes); // 같은 한자는 같은 인덱스를 다시 받는다.
    CHECK(std::count(bytes.begin(), bytes.end(), kExtendedTextLead) >= 10); // 한자·가나가 모두 확장 시퀀스가 되었다.
    CHECK(DecodeDisplayText(JapaneseView(EncodeDisplayText("English 한국어 日本語"))) == "English 한국어 日本語"); // 두 언어가 같은 표시 경계를 공유한다.
}

// 언어 번호 판정과 번역 파일 탐색: 일본어는 원문 이름표의 5번이며 확장 언어다.
TEST_CASE(JapaneseText_LanguageNumberAndFontLookup) {
    using namespace netstorm::client;
    CHECK(kJapaneseLanguageNumber == 5 && OriginalLanguageName(kJapaneseLanguageNumber) == "japanese" && FindOriginalLanguage("japanese") == kJapaneseLanguageNumber);
    CHECK(IsExtendedLanguageNumber(kJapaneseLanguageNumber) && IsExtendedLanguageNumber(kKoreanLanguageNumber) && !IsExtendedLanguageNumber(0) && !IsExtendedLanguageNumber(3));
    const auto roots = CloneDataRoots(JapaneseRepo() / "originals", JapaneseRepo() / "cpppj" / "build" / "bin");
    const auto font = FindJapaneseFontFile(roots);
    CHECK(!font.empty() && font.filename().string().starts_with("NotoSansJP") && font.extension() == ".ttf"); // 외부 파일 글꼴이며 exe에 내장하지 않는다.
}

// 언어별 번역 범위를 독립 확인하고 한국어 기준 키의 일본어 번역/영어 대체가 표시 부호화로 왕복하는지 검사한다.
TEST_CASE(JapaneseText_TranslationTableMatchesKoreanKeysAndEncodes) {
    using namespace netstorm::client;
    const auto roots = CloneDataRoots(JapaneseRepo() / "originals", JapaneseRepo() / "cpppj" / "build" / "bin");
    const auto japanese = ReadLanguageTranslationFile(roots, "japanese"), korean = ReadLanguageTranslationFile(roots, "korean");
    CHECK(japanese.has_value() && korean.has_value() && !japanese->starts_with("\xEF\xBB\xBF"));
    if (!japanese || !korean) return;
    netstorm::o::XlatTable ja(*japanese), ko(*korean);
    CHECK(ja.Size() >= 500 && ko.Size() >= 500); // 언어별 추가 번역 키는 서로 다를 수 있으므로 각 표의 기본 번역 범위를 독립 확인한다.
    // 한국어 키의 일본어 번역 수와 서식 코드 보존을 확인한다. 새 한국어 전용 키는 기존 영어 대체 정책을 따른다.
    std::set<std::string> keys;
    std::string text = *korean;text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    for (std::size_t at = 0; (at = text.find("\n***\n", at)) != std::string::npos; at += 5) { const auto end = text.find("\n---\n", at + 5);if (end == std::string::npos) break;keys.insert(text.substr(at + 5, end - at - 5)); }
    CHECK(keys.size() >= 500);
    std::size_t translated{};
    for (const auto& key : keys) {
        const auto value = ja.Translate(key, static_cast<int>(kJapaneseLanguageNumber));
        if (value != key) ++translated;
        const auto bytes = netstorm::o::EncodeDisplayText(value);
        CHECK(netstorm::o::DecodeDisplayText(JapaneseView(bytes)) == value); // 표시 경계 왕복이다.
        const auto count = [](const std::string& s, char c) { return std::count(s.begin(), s.end(), c); };
        CHECK(count(value, '%') == count(key, '%')); // 서식 인자 수 보존이다.
    }
    CHECK(translated >= 480); // 영어 그대로인 값은 일부 고유명/기호뿐이다.
    CHECK(ja.Translate("Cancel", 5) == "キャンセル" && ja.Translate("Credits", 5) == "クレジット");
}

// 11.01 패치 크레딧의 세 번째 항목과 본문이 일본어 모드에서 일본어로 나온다(영어/한국어 표기는 기존 시험이 보호한다).
TEST_CASE(JapaneseText_ProductCreditsEntryAndBody) {
    using namespace netstorm::client;
    const std::string list = "<h2>{H2}NetStorm Credits</h2>\n$Menu=NetStorm 10.72 Patch Credits,Tell,CreditsNew\n";
    const auto menu = ProductTellBody("credits", list, kJapaneseLanguageNumber);
    CHECK(menu.find("$Menu=NetStorm 11.01 パッチ クレジット,Tell,Patch1101.Credits") != std::string::npos && menu.find("Netstorm 11.01 Patch Credits") == std::string::npos);
    const auto body = ProductTellBody("Patch1101.Credits", "original", kJapaneseLanguageNumber);
    CHECK(body.find("<h2>{H2}NetStorm 11.01 パッチ クレジット</h2>") != std::string::npos && body.find("HJOW") != std::string::npos && body.find("$Button=戻る,Tell,Credits") != std::string::npos);
}
