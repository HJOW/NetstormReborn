// 외부 갈무리11의 독립 확인 픽셀과 실제 FontStore 선택·정수 확대·원본 라틴 캐시 보존을 창 없이 검사한다.
#include "TestSupport.h"
#include "client/BitmapFont.h"
#include "client/EmbeddedBitmapFont.h"
#include "client/KoreanFontSettings.h"
#include "client/KoreanResources.h"
#include "o/BaseFile.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace { // 제공 글꼴의 경로와 실제 VFX 잉크 수를 검사에 공급한다.
// 저장소 루트를 이 검사 소스 위치에서 찾는다.
std::filesystem::path FontRepo() { return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path(); }
// 실제 생성된 글리프의 투명하지 않은 픽셀 수다. VFX의 현재 팔레트 글자색도 함께 검사한다.
std::size_t InkCount(const netstorm::client::FontGlyph* glyph) {
    if (!glyph || !glyph->shape) return 0;
    const auto& frame = glyph->shape->Blocks()[0].frames[0];
    if (frame.IsSpecial()) return 0;
    const auto image = glyph->shape->Decode(0, 0);
    std::size_t count{};
    for (std::size_t index = 0; index < image.opacity.size(); ++index) { // 투명 구간은 글자색으로 세지 않는다.
        if (image.opacity[index]) { CHECK(image.indices[index] == 100); ++count; }
    }
    return count;
}
}

// 별도 SFNT 관측으로 읽은 '한'의 11×14 원형과 대조한다. 굵은 글꼴은 합성 두께가 아닌 제공 파일의 66픽셀이다.
TEST_CASE(KoreanBitmap_ProvidedRegularAndBoldKeepExactNativePixels) {
    using namespace netstorm::client;
    const std::array<std::string_view, 14> regular{{
        "...........", "...#.....#.", "########.#.", "..####...#.", ".#....#..##", ".#....#..#.", "..####...#.",
        ".........#.", ".#.......#.", ".#.........", ".#.........", ".#########.", "...........", "..........."}};
    const auto first = EmbeddedBitmapFont::Load(FontRepo() / "fonts" / kKoreanFontRegularFile);
    const auto second = EmbeddedBitmapFont::Load(FontRepo() / "fonts" / kKoreanFontBoldFile);
    CHECK(first && second && first != second);
    CHECK(std::filesystem::is_regular_file(FontRepo() / "fonts" / kKoreanFontLicenseFile));
    if (!first || !second) return;
    const auto glyph = first->Glyph(0xd55c), bold = second->Glyph(0xd55c);
    CHECK(glyph && bold);
    if (!glyph || !bold) return;
    CHECK(glyph->width == 11 && glyph->height == 14 && glyph->advance == 12 && glyph->bearingX == 0 && glyph->bearingY == 12 && glyph->ppem == 16);
    CHECK(std::count(glyph->pixels.begin(), glyph->pixels.end(), 1) == 42);
    CHECK(std::count(bold->pixels.begin(), bold->pixels.end(), 1) == 66 && bold->advance == 12);
    for (std::size_t y = 0; y < regular.size(); ++y) // 기대 행은 제품의 디코더로 생성하지 않은 고정 관측값이다.
        for (std::size_t x = 0; x < regular[y].size(); ++x) CHECK(glyph->pixels[y * 11 + x] == (regular[y][x] == '#'));
    CHECK(EmbeddedBitmapFont::Load(FontRepo() / "fonts" / kKoreanFontRegularFile) == first);
    const auto blank = first->Glyph(0x3000); // image2의 빈 전각 공백도 유효한 12픽셀 전진 폭을 가진다.
    CHECK(blank && blank->advance == 12 && blank->pixels.empty());
    CHECK(!first->Glyph(0x1f600) && !first->Glyph(0x0378));
}

