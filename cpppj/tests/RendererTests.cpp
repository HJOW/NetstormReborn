// 실제 x86 기대값과 화면의 이동·투명·색 변환 결과를 검사한다.
#include "TestSupport.h"
#include "client/Renderer.h"
#include "client/Cursor.h"
#include <algorithm>
#include <bit>
#include <climits>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
// 단순한 한 픽셀 VFX 프레임. 불투명 색 0과 255도 검사할 수 있다.
std::vector<std::uint8_t> PixelShape(std::uint8_t color) {
    std::vector<std::uint8_t> bytes(40);
    // 정수를 리틀 엔디언으로 쓴다.
    const auto put = [&bytes](std::size_t offset, std::uint32_t value) {
        // 네 바이트 머리 값.
        for (std::size_t i = 0; i < 4; ++i) bytes.at(offset + i) = static_cast<std::uint8_t>(value >> (i * 8));
    };
    put(0, 0x30312e31); put(4, 1); put(8, 16);
    bytes.insert(bytes.end(), {2, color, 0});
    return bytes;
}
// 원본 변경 표 기대값의 정수 다섯 개씩을 읽는다.
std::vector<std::array<int, 5>> ParseEntries(std::string text) {
    if (text == "-") return {};
    std::replace(text.begin(), text.end(), ',', ' '); std::replace(text.begin(), text.end(), ';', ' ');
    std::istringstream input(text); std::vector<std::array<int, 5>> result;
    std::array<int, 5> entry{};
    // 입력에 있는 항목을 모두 읽는다.
    while (input >> entry[0] >> entry[1] >> entry[2] >> entry[3] >> entry[4]) result.push_back(entry);
    return result;
}
// 정상/오류 경로를 구분하는 예외 검사.
template<class Function> bool Throws(Function function) { try { function(); } catch (const std::exception&) { return true; } return false; }
}

// 두 판본의 실제 기계어 반환값과 전체 변경 표를 대조한다.
TEST_CASE(Renderer_OrderAndDirtyRegions_MatchBothOriginalX86Editions) {
    std::ifstream file(NETSTORM_RENDERER_FIXTURE); CHECK(file.good());
    std::string line; int orders = 0, sequences = 0;
    // 각 고정 기대값을 독립 C++ 구현에 입력한다.
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream input(line); std::string kind; input >> kind;
        if (kind == "Order") {
            netstorm::client::DrawOrder a, b; int expected;
            input >> a.x >> a.y >> a.depth >> b.x >> b.y >> b.depth >> expected;
            CHECK(netstorm::client::CompareDrawOrder(a, b) == expected); ++orders;
        } else {
            int width, height, full; std::string operations, expected;
            input >> width >> height >> operations >> full >> expected;
            netstorm::client::DirtyRegions regions(width, height);
            // 원본 기계어에 입력한 같은 시퀀스를 등록한다.
            for (const auto& op : ParseEntries(operations)) regions.Add({op[0], op[1], op[2], op[3]}, static_cast<std::uint32_t>(op[4]));
            const auto entries = ParseEntries(expected); CHECK(regions.FullRedraw() == (full != 0)); CHECK(regions.Entries().size() == entries.size());
            // 무시한 항목의 위치·플래그도 포함해 대조한다.
            for (std::size_t i = 0; i < std::min(entries.size(), regions.Entries().size()); ++i) {
                const auto& actual = regions.Entries()[i]; const auto& e = entries[i];
                CHECK(actual.rect.left == e[0]); CHECK(actual.rect.top == e[1]); CHECK(actual.rect.right == e[2]); CHECK(actual.rect.bottom == e[3]);
                CHECK(actual.flags == static_cast<std::uint32_t>(e[4]));
            }
            ++sequences;
        }
    }
    CHECK(orders == 261); CHECK(sequences == 133);
}

