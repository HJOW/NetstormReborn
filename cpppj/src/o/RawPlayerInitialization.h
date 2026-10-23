// PATCH004910a0의 이전 Player/factory 해제와 새9기록 전체 초기화를 원문 순서로 실행한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
inline constexpr std::uint32_t kInitializedPlayerCount=9,kInitializedPlayerStride=180; // 실제 Player0..8의 기록 수와 BYTE 간격이다.
inline constexpr std::uint32_t kInitializedPlayerBytes=1620,kInitializedFactoryBytes=120; // malloc Player와 각 operator new factory30칸의 원문 크기다.
// 실제 두 카운터와 현재32비트 Player 주소 슬롯을 빌린다. 주소는 소유자가 발급한 같은 물리 식별자다.
struct PlayerInitializationGlobals {
    std::uint32_t& fortCount; //005951f8: 첫 현재base load 뒤0을 쓰는 로그인 Fort 수다.
    std::uint32_t& lastLoginPlayer; //005951fc: 그다음0을 쓰는 마지막 로그인 Player다.
    std::function<std::uint32_t()> playerBase; //0059534c의 현재주소를 각 원문 load 위치에서 순수 조회한다.
    std::function<void(std::uint32_t)> writePlayerBase; //동일 현재 슬롯에 NULL/실제 malloc 반환 주소를 직접 기록한다.
};
// 큰 계산을 기본 성공으로 채우지 않고 현재 실제 할당/메모리/동맹 몸체에 연결한다.
struct PlayerInitializationHooks {
    std::function<std::span<std::uint8_t>(std::uint32_t,std::uint32_t)> memory; //현재 주소/요청폭의 같은 물리뷰다. 해제/재사용 뒤에도 그때의 실제 영역을 다시 해석한다.
    std::function<void(std::uint32_t)> freeFactory; //004e4354: 현재각Player+0의 토큰을 NULL도 포함하여 아홉 번 전달한다.
    std::function<void(std::uint32_t)> freePlayers; //0043fa90: 그때 다시 읽은 현재 Player base를 반납한다.
    std::function<std::uint32_t(std::uint32_t)> mallocPlayers; //0043fa80(1620)의 실제 주소 또는NULL/외부 예외다. 부모가 전체를0으로 쓴다.
    std::function<std::uint32_t(std::uint32_t)> newFactory; //004e4391(120)의 실제 opaque32 할당 주소다. 부모는 할당 내용 자체를 초기화하지 않는다.
    std::function<void(std::uint32_t,std::uint32_t)> alliance; //기존00490ea0의 실제 방향별동맹/조건부Chat 몸체다.
};
//236BYTE 부모 전체다. Core 선택/known0/PlayerLinks/Kernel 정리는 호출자의 앞선 원문 단계이며 여기서 암묵 실행하지 않는다.
// 메모리/getter/setter는 같은 실제 owner의 순수 물리 장치다. NULL·짧은물리/누락장치는 도달한 위치에서만 예외로 거부한다.
void InitializePlayerRecords(const PlayerInitializationGlobals& globals,const PlayerInitializationHooks& hooks);
}
