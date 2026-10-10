// 파일 로더와 전체 색 표를 세 실제 PE 관찰에 대조한다. 창·원본 게임·소리 장치는 실행하지 않는다.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "TestSupport.h"
#include "client/Screen.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace netstorm::client;
namespace {
// 검사 중 참조 메모리 DC를 소유한다. Screen보다 오래 살아 있어 선택 객체를 정상 해제하게 한다.
struct ReferenceDc {
    HDC value{CreateCompatibleDC(nullptr)};
    // 참조 DC만 해제한다. 검사 화면의 팔레트/DC는 Screen이 먼저 정리한다.
    ~ReferenceDc() { if (value) DeleteDC(value); }
};
// 실제 관찰의 byte hex를 채널 재배치 없이 읽는다.
std::vector<std::uint8_t> ReadHex(const std::string& hex) {
    std::vector<std::uint8_t> result(hex.size()/2);
    // 파일/메모리 원래 순서를 그대로 유지한다.
    for (std::size_t i=0;i<result.size();++i) result[i]=static_cast<std::uint8_t>(std::stoul(hex.substr(i*2,2),nullptr,16));
    return result;
}
// 배열 fixture의 little-endian DWORD를 읽는다.
std::uint32_t ReadWord(const std::vector<std::uint8_t>& bytes,std::size_t offset) {
    return bytes[offset]|(std::uint32_t{bytes[offset+1]}<<8)|(std::uint32_t{bytes[offset+2]}<<16)|(std::uint32_t{bytes[offset+3]}<<24);
}
// 파일 색 표의 기본/날씨 배열과 각 RGB/색 번호를 모두 비교한다.
bool SameTable(const PaletteColorTable& a,const PaletteColorTable& b) {
    if (a.basic!=b.basic || a.weather!=b.weather || a.named.size()!=b.named.size()) return false;
    // 별도 원본 전역에 저장한 중복 RGB 항목도 개별 비교한다.
    for (std::size_t i=0;i<a.named.size();++i) {
        const auto& x=a.named[i];const auto& y=b.named[i];
        if (x.red!=y.red || x.green!=y.green || x.blue!=y.blue || x.index!=y.index) return false;
    }
    return true;
}
}

// 원본의 파일 변환/예약 바이트/기본 9색/전체 58·50색/날씨 별칭을 같은 입력으로 검증한다.
TEST_CASE(PaletteColors_MatchesThreeNativeFileLoaders) {
    ReferenceDc dc;CHECK(dc.value!=nullptr);if (!dc.value) return;
    std::ifstream input(NETSTORM_PALETTECOLORS_FIXTURE);CHECK(input.good());std::string line;std::size_t cases=0;
    // 입력과 기대값을 모두 fixture에서 읽는다. C++ 검색 구현으로 기대값을 만들지 않는다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        std::istringstream row(line);std::string edition,fileHex,previousHex,savedHex,basicHex,weatherHex,logicalHex,named;
        unsigned number{},reuse{};row>>edition>>number>>reuse>>fileHex>>previousHex>>savedHex>>basicHex>>weatherHex>>logicalHex>>named;
        CHECK(!row.fail());if (row.fail()) continue;
        const auto format=edition=="originals" ? netstorm::o::OriginalEdition::Patch1078 : netstorm::o::OriginalEdition::Cd1072;
        Screen screen(nullptr,dc.value,640,480);screen.Init();
        screen.LoadPalette(GamePalette(ReadHex(previousHex)),format);screen.LoadPalette(GamePalette(ReadHex(fileHex)),format);
        const auto saved=ReadHex(savedHex),basic=ReadHex(basicHex),weather=ReadHex(weatherHex),logical=ReadHex(logicalHex);
        const auto failures=netstorm::test::FailureCount();
        CHECK(std::memcmp(screen.Palette().data(),saved.data(),saved.size())==0);
        const auto& colors=screen.Colors();
        // 기본 배열과 날씨 별칭은 실제 전역의 최종 바이트를 비교한다.
        for (std::size_t i=0;i<colors.basic.size();++i) CHECK(colors.basic[i]==ReadWord(basic,i*4));
        // 원소 순서는 바람·비·천둥·해다. 색 검색 결과와 별칭 복사를 별개로 확인한다.
        for (std::size_t i=0;i<colors.weather.size();++i) CHECK(colors.weather[i]==ReadWord(weather,i*4));
        std::istringstream items(named);std::string item;std::size_t count=0;
        // 실제 검색에서 관찰한 RGB 인자와 반환 색 번호를 원본 순서로 비교한다.
        while (std::getline(items,item,'|')) {
            std::istringstream fields(item);std::string value;unsigned values[4]{};
            // 각 항목은 R,G,B,index 네 정수다.
            for (unsigned i=0;i<4;++i) { CHECK(static_cast<bool>(std::getline(fields,value,',')));values[i]=static_cast<unsigned>(std::stoul(value)); }
            CHECK(count<colors.named.size());if (count>=colors.named.size()) break;
            const auto& color=colors.named[count];
            CHECK(color.red==values[0] && color.green==values[1] && color.blue==values[2] && color.index==values[3]);
            CHECK(screen.FindColor(static_cast<std::int32_t>(values[0]),static_cast<std::int32_t>(values[1]),static_cast<std::int32_t>(values[2]))==values[3]);
            ++count;
        }
        CHECK(count==colors.named.size() && count==(edition=="originals" ? kPatchPaletteColorCount : kCdPaletteColorCount));
        // 원본 논리 플래그는 예약 바이트가 아닌 4다. RGB 계약은 저장 팔레트와 함께 검사한다.
        for (std::size_t i=0;i<256;++i) {
            const auto& color=screen.Palette()[i];const auto word=ReadWord(logical,i*4);
            CHECK((word&0xffffffU)==(color.red|(std::uint32_t{color.green}<<8)|(std::uint32_t{color.blue}<<16)));
            CHECK((word>>24)==((i==0 || i==255) ? 0U : 4U));
        }
        if (failures!=netstorm::test::FailureCount()) std::fprintf(stderr,"PaletteColors fixture: %s case %u reuse %u\n",edition.c_str(),number,reuse);
        ++cases;
    }
    CHECK(cases==24);
}