// 0/255 불투명 색·소스 변환·그림자의 대상 변환과 범위 보호를 함께 검사한다.
TEST_CASE(Renderer_ColorMapsAndShadows_PreserveTransparencyAndClip) {
    netstorm::client::IndexedImage image{3, 1, {0, 255, 100}, {255, 0, 255}};
    std::vector<std::uint8_t> pixels(8, 7);
    netstorm::client::ColorMap map{};
    // 검사 표는 원본 256항목 치환 방식대로 만든다.
    for (int c = 0; c < 256; ++c) map[c] = static_cast<std::uint8_t>(255 - c);
    netstorm::client::DrawIndexedImage(pixels, 4, 2, image, 1, 0, {0, 0, 4, 2}, &map);
    CHECK(pixels == std::vector<std::uint8_t>({7, 255, 7, 155, 7, 7, 7, 7}));
    netstorm::client::DrawIndexedImage(pixels, 4, 2, image, -1, 1, {0, 0, 2, 2}, &map, true);
    CHECK(pixels[4] == 7); CHECK(pixels[5] == 248); CHECK(pixels[6] == 7);
    CHECK(Throws([&] { netstorm::client::DrawIndexedImage(pixels, 4, 2, image, 0, 0, {0, 0, 4, 2}, nullptr, true); }));
}

// 원본 깊이 순서·이동 흔적 제거·변화 없는 프레임의 출력 0회를 확인한다.
TEST_CASE(Renderer_MovementDepthAndUnchangedFrames_UseDirtyPresentation) {
    netstorm::client::ShapeDatabase back(PixelShape(0)), front(PixelShape(255));
    netstorm::client::Renderer renderer(8, 8); std::vector<std::uint8_t> pixels(64, 9);
    netstorm::client::RenderSprite a{&back, 0, 0, 2, 2, {0, 0, 8, 8}, {2, 2, 10}, {}, false};
    netstorm::client::RenderSprite b{&front, 0, 0, 2, 2, {0, 0, 8, 8}, {2, 2, -10}, {}, false};
    auto invalid = a; invalid.block = back.Blocks().size();
    CHECK(Throws([&] { renderer.SetScene({invalid}); }));
    invalid = a; invalid.x = INT_MAX;
    CHECK(Throws([&] { renderer.SetScene({invalid}); }));
    renderer.SetScene({b, a}); CHECK(renderer.Draw(pixels, 8).size() == 1);
    CHECK(pixels[18] == 255); int updates = 0;
    renderer.Present([&](netstorm::client::ScreenRect) { ++updates; }); CHECK(updates == 1);
    CHECK(renderer.Draw(pixels, 8).empty()); renderer.Present([&](netstorm::client::ScreenRect) { ++updates; }); CHECK(updates == 1);
    b.x = 5; b.y = 5; renderer.SetScene({b}); const auto changed = renderer.Draw(pixels, 8);
    CHECK(!changed.empty()); CHECK(changed[0].right - changed[0].left < 8); CHECK(pixels[18] == 0); CHECK(pixels[45] == 255);
    renderer.Present([](netstorm::client::ScreenRect) {}); CHECK(renderer.DrawCount() == 2);
    renderer.SetScene({}); renderer.Draw(pixels, 8); renderer.Present([](netstorm::client::ScreenRect) {}); CHECK(pixels[45] == 0);
}

// 소프트웨어 커서가 움직일 때 이전 그림이 남지 않아야 한다.
TEST_CASE(Renderer_SoftwareCursor_MovesAndDisappearsWithoutTrails) {
    netstorm::client::Renderer renderer(6, 2); std::vector<std::uint8_t> pixels(12);
    const netstorm::client::IndexedImage image{2, 1, {100, 200}, {255, 0}};
    renderer.SetSoftwareCursor(&image, {1, 0}); renderer.Draw(pixels, 6); renderer.Present([](netstorm::client::ScreenRect) {});
    CHECK(pixels[1] == 100); CHECK(pixels[2] == 0);
    renderer.SetSoftwareCursor(&image, {3, 0}); renderer.Draw(pixels, 6); renderer.Present([](netstorm::client::ScreenRect) {});
    CHECK(pixels[1] == 0); CHECK(pixels[3] == 100);
    renderer.SetSoftwareCursor(nullptr, {}); renderer.Draw(pixels, 6); renderer.Present([](netstorm::client::ScreenRect) {}); CHECK(pixels[3] == 0);
    CHECK(Throws([&] { renderer.SetSoftwareCursor(&image, {INT_MAX, 0}); }));
}

