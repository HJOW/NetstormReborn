// 실제 CRT의 저장 바이트 관찰과 스프라이트/화면 연결을 검사한다. 게임·창·원본 PE는 실행하지 않는다.
#include "ConstructionSupport.h"
#include "client/DrawSort.h"
#include "client/Renderer.h"
#include <cstring>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 독립 원본 fixture는 완성 입력과 전체 정렬 출력 바이트를 저장한다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_DRAWSORT_FIXTURE).rows;return rows; }
// 입력/관찰의 16진수 바이트를 실제 16바이트 항목으로 읽는다. 정렬 결과를 계산하지 않는다.
std::vector<RenderCollectionEntry> Entries(const std::string& hex) {
    if (hex=="-") return {};CHECK(hex.size()%32==0);std::vector<RenderCollectionEntry> result;
    // 원본 float/WORD/종류/tail은 숫자 문자열로 다시 변환하지 않고 비트를 그대로 읽는다.
    for (std::size_t offset=0;offset<hex.size();offset+=32) {
        std::array<std::uint8_t,16> bytes{};
        // 각 바이트를 입력 순서대로 little endian 구조에 공급한다.
        for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(std::stoul(hex.substr(offset+index*2,2),nullptr,16));
        result.push_back(std::bit_cast<RenderCollectionEntry>(bytes));
    }
    return result;
}
// 실제 출력 항목과 미사용 바이트를 원본 관찰과 같은 형태로 직렬화한다.
std::string Packed(std::span<const RenderCollectionEntry> entries) {
    return entries.empty() ? "-" : Hex({reinterpret_cast<const std::uint8_t*>(entries.data()),entries.size_bytes()});
}
// 다음 통합 검사는 같은 키 9개의 원본 최종 순서를 조회한다. host 정렬로 기대값을 만들지 않는다.
const auto& TieRow(std::string_view edition) {
    // 저장한 9항목/전체 동률/정렬 켬 행만 고른다.
    for (const auto& row:Fixture()) if (row[0]==edition && row[1]=="Sort" && row[2].starts_with("9,0,")) return row;
    throw std::runtime_error("9항목 동률 원본 관찰 누락");
}
// 세 원본의 qsort/작은 구간/옵션 끔 관찰을 실제 C++ 표시 목록 몸체에 대조한다.
void Replay(std::string_view edition) {
    const auto version=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;std::size_t count=0;
    // 기대 순서와 키 외 바이트는 원본 fixture에서만 읽는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==8);auto source=Entries(row[3]);const auto input=Inputs(row[2]);
        CHECK(source.size()==input[0]);std::vector<RenderCollectionEntry> guarded(source.size()+2);std::memset(guarded.data(),0xa5,guarded.size()*sizeof(RenderCollectionEntry));
        std::copy(source.begin(),source.end(),guarded.begin()+1);auto work=std::span(guarded).subspan(1,source.size());
        if (row[1]!="Wrapper" || input[3]) SortDrawEntries(work,version);
        const bool same=Packed(work)==row[4] && Packed(std::span(guarded).first(1))+Packed(std::span(guarded).last(1))==row[5];CHECK(same);
        if (!same) { std::printf("표시 CRT 정렬 %s 행 %zu %s 불일치\n",std::string(edition).c_str(),count,row[1].c_str());break; }++count;
    }
    CHECK(count==(edition=="originals" ? 325U : 415U));
}
// 같은 위치의 한 픽셀 프레임으로 실제 Renderer의 마지막 그리기 색을 관찰한다.
std::vector<std::uint8_t> PixelShape(std::uint8_t color) {
    std::vector<std::uint8_t> bytes(40);Put(bytes,0,0x30312e31);Put(bytes,4,1);Put(bytes,8,16);bytes.insert(bytes.end(),{2,color,0});return bytes;
}
}
// 10.78의 중앙값·피벗 갱신·구간 스택·작은 정렬 몸체를 대조한다.
TEST_CASE(DrawSort_PatchNative) { Replay("originals"); }
// CD의 첫 칸 피벗·byte swap·옵션 wrapper를 대조한다.
TEST_CASE(DrawSort_CdNative) { Replay("originalCD"); }
// 추가 판본도 별도 PE의 전체 출력 관찰과 대조한다.
TEST_CASE(DrawSort_AdditionalNative) { Replay("original1037"); }

