// Player의 실제 내용물 통지와 현재 GumpForm 소비자 순회를 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 원본 전역의 현재 값을 읽는 불변 연결이다. 직접 lvalue 참조 또는 실제 getter만 받고 값을 저장하지 않는다.
template<class T> class PlayerContentNoticeRead {
public:
    // 기존 raw fixture의 같은 변수 주소를 빌린다. 변수는 연결 객체/통지보다 오래 살아야 한다.
    PlayerContentNoticeRead(const T& value):read_([&value] { return value; }) {}
    // 임시 getter 반환을 const참조로 연장한 뒤 현재 전역처럼 보관하는 연결을 컴파일 단계에서 금지한다.
    PlayerContentNoticeRead(T&&)=delete;
    // const 임시값도 같은 수명 오류이므로 lvalue 연결에서 제외한다.
    PlayerContentNoticeRead(const T&&)=delete;
    // signed DWORD/실제 포인터 등 표현이 다른 동일 원본 필드의 현재 조회를 연결한다. 빈 getter는 거부한다.
    static PlayerContentNoticeRead Bind(std::function<T()> read) {
        if (!read) throw std::invalid_argument("Player 내용물의 현재 SP 조회기 누락");return PlayerContentNoticeRead(std::move(read),0);
    }
    // 호출한 원본 명령 위치에서 한 번 현재 값을 읽는다. 초기값/프레임 사본을 재사용하지 않는다.
    T Read() const { return read_(); }
private:
    // 검증을 마친 실제 getter만 보관하는 내부 생성자다.
    PlayerContentNoticeRead(std::function<T()> read,int):read_(std::move(read)) {}
    std::function<T()> read_; // 실제 같은 전역의 수명을 빌리는 필수 현재 조회기다.
};
// 원본 SP 저장소의 정합성/진단 접두 전역을 주소별로 공유한다. 저장소를 별도 값으로 복사하지 않는다.
struct PlayerContentNoticeState {
    PlayerContentNoticeRead<std::uint32_t> cursorA; // 00568af8: 첫 비교의 현재 값이다.
    PlayerContentNoticeRead<std::uint32_t> cursorB; // 0054d4e0: 첫 비교의 현재 상대값이다.
    std::uint32_t& problemCode; // 005318ec: 불일치/기한 조건에서 쓰는 현재 진단 코드다.
    const std::uint32_t& fallbackCode; // 00540cb8: 호출 앞과 조건부 SP조회 뒤에 읽는 코드다.
    std::uint32_t& moneyChecks; // 005c89c4: 비영이면 SP검사 뒤 DWORD감김으로 하나 줄인다.
    PlayerContentNoticeRead<std::uint32_t> battleShown; // 00595344: 실제BOOL/DWORD의현재비제로조건을수명안정조회한다.
    const std::uint32_t& localPlayer; // 00540c70: 현재 SP조회 인자다.
    PlayerContentNoticeRead<std::uint32_t> firstSpPool; // 00594fd0: 비영 여부만 읽는 현재 첫 SP 분산배열 주소다.
    PlayerContentNoticeRead<std::int16_t> firstPermutation; // 00531890: 정확WORD -1비교의 현재 SP 첫 순열값이다.
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