// 캐시 없는 PC에서 GDI 생성·글자 출력이 원본 자료 없이도 동작하는지 검사한다.
TEST_CASE(BitmapFont_GdiFallback_ProducesReadableCachedGlyphs) {
    const auto font = netstorm::client::BitmapFont::Generate("Arial", 14, 700, netstorm::client::FontStyle::Normal);
    CHECK(font->Height() > 0); CHECK(font->Ascent() > 0); CHECK(font->Measure("NetStorm") > font->Measure("Net"));
    netstorm::client::Renderer renderer(160, 40); std::vector<std::uint8_t> pixels(6400);
    renderer.SetScene({}, {{font.get(), "NetStorm", 2, 2, 255, 0, {1, 1}, true, true}});
    renderer.Draw(pixels, 160); renderer.Present([](netstorm::client::ScreenRect) {});
    CHECK(std::count(pixels.begin(), pixels.end(), 255) > 40);
    CHECK(Throws([] { netstorm::client::BitmapFont font(std::vector<std::uint8_t>(20)); }));
}

// 원본 커서 번호의 경계와 GDI 자원 수명을 검사한다.
TEST_CASE(Cursor_DefaultResourceMappingAndSoftwareFrames_AreUsable) {
    CHECK(netstorm::client::kCursorResources[1] == 113); CHECK(netstorm::client::kCursorResources[18] == 148);
    netstorm::client::Cursor cursor(nullptr, nullptr); CHECK(cursor.Index() == 1);
    std::array<netstorm::client::ScreenColor, 256> colors{};
    // 8비트 커서를 만들 검사 팔레트.
    for (int i = 0; i < 256; ++i) colors[i] = {static_cast<std::uint8_t>(i), static_cast<std::uint8_t>(i), static_cast<std::uint8_t>(i), 0};
    cursor.BuildSoftware(colors); CHECK(cursor.SoftwareImage() != nullptr); CHECK(cursor.SoftwareImage()->indices.size() == 1024);
    CHECK(std::count(cursor.SoftwareImage()->opacity.begin(), cursor.SoftwareImage()->opacity.end(), 255) > 0);
    CHECK(Throws([&] { cursor.Set(0); })); CHECK(Throws([&] { cursor.Set(19); }));
}

// 동적 커서 수집은 dirty마다 한 번 호출하고 정적 월드와 같은 원본 깊이 순서로 합성한다.
TEST_CASE(Renderer_ExtraCollectionUsesDirtyOrderAndCachedSceneImage) {
    using namespace netstorm::client;Renderer renderer(20,10);std::size_t collections{};std::vector<int> draws;std::vector<std::uint8_t> pixels(200);
    const auto sprite=[&](int label,std::int16_t depth) { // 실제 정렬 항목의 출력 순서와 한 픽셀 덮음을 관찰한다.
        RenderSprite value;value.clip={0,0,20,10};value.order={1,1,depth};value.procedure=RenderProcedure{{0,0,20,10},[&,label](std::span<std::uint8_t> target,int,int,ScreenRect cut) { draws.push_back(label);if (cut.left<=1 && cut.right>1 && cut.top<=1 && cut.bottom>1) target[21]=static_cast<std::uint8_t>(label); }};return value;
    };
    renderer.SetScene({sprite(3,3),sprite(1,9)});renderer.SetExtraCollection([&](ScreenRect,std::uint32_t flags) { ++collections;CHECK(flags==0);return std::vector<RenderSprite>{sprite(2,5)}; });
    CHECK(renderer.HasExtraCollection());renderer.Draw(pixels,20);renderer.Present([](ScreenRect) {});CHECK(collections==1 && draws==std::vector<int>({1,2,3}) && pixels[21]==3);draws.clear();
    renderer.Invalidate({0,0,3,3},0);renderer.Invalidate({12,5,15,8},0);renderer.Draw(pixels,20);renderer.Present([](ScreenRect) {});CHECK(collections==3 && draws==std::vector<int>({1,2,3,1,2,3}));draws.clear();
    const auto image=renderer.SceneImage();CHECK(collections==3 && draws==std::vector<int>({1,2,3}) && image.indices[21]==3);
    renderer.SetExtraCollection({});CHECK(!renderer.HasExtraCollection());draws.clear();renderer.Draw(pixels,20);renderer.Present([](ScreenRect) {});CHECK(collections==3 && draws==std::vector<int>({1,3}));
}
