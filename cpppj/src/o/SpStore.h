// 패치판 SP 저장소(0040eba0/0040ec50/0040edb0)와 게임 전역 난수(004558c0)를 복원한다.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
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
    // state_: 다음 난수 계산에 소비·갱신하는 32비트 생성기 상태.
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
    ScrambledSpStore(GameRandom& rng,std::function<std::uint32_t()> reseed,std::function<void(std::span<std::uint8_t>)> allocation); // 실제32개 operator_new(36)의 입력을 순서대로 제공한다. 부분 실패를 전체 초기화로 발행하지 않는다.
    // 실제 첫 배열 포인터/외부 추적 상태의 주소를 빌리므로 저장소의 복사/이동을 금지한다.
    ScrambledSpStore(const ScrambledSpStore&)=delete;
    ScrambledSpStore& operator=(const ScrambledSpStore&)=delete;
    ScrambledSpStore(ScrambledSpStore&&)=delete;
    ScrambledSpStore& operator=(ScrambledSpStore&&)=delete;
    // 0054d4e0을 이미 소유한 외부 사제 상태를 첫 Initialize 전에 연결한다. 값 복사/프레임 동기화는 하지 않는다.
    void BindTrackingOffset(std::int32_t& tracking);
    // 0040eba0 이후인지 반환한다. DAT_0054db24와 같다.
    bool Initialized() const;
    // 0040eba0: 난수를 재시드하고 offset을 고른 뒤 순열 섞기와 32개 배열 확보를 수행한다. 순열은 결국 항등으로 되돌아간다.
    void Initialize();
    // 0040ec50: 초기화 전이면 먼저 초기화하고 소유자의 32비트 값을 모은다. 소유자는 0~8만 허용한다.
    std::uint32_t Get(std::uint32_t owner);
    // 0040edb0: 초기화 전이면 먼저 초기화하고 값 비트를 쓴 뒤 칸마다 난수 잡음 5개를 섞는다.
    void Set(std::uint32_t owner,std::uint32_t bits);
    // 00491680: 필요하면 초기화하고 실제 비트 위치 offset을 signed DWORD로 3 증가시킨다.
    // 9600을 넘으면 한 번만 9600을 빼고 모든 32×9 DWORD를 왼쪽으로 3비트 순환한다.
    // 저장된 SP 비트값과 게임 난수는 유지한다. 사제 회복 등의 호출자는 Offset을 추적 전역과 공유한다.
    void Rotate();
    // 진단용 비트 위치 기준과 배열 내용이다. 배열 k의 owner번째 DWORD가 pools[k*9+owner]다.
    std::uint32_t Offset() const;
    const std::array<std::uint32_t,kBits*kOwners>& Pools() const;
    // 실제0040ec50/0040edb0/00491680이 읽는00568af8의 안정 참조다. 값 반환 Offset과 구별한다.
    const std::uint32_t& OffsetCurrent() const;
    // 실제0054d4e0 소유자의 안정 참조다. 미연결은 임의 초기값 대신 오류다.
    const std::int32_t& TrackingOffsetCurrent() const;
    std::int32_t& TrackingOffsetCurrent(); // 실제 사제 회복이 같은 장기0054d4e0을 직접 갱신하는 쓰기 참조다.
    // 초기화 전 NULL이며 이후 같은 실제 pools_.data()를 가리키는 첫 분산배열 주소의 안정 참조다.
    const std::uint32_t* const& FirstPoolCurrent() const;
    // SP 읽기/쓰기/회전이 공유하는00531890의 실제32개 WORD 배열이다.
    const std::array<std::int16_t,kBits>& Permutation() const;
private:
    // 호출마다 같은 오류 문구로 소유자 범위를 거부한다.
    static void CheckOwner(std::uint32_t owner);
    // rng_: 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다 (GameRandom).
    GameRandom& rng_;
    // reseed_: 시각 기반 난수 재시드 값을 공급하는 콜백.
    std::function<std::uint32_t()> reseed_;
    // fill_: 초기화 전 힙 내용을 재현할 DWORD 채움값.
    std::uint32_t fill_{};
    std::function<void(std::span<std::uint8_t>)> allocation_; // 실제 Client만 각36BYTE의 현재 할당 입력을 제공하며 기존 독립 fill 입력은 별도다.
    // initialized_: 초기화가 완료되어 저장소를 사용할 수 있는지.
    bool initialized_{};
    // offset_: 각 플레이어의 SP 비트를 저장한 위치의 회전 기준값.
    std::uint32_t offset_{};
    // trackingOffset_: 기존 사제 상태가 소유한0054d4e0의 주소다. 초기화만 두 offset을 함께 쓴다.
    std::int32_t* trackingOffset_{};
    // permutation_: Initialize가 섞은 뒤 항등으로 되돌리는 실제 WORD순열이다. SP몸체도 같은 배열을 쓴다.
    std::array<std::int16_t,kBits> permutation_{};
    // pools_: 플레이어별 SP 비트를 분산해 저장하는 32×9 DWORD 배열.
    std::array<std::uint32_t,kBits*kOwners> pools_{};
    // firstPool_:00594fd0의NULL조건을실제배열주소로표현한다. 값1/가짜토큰을보관하지않는다.
    const std::uint32_t* firstPool_{};
};
}
