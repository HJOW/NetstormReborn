// 실제10.78 셀 원문 관측의 동일 raw 월드 입력과 전체128×128 bitmap을 대조한다.
#include "TestSupport.h"
#include "o/RawMinimapRaster.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace netstorm::o;
namespace {
constexpr std::size_t kBitmapSize=128U*128U,kNativeRows=4722,kColumnCount=16; // 독립 원문의 물리 출력과 두 정밀도 관측 수다.
constexpr std::uint8_t kInitialPixel=0xA5; // 실제 원문 실행기에 공급한 미수정 bitmap 표식이다.
using RasterRow=std::array<std::string,kColumnCount>; // 앞9열 입력/뒤7열 관측을 분리하지 않고 보존한다.

// TSV/레코드의 끝 빈 필드도 보존하며 원문 바이트를 바꾸지 않는다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { // 현재 구분자까지의 원문 필드만 순서대로 복사한다.
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));
        if (end==text.npos) return result;begin=end+1;
    }
}
// 원문 좌표 float 비트와 전역의 전체DWORD를 동일한 unsigned 값으로 읽는다.
std::uint32_t Number(std::string_view text) { return static_cast<std::uint32_t>(std::stoull(std::string(text))); }
// 명시 raw 필드의 지정 폭만 바꾸며 옆 필드는 원문의 A5를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("미니맵 fixture raw 쓰기 범위");
    for (std::size_t index=0;index<width;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // 실제little endian BYTE를 조합한다.
}
// 현재 실제 Pool/Hash/TYPE를 소유하되 기대 픽셀은 계산하지 않는 검사 장면이다.
struct RasterScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // Slot은 물리SID1도 읽으므로 입력을 임의 재할당하지 않는다.
    SquidHash hash; // 원문처럼 지면 WORD와 level0 머리는 같은 배열이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 실제+ec에 대응하는 flags2 입력이다.
    MinimapRasterState state; // 현재모드/색억제/프레임과 원문의12개 색 DWORD다.
    std::array<std::uint8_t,kBitmapSize> bitmap; // 제품이 변경하는 유일한 출력이며 주변 픽셀도 대조한다.
    // 독립 도구의 명시 입력 열을 같은 원시 슬롯/배열에 공급한다.
    explicit RasterScene(const RasterRow& row) {
        bitmap.fill(kInitialPixel);state.challenge=Number(row[3]);state.suppressColors=Number(row[4]);state.frame=Number(row[5]);
        const auto colors=Split(row[6],':');if (colors.size()!=12) throw std::invalid_argument("미니맵 fixture 색 DWORD 개수");
        state.black=Number(colors[0]);state.neutralIsle=Number(colors[9]);state.neutralNoIsland=Number(colors[10]);state.challengeGreen=Number(colors[11]);
        for (std::size_t index=0;index<state.owners.size();++index) state.owners[index]=Number(colors[index+1]); // owner1..8의 전용 색을 그대로 제공한다.
        if (row[7]!="-") for (const auto& item:Split(row[7],';')) { // sid:type:owner:flags2:xBits:yBits:next:state:extra 입력이다.
            const auto fields=Split(item,':');if (fields.size()!=9) throw std::invalid_argument("미니맵 fixture 슬롯 필드 개수");
            const auto sid=Number(fields[0]),kind=Number(fields[1]);if (sid>=pool.Capacity() || kind>=types.size()) throw std::out_of_range("미니맵 fixture Pool/TYPE 범위");
            const auto slot=pool.Slot({static_cast<std::uint16_t>(sid)});
            std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()}; // 비const 검사 소유 Pool의 물리 입력만 준비하며 제품은 const 조회만 한다.
            std::fill(raw.begin(),raw.end(),kInitialPixel);Put(raw,4,Number(fields[6]),2);Put(raw,10,kind,1);Put(raw,11,Number(fields[7]),1);
            Put(raw,14,Number(fields[4]));Put(raw,18,Number(fields[5]));Put(raw,34,Number(fields[2]),1);Put(raw,40,Number(fields[8]),1);types[kind].flags2=Number(fields[3]);
        }
        if (row[8]!="-") for (const auto& item:Split(row[8],';')) { // 실제level:x:y:head의 명시 배열 입력이다.
            const auto fields=Split(item,':');if (fields.size()!=4) throw std::invalid_argument("미니맵 fixture Hash 필드 개수");
            hash.Cell(static_cast<int>(Number(fields[0])),static_cast<int>(Number(fields[1])),static_cast<int>(Number(fields[2])))=static_cast<std::uint16_t>(Number(fields[3]));
        }
    }
};
// 실제 원문이 바꾼 위치의 관측만 펼쳐 미수정 주변까지 포함한 기대 bitmap을 복원한다.
std::array<std::uint8_t,kBitmapSize> ObservedBitmap(std::string_view pixels) {
    std::array<std::uint8_t,kBitmapSize> expected;expected.fill(kInitialPixel);
    if (pixels!="-") for (const auto& item:Split(pixels,';')) { // 출력offset:BYTE는 원문 관측이며 타입/색 선택을 재계산하지 않는다.
        const auto fields=Split(item,':');if (fields.size()!=2) throw std::invalid_argument("미니맵 fixture 픽셀 필드 개수");
        const auto offset=Number(fields[0]),value=Number(fields[1]);if (offset>=expected.size() || value>255) throw std::out_of_range("미니맵 fixture 픽셀 범위");
        expected[offset]=static_cast<std::uint8_t>(value);
    }
    return expected;
}
}

// 하위 대체 없이 정상RET한4722관측 전체에 대해 실제 월드의 완전한bitmap을 대조한다.
TEST_CASE(MinimapRaster_Native4722WholeBitmap) {
    std::ifstream input(NETSTORM_MINIMAP_CELL_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t count{};
    while (std::getline(input,line)) { // 두 정밀도별 원문 관측을 빠뜨리지 않고 같은 제품 본체에 재생한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with('#')) continue;
        const auto fields=Split(line,'\t');CHECK(fields.size()==kColumnCount);if (fields.size()!=kColumnCount) continue;
        RasterRow row;for (std::size_t index=0;index<row.size();++index) row[index]=fields[index]; // 독립 입력/관측 열을 수정하지 않는다.
        CHECK(row[9]=="1");CHECK(row[13]=="0");CHECK(Number(row[14])>0);CHECK(row[15].size()==64);
        RasterScene scene(row);const RawMinimapRaster raster(scene.pool,scene.hash,scene.types,scene.state);
        raster.DrawCell(scene.bitmap,static_cast<std::int32_t>(Number(row[1])),static_cast<std::int32_t>(Number(row[2])));
        CHECK(scene.bitmap==ObservedBitmap(row[10]));++count;
    }
    CHECK(count==kNativeRows);
}