// 실패한 키 검사나 빈 목록은 항목을 부분 교환하지 않는다.
TEST_CASE(DrawSort_InvalidKeysPreserveWholeInput) {
    auto entries=Entries(TieRow("originals")[3]);const auto original=Packed(entries);SortDrawEntries({});
    // NaN/양·음 무한대와 x/y 양쪽 오류를 실제 계약에 전달한다.
    for (const auto invalid:{0x7fc00000U,0x7f800000U,0xff800000U}) {
        entries[4].x=Float(invalid);auto before=Packed(entries);CHECK(Throws([&] { SortDrawEntries(entries); }));CHECK(Packed(entries)==before);
        entries[4].x=0;entries[4].y=Float(invalid);before=Packed(entries);CHECK(Throws([&] { SortDrawEntries(entries,OriginalEdition::Cd1072); }));CHECK(Packed(entries)==before);entries[4].y=0;
    }
    entries=Entries(TieRow("originals")[3]);SortDrawEntries(std::span(entries).first(1));CHECK(Packed(entries)==original);
}

// 키와 함께 자산/프레임/좌표/클립/색 변환/그림자도 원래 스프라이트에서 옮겨야 한다.
TEST_CASE(DrawSort_SpriteFieldsMoveWithNativePermutation) {
    // 실제 두 알고리즘의 서로 다른 동률 순서를 모두 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto& row=TieRow(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD");const auto keys=Entries(row[3]),expected=Entries(row[4]);
        ShapeDatabase shape(PixelShape(77));std::vector<RenderSprite> sprites;
        // 서로 다른 부가 필드를 같은 원본 키에 연결한다.
        for (const auto& entry:keys) {
            ColorMap colors{};colors[7]=static_cast<std::uint8_t>(entry.token);
            sprites.push_back({&shape,entry.token,entry.token+1,static_cast<int>(entry.token),-static_cast<int>(entry.token),
                {1,2,static_cast<int>(entry.token)+3,4},{entry.x,entry.y,entry.depth},colors,(entry.token&1)!=0});
        }
        const auto before=sprites;SortRenderSprites(sprites,edition);
        // 전체 필드가 원본 관찰의 토큰에 대응하는 원래 항목과 같은지 비교한다.
        for (std::size_t index=0;index<sprites.size();++index) {
            const auto& actual=sprites[index];const auto& wanted=before[expected[index].token-1];
            CHECK(actual.database==wanted.database && actual.block==wanted.block && actual.frame==wanted.frame && actual.x==wanted.x && actual.y==wanted.y);
            CHECK(actual.clip.left==wanted.clip.left && actual.clip.top==wanted.clip.top && actual.clip.right==wanted.clip.right && actual.clip.bottom==wanted.clip.bottom);
            CHECK(actual.colors==wanted.colors && actual.shadow==wanted.shadow && Bits(actual.order.x)==Bits(wanted.order.x) && Bits(actual.order.y)==Bits(wanted.order.y) && actual.order.depth==wanted.order.depth);
        }
    }
}

// 실제 Renderer가 정렬 결과대로 겹친 SHP를 그리며 정렬 끔은 제출 순서로 그린다.
TEST_CASE(DrawSort_RendererUsesNativeTieOrderAndSortOption) {
    // 장치를 열지 않고 두 판본의 소프트웨어 화면 픽셀을 관찰한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto& row=TieRow(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD");const auto keys=Entries(row[3]),expected=Entries(row[4]);
        std::vector<std::unique_ptr<ShapeDatabase>> shapes;std::vector<RenderSprite> sprites;
        // 같은 화면 위치의 서로 다른 픽셀 색을 원본 입력 순서로 제출한다.
        for (const auto& entry:keys) {
            shapes.push_back(std::make_unique<ShapeDatabase>(PixelShape(static_cast<std::uint8_t>(entry.token))));
            sprites.push_back({shapes.back().get(),0,0,1,1,{0,0,4,4},{entry.x,entry.y,entry.depth},{},false});
        }
        Renderer renderer(4,4,edition);renderer.SetScene(sprites);CHECK(renderer.SceneImage().indices[5]==expected.back().token);
        std::vector<std::uint8_t> pixels(16);renderer.Draw(pixels,4);CHECK(pixels[5]==expected.back().token);renderer.Present([](ScreenRect) {});
        renderer.SetScene(sprites,{},false);renderer.Draw(pixels,4);CHECK(pixels[5]==keys.back().token);renderer.Present([](ScreenRect) {});
        CHECK(renderer.SceneImage().indices[5]==keys.back().token);
    }
}
