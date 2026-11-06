// 일반 전투의 현재 참가자/동맹/사제 상태에서 원문 종료 관문을 계산한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
struct BattleCompletionGlobals { // 실제9 Player/방향별81동맹/옵션87BYTE를 같은 장기 소유자에서 빌린다.
    std::span<const std::uint8_t> players,options;std::span<const std::uint32_t> alliances; // 각 원문 조회 위치의 실제 물리다.
    const bool& battle;const bool& missionPost;const bool& boss; //00594fbc/00594fa4/00540bc4의 현재 서로 다른 관문이다.
    const std::uint32_t& scriptBreak;const double& loginEnd; //00542544와005c85f0의 현재 입력이다.
};
struct BattleCompletionHooks { // 단순 현재 조회/사제 판정을 실제 기존 부모로 연결한다.
    std::function<bool()> serverConnection; //004e2b10(9)의 객체 존재이며 open/전송상태로 바꾸지 않는다.
    std::function<bool(std::uint32_t)> priestDead; //004922e0의 같은 실제 Player/사제 상태 판정이다.
};
class RawBattleCompletion {
public:
    RawBattleCompletion(BattleCompletionGlobals globals,BattleCompletionHooks hooks); // 잘린 물리만 거부하며 선택되지 않은 장치를 호출하지 않는다.
    bool AllAllied() const; //00448f50: subscriber2명 이상·비관전자2명 이상이며 모두 양방향 동맹이다.
    bool TeamFinished() const; //00448fd0: 실제 전투/연결 관문 뒤 동맹 또는 살아 있는 비관전자 팀 수로 판정한다.
    bool FlagFinished(std::int32_t mode) const; //004490d0: 로그인 종료 뒤 실제 flags1/80·사제·boss/인자로 판정한다.
    bool Finished() const; //00449200: TeamFinished가 참이면 두 번째 판정을 생략하고 아니면 FlagFinished(1)이다.
private:
    std::uint32_t Read(std::uint32_t owner,std::size_t offset) const; // 현재 행의 정렬되지 않은 DWORD를 읽는다.
    bool Allies(std::uint32_t first,std::uint32_t second) const; // 두 방향 DWORD가 모두 비영인지 현재 표에서 읽는다.
    BattleCompletionGlobals globals_;BattleCompletionHooks hooks_; // 같은 물리/현재 효과만 보유하며 상태/Ready를 쓰지 않는다.
};
}