// 번개/일시 적용은 파일 로더의 색 표를 갱신하지 않는다. 새 파일을 읽을 때만 새 RGB로 계산한다.
TEST_CASE(PaletteColors_TemporaryFlashKeepsFileTable) {
    ReferenceDc dc;CHECK(dc.value!=nullptr);if (!dc.value) return;
    Screen screen(nullptr,dc.value,640,480);screen.Init();std::array<std::uint8_t,1024> bytes{};
    // 초기 팔레트의 각 번호를 서로 다른 회색으로 채운다.
    for (std::size_t i=0;i<256;++i) { bytes[i*4]=bytes[i*4+1]=bytes[i*4+2]=static_cast<std::uint8_t>(i);bytes[i*4+3]=91; }
    screen.LoadPalette(GamePalette(bytes));const auto original=screen.Colors();ScreenColor flash{255,255,255,47};
    screen.SetPalette(9,1,&flash,true);CHECK(SameTable(screen.Colors(),original));
    screen.SetPalette(0,256,nullptr,true);CHECK(SameTable(screen.Colors(),original));
    std::fill(bytes.begin(),bytes.end(),std::uint8_t{19});screen.LoadPalette(GamePalette(bytes));
    CHECK(!SameTable(screen.Colors(),original));CHECK(screen.Colors().weather==screen.WeatherTints());
}

// RGB COL은 이전 BGRX의 예약 바이트를 보존하고 두 끝점은 SetPalette에서 항상 0으로 바뀐다.
TEST_CASE(PaletteColors_ColPreservesPreviousReservedBytes) {
    ReferenceDc dc;CHECK(dc.value!=nullptr);if (!dc.value) return;
    Screen screen(nullptr,dc.value,640,480);screen.Init();std::array<std::uint8_t,1024> previous{};std::array<std::uint8_t,776> col{};
    // 앞 파일의 예약 값과 뒤 COL의 채널을 서로 구별되는 값으로 만든다.
    for (std::size_t i=0;i<256;++i) { previous[i*4+3]=static_cast<std::uint8_t>(i);col[8+i*3]=30;col[9+i*3]=50;col[10+i*3]=70; }
    const GamePalette bgrx(previous),rgb(col);CHECK(bgrx.Reserved(43)==43);CHECK(!rgb.Reserved(43));
    screen.LoadPalette(bgrx);screen.LoadPalette(rgb);
    // 전체 내부 색의 RGB가 바뀌어도 예약값은 앞 파일에 남아 있는 값이다.
    for (std::size_t i=1;i<255;++i) {
        const auto& color=screen.Palette()[i];CHECK(color.red==30 && color.green==50 && color.blue==70 && color.reserved==i);
    }
    CHECK(screen.Palette()[0].reserved==0 && screen.Palette()[255].reserved==0);
}

// 해상도 변경의 현재 BGRX 복사는 예약 바이트와 선택 판본의 전체 색 표를 보존한다.
TEST_CASE(PaletteColors_ResolutionCopyRetainsTableAndReservedBytes) {
    ReferenceDc dc;CHECK(dc.value!=nullptr);if (!dc.value) return;
    Screen first(nullptr,dc.value,640,480);first.Init();std::array<std::uint8_t,1024> bytes{};
    // 각 색의 네 바이트를 전부 다르게 채워 RGB만 복사하는 오류를 드러낸다.
    for (std::size_t i=0;i<bytes.size();++i) bytes[i]=static_cast<std::uint8_t>((i*17+i/4)%256);
    first.LoadPalette(GamePalette(bytes),netstorm::o::OriginalEdition::Cd1072);const auto table=first.Colors();
    std::memcpy(bytes.data(),first.Palette().data(),bytes.size());
    ReferenceDc secondDc;CHECK(secondDc.value!=nullptr);if (!secondDc.value) return;
    Screen second(nullptr,secondDc.value,800,600);second.Init();second.LoadPalette(GamePalette(bytes),netstorm::o::OriginalEdition::Cd1072);
    CHECK(std::memcmp(second.Palette().data(),bytes.data(),bytes.size())==0);CHECK(SameTable(second.Colors(),table));
    CHECK(second.Colors().named.size()==kCdPaletteColorCount);
}
