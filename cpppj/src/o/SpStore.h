// 패치판 SP 저장소(0040eba0/0040ec50/0040edb0)와 게임 전역 난수(004558c0)를 복원한다.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace netstorm::o {
// 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다.
// 이 전역 상태(패치 00532710)는 SP 저장소 갱신처럼 게임 로직 밖의 호출에서도 소비된다.
class GameRandom {
public:
    // 원본 전역의 현재 상태를 그대로 받는다. 0은 첫 호출에서 고정 시드로 바뀐다.
    explicit GameRandom(std::uint32_t state=0);
    // 한 번 전진한 뒤 [0,limit) 값을 반환한다. limit 0은 원본의 0 나눗셈이므로 거부한다.
    std::uint32_t Next(std::uint32_t limit);
    // 진단/기계어 대조용 현재 상태다.
    std::uint32_t State() const;
    // 시각 기반 재시드(004559d0)가 만든 새 상태를 주입한다.
    void SetState(std::uint32_t state);
private:
    std::uint32_t state_;
};

// 패치판이 플레이어별 SP를 32개의 9칸 DWORD 배열에 흩뿌려 보관하는 방식이다.
// 값의 k번째 비트는 배열 k의 (offset+k)&31번째 비트에 두고, 같은 칸의 다른 비트에는 난수 잡음을 섞는다.
class ScrambledSpStore {
public:
    // 비트 수와 소유자(0~8) 수다. 배열 하나는 원본 operator_new(0x24)의 9 DWORD다.
    static constexpr std::size_t kBits=32,kOwners=9;
    // reseed는 시각에서 새 난수 상태를 만드는 004559d0의 결과를 주입한다. fill은 초기화되지 않은 힙의 내용이다.
    ScrambledSpStore(GameRandom& rng,std::function<std::uint32_t()> reseed,std::uint32_t fill=0);
    // 0040eba0 이후인지 반환한다. DAT_0054db24와 같다.
    bool Initialized() const;
    // 0040eba0: 난수를 재시드하고 offset을 고른 뒤 순열 섞기와 32개 배열 확보를 수행한다. 순열은 결국 항등으로 되돌아간다.
    void Initialize();
    // 0040ec50: 초기화 전이면 먼저 초기화하고 소유자의 32비트 값을 모은다. 소유자는 0~8만 허용한다.
    std::uint32_t Get(std::uint32_t owner);
    // 0040edb0: 초기화 전이면 먼저 초기화하고 값 비트를 쓴 뒤 칸마다 난수 잡음 5개를 섞는다.
    void Set(std::uint32_t owner,std::uint32_t bits);
    // 진단용 비트 위치 기준과 배열 내용이다. 배열 k의 owner번째 DWORD가 pools[k*9+owner]다.
    std::uint32_t Offset() const;
    const std::array<std::uint32_t,kBits*kOwners>& Pools() const;
private:
    // 호출마다 같은 오류 문구로 소유자 범위를 거부한다.
    static void CheckOwner(std::uint32_t owner);
    GameRandom& rng_;
    std::function<std::uint32_t()> reseed_;
    std::uint32_t fill_{};
    bool initialized_{};
    std::uint32_t offset_{};
    std::array<std::uint32_t,kBits*kOwners> pools_{};
};
}
