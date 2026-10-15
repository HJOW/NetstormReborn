// Player의 실제 내용물 통지와 현재 GumpForm 소비자 순회를 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 원본 SP 저장소의 정합성/진단 접두 전역을 주소별로 공유한다. 저장소를 별도 값으로 복사하지 않는다.
struct PlayerContentNoticeState {
    const std::uint32_t& cursorA; // 00568af8: 첫 비교의 현재 값이다.
    const std::uint32_t& cursorB; // 0054d4e0: 첫 비교의 현재 상대값이다.
    std::uint32_t& problemCode; // 005318ec: 불일치/기한 조건에서 쓰는 현재 진단 코드다.
    const std::uint32_t& fallbackCode; // 00540cb8: 호출 앞과 조건부 SP조회 뒤에 읽는 코드다.
    std::uint32_t& moneyChecks; // 005c89c4: 비영이면 SP검사 뒤 DWORD감김으로 하나 줄인다.
    const std::uint32_t& battleShown; // 00595344: 조건부 현재 SP조회 허용값이다.
    const std::uint32_t& localPlayer; // 00540c70: 현재 SP조회 인자다.
    const std::uint32_t& firstSpPool; // 00594fd0: 비영 여부만 읽는 현재 첫 SP 분산배열 토큰이다.
    const std::int16_t& firstPermutation; // 00531890: 정확WORD -1비교의 현재 SP 첫 순열값이다.
    const std::uint32_t& gumpFormType; // 0054107c: 종속 raw타입BYTE와 비교하는 현재 Form타입DWORD다.
    const std::uint32_t& contentEvent; // 005427bc: 각 현재 Gump가상+28에 전달하는 사건이다.
};
// gameplay/SP/등록표를 관찰 사본으로 대체하지 않으며 최종 가상 처리를 실제 UI에 보낸다.
struct PlayerContentNoticeHooks {
    std::function<float(std::uint32_t)> money; // 실제0040ec50의현재같은SP저장소조회다.
    std::function<std::uint32_t(std::uint32_t)> gump; // 실제00463ff0의현재등록ID조회다. 없는등록은0이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> handle; // 현재Gump토큰의실제가상+28(event,form,reason)다.
};
// PATCH00490750→004d55e0. 부모/자식 상태를 임의 필터하지 않고 미리 저장한 next를 순회한다.
class RawPlayerContentNotice {
public:
    // 모든 현재 전역/실제UI소비자와 같은 rawPool은 모듈보다 오래 살아야 한다.
    RawPlayerContentNotice(const SidPool& pool,PlayerContentNoticeState state,PlayerContentNoticeHooks hooks);
    // 원본 Player의 부모 내용물 알림과 직접004d55e0 호출이 사용하는 같은 몸체다.
    void Notify(Sid parent,std::uint32_t form,std::uint32_t reason) const;
    // 상위가 현재 PlayerVT를 분배할 때 같은 Pool인지 검증한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_; // 원본현재head/next/type/Gump등록ID를읽는유일한저장소다.
    PlayerContentNoticeState state_; // 값복사대신현재진단/Player/Form/UI사건전역을참조한다.
    PlayerContentNoticeHooks hooks_; // 실제SP/현재등록표/가상UI처리의필수하위다.
};
}
