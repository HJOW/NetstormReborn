// 11차 일본어: 히라가나/가타카나 정적 구간, 한자 동적 배정, 일본어 번역표(assets/lang/japanese/xlat.japanese)의 계약을 게임/GUI 없이 검사한다.
#include "TestSupport.h"
#include "client/KoreanResources.h"
#include "client/LanguageOptions.h"
#include "client/ProductPresentation.h"
#include "o/OriginalText.h"
#include "o/Xlat.h"
#include <algorithm>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace { // 저장소 루트를 이 검사 파일 위치에서 찾는다.
std::filesystem::path JapaneseRepo() { return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path(); }
std::span<const std::uint8_t> JapaneseView(const std::vector<std::uint8_t>& bytes) { return {bytes.data(), bytes.size()}; }
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

// 일본어 번역표는 한국어 번역표와 같은 키 집합을 가지며 모든 값이 비어 있지 않고 표시 부호화로 왕복한다.
TEST_CASE(JapaneseText_TranslationTableMatchesKoreanKeysAndEncodes) {
    using namespace netstorm::client;
    const auto roots = CloneDataRoots(JapaneseRepo() / "originals", JapaneseRepo() / "cpppj" / "build" / "bin");
    const auto japanese = ReadLanguageTranslationFile(roots, "japanese"), korean = ReadLanguageTranslationFile(roots, "korean");
    CHECK(japanese.has_value() && korean.has_value() && !japanese->starts_with("\xEF\xBB\xBF"));
    if (!japanese || !korean) return;
    netstorm::o::XlatTable ja(*japanese), ko(*korean);
    CHECK(ja.Size() == ko.Size() && ja.Size() >= 500);
    // 한국어 키마다 일본어 값이 있고(원문과 다름) 서식 코드 개수가 같다. 키 목록은 한국어 파일의 "***" 다음 줄에서 읽는다.
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
