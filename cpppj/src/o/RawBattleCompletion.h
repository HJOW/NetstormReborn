// 일반 전투의 현재 참가자/동맹/사제 상태에서 원문 종료 관문을 계산한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view> // 실제 Dais 네 진단 식/파일명을 같은 Client 장치에 전달한다.

namespace netstorm::o {
struct BattleCompletionGlobals { // 실제9 Player/방향별81동맹/옵션87BYTE를 같은 장기 소유자에서 빌린다.
    std::span<const std::uint8_t> players,options;std::span<const std::uint32_t> alliances; // 각 원문 조회 위치의 실제 물리다.
    const bool& battle;const bool& missionPost;const bool& boss; //00594fbc/00594fa4/00540bc4의 현재 서로 다른 관문이다.
    const std::uint32_t& scriptBreak;const double& loginEnd; //00542544와005c85f0의 현재 입력이다.
};
struct BattleCompletionHooks { // 단순 현재 조회/사제 판정을 실제 기존 부모로 연결한다.
    std::function<bool()> serverConnection; //004e2b10(9)의 객체 존재이며 open/전송상태로 바꾸지 않는다.
    std::function<bool(std::uint32_t)> priestDead; //004922e0의 같은 실제 Player/사제 상태 판정이다.
    std::function<std::span<std::uint8_t>()> currentPlayers; //실제 Client는 효과뒤현재9Player할당을읽고쓴다. 기존독립고정뷰계약은유지한다.
};
struct BattleCompletionFrameGlobals { //44ac30의 현재 State/기한/별도BYTE/결과 장기 owner를 빌린다.
    std::span<std::uint8_t> players; //기존독립 고정뷰의 실제mutable fallback이며 production은currentPlayers를재조회한다.
    const bool& server;const std::uint32_t& phase;const std::uint32_t& localOwner; //server와boss를구별하고4b62a0은State.phase==14다.
    double& captureEnd;double& drawDeadline;std::uint8_t& settleFrames;std::uint32_t& result; //5c8618/595358/55a44c/5c85ac의같은실제주소다.
    const double& gameSeconds; //55b4d0의현재프레임시각이며wall과합치지않는다.
};
struct BattleCompletionFrameHooks { //선택된원문외부효과만기존부모에연결한다.
    std::function<double()> wallSeconds; //captureEnd만료에서만현재460d70을조회한다.
    std::function<std::uint32_t()> currentDialog; //565dd4의실제모달이며호스트phase Dialog가아니다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //진단정상복귀뒤부분효과/후속순서를유지한다.
    std::function<void()> deleteSavedState,rejectDraw,queueMainMenu; //4b5880→별도BYTE0→결과→452fe0(MainMenu,1,1)의실제장치다.
};
class RawBattleCompletion {
public:
    RawBattleCompletion(BattleCompletionGlobals globals,BattleCompletionHooks hooks); // 잘린 물리만 거부하며 선택되지 않은 장치를 호출하지 않는다.
    bool AllAllied() const; //00448f50: subscriber2명 이상·비관전자2명 이상이며 모두 양방향 동맹이다.
    bool TeamFinished() const; //00448fd0: 실제 전투/연결 관문 뒤 동맹 또는 살아 있는 비관전자 팀 수로 판정한다.
    bool FlagFinished(std::int32_t mode) const; //004490d0: 로그인 종료 뒤 실제 flags1/80·사제·boss/인자로 판정한다.
    bool Finished() const; //00449200: TeamFinished가 참이면 두 번째 판정을 생략하고 아니면 FlagFinished(1)이다.
    bool UnloggedFinished(const std::uint32_t& localOwner) const; //44ab20의현재사제사망/boss뒤미접속·관전자·flag80집계를수행한다.
    void Frame(BattleCompletionFrameGlobals globals,const BattleCompletionFrameHooks& hooks) const; //44ac30을현재실제후반프레임한자리에서호출한다.
private:
    std::uint32_t Read(std::uint32_t owner,std::size_t offset) const; // 현재 행의 정렬되지 않은 DWORD를 읽는다.
    bool Allies(std::uint32_t first,std::uint32_t second) const; // 두 방향 DWORD가 모두 비영인지 현재 표에서 읽는다.
    BattleCompletionGlobals globals_;BattleCompletionHooks hooks_; // 같은 물리/현재 효과만 보유하며 상태/Ready를 쓰지 않는다.
};
}
