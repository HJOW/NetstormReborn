// Ch.cpp의 실제 CS 기록과 최초 한 번의 메타 배치 좌표를 별도 상태 사본 없이 구성한다.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace netstorm::o { // 메타 표시/목록이 함께 빌릴 원문 CS의 물리와 순수 초기화다.
inline constexpr std::size_t kChallengeBytes=0x5350; //0042cec0 생성 요청의 전체21328BYTE다.
inline constexpr std::size_t kChallengeBattleOffset=8,kChallengeBattleBytes=0x124,kChallengeBattleCount=10; // 첫 두DWORD 뒤의 실제 전투10기록이다.
inline constexpr std::size_t kChallengeParticipantOffset=0xb70,kChallengeParticipantBytes=0x1cc,kChallengeParticipantCount=40; // 뒤의 실제 참가자40기록이다.
using ChallengePoint=std::array<float,2>; // Native 두 float32를 같은 x/y 순서로 저장한다.
struct ChallengeGeometry { // 실제 Client BSS/최초 생성 캐시이며 CS 재사용 때 지우지 않는다.
    std::uint32_t initialized{}; //005467c8: 좌표와 크기 계산이 모두 끝난 뒤1을 쓴다.
    float firstRadius{},secondRadius{}; //005467c0/c4의 각각12+반경/4+반경이다. 용도 추정으로 합치지 않는다.
    std::int32_t width{},height{}; //005467cc/d0의 FTOL(2*저장된center+26)/FTOL(2*center)다.
    float originX{},originY{}; //005485b8/bc의 저장된center+13/center다.
    std::array<ChallengePoint,9> ring{}; //005468b0..68f7의 실제9쌍이며 마지막 누적 각도는 저장하지 않는다.
    std::array<ChallengePoint,40> grid{}; //005485c0..86ff의 실제40쌍이다. 최초 루프는0번을 변경하지 않는다.
};
void ResetChallengeState(std::span<std::uint8_t> bytes); //0042ba10의 두DWORD→참가자40→전투10 순서로 지운다.
void ConstructChallengeState(std::span<std::uint8_t> bytes,ChallengeGeometry& geometry); //0042cec0의 최종 기록 초기화와 조건부 최초 좌표/크기 계산이다.
}
