// Carrier의 운반물·주문 메뉴를 같은 원본 contained 탐색과 controlled 메뉴에 연결한다.
#pragma once
#include "client/RawObjectMenu.h"
#include "o/RawContainedLookup.h"

namespace netstorm::client {
// 다른 메뉴/월드와 같은 현재 전역을 참조한다. 로컬 번호·모드·맵 크기·CD SP 사본은 만들지 않는다.
struct CarrierMenuState {
    std::uint32_t& localOwner; // PATCH00540c70/CD0050f6c8이다.
    bool& editor; // PATCH005c85a4/CD00518904다.
    std::int32_t& mapSize; // PATCH00531928/CD0052e9a8의 현재 지도 경계다.
    float& cdSp; // CD005120d4이며 PATCH는 실제 분산 SP 조회를 사용한다.
    std::uint32_t& helpCommand; // 기존 공통 객체 메뉴와 같은 도움말 명령이다.
    std::uint32_t priestType{158}; // PATCH005412d0/CD0051cbbc의 초기값이다.
    std::uint32_t dropCommand{0x03000076},castCommand{0x0300011a}; // PATCH005428d0/00542b38이다. CD는 Drop 명령을 Cast에도 사용한다.
    std::function<std::uint32_t()> localOwnerRead; // 실제 세션의 같은 현재 Player 번호를 각 조회 시점에 읽는다.
    std::function<bool()> editorRead; // DWORD 편집 전역을 bool 사본으로 동기화하지 않고 직접 읽는다.
};
// 큰 위치/현재 이동 조회와 번역/메뉴/분산 SP만 실제 호출자가 공급한다.
struct CarrierMenuHooks {
    std::function<bool(o::Sid)> groundImmobile; // 기존 RawPriestState::GroundImmobile이며 extra20 단락평가는 메뉴가 보존한다.
    std::function<std::uint32_t(std::uint32_t)> movementType; // 같은 현재00488fe0/CD0047d020이다.
    std::function<o::Sid(o::Sid,std::uint32_t)> findPath; // 실제004b1a90/CD004eb310(parent,type,0,0)이다.
    std::function<bool(o::Sid)> pathActive; // 실제004ae2d0/CD004af4e0의 현재 process 포인터 조회다.
    std::function<std::array<float,2>(o::Sid,std::uint32_t)> dropPoint; // 실제00426a70/CD004e46f0 전체 배치 후보 몸체다.
    std::function<std::int32_t(std::uint32_t)> patchCost; // 현재 인코딩 표를 디코딩한 정수 비용이다.
    std::function<float(std::uint32_t)> money; // PATCH0040ec50의 현재 분산 SP다.
    std::function<bool(o::Sid,std::uint32_t)> hasEvent; // 같은 Host의493/25d 조회다.
    std::function<std::string(std::uint32_t)> description; // 현재 TYPE의 번역된 설명이다.
    std::function<std::string(std::string_view,std::span<const PriestMenuArgument>)> format; // 원본 번역·고정%s/%d 서식이다.
    std::function<void(std::string_view)> header; // 실제 controlled AddHeader다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,bool)> item; // include1/forceColor0을 보존한다.
    std::function<void(o::Sid)> damageableMenu; // 항상 마지막의 실제 RawObjectMenu::OpenDamageable이다.
};
// PATCH004288f0/CD004e5180의 전체 메뉴 부모 몸체다. 내려놓기/주문 실행 자체와 구분한다.
class RawCarrierMenu {
public:
    // 현재 Pool/TYPE/contained 상태와 전역/하위 몸체는 이 처리기보다 오래 살아야 한다.
    RawCarrierMenu(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,const o::ContainedFinderState& contained,
        CarrierMenuState& state,CarrierMenuHooks hooks);
    // 현재 전체 내용물 순서로 Carrying/Drop/Cast/About을 작성하고 기본 메뉴를 마지막에 호출한다.
    void Open(o::Sid carrier) const;
private:
    std::uint32_t LocalOwner() const; // 현재 getter 또는 독립 fixture의 참조 전역을 읽는다.
    bool Editor() const; // 현재 getter 또는 기존 편집 참조를 읽는다.
    // 현재 물리 슬롯/TYPE/WORD/소유자만 읽고 원본에 없는 free/dead 필터는 추가하지 않는다.
    std::span<const std::uint8_t> Raw(o::Sid sid) const;
    const o::RiftTypeRecord& Type(std::uint32_t type) const;
    std::uint32_t Owner(o::Sid sid) const;
    static std::uint32_t Word(std::span<const std::uint8_t> raw,std::size_t offset);
    // 원본 고정 서식과256바이트 버퍼/NUL 종료를 유지한다.
    std::string Format(std::string_view key,std::initializer_list<PriestMenuArgument> arguments={}) const;
    // CD의 실제 __ftol signed64 절삭과 하위DWORD를 보존한다.
    static std::int32_t Truncate(float value);
    // 두 판본의 실제 좌표 비트/부동소수 비교와 지도 경계를 보존한다.
    bool ValidPoint(std::array<float,2> point) const;
    const o::SidPool& pool_; // 모든 부모/내용물의 같은 저장소다.
    std::span<const o::RiftTypeRecord> types_; // 실제 현재 TYPE 표를 빌린다.
    const o::ContainedFinderState& contained_; // 같은 현재 contained 타입/권한 전역이다.
    CarrierMenuState& state_; // 다른 메뉴와 공유하는 참조 전역이다.
    CarrierMenuHooks hooks_; // 실제 의존 몸체와 메뉴 효과 경계다.
};
// 실제 controlled 작성/기본 객체 사슬/사제 기본 호출을 연결한다. 임시 별도 메뉴는 생성하지 않는다.
void BindCarrierMenuControls(CarrierMenuHooks& hooks,const MenuGumpControls& controls);
void BindCarrierMenuBase(CarrierMenuHooks& hooks,const RawObjectMenu& objects);
void BindPriestMenuCarrier(PriestMenuHooks& hooks,const RawCarrierMenu& carrier);
// 같은 Pool의 기존 사제/Host/종속/비용을 연결한다. 위치/분산 SP/번역/기본 메뉴는 유지한다.
void BindCarrierMenuWorld(CarrierMenuHooks& hooks,const o::SidPool& pool,const o::RawPriestState& priest,
    const o::SquidProcessHost& processes,const o::RawContainedLookup& lookup,const o::SquidReward& reward,
    std::function<std::uint32_t(std::uint32_t)> movementType,std::function<bool(o::Sid)> pathActive);
}
