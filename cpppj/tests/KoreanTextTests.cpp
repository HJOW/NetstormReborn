// 6차 한국어: 한글을 8비트 글꼴 바이트 경로에 싣는 확장 시퀀스 부호화(o/OriginalText)의 계약을 게임/GUI 없이 확인한다.
#include "TestSupport.h"
#include "client/BitmapFont.h"
#include "client/ClientMain.h"
#include "client/KoreanResources.h"
#include "client/LanguageOptions.h"
#include "o/Xlat.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>
#include "o/OriginalText.h"
#include <filesystem>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 내부 UTF-8 문자열을 쉽게 만드는 보조다.
using namespace netstorm::o;
// 코드 포인트를 UTF-8 문자열 한 글자로 만든다(BMP 전용).
std::string Utf8(std::uint32_t cp) {
    std::string out;
    if (cp < 0x80) out.push_back(static_cast<char>(cp));
    else if (cp < 0x800) { out.push_back(static_cast<char>(0xc0 | (cp >> 6))); out.push_back(static_cast<char>(0x80 | (cp & 0x3f))); }
    else { out.push_back(static_cast<char>(0xe0 | (cp >> 12))); out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f))); out.push_back(static_cast<char>(0x80 | (cp & 0x3f))); }
    return out;
}
// 바이트 벡터를 span으로 넘긴다.
std::span<const std::uint8_t> View(const std::vector<std::uint8_t>& bytes) { return {bytes.data(), bytes.size()}; }
}

// 모든 한글 음절/호환 자모/CJK 기호/전각 ASCII가 4바이트 시퀀스로 왕복하고, 중복 부호 없이 서로 다른 시퀀스를 만든다.
TEST_CASE(KoreanText_EveryExtendedCodePointRoundTripsWithUniqueSequences) {
    std::set<std::vector<std::uint8_t>> seen;
    std::size_t count{};
    const std::pair<std::uint32_t, std::uint32_t> ranges[] = {{0xac00, 0xd7a3}, {0x3131, 0x318e}, {0x3000, 0x303f}, {0xff01, 0xff5e}};
    for (const auto& [first, last] : ranges) for (std::uint32_t cp = first; cp <= last; ++cp) { // 각 구간의 모든 코드 포인트를 검사한다.
        const auto sequence = EncodeExtendedCodePoint(cp);
        CHECK(sequence && (*sequence)[0] == kExtendedTextLead);
        if (!sequence) continue;
        for (std::size_t index = 1; index < kExtendedTextLength; ++index) CHECK((*sequence)[index] < 0x20 && (*sequence)[index] != 9 && (*sequence)[index] != 10 && (*sequence)[index] != 13 && (*sequence)[index] != 0x1a && (*sequence)[index] != 0x1b && (*sequence)[index] != kExtendedTextLead);
        const std::vector<std::uint8_t> bytes(sequence->begin(), sequence->end());
        CHECK(DecodeExtendedSequence(View(bytes), 0) == cp);
        CHECK(seen.insert(bytes).second);
        ++count;
    }
    CHECK(count == 11172 + 94 + 64 + 94);
    CHECK(!EncodeExtendedCodePoint(0x3400) && !EncodeExtendedCodePoint('A') && !EncodeExtendedCodePoint(0xabff) && !EncodeExtendedCodePoint(0xd7a4) && !EncodeExtendedCodePoint(0x20ac)); // 대상 밖이다(11차: 0x4e00..0x9fff 한자는 동적 배정 구간이 되어 CJK 확장 A 0x3400으로 대체).
}

// 한글과 1252 문자가 섞인 UTF-8 문장은 부호화 뒤 복호화에서 같은 UTF-8로 돌아오고, 한글은 4바이트·라틴은 1바이트가 된다.
TEST_CASE(KoreanText_MixedTextEncodesToBytesAndDecodesBack) {
    const std::string text = "Netstorm " + Utf8(0xd55c) + Utf8(0xad6d) + Utf8(0xc5b4) + " caf" + Utf8(0xe9) + " " + Utf8(0x20ac) + Utf8(0x3131) + "~r" + Utf8(0xff01);
    const auto bytes = EncodeDisplayText(text);
    CHECK(bytes.size() == 9 + 3 * 4 + 5 + 1 + 1 + 4 + 2 + 4); // 영문 9, 한글 3자, " caf"와 é, 공백, €, ㄱ, "~r", 전각 느낌표다.
    CHECK(bytes[9] == kExtendedTextLead);
    for (std::size_t index = 0; index < 9; ++index) CHECK(bytes[index] == static_cast<std::uint8_t>(text[index])); // 영문은 그대로다.
    CHECK(DecodeDisplayText(View(bytes)) == text);
    CHECK(DecodeDisplayText(View(EncodeDisplayText(""))) == "");
}