// 두 굵기의 한글 11,172자와 현대 호환 자모를 확인하고 굵은 파일의 실제 옛 자모 부재도 명시한다.
TEST_CASE(KoreanBitmap_AllHangulAndCompatibilityJamoHaveNativeCells) {
    using namespace netstorm::client;
    for (const auto* name : {kKoreanFontRegularFile, kKoreanFontBoldFile}) { // 일반/굵은 파일의 서로 다른 cmap을 각각 검사한다.
        const auto font = EmbeddedBitmapFont::Load(FontRepo() / "fonts" / name);
        CHECK(font);
        if (!font) continue;
        for (std::uint32_t cp = 0xac00; cp <= 0xd7a3; ++cp) { // 현대 한글의 첫 글자부터 마지막 글자까지 원형을 확인한다.
            const auto glyph = font->Glyph(cp);
            CHECK(glyph && glyph->advance == 12 && std::count(glyph->pixels.begin(), glyph->pixels.end(), 1) > 0);
        }
        for (std::uint32_t cp = 0x3131; cp <= 0x318e; ++cp) { // 현대 자모/채움 문자는 두 파일에, 옛 자모42자는 일반 파일에만 있다.
            const auto glyph = font->Glyph(cp);
            if (name == kKoreanFontBoldFile && cp >= 0x3165) CHECK(!glyph); // 별도 cmap 관측에서 확인한 실제 파일의 부재 범위다.
            else CHECK(glyph && glyph->advance == 12);
        }
    }
}

// GDI에 존재하지 않는 family라도 지정한 외부 파일의 픽셀을 쓴다. 작은 크기는 원형, 큰 크기는 정수 3배이며 라틴 캐시는 불변이다.
TEST_CASE(KoreanBitmap_RenderUsesFilePixelsAndIntegerScaleWithoutSystemFallback) {
    using namespace netstorm::client;
    auto font = BitmapFont::Generate("Arial", 14, 700, FontStyle::Normal);
    const auto original = font->CacheBytes();
    font->ConfigureExtended({"Uninstalled bitmap test face", FontRepo() / "fonts" / kKoreanFontRegularFile, 14, 0, false, false, false});
    CHECK(InkCount(font->ExtendedGlyph(0xd55c)) == 42 && font->ExtendedAdvance() == 12);
    CHECK(font->CacheBytes() == original);
    font->ConfigureExtended({kKoreanFontFamily, FontRepo() / "fonts" / kKoreanFontRegularFile, 48, 0, false, false, false});
    CHECK(InkCount(font->ExtendedGlyph(0xd55c)) == 42 * 9 && font->ExtendedAdvance() == 36);
    CHECK(!font->ExtendedGlyph(0x0378));
    CHECK(font->CacheBytes() == original);
    font->ConfigureExtended({kKoreanFontFamily, FontRepo() / "fonts" / kKoreanFontRegularFile, 14, 0, false, true, false});
    const auto underlined = InkCount(font->ExtendedGlyph(0xd55c));
    font->ConfigureExtended({kKoreanFontFamily, FontRepo() / "fonts" / kKoreanFontRegularFile, 14, 0, false, false, true});
    const auto struck = InkCount(font->ExtendedGlyph(0xd55c));
    font->ConfigureExtended({kKoreanFontFamily, FontRepo() / "fonts" / kKoreanFontRegularFile, 14, 0, false, true, true});
    CHECK(InkCount(font->ExtendedGlyph(0xd55c)) > std::max(underlined, struck)); // 밑줄과 취소선을 모두 남긴다.
}

