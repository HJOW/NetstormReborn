// PATCH10.78의 사제 건설 메뉴·건설 행·선택 관문을 현재 raw 월드와 controlled 메뉴에 연결한다.
#pragma once
#include "client/MenuGumpControls.h"
#include "client/MenuGumpWidgets.h"
#include "o/RawContainedFinder.h"
#include "o/RawContainedCreate.h"
#include "o/RawPlayerWorld.h"
#include "o/RawPriestState.h"
#include "o/RawCarrierPreDestroy.h"
#include "o/SquidProcess.h"
#include <variant>

namespace netstorm::client {
// 번역된 원본 printf 서식에 전달하는 문자열/32비트 정수 인자다. 포인터를 호스트 주소로 재해석하지 않는다.
using PriestMenuArgument=std::variant<std::string,std::int32_t>;
// 원본의 mutable 타입·명령·모드 전역이다. 기본값은 보호 PE의 초기 DWORD이며 매 사용 시 재조회한다.
struct PriestMenuState {
    std::uint32_t localOwner{1}; // 00540c70의 현재 소유자 번호다.
    std::array<std::uint32_t,3> temples{123,124,125}; // 00541244/48/4c의 원본 신전 표시 순서다.
    std::array<std::uint32_t,4> workshops{85,86,87,88}; // 005411ac/b0/b4/b8의 워크샾 표시 순서다.
    std::uint32_t outpost{126},dais{165},altar{164},devastation{168}; // 원본00541250/ec/e8/f8의 현재 타입이다.
    std::uint32_t buildCommand{0x02000060},prayCommand{0x030000de},knowledgeCommand{0x020000a0}; // 원본00542878/2a6c/2974의 입력 토큰이다.
    bool workshopMode{},special{},editor{},chargeBlocked{}; // 00594fbc/0055a480/005c85a4/00594fa4를 각각 보관한다.
};
// 복원한 계산을 대신하지 않는 실제 현재 조회·번역·GUI/커서 효과 경계다. 도달한 경계만 필수로 검사한다.
struct PriestMenuHooks {
    std::function<bool(o::Sid,std::uint32_t)> hasEvent; // 같은 Host의004afb40이며493/25d를 구분한다.
    std::function<bool(o::Sid)> immobile; // 같은 RawPriestState::Immobile의00427030이다.
    std::function<std::uint32_t()> currentTechnology; // 같은 RawPlayerWorld::CurrentTechnology의0048fc60이다.
    std::function<std::uint32_t(o::Sid,std::uint32_t,std::uint32_t)> containedType; // 004acac0의mask/kind/index0이며 SID가 아닌 타입 WORD를 반환한다.
    std::function<bool(std::uint32_t)> allowed; // 같은 현재 RawTotalMade의004c2400 타입 허용 조회다.
    std::function<std::int32_t(std::uint32_t)> cost; // 실제 인코딩 비용 표를 디코딩한 정수다.
    std::function<float(std::uint32_t)> money; // 0040ec50의 현재 SP다. 두 선택 조회를 합치지 않는다.
    std::function<std::string(std::uint32_t)> description; // 0049af80의 실제 번역된 타입 설명이다.
    std::function<std::string(std::string_view,std::span<const PriestMenuArgument>)> format; // 번역004de9a0와 원본 고정printf 서식 처리다.
    std::function<void()> beginFancy,beginVertical,separator; // 기존00477ae0/00477bb0(0)/00478260이다.
    std::function<void(std::string_view)> title; // 기존00477ee0이다.
    std::function<std::uint32_t()> takeMenu; // 기존00475c00(의미없는 인자는 생략)이다.
    std::function<void(std::string_view,std::uint32_t,bool)> submenu; // 기존00478160(label,child,enabled,0)이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,bool)> item; // 기존00477cb0의include1/forceColor0이다.
    std::function<void(o::Sid)> baseMenu; // 항상 마지막에 실행하는004288f0 기본 객체 메뉴다.
    std::function<void(std::string_view)> tell; // 선택 실패의 실제 NoTechTheme/BuildNoBucks 안내다.
    std::function<o::Sid(o::Sid,std::uint32_t)> createPreview; // 실제Contained(parent,type,0,0x20,0x10,0)이다.
    std::function<void(o::Sid,std::uint32_t)> takePreview; // 실제00447000(form,0x160) 커서 소유권 이동이다.
};
// 메뉴 계산은 같은 Pool/TYPE/프레임/9Player/제외목록을 읽는다. 새 게임 상태나 기술 목록 사본은 만들지 않는다.
class RawPriestMenu {
public:
    // 모든 참조와 훅의 실제 대상은 메뉴보다 오래 살아야 한다. 이 첫 복원은 PATCH10.78만 받는다.
    RawPriestMenu(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::span<const o::RiftTypeFrames> frames,
        std::span<const std::uint8_t> players,const o::SquidPostPopList& excluded,const o::ContainedFinderState& contained,
        PriestMenuState& state,PriestMenuHooks hooks);
    // 00494870: 내 사제·493부재에서만 하위 메뉴를 구성하고, 어떤 정상 분기에서도 기본 메뉴를 마지막 호출한다.
    void Open(o::Sid priest) const;
    // 00461cf0: 금액은 문구 색/아이콘만 바꾼다. 행 활성화는 명시금지와 제외목록으로만 결정한다.
    void AddBuild(std::uint32_t type,bool disabled,o::Sid priest,bool manaIcon) const;
    // 00461bd0: 워크샾테마→첫float금액→둘째정수금액 검사 뒤 실제 미리보기 Form을 커서로 넘긴다.
    bool SelectBuild(std::uint32_t type,o::Sid priest) const;
    // 00493ac0: 현재 Technology의 flags4·동일테마·!flags2 800인 실제 내용물을 찾는다.
    bool HasTheme(std::int32_t theme) const;
    // 004ad710: 현재물리frame의첫signed문자-'A'이며 WORD+12나 프레임 번호 자체가 아니다.
    std::int32_t FrameState(o::Sid priest) const;
private:
    // 도달한 SID/타입·Player필드를 little endian으로 안전하게 조회한다.
    std::span<const std::uint8_t> Raw(o::Sid sid) const;
    const o::RiftTypeRecord& Type(std::uint32_t type) const;
    std::uint32_t Player(std::uint32_t owner,std::size_t offset) const;
    // 원본 고정 번역서식에만 인자를 넘기며 256바이트 스택버퍼 한도를 보존한다.
    std::string Format(std::string_view key,std::initializer_list<PriestMenuArgument> arguments={}) const;
    const o::SidPool& pool_; // 메뉴·내용물·사제의 단일 raw SID 풀이다.
    std::span<const o::RiftTypeRecord> types_; // 같은 현재 TYPE 속성 표다.
    std::span<const o::RiftTypeFrames> frames_; // 현재 물리프레임의 원본 코드 표다.
    std::span<const std::uint8_t> players_; // 같은 9×0xb4 Player 기록이다.
    const o::SquidPostPopList& excluded_; // 0052f7e0의 실제 건설금지 타입 목록이다.
    const o::ContainedFinderState& contained_; // 같은 내용물 타입/권한 전역이다.
    PriestMenuState& state_; // 호출 중에도 현재 값을 다시 읽는 원본 전역이다.
    PriestMenuHooks hooks_; // 필수 실제조회와 GUI/커서 효과의 외부 경계다.
};
// 기존 controlled 메뉴의 생성·작성·스택을 그대로 연결한다. baseMenu/번역/커서·월드조회는 유지한다.
void BindPriestMenuControls(PriestMenuHooks& hooks,const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& menus);
// 동일 Pool의 실제 HP/Host/Player/내용물/비용·미리보기 생성 몸체를 연결한다. allowed/커서 효과는 상위가 공급한다.
void BindPriestMenuWorld(PriestMenuHooks& hooks,const o::SidPool& pool,const o::RawPriestState& priest,
    const o::SquidProcessHost& processes,const o::RawPlayerWorld& players,const o::RawCarrierPreDestroy& carrier,
    const o::SquidReward& reward,const o::RawContainedCreate& create);
}