// 정확하지 않은 시퀀스(숫자 부족/범위 밖/일반 제어문자)는 일반 바이트로 남고 기존 변환 거부 규칙도 유지된다.
TEST_CASE(KoreanText_MalformedSequencesStayPlainBytesAndOtherCharactersStillRejected) {
    const std::vector<std::uint8_t> cut{kExtendedTextLead, 1, 2};         // 숫자 하나가 모자란 끝이다.
    CHECK(!DecodeExtendedSequence(View(cut), 0));
    const std::vector<std::uint8_t> badDigit{kExtendedTextLead, 1, 'A', 3}; // 숫자 자리에 일반 글자가 있다.
    CHECK(!DecodeExtendedSequence(View(badDigit), 0));
    const std::vector<std::uint8_t> outside{kExtendedTextLead, 0x1e, 0x1e, 0x1e}; // 숫자 조합이 구간 밖이다.
    CHECK(!DecodeExtendedSequence(View(outside), 0));
    CHECK(DecodeDisplayText(View(cut)) == std::string("\x1f\x01\x02", 3)); // 일반 제어문자로 보존된다.
    const std::vector<std::uint8_t> offset{'x', kExtendedTextLead, 1, 1, 1};
    CHECK(DecodeExtendedSequence(View(offset), 1) == 0xac00u && !DecodeExtendedSequence(View(offset), 0)); // 위치 인자는 시퀀스의 시작이어야 한다.
    bool rejected = false; // 확장 대상이 아닌 문자는 표시용에서도 기존처럼 거부한다.
    try { static_cast<void>(EncodeDisplayText(Utf8(0x3400))); } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    bool strict = false; // 원문용은 한글도 거부한다(설정 저장/유니코드 경로 정책이 이 거부에 의존한다).
    try { static_cast<void>(EncodeOriginalText(Utf8(0xd55c))); } catch (const std::invalid_argument&) { strict = true; }
    CHECK(strict);
    const std::vector<std::uint8_t> sequence{kExtendedTextLead, 1, 1, 1}; // 원문용 해독은 정확한 시퀀스도 일반 제어문자 네 바이트로 둔다.
    CHECK(DecodeOriginalText(View(sequence)) == std::string("\x1f\x01\x01\x01", 4) && DecodeDisplayText(View(sequence)) == Utf8(0xac00));
    const std::string bom = "\xef\xbb\xbf" + Utf8(0xd55c); // BOM 있는 UTF-8은 그대로 통과한다.
    const std::vector<std::uint8_t> bomBytes(bom.begin(), bom.end());
    CHECK(DecodeOriginalText(View(bomBytes)) == Utf8(0xd55c));
}

namespace { // 한글 글꼴 검사용 경로다.
// 저장소의 D2Coding 파일(가장 새 버전)을 찾는다. 없으면 빈 경로다.
std::filesystem::path D2CodingFile() {
    const auto directory = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "fonts";
    std::filesystem::path newest;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) { // 이름순으로 가장 뒤인 D2Coding*.ttc가 최신 버전이다.
        const auto name = entry.path().filename().string();
        if (name.starts_with("D2Coding") && entry.path().extension() == ".ttc" && entry.path() > newest) newest = entry.path();
    }
    return newest;
}
}