// 실제 FontStore가 기본 굵기/스타일을 물리 파일로 선택하고 언어/글꼴 변경 뒤 캐시를 비운다.
TEST_CASE(KoreanBitmap_FontStoreSelectsBothFilesAndInvalidatesOldGlyphs) {
    using namespace netstorm::client;
    const auto root = FontRepo() / "cpppj/build/test-data/korean-bitmap-store";
    std::filesystem::create_directories(root / "d");
    netstorm::o::BaseFileSystem files(root);
    FontStore store(files, "Arial", [](const auto&) {});
    store.SetUserCacheDirectory(root / "cache");
    store.SetExtendedFace(kKoreanFontFamily, FontRepo() / "fonts" / kKoreanFontRegularFile, FontRepo() / "fonts" / kKoreanFontBoldFile);
    CHECK(InkCount(store.Get(5, FontStyle::Normal).ExtendedGlyph(0xd55c)) == 42);
    CHECK(InkCount(store.Get(0, FontStyle::Normal).ExtendedGlyph(0xd55c)) == 66); // 원래 0번 슬롯은 기본 굵기700이다.
    CHECK(InkCount(store.Get(5, FontStyle::Bold).ExtendedGlyph(0xd55c)) == 66);
    CHECK(InkCount(store.Get(5, FontStyle::Italic).ExtendedGlyph(0xd55c)) == 42);
    CHECK(InkCount(store.Get(5, FontStyle::Underline).ExtendedGlyph(0xd55c)) > 42);
    CHECK(InkCount(store.Get(5, FontStyle::Strikeout).ExtendedGlyph(0xd55c)) > 42);
    store.SetExtendedFace(kKoreanFontFamily, FontRepo() / "fonts" / kKoreanFontBoldFile);
    CHECK(InkCount(store.Get(5, FontStyle::Normal).ExtendedGlyph(0xd55c)) == 66);
    store.SetExtendedFace("", {});
    CHECK(!store.Get().HasExtended());
}

// SFNT 디렉터리나 참조 테이블이 잘리면 예외로 거부한다. 보호 파일의 바이트는 수정하지 않는다.
TEST_CASE(KoreanBitmap_TruncatedFontIsRejected) {
    using namespace netstorm::client;
    const auto bytes = netstorm::o::ReadFileBytes(FontRepo() / "fonts" / kKoreanFontRegularFile);
    for (const auto size : {std::size_t(0), std::size_t(12), bytes.size() - 200}) { // 머리·디렉터리·EBLC 끝의 절단을 각각 확인한다.
        bool rejected{};
        try { EmbeddedBitmapFont font(std::span<const std::uint8_t>(bytes).first(size)); }
        catch (const std::runtime_error&) { rejected = true; }
        CHECK(rejected);
    }
}

// 굵은 파일에 없는 옛 자모42자는 같은 일반 파일의 정확한 VFX 픽셀을 사용하고 정상 한글은 실제 굵은체를 유지한다.
TEST_CASE(KoreanBitmap_BoldMissingJamoUsesSameFamilyRegularPixels) {
    using namespace netstorm::client;
    auto normal = BitmapFont::Generate("Arial", 14, 700, FontStyle::Normal);
    BitmapFont bold(normal->CacheBytes());
    normal->ConfigureExtended({kKoreanFontFamily, FontRepo() / "fonts" / kKoreanFontRegularFile, 14, 0, false, false, false});
    bold.ConfigureExtended({kKoreanFontFamily, FontRepo() / "fonts" / kKoreanFontBoldFile, 14, 700, false, false, false});
    for (std::uint32_t cp = 0x3165; cp <= 0x318e; ++cp) { // 부재 범위 전체를 실제 그리기 경계에서 대조한다.
        const auto* expected = normal->ExtendedGlyph(cp);
        const auto* actual = bold.ExtendedGlyph(cp);
        CHECK(expected && actual && actual->advance == 12);
        if (!expected || !actual) continue;
        const auto left = expected->shape->Bytes(), right = actual->shape->Bytes();
        CHECK(std::equal(left.begin(), left.end(), right.begin(), right.end()));
    }
    CHECK(InkCount(bold.ExtendedGlyph(0xd55c)) == 66 && InkCount(normal->ExtendedGlyph(0xd55c)) == 42);
}

// 비트맵이 없는 기존 일본어 글꼴은 새 판독기가 가로채지 않고 같은 GDI 경로에서 계속 그린다.
TEST_CASE(KoreanBitmap_JapaneseOutlineFontRetainsGdiGlyphPath) {
    using namespace netstorm::client;
    const auto file = FindJapaneseFontFile({FontRepo()});
    CHECK(!file.empty() && !EmbeddedBitmapFont::Load(file));
    auto font = BitmapFont::Generate("Arial", 14, 700, FontStyle::Normal);
    font->ConfigureExtended({"Noto Sans JP", file, 14, 400, false, false, false});
    CHECK(font->ExtendedAdvance() > 0 && InkCount(font->ExtendedGlyph(0x3042)) > 0);
}
