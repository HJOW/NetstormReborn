// 일반 로그인 완료의 Core/미도착 Player/스펠 예약/같은 팀 동맹 요청을 현재 저장소에 적용한다.
#pragma once
#include <cstddef> // 원문 기록 안의 BYTE 오프셋을 호스트 크기로 검증한다.
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { // Native 전역을 별도 성공 상태나 Player 사본으로 대체하지 않는다.
//004b4a20/004b6250/00485f50과004b67c0의 팀 순회가 공유하는 현재 실제 데이터다.
struct OrdinaryBattleStartGlobals {
    std::span<std::uint8_t> players; // 같은9×180BYTE 기록이며 각 원문 위치에서 다시 읽고 쓴다.
    std::span<const std::uint8_t> options; // 같은87BYTE의 스펠/관전자/팀 설정이다.
    const bool& server;const bool& battle; // 서로 다른 현재00540bc0/00594fbc를 빌린다.
    const std::uint32_t& templateOwner;const std::uint32_t& battleOwner; // Core 쓰기/Notify와 상태 안내가 서로 다른 현재 owner를 읽는다.
    std::uint32_t& spellCount;double& spellDeadline; //00595090의 현재 수와005950d0의 게임 시각+180 예약이다.
};
// 현재 Core/Template/전송/입력/UI/로그만 기존 실제 몸체에 위임한다.
struct OrdinaryBattleStartHooks {
    std::function<std::span<std::uint8_t>()> core; //0048f0d0은 효과 뒤 현재 포인터를 재조회하므로 한 번 캐시하지 않는다.
    std::function<void(std::uint32_t)> writeCore,touchFort; //004beb30/004bd360의 같은 현재 Template 부모다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> status; //004d3090의 kind/owner/parameter이며 enabled1/빈문자열은 호출자가 유지한다.
    std::function<void(std::uint32_t)> broadcastPlayer; //00491270의 현재 Player를 실제 접속자에게 송신한다.
    std::function<void(std::uint32_t)> queueAlliance; //00542b50=02000122의 두 인자에 같은 packed owner/target을 예약한다.
    std::function<double()> gameSeconds; // 현재0055b4d0 프레임 게임 시각이며 벽시각이 아니다.
    std::function<void(std::uint32_t,std::string_view)> log; //004c2630의1d 로그 채널이다.
};
// 원문 일반 초기화의 실제 하위만 조립한다. 바깥 savedGame/mission 분기는 타이머가 소유한다.
class RawOrdinaryBattleStart {
public:
    RawOrdinaryBattleStart(OrdinaryBattleStartGlobals globals,OrdinaryBattleStartHooks hooks); // 같은 물리 폭을 검증하고 아직 게임 상태를 쓰지 않는다.
    void Run() const; // Core→현재 서버 미도착 정리→현재 옵션의 스펠 예약→현재 서버 팀 요청 순서다.
private:
    std::uint32_t Read(std::uint32_t owner,std::size_t offset) const; // 원문 비정렬 DWORD의 실제9기록 범위를 확인한다.
    void Put(std::uint32_t owner,std::size_t offset,std::uint32_t value) const; // 주변 필드와 padding은 유지한다.
    std::uint32_t CoreWord() const; // 같은 getter를 매번 호출한 뒤 실제 첫 WORD를 읽는다.
    void StartPlayer() const; //004b4a20의 재조회·32700 상한·Core/Notify/종류9 안내다.
    void RemoveMissingPlayers() const; //004b6250의 subscriber/예측카운터0 조건과 각 실제 송신이다.
    void ScheduleSpells() const; //0041cab0/00485f50의 현재 비관전자 수+3·로그·게임 시각180초 예약이다.
    void QueueTeams() const; // 원문64DWORD 지역 표와 같은 단방향/대칭 중복 방지 후 일반 입력만 예약한다.
    OrdinaryBattleStartGlobals globals_; // 현재 참조이며 Server/옵션/owner 사본을 보유하지 않는다.
    OrdinaryBattleStartHooks hooks_; // 실제 선택된 큰 효과만 요구한다.
};
}
