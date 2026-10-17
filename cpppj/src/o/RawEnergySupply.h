// Mana.cpp의 실제 공급원 수집·요구 문자 대응·배치 허용값을 단일 월드에 적용한다.
#pragma once
#include "o/SquidPostPop.h"
#include <string>

namespace netstorm::o {
inline constexpr std::size_t kEnergySourceCapacity=30; // 004fcb80의 push1e로 생성되는 실제 lastSource 용량이다.
inline constexpr std::size_t kEnergyProviderCapacity=256; // 004fcb60의 push100→592f80→44fb20이 확보하는 공급 장부 용량이다.
// 보호 PE에서 확인한83개 PATCH 가상 표의+a0를 조회한다. 표 밖은 임의 기본 함수로 보내지 않는다.
std::uint32_t EnergyProviderEntry(std::uint32_t vtable);
// 커서 반경0052f488과 별개인 제곱 반경0052f48c 및 현재 Mana 전역이다.
struct EnergySupplyState {
    float radiusSquared{484}; // 공급 가상+a0가 매 호출 읽는 현재 제곱 반경이다.
    std::uint32_t editor{},special{},mission{},userCheat{},ignore{},allied{1}; // 5c85a4/55a480/594fa4/5cab30/592f48/540cb0이다.
};
// Session이 이미 소유한 전역은 해당 읽기 위치에서 조회한다. 독립 원문 검사만 값형 상태를 사용한다.
struct EnergySupplyReads {
    std::function<std::uint32_t()> editor,special,mission; // 실제 편집/특수/미션 저장소의 현재 DWORD다.
};
// +a0의 정확한 가상 표 분배와 TYPE+a0 요구 문자열만 외부에서 공급한다.
struct EnergySupplyHooks {
    std::function<std::uint32_t(std::uint32_t)> providerEntry; // 현재 가상 표의 실제+a0 주소다. 알 수 없는 표는 거부한다.
    std::function<std::string(std::uint32_t)> requirement; // 현재 TYPE와 보호 AST로 만든 실제 mana 바이트다.
};
// 4730c0/473330/4734d0/473570 및 실제 두 공급 가상 몸체를 구현한다.
class RawEnergySupply {
public:
    // providers/selected는 현재 월드의 원본 DWORD 목록이다. selected는 Builder와 같은 저장소를 빌린다.
    RawEnergySupply(const SidPool& pool,std::span<const RiftTypeRecord> types,const SquidPostPopList& providers,
        SquidPostPopList& selected,std::span<const std::uint32_t> alliances,const std::int32_t& mapLimit,
        EnergySupplyState& state,EnergySupplyHooks hooks,EnergySupplyReads reads={});
    // 4730c0: owner/alliance와 가상 범위를 검사해 목록을 갱신하고 최대254바이트의 공급 문자를 만든다.
    void Collect(std::string& output,float x,float y,std::uint32_t type,std::uint32_t owner,std::int32_t theme=-1) const;
    // 473330: 대응한 공급 문자를 점으로 소모한다. remainder가 있으면 남은 원본 공급 문자열도 돌려준다.
    std::uint32_t Match(std::string& output,std::string_view required,std::string* remainder=nullptr,std::int32_t theme=-1) const;
    // 4734d0: 빈 요구는1, 나머지는 대응 성공 시 수집 문자열의 원래 길이를 반환한다. bool로 축약하지 않는다.
    std::uint32_t Evaluate(std::string& output,float x,float y,std::uint32_t type,std::uint32_t owner) const;
    // 41c0f0/41c170: base는0, 공급형은 실제 중심과 대상 크기 항을 더한 엄격한 거리 비교다.
    bool Covers(Sid provider,const std::array<float,4>& bounds,float centerX,float centerY,float sizeSquared,std::uint32_t flags) const;
    // 473570: 첫 비영 DWORD만0으로 바꿔 반환하며 목록의 count를 줄이지 않는다.
    std::uint32_t TakeSource() const;
    const SidPool& Pool() const; // 다른 월드의 커서/AI/Builder가 잘못 연결되는 것을 막는다.
private:
    const RiftTypeRecord& Type(std::uint32_t type) const; // 현재 TYPE 번호를 검증한다.
    std::span<const std::uint8_t> Raw(Sid sid) const; // 원본의 읽기 범위만 검사하며 void/dead를 임의 배제하지 않는다.
    std::array<float,4> Bounds(std::uint32_t type,float x,float y) const; // 실제49ae80(flags0)의 클램프/유효 점 처리를 실행한다.
    bool Override() const; // 같은 원본의 특수 또는 미션+userCheat, 그리고 ignore 조건이다.
    std::uint32_t Read(std::uint32_t fallback,const std::function<std::uint32_t()>& current) const; // 연결된 현재 전역만 우선한다.
    const SidPool& pool_; // 변경하지 않고 현재 공급 객체의 raw를 읽는 단일 풀이다.
    std::span<const RiftTypeRecord> types_; // 같은 현재188TYPE다.
    const SquidPostPopList& providers_; // postPop/삭제가 갱신하는 현재 공급자 목록이다.
    SquidPostPopList& selected_; // 커서/생산/Builder의 마지막 선택 목록이다.
    std::span<const std::uint32_t> alliances_; // 현재 방향별9×9 DWORD 표다.
    const std::int32_t& mapLimit_; // 마나/사정거리와 같은531928의 현재 정수 경계다.
    EnergySupplyState& state_; // 공급 판정의 현재 원본 전역이다.
    EnergySupplyHooks hooks_; // TYPE 문자열/검증한 가상 진입점의 실제 조회다.
    EnergySupplyReads reads_; // 이미 Session이 소유한 현재 전역 조회다.
};
}
