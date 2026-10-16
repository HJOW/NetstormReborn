// 공통 객체·Damageable·회수 메뉴의 원본 조건과 명령 인자를 보존한다.
#pragma once
#include "client/RawPriestMenu.h"
#include <optional>

namespace netstorm::client {
// 원본 명령 인자는 정수 또는 수명이 있는 문자열이다. PE 포인터를 호스트 주소로 해석하지 않는다.
using ObjectMenuParameter=std::variant<std::uint32_t,std::string>;
// 두 보호 PE의 동일 의미 전역이다. 메뉴 작성 중에도 현재 값을 다시 읽는다.
struct ObjectMenuState {
    std::uint32_t localOwner{1}; // PATCH00540c70/CD0050f6c8의 로컬 번호다.
    bool editor{},special{},chargeBlocked{}; // PATCH005c85a4/0055a480/00594fa4의 모드다.
    std::uint32_t denySalvage{}; // PATCH00595078/CD00518678의 현재 금지 DWORD다.
    std::uint32_t growingRainBlocker{150},nugget{154},suppressedSalvage{122}; // PATCH5412b0/5412c0/541240이다. 마지막 값은 CD에서 사용하지 않는다.
    std::uint32_t hurtCommand{0x030000e2},destroyCommand{0x03000015},makeYoursCommand{0x030000af}; // 원본 직접 객체 명령이다.
    std::uint32_t helpCommand{0x02000017},tellCommand{0x02000095},salvageCommand{0x0200005d}; // 원본 도움말/문자열 안내/회수 명령이다.
    std::optional<std::string> helpWord; // PATCH0054db38/CD0052e8cc의 NUL 포인터와 실제 문자열을 구별한다.
};
// 메뉴 계산 밖의 실제 가상 조회·환불·현재 설정·번역·Player 메뉴를 공급한다.
struct ObjectMenuHooks {
    std::function<bool(o::Sid)> hasHitPoints; // 기존004adc60/CD004aecf0이다.
    std::function<std::uint32_t(o::Sid)> count; // 실제 객체의 가상+80이며 로컬 Damageable에서 한 번 호출한다.
    std::function<std::int32_t(o::Sid,std::uint32_t,std::uint32_t)> salvageRefund; // 기존0044c240/CD00462610의 현재 타입/개수 인자다.
    std::function<std::string(std::string_view,std::span<const PriestMenuArgument>)> format; // 원본 번역/고정%s/%d 서식이다.
    std::function<std::uint32_t()> itemCount; // 실제 controlled 메뉴의 현재 항목 수다.
    std::function<void()> separator; // 실제 controlled 구분선 작성이다.
    std::function<void(std::string_view,std::uint32_t,const ObjectMenuParameter&)> item; // 실제 include1/enabled1/forceColor0 항목 작성이다.
    std::function<void(std::string_view,std::uint32_t)> submenu; // 실제 enabled1/checked0의 Player 하위 메뉴다.
    std::function<void(std::string_view,std::string_view)> setConfig; // 원본 global.salvageSid를 같은 현재 설정에 쓴다.
    std::function<std::string(std::uint32_t)> playerName; // 실제004844f0/CD004019e0이다.
    std::function<std::uint32_t(std::string_view,bool,bool)> playerMenu; // 실제004c9700/CD004c4cd0의(name,1,0)이다.
};
// 같은 Pool/TYPE를 조회하는 세 원본 몸체다. Carrier/Priest는 마지막 기본 메뉴 호출에 이 객체를 연결한다.
class RawObjectMenu {
public:
    // 참조와 훅은 처리기보다 오래 살아야 한다. 풀의 판본을 두 PE의 필드 배치에 사용한다.
    RawObjectMenu(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,ObjectMenuState& state,ObjectMenuHooks hooks);
    // PATCH004b1270/CD004ae780: 편집/치트·도움말·소유자·Player 메뉴다.
    void Open(o::Sid sid) const;
    // PATCH0044cbd0/CD00462a70: 내 객체의 가상 개수→회수 뒤 공통 객체 메뉴다.
    void OpenDamageable(o::Sid sid) const;
    // PATCH0044ca00/CD004628a0: 현재 슬롯의 조건과 명시 타입/개수를 각각 보존한다.
    void AddSalvage(o::Sid sid,std::uint32_t type,std::uint32_t count) const;
private:
    // 현재 물리 슬롯/TYPE/소유자만 조회하며 원본에 없는 생존 필터를 추가하지 않는다.
    std::span<const std::uint8_t> Raw(o::Sid sid) const;
    const o::RiftTypeRecord& Type(std::uint32_t type) const;
    std::uint32_t Owner(o::Sid sid) const;
    // 실제 고정 서식의 NUL 종료와 원본256바이트 한도를 보존한다.
    std::string Format(std::string_view key,std::initializer_list<PriestMenuArgument> arguments={}) const;
    const o::SidPool& pool_; // 실제 게임과 같은 SID 슬롯이다.
    std::span<const o::RiftTypeRecord> types_; // 현재 TYPE 표를 빌린다.
    ObjectMenuState& state_; // 현재 원본 전역이다.
    ObjectMenuHooks hooks_; // 실제 조회/메뉴/설정 효과의 연결부다.
};
// 기존 controlled 메뉴·설정에 연결한다. encodeString은 실제 명령 소비자와 같은 수명 있는 상수 문자열 표여야 한다.
// 원본 문자열은 정적 수명이므로 80칸 스크립트 고리의 임시 토큰으로 대체하지 않는다.
void BindObjectMenuControls(ObjectMenuHooks& hooks,const MenuGumpControls& controls,const MenuGump& menus,
    o::ConfigInterface& config,std::function<std::uint32_t(std::string_view)> encodeString);
}
