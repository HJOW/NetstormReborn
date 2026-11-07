// R CCh의 기록 배치와 프로세스 수명의 최초 좌표 표를 클라이언트와 구분한다.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace netstorm::root { // 제공된 R의344/188BYTE 배열을 사용하는 실제 서버 저장소다.
inline constexpr std::size_t kRootChallengeBytes=0x2b80; //0044ca80의 전체 CCh 요청 크기다. 마지막 DWORD는 생성자가 쓰지 않는다.
inline constexpr std::size_t kChallengeBattleOffset=8,kChallengeBattleBytes=0x158,kChallengeBattleCount=10; // 두 DWORD 뒤에 이어지는 실제 전투10개다.
inline constexpr std::size_t kChallengeParticipantOffset=0xd78,kChallengeParticipantBytes=0xbc,kChallengeParticipantCount=40; // 실제 참가자40개이며 클라이언트의0xb70/460BYTE와 다르다.
inline constexpr std::size_t kChallengeConnectionOffset=0x2ad8,kChallengeCounterOffset=0x2b78; // 참가자 전송40토큰 뒤의 초기화되는 마지막 DWORD다.
using ChallengePoint=std::array<float,2>; //0044ce50의 두 float32를 x/y 순서로 저장한다.
struct ChallengeGeometry { // CCh 객체가 재생성되어도 유지되는 같은 R 프로세스 BSS다.
    std::uint32_t initialized{}; //004d05e0은 모든 좌표/크기 쓰기 뒤에1로 바뀐다.
    std::array<float,2> radii{}; //004d05d8/dc의12+저장된반경,4+저장된반경이다.
    std::int32_t width{},height{}; //004d05e4/e8의 FTOL(center*2+26)/FTOL(center*2)다.
    std::array<ChallengePoint,10> ring{}; //004d06e0의10쌍이며 원문은1..9만 쓰고0번을 보존한다.
    std::array<ChallengePoint,40> grid{}; //004d0730의40쌍이며 원문은1..39만 쓰고0번을 보존한다.
    ChallengePoint origin{}; //004d0880/84의 저장된center+13/center다.
};
void ResetChallengeState(std::span<std::uint8_t> bytes); //0044cfb0: 두 DWORD→참가자→전투→연결표→카운터를 지우고+2b7c는 보존한다.
void ConstructChallengeState(std::span<std::uint8_t> bytes,ChallengeGeometry& geometry); //0044ca80: 덮어쓰이는 자식 초기화의 최종 기록과 조건부 최초 좌표 표다.
}