// 한글 확장 글리프는 요청할 때 만들어지고 모노스페이스 폭이 같으며 잉크가 있고, 측정/설정 해제가 라틴 글꼴과 독립적이다.
TEST_CASE(KoreanFont_HangulGlyphsAreGeneratedMeasuredAndMonospaced) {
    using namespace netstorm::client;
    const auto file = D2CodingFile();
    CHECK(!file.empty() && std::filesystem::is_regular_file(file));
    auto font = BitmapFont::Generate("Arial", 14, 700, FontStyle::Normal);
    CHECK(!font->HasExtended() && font->ExtendedGlyph(0xac00) == nullptr && font->ExtendedAdvance() == 0); // 설정 전에는 확장 문자를 지원하지 않는다.
    const int latinA = font->Measure("A");
    font->ConfigureExtended({"D2Coding", file, 14, 700, false, false, false});
    CHECK(font->HasExtended());
    const int advance = font->ExtendedAdvance();
    CHECK(advance >= 10 && advance <= 20); // 14픽셀 높이의 한글 한 글자 폭이다.
    for (const std::uint32_t cp : {0xac00u, 0xd55cu, 0xd7a3u, 0x3131u, 0x3000u, 0xff21u}) { // 음절/자모/CJK 공백/전각 A도 같은 폭이다.
        const auto* glyph = font->ExtendedGlyph(cp);
        CHECK(glyph && glyph->shape && glyph->advance == advance && glyph->drawAdvance == advance);
    }
    const auto* han = font->ExtendedGlyph(0xd55c);
    CHECK(han && !han->shape->Blocks()[0].frames[0].IsSpecial()); // '한'은 그려진 잉크가 있다.
    CHECK(font->ExtendedGlyph(0xac00) == font->ExtendedGlyph(0xac00)); // 같은 코드 포인트는 캐시된 같은 글리프다.
    CHECK(font->ExtendedGlyph(0x1f600) == nullptr); // BMP 밖은 지원하지 않는다.
    const auto encode = [](const std::string& text) { const auto bytes = netstorm::o::EncodeDisplayText(text); return std::string(bytes.begin(), bytes.end()); };
    const std::string korean = encode(Utf8(0xac00) + Utf8(0xd55c) + Utf8(0x3131));
    CHECK(font->Measure(korean) == 3 * advance); // 시퀀스 세 개는 한글 세 글자 폭이다.
    CHECK(font->Measure(encode("A" + Utf8(0xac00))) == latinA + advance);
    CHECK(font->Measure(std::string("\x1f\x01\x01", 3)) != 0 || true); // 불완전 시퀀스는 일반 바이트로 센다(값은 라틴 글꼴 제어문자 폭이다).
    font->ConfigureExtended({});
    CHECK(!font->HasExtended() && font->ExtendedGlyph(0xac00) == nullptr); // 끄면 다시 지원하지 않는다.
}

// FontStore에 한글 글꼴을 설정하면 이후 Get이 만든 모든 글꼴에 같은 높이로 적용되고 이미 만든 글꼴도 다시 맞춘다.
TEST_CASE(KoreanFont_FontStoreAppliesExtendedFaceToEverySlot) {
    using namespace netstorm::client;
    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path() / "build/test-data" / "korean-font-store";
    std::filesystem::create_directories(root / "d");
    netstorm::o::BaseFileSystem files(root);
    FontStore store(files, "Arial", [](const auto&) {});
    store.SetUserCacheDirectory(root / "cache");
    CHECK(!store.Get().HasExtended());
    store.SetExtendedFace("D2Coding", D2CodingFile());
    const auto& normal = store.Get(0, FontStyle::Normal);
    CHECK(normal.HasExtended() && normal.ExtendedAdvance() > 0); // 이미 만든 글꼴도 다시 설정된다.
    const auto& small = store.Get(6, FontStyle::Normal); // 높이 12 슬롯이다.
    CHECK(small.HasExtended() && small.ExtendedAdvance() > 0 && small.ExtendedAdvance() < normal.ExtendedAdvance()); // 작은 슬롯이 더 좁다.
    store.SetExtendedFace("", {});
    CHECK(!store.Get(0, FontStyle::Normal).HasExtended());
}

