// 미적용 minimap-blit 제안 patch의 실제8BIT 출력과 독립 PE 전체 표면을 대조한다.
// GumpPixelTarget::RawBitmap/LinesClipped가 등록되기 전에는 이 파일도 빌드에 등록하지 않는다.
#include "TestSupport.h"
#include "client/GumpDraw.h"
#include "client/MinimapGeometry.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace netstorm::client;
namespace {
constexpr int kWidth=192,kHeight=160; // 독립 native의 격리 대상 표면 크기다.
constexpr std::size_t kFields=33; //24개 입력과9개 실제 원문 출력의 고정 열 수다.

// 원시 TSV/trace/run의 마지막 빈 항목도 보존한다.
std::vector<std::string> Split(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t begin{}; // 입력을 순서대로 분해하는 현재 위치다.
    for (;;) { // 정해진 단일 구분자만 분리한다.
        const auto end=text.find(delimiter,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));
        if (end==text.npos) return result;begin=end+1;
    }
}
// 입력 원문을 범위 내 signed 정수로 읽는다.
std::int64_t Number(std::string_view value) { return std::stoll(std::string(value)); }
// 입력 DWORD의 float 비트를 반올림 없이 복원한다.
float Float(std::string_view value) { return std::bit_cast<float>(static_cast<std::uint32_t>(Number(value))); }
// Python의 signed 우측 이동과 같은 바닥 나눗셈으로 원시 입력 BYTE만 복원한다.
std::uint8_t Pattern(std::int64_t index,std::int64_t seed) {
    const auto row=index>=0 ? index/128 : -((-index+127)/128); // 음수 guard 입력에서도 동일한 입력 메모리 패턴이다.
    return static_cast<std::uint8_t>(index*73+row*19+seed);
}
// hex 숫자 하나를 BYTE의 반쪽으로 읽는다.
std::uint8_t Nibble(char value) {
    if (value>='0' && value<='9') return static_cast<std::uint8_t>(value-'0');
    if (value>='a' && value<='f') return static_cast<std::uint8_t>(value-'a'+10);
    throw std::invalid_argument("미니맵 native hex 입력 오류");
}
// 독립 원문 출력의 변경 구간을 전체 배경 위에 무손실 복원한다. 복사 기대값을 재계산하지 않는다.
std::vector<std::uint8_t> NativePixels(std::string_view runs,std::uint8_t background) {
    std::vector<std::uint8_t> result(kWidth*kHeight,background); // 원문 Draw 전의 명시 배경도 전체 비교한다.
    if (runs=="-") return result;
    for (const auto& run:Split(runs,';')) { // 원문이 실제 변경한 구간만 적용한다.
        const auto pair=Split(run,':');if (pair.size()!=2 || pair[1].size()%2) throw std::invalid_argument("미니맵 native run 오류");
        const auto first=static_cast<std::size_t>(Number(pair[0]));if (first+pair[1].size()/2>result.size()) throw std::out_of_range("미니맵 native run 범위");
        for (std::size_t byte=0;byte<pair[1].size()/2;++byte) result[first+byte]=static_cast<std::uint8_t>((Nibble(pair[1][byte*2])<<4)|Nibble(pair[1][byte*2+1])); // 기록된 실제 BYTE다.
    }
    return result;
}
// native 읽기 구간에서 bitmap 할당 밖 실제 BYTE 주소 집합만 추출한다.
std::set<std::int64_t> NativeGuardReads(std::string_view ranges) {
    std::set<std::int64_t> result; // 원문 dword 읽기 횟수와 BYTE 주소 수를 혼동하지 않는다.
    if (ranges=="-") return result;
    for (const auto& range:Split(ranges,';')) { // 실제 CPU의 연속 읽기 주소를 확장한다.
        const auto pair=Split(range,':');if (pair.size()!=2) throw std::invalid_argument("미니맵 native 읽기 구간 오류");
        const auto first=Number(pair[0]),length=Number(pair[1]);
        for (std::int64_t byte=first;byte<first+length;++byte) if (byte<0 || byte>=0x4000) result.insert(byte); // 앞뒤 guard만 별도 장치에 해당한다.
    }
    return result;
}
}

