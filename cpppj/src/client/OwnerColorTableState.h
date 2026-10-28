// 원본557938/5319c0/557940의 수명과 현재 색표를 Client 전체 소비자가 공유한다.
#pragma once
#include "client/OwnerColorMaps.h"
#include <functional>
#include <memory>
#include <vector>

namespace netstorm::o { class RawColorSortPrimary; } // 최초 ColorSort의 실제 저장 명도를 직접 빌린다.
namespace netstorm::client { // 장치 없는 색표 수명과 실제 Client 공유 저장소다.
struct OwnerColorTableTypes { // 보호10.78의 현재 TYPE 전역이며 순서도 실제 호출 순서다.
    std::array<std::uint32_t,5> banner{85,71,128,97,94}; // 켤 때 반납할 표이며 끌 때는 마지막 두 표만 만든다.
    std::array<std::uint32_t,12> standard{82,155,158,164,135,133,156,73,170,146,144,126}; // 최초 일반 표 등록 순서다.
    std::array<std::uint32_t,3> special{111,141,137}; // 최초 전용 명도 표다.
};
struct OwnerColorTableGlobals { // 실제 현재 DWORD와 포인터 표를 복사하지 않고 빌린다.
    std::uint32_t &ready,&enabled,&redraw; // 557938/5319c0/59a8b0의 현재 값이다.
    std::span<std::uint32_t> tables; // 557940의 TYPE별 현재 표 토큰이다.
};
struct OwnerColorTableHooks { // 생성 장치와 반납 장치만 외부 효과로 분리한다.
    std::function<void(std::uint32_t)> standard; //43ba00의 실제 일반9표 생성이다.
    std::function<void(std::uint32_t,double,std::span<const std::uint32_t>)> special; //43bbe0의 타입/명도 보정/가변 소스다.
    std::function<void(std::uint32_t)> release; // 현재 비제로 포인터의 CRT free다. 뒤0쓰기는 원본 몸체가 수행한다.
};
// 43bc40의 준비/동일 값 관문·DWORD store·생성/반납 순서를 그대로 실행한다.
void SetBannerOption(OwnerColorTableGlobals globals,std::uint32_t requested,const OwnerColorTableTypes& types,const OwnerColorTableHooks& hooks);
// 43be30의 강제-1 설정과 일반12/전용3표 등록이다. skip은59ab28의 현재 값이다.
void InitializeOwnerColorTables(OwnerColorTableGlobals globals,std::uint32_t skip,const OwnerColorTableTypes& types,const OwnerColorTableHooks& hooks);

class OwnerColorTableState { // 모든 화면·미리보기·이미지·픽셀 선택이 빌리는 단일 현재 저장소다.
public:
    OwnerColorTableState()=default; // PE 초기 ready0/useBanners1만 설치한다.
    OwnerColorTableState(const OwnerColorTableState&)=delete; // 현재 포인터 표 주소를 복제하지 않는다.
    OwnerColorTableState& operator=(const OwnerColorTableState&)=delete; // 이미 빌린 span을 다른 저장소로 바꾸지 않는다.
    void Initialize(std::shared_ptr<o::RawColorSortPrimary> primary,std::size_t typeCount,std::uint32_t bridgeType,std::uint32_t& redraw,std::uint32_t skip=0); // 최초 등록 위치에서만 호출한다.
    void SetEnabled(std::uint32_t value,std::uint32_t& redraw); // 설정 해석은 초기 준비 전에도 호출 가능하며 원문처럼 무시된다.
    const std::uint32_t& Enabled() const; // SpriteDraw가 같은5319c0을 출력 순간에 읽는다.
    bool Ready() const; // 실제557938이며 별도 화면/Session 생성 여부로 대체하지 않는다.
    std::span<const std::uint32_t> Tokens() const; // 초기화 뒤 주소가 바뀌지 않는 실제 현재 TYPE 표다.
    const OwnerColorMaps::value_type& Color(std::uint32_t type,std::uint32_t palette) const; // 같은 소유자 색 번호0..8의 현재 표다.
    const OwnerColorMaps::value_type& ColorToken(std::uint32_t token) const; // ImageGump의 같은 base+색번호*256 토큰을 판독한다.
private:
    OwnerColorTableHooks Hooks(); // 현재 자신의 표만 생성/반납하는 실제 장치를 만든다.
    void Standard(std::uint32_t type); // 기존 일반 색띠 계산을 같은 저장소에 설치한다.
    void Special(std::uint32_t type,double offset,std::span<const std::uint32_t> sources); // 기존 실제 primary 명도 계산을 설치한다.
    std::uint32_t ready_{},enabled_{1},bridgeType_{82}; // 보호PE 초기값과 실제 다리 TYPE다.
    OwnerColorTableTypes types_; // 실제10.78 타입 전역의 확인된 값이다.
    std::shared_ptr<o::RawColorSortPrimary> primary_; // 저장된 명도가 모든 표보다 오래 살아 있게 한다.
    std::vector<std::uint32_t> tokens_; // 같은 TYPE별 현재 포인터 별칭이며 F7은 원소만 바꾼다.
    std::vector<std::unique_ptr<OwnerColorMaps>> maps_; // 각 비제로 토큰의 실제9×256 BYTE 소유자다.
};
}