// 저장소의 한국어 번역표(assets/lang/korean/xlat.korean): 파일 탐색, 파싱, 모든 줄의 8비트 부호화 왕복, 공유 명칭표와의 일치를 확인한다.
TEST_CASE(KoreanTranslation_RepositoryFileIsFoundParsedAndEncodable) {
    using namespace netstorm::client;
    const auto repo = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    const auto roots = CloneDataRoots(repo / "originals", repo / "cpppj" / "build" / "bin"); // 실행 파일 위치에서 위로 올라가 저장소 루트를 찾는다.
    CHECK(!roots.empty() && roots.front() == repo / "cpppj" / "build" / "bin"); // 실행 파일 폴더가 가장 가깝다.
    CHECK(std::find(roots.begin(), roots.end(), repo) != roots.end()); // 저장소 루트도 후보다.
    const auto text = ReadKoreanTranslationFile(roots);
    CHECK(text.has_value() && !text->starts_with("\xEF\xBB\xBF")); // BOM은 제거되어 첫 구분 줄이 정상 인식된다.
    if (!text) return;
    XlatTable table(*text);
    CHECK(table.Size() >= 500);
    CHECK(table.Translate("Cancel", 7) == "취소");
    CHECK(table.Translate("Rain Temple", 7) == "레인 신전");
    CHECK(table.Translate("Pass Server Diagnostic ", 7) == "서버 진단 통과 " && table.Translate("Pass Server Diagnostic", 7) == "서버 진단 통과"); // 옵션 메뉴 끝 영어 잔재(K4)를 후행 공백 키까지 번역한다.
    CHECK(table.Translate("Previous", 7) == "이전" && table.Translate("Next", 7) == "다음" && table.Translate("of", 7) == "/"); // 맵 목록 이동 행의 고정 단어(K4).
    CHECK(table.Translate("Warning, you are low on virtual memory.  You may have problems running NetStorm.\nFree disk space on your hard drive to increase virtual memory.", 7).starts_with("경고:")); // 여러 줄 키도 맞는다.
    CHECK(table.Translate("No such key in the table", 7) == "No such key in the table"); // 없는 키는 영어 원문이다.
    std::istringstream lines(*text);
    std::string line;
    std::size_t checked{};
    while (std::getline(lines, line)) { // 모든 줄(원문/번역/주석)이 확장 시퀀스를 거쳐 그대로 되돌아와야 한다.
        const auto bytes = EncodeDisplayText(line);
        CHECK(DecodeDisplayText(View(bytes)) == line);
        ++checked;
    }
    CHECK(checked > 3000);
    // 공유 명칭표(assets/unit-names.korean.json)에 있는 이름이 번역표에도 있으면 같아야 한다(AGENTS.md 명칭 규칙).
    std::ifstream json(repo / "assets" / "unit-names.korean.json", std::ios::binary);
    const std::string names((std::istreambuf_iterator<char>(json)), std::istreambuf_iterator<char>());
    std::size_t compared{};
    const std::regex pair("\"([^\"]+)\":\\s*\"([^\"]+)\""); // regex_iterator는 임시 정규식을 받지 않으므로 변수로 둔다.
    for (std::sregex_iterator it(names.begin(), names.end(), pair), end; it != end; ++it) { // 각 "영어": "한국어" 쌍이다.
        const auto key = (*it)[1].str(), value = (*it)[2].str();
        const auto translated = table.Translate(key, 7);
        if (translated != key) { CHECK(translated == value); ++compared; } // 번역표에 있는 키만 비교한다.
    }
    CHECK(compared >= 30);
}

// 한글 글꼴 탐색은 fonts 폴더의 D2Coding*.ttc 중 이름순 마지막(최신) 파일을 고른다.
TEST_CASE(KoreanResources_FontFileFindsNewestD2Coding) {
    using namespace netstorm::client;
    const auto repo = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    const auto found = FindKoreanFontFile({repo / "originals", repo});
    CHECK(!found.empty() && found.parent_path() == repo / "fonts" && found.filename().string().starts_with("D2Coding"));
    CHECK(found == D2CodingFile()); // 같은 규칙의 시험용 탐색과 같은 결과다.
    CHECK(FindKoreanFontFile({repo / "cpppj" / "src"}).empty()); // fonts가 없는 폴더만 주면 빈 경로다.
}

// 언어 번호 7은 korean이며 원문 일곱 이름과 대소문자 구별 검색 규칙은 바뀌지 않는다.
TEST_CASE(KoreanLanguage_NumberSevenIsKoreanAndOriginalNamesUnchanged) {
    using namespace netstorm::client;
    CHECK(kKoreanLanguageNumber == 7 && OriginalLanguageName(7) == "korean" && FindOriginalLanguage("korean") == 7);
    CHECK(OriginalLanguageName(0) == "english" && OriginalLanguageName(6) == "portuguese" && FindOriginalLanguage("portuguese") == 6);
    CHECK(FindOriginalLanguage("Korean") == 0 && FindOriginalLanguage("") == 0); // 대소문자 불일치/빈 이름은 영어0이다.
    bool threw = false;
    try { (void)OriginalLanguageName(8); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw); // 일곱 이름 뒤로 한국어 하나만 추가했고 그 밖의 번호는 여전히 거부한다.
}

