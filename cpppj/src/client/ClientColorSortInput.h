// 실제ColorSort가최초논리RGB를복사하는수명을소유하고기존색표계산을같은고정입력에합성한다.
#pragma once
#include "client/Renderer.h"
#include <array>
#include <cstdint>
#include <span>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include <functional>

namespace netstorm::o { class RawColorSortPrimary;class BaseFileSystem; } // 실제0x3800저장소와 현재 FILE 검색 장치를 빌린다.
namespace netstorm::client {
class OwnerColorTableState; // 5319c0/557940의 같은 Client 수명이며 각 표시기에 별도 생성하지 않는다.
// 실제Client는고정초기입력을필수로주며소유자없는기존독립표시검사만명시current fallback을사용한다.
inline std::span<const std::uint32_t,256> ColorSortLogicalInput(std::span<const std::uint32_t> fixed,std::span<const std::uint32_t,256> independentCurrent) {
    if (fixed.empty()) return independentCurrent;
    if (fixed.size()!=256) throw std::invalid_argument("ColorSort 고정 초기RGB256색 계약");
    return std::span<const std::uint32_t,256>(fixed.data(),256);
}
// 같은raw0x3800와고정RGB/기존9표를소유한다.검색0x5008/qsort/cache전체부모는별도다.
class ClientColorSortInput {
public:
    ClientColorSortInput(); //실제439de0입력캡처전에는현재ScreenRGB로빈성공을만들지않는다.
    ClientColorSortInput(const ClientColorSortInput&)=delete; //같은입력/표주소를빌린소비자에게사본을주지않는다.
    ClientColorSortInput& operator=(const ClientColorSortInput&)=delete; //현재주소를대입으로바꾸지않는다.
    ClientColorSortInput(ClientColorSortInput&&)=delete; //Client수명의고정주소를유지한다.
    ClientColorSortInput& operator=(ClientColorSortInput&&)=delete; //이미빌린span을이전소유자에남기지않는다.
    void Capture(std::span<const std::uint32_t,256> logical); //439de0의명시생성위치에서만전체RGBX입력을복사한다.일반palette변경은호출하지않는다.
    void CaptureWithPrimary(std::span<const std::uint32_t,256> logical); // 기존 실제malloc 초기BYTE 장치를 재사용하는 Client 시작 경로다.
    void CaptureWithPrimary(std::span<const std::uint32_t,256> logical,std::span<const std::uint8_t> allocationBytes); // 명시 실제0x3800초기물리로만 같은raw생성을 수행한다.
    const std::shared_ptr<o::RawColorSortPrimary>& Primary() const; // 누락/미완성은 명시거부하며 RGB/COL에서 즉석대체하지않는다.
    const std::shared_ptr<OwnerColorTableState>& OwnerTables() const; // 현재 배너/소유자 표를 모든 표시 소비자에게 같은 주소로 빌린다.
    bool MapsReady() const; // 실제9표 초기화/재생성 완료 관문이며 독립 headless 호출과 실제Client를 구분한다.
    bool Captured() const; //호스트입력계약의준비상태이며원문paletteLoaded와별개다.
    std::span<const std::uint32_t,256> Logical() const; //초기입력의같은주소를빌린다.미캡처는명시오류다.
    const std::array<ColorMap,9>& Rebuild(std::uint32_t currentRed); //43c010의현재빨강과같은초기입력으로기본53비트9표를갱신한다.64비트별도출력은혼합하지않는다.
    const std::array<ColorMap,9>& Maps() const; //명시Rebuild뒤같은현재표를빌린다.
    void InitializeCache(const o::BaseFileSystem& files,const std::filesystem::path& path,std::uint32_t currentRed,const std::function<void()>& initializeOwners); // 최초43c4e0의 두 소유자 표/등록→현재FILE9read 또는 재생성9write를 같은 저장소에 연결한다. path는 사용자Cache다.
    std::span<const std::uint8_t,2048> OwnerHighlight() const; // 같은43c4e0의8행 명도 치환 출력이며 캐시 파일에는 포함하지 않는다.
    std::span<const std::uint8_t,2048> OwnerSolid() const; // 같은43c4e0의8행 고정색 출력이며 캐시 hit에서도 먼저 작성한다.
private:
    std::array<std::uint32_t,256> logical_{}; //주소가변하지않는최초ColorSort입력이다.미캡처0은유효입력이아니다.
    std::shared_ptr<o::RawColorSortPrimary> primary_; // Scene/Screen/Session교체와무관하게같은저장double을보유한다.
    std::shared_ptr<OwnerColorTableState> ownerTables_; // 초기 옵션 해석 전부터 준비0 상태로 존재하며 실제 최초 등록은 Client 시작 경계가 수행한다.
    std::array<ColorMap,9> maps_{}; //555e38/55f38/56038/56138/56238/56338/56438/57738/57838순서다.
    std::array<std::uint8_t,2048> ownerHighlight_{},ownerSolid_{}; // 556538/556e38의 현재8×256BYTE이며 별도 파일 snapshot으로 대체하지 않는다.
    bool captured_{},built_{}; //실제원문DWORD를대신하지않는host물리준비계약이다.
};
}