// 전체120관측의 bitmap·guard 입력만 재생하고 독립 native의 전체 대상 BYTE와 대조한다.
TEST_CASE(MinimapBlit_IndependentNativeFullSurfaceAndGuardReads) {
    std::ifstream input(NETSTORM_MINIMAP_BLIT_FIXTURE);CHECK(input.good());std::string line;std::size_t rows{},guardRows{}; // 새 fixture는 적용 단위가 별도로 등록한다.
    while (std::getline(input,line)) { // 원문 출력 열로 제품 입력을 계산하지 않는다.
        if (line.empty() || line.starts_with('#')) continue;const auto fields=Split(line,'\t');CHECK(fields.size()==kFields);if (fields.size()!=kFields) continue;
        const auto failures=netstorm::test::FailureCount();const auto width=Float(fields[2]),height=Float(fields[3]); // 원문 객체의 현재 두 float 필드다.
        MinimapGeometryState geometry{width,height,static_cast<std::int32_t>(Number(fields[4])),static_cast<std::int32_t>(Number(fields[5])),0};
        MinimapGeometryView view{{static_cast<std::uint32_t>(Number(fields[12])),static_cast<std::uint32_t>(Number(fields[13]))},
            {static_cast<int>(Number(fields[14])),static_cast<int>(Number(fields[15])),static_cast<int>(Number(fields[14])+Number(fields[16])),static_cast<int>(Number(fields[15])+Number(fields[17]))},static_cast<int>(Number(fields[18])),static_cast<int>(Number(fields[19]))};
        const ScreenPoint destination{static_cast<int>(Number(fields[6])),static_cast<int>(Number(fields[7]))}; // 명시 Bounds 장치와 같은 원점이다.
        const auto plan=PlanMinimapDraw(geometry,view,destination);const auto nativeBlit=Split(fields[28],':'),nativeFrame=Split(fields[29],':'); // 실제 원문 경계 인자다.
        CHECK(nativeBlit.size()==8 && nativeFrame.size()==5);if (nativeBlit.size()!=8 || nativeFrame.size()!=5) continue;
        const auto sourceBase=std::int64_t(plan.source.y)*128+plan.source.x; // signed flat source BYTE 주소다.
        CHECK(plan.destination.x==Number(nativeBlit[0]) && plan.destination.y==Number(nativeBlit[1]) && sourceBase==Number(nativeBlit[2]));
        CHECK(Number(nativeBlit[3])==128 && nativeBlit[4]=="0" && nativeBlit[5]=="0");CHECK(plan.size.x==Number(nativeBlit[6]) && plan.size.y==Number(nativeBlit[7]));
        CHECK(plan.camera.left==Number(nativeFrame[0]) && plan.camera.top==Number(nativeFrame[1]) && plan.camera.right==Number(nativeFrame[2]) && plan.camera.bottom==Number(nativeFrame[3]));
        const auto background=static_cast<std::uint8_t>(Number(fields[22]));const auto ink=static_cast<std::uint32_t>(Number(fields[23]));CHECK(ink==Number(nativeFrame[4])); // 전용 외부 팔레트 입력이다.
        std::vector<std::uint8_t> bitmap(0x4000),pixels(kWidth*kHeight,background); // 실제 원문과 같은 입력 저장소다.
        for (std::size_t byte=0;byte<bitmap.size();++byte) bitmap[byte]=Pattern(static_cast<std::int64_t>(byte),Number(fields[20])); // 입력 bitmap만 만든다.
        GumpDrawState state{};const ScreenRect clip{static_cast<int>(Number(fields[8])),static_cast<int>(Number(fields[9])),static_cast<int>(Number(fields[10])),static_cast<int>(Number(fields[11]))};SetGumpDrawClip(state,clip);
        const GumpPixelTarget target(pixels,kWidth,kHeight,state);const ScreenRect bounds{destination.x,destination.y,destination.x+static_cast<int>(width),destination.y+static_cast<int>(height)};std::set<std::int64_t> guardReads; // bitmap 폭과 별개인 실제 창 clip이다.
        target.RawBitmap(bitmap,128,sourceBase,plan.size,plan.destination,bounds,[&](std::int64_t offset) { guardReads.insert(offset);return Pattern(offset,Number(fields[21])); });
        const auto left=plan.camera.left,top=plan.camera.top,right=plan.camera.right-1,bottom=plan.camera.bottom-1; // 원본 네 선의 끝점 포함 좌표다.
        const std::array<GumpLine,4> lines{{{{left,top},{right,top},ink},{{left,bottom},{right,bottom},ink},{{left,top},{left,bottom},ink},{{right,top},{right,bottom},ink}}};
        target.LinesClipped(lines,bounds);CHECK(pixels==NativePixels(fields[25],background));CHECK(guardReads==NativeGuardReads(fields[26]));
        CHECK(guardReads.empty()==(Number(fields[27])==0));if (!guardReads.empty()) ++guardRows;
        CHECK(state.clip.left==clip.left && state.clip.top==clip.top && state.clip.right==clip.right && state.clip.bottom==clip.bottom); // 출력은 현재 clip 전역을 변경하지 않는다.
        CHECK(fields[24]=="1" && Number(fields[31])>0 && fields[32]=="0");++rows;
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"미니맵 raw 블릿 native 입력: %s / cw%s\n",fields[0].c_str(),fields[1].c_str());
    }
    CHECK(rows==120 && guardRows==40);
}