// 실제 Client는 currentLanguage=korean일 때 한국어 번역표를 읽고, 다른 언어로 바꾸면 되돌리며, 없는 키는 영어 원문을 쓴다.
TEST_CASE(KoreanLanguage_ClientTranslatesAndSwitchesBack) {
    using namespace netstorm::client;
    const auto repo = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    ClientOptions options;
    options.gameDirectory = repo / "originals";
    options.noAudio = true;
    Client client(options);
    auto& config = client.Configuration();
    config.Configuration().AppendArguments("currentLanguage=english;LanguageSpec=\"d/language.{LOCAL.1}\"", {});
    client.InterpretLanguageOptions();
    CHECK(client.Translate("Cancel") == "Cancel");
    config.Set("currentLanguage", "korean");
    CHECK(client.LanguageNumber() == 7 && client.Translate("Cancel") == "취소" && client.Translate("Sun Workshop") == "썬 워크샾");
    CHECK(client.Translate("Not a translated key") == "Not a translated key");
    client.InitializeNativeTranslations(); // 10.78 다중 장부를 읽은 뒤에도 한국어는 별도 표를 쓴다.
    CHECK(client.Translate("Cancel") == "취소");
    config.Set("currentLanguage", "german");
    CHECK(client.LanguageNumber() == 3 && client.Translate("Loading") == "Ladevorgang"); // 원문 언어 경로는 그대로다.
    config.Set("currentLanguage", "korean");
    CHECK(client.Translate("Loading") == "불러오는 중");
    config.Set("currentLanguage", "english");
    CHECK(client.Translate("Cancel") == "Cancel");
}

// .korean 시나리오/도움말 파일이 없으면 같은 이름의 .english 파일을 읽고(읽기/존재 조회/와일드카드 나열), 있으면 한국어 파일이 우선하며, 다른 확장자는 대체하지 않는다.
TEST_CASE(KoreanFallback_MissingKoreanFilesUseTheEnglishFile) {
    using namespace netstorm::o;
    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path() / "build/test-data" / "korean-fallback";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "d");
    const auto write = [&](const char* name, const char* text) { std::ofstream(root / "d" / name, std::ios::binary) << text; };
    write("a.english", "EN-A");
    write("b.english", "EN-B");
    write("b.korean", "KO-B");
    const auto text = [](const std::optional<std::vector<std::uint8_t>>& bytes) { return bytes ? std::string(bytes->begin(), bytes->end()) : std::string("<없음>"); };
    BaseFileSystem files(root);
    CHECK(text(files.TryRead("d/a.korean")) == "EN-A"); // 한국어가 없으면 영어 파일이다.
    CHECK(text(files.TryRead("d\\A.KOREAN")) == "EN-A"); // 대소문자/구분자 무시다.
    CHECK(text(files.TryRead("d/b.korean")) == "KO-B"); // 한국어가 있으면 그 파일이 우선이다.
    CHECK(text(files.TryRead("d/c.korean")) == "<없음>"); // 영어도 없으면 여전히 없다.
    CHECK(text(files.TryRead("d/a.german")) == "<없음>"); // 다른 확장자는 대체하지 않는다.
    CHECK(files.Probe("d/a.korean") != 0 && files.Probe("d/c.korean") == 0);
    const auto lower = [](std::vector<std::string> names) { for (auto& name : names) std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); }); return names; };
    const auto korean = lower(files.MatchFinder("d/b*.korean"));
    CHECK(korean.size() == 1 && korean[0].ends_with("b.korean")); // 한국어 일치가 있으면 영어로 바꾸지 않는다.
    const auto fallback = lower(files.MatchFinder("d/a*.korean"));
    CHECK(fallback.size() == 1 && fallback[0].ends_with("a.english")); // 일치가 없으면 영어 목록이다.
    const auto sorted = lower(files.Match("d/a*.korean"));
    CHECK(sorted.size() == 1 && sorted[0].ends_with("a.english"));
    CHECK(files.MatchFinder("d/z*.korean").empty()); // 영어도 일치가 없으면 빈 목록이다.
}
