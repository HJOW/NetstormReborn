// 실제 원본 VFX 타원/퇴화 선의 전체8비트 화면을 격리 대조한다.
#include "TestSupport.h"
#include "client/VFXEllipse.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace netstorm::client;
namespace { // 원문 인자와 픽셀 BYTE만 읽는 작은 검사 도우미다.
std::vector<std::string> Split(std::string_view text,char delimiter) { // 마지막 열도 보존한다.
    std::vector<std::string> result;std::size_t start=0; // 읽은 열과 현재 시작이다.
    for (;;) { const auto end=text.find(delimiter,start);result.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));if (end==std::string_view::npos) return result;start=end+1; } // 각 구분자를 한 번씩 소비한다.
}
std::vector<std::uint8_t> Hex(std::string_view text) { // Native 기대 픽셀을 다시 계산하지 않는다.
    if (text.size()%2) throw std::runtime_error("VFX 타원 hex 폭 오류");std::vector<std::uint8_t> result; // 원본 순서의 화면 BYTE다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16))); // 두 문자마다 한 BYTE다.
    return result;
}
}
TEST_CASE(VfxEllipse_Native270WholePixelsSignedRadiiDegenerateLineAndClip) { // 실제 EvilRay 음수 반경·두 정수 구간·퇴화/오프스크린/한픽셀 pane을 원문과 비교한다.
    std::ifstream input(std::filesystem::path(__FILE__).parent_path()/"fixtures/vfx-ellipse-x86.tsv");CHECK(input.good());std::string line;std::size_t count=0; // Native 독립 입력 재생 수다.
    while (std::getline(input,line)) { // 입력만 읽고 제품 래스터를 호출하여 기대 화면과 비교한다.
        if (line.empty() || line.front()=='#') continue;const auto row=Split(line,'\t');CHECK(row.size()==2);if (row.size()!=2) return;const auto arguments=Split(row[0],',');CHECK(arguments.size()==9);if (arguments.size()!=9) return;const auto expected=Hex(row[1]);CHECK(expected.size()==64*48);if (expected.size()!=64*48) return;
        const ScreenRect clip{std::stoi(arguments[0]),std::stoi(arguments[1]),std::stoi(arguments[2]),std::stoi(arguments[3])};const ScreenPoint center{std::stoi(arguments[4]),std::stoi(arguments[5])};std::vector<std::uint8_t> protectedPixels(64*48+64,0x7d);auto pixels=std::span<std::uint8_t>(protectedPixels).subspan(32,64*48);std::fill(pixels.begin(),pixels.end(),std::uint8_t{199}); // 앞뒤32BYTE는 원문과 같은 sentinel이다.
        DrawVfxEllipse(pixels,64,48,clip,center,std::stoi(arguments[6]),std::stoi(arguments[7]),static_cast<std::uint32_t>(std::stoul(arguments[8])));const auto mismatch=std::mismatch(pixels.begin(),pixels.end(),expected.begin());
        if (mismatch.first!=pixels.end()) { const auto at=static_cast<std::size_t>(mismatch.first-pixels.begin());const auto message="VFX 타원 입력"+row[0]+" 픽셀("+std::to_string(at%64)+","+std::to_string(at/64)+") 실제"+std::to_string(*mismatch.first)+" 기대"+std::to_string(*mismatch.second);netstorm::test::ReportFailure(__FILE__,__LINE__,message.c_str()); }
        CHECK(std::all_of(protectedPixels.begin(),protectedPixels.begin()+32,[](auto value) { return value==0x7d; }) && std::all_of(protectedPixels.end()-32,protectedPixels.end(),[](auto value) { return value==0x7d; }));++count;
    }
    CHECK(count==270);
}
