// 다리의 연결·열린 끝·붕괴 방문 목록·수명 비트 갱신 부분을 복원한다.
// 전역 표면 지도/Squid 번호를 명시적 입력으로 받는다. 배치·전체 붕괴 갱신·세계 객체 삭제는 후속이다.
#pragma once
#include "o/RiftType.h"
#include <cstdint>
#include <span>
#include <vector>

namespace netstorm::o {
class SurfaceFinder;
// 원본 Squid +0xc의 3~6비트가 다리 수명이다. 정상 범위는 0~7이다.
inline constexpr std::uint16_t kBridgeLifeMask = 0x78;
// 원본 J/K 판자 제거 시 가상 함수에 넘기는 플래그다.
inline constexpr std::uint32_t kBridgePlankRemoval = 0x02000000;
struct BridgeLifeChange {
    std::uint16_t word{}; // 제거 경로에서는 원본과 같이 이전 단어를 그대로 둔다.
    int previous{}, remaining{};
    bool remove{};
    std::uint32_t removalFlags{};
};
// 004218b0가 남기는 방문 목록과 두 전역 플래그. 실제 수명/그림/삭제는 별도 처리다.
struct BridgeDecayWalk {
    std::vector<std::uint16_t> visited;
    bool shortJunction{}, canDecay{};
    bool complete{true}; // 원본이 무한 재귀할 접합 고리/과도한 깊이는 새 코드에서 중단한다.
};
class Bridge {
public:
    // 원본 00441e40 ↔ CD 004d2e00: 짝수 방향은 side, 홀수는 variant의 연결을 양쪽에서 검사한다.
    // emplacement의 P 처리와 다리/폭탄↔섬/건물의 A 처리를 원본 순서로 적용한다.
    static bool Connects(FrameCode first, std::uint32_t firstFlags2,
        FrameCode second, std::uint32_t secondFlags2, int direction);
    // 원본 004218b0 ↔ CD 00449f10: flag 8 이웃으로 접합 칸 재귀·경계 처리·목록 용량/순서를 보존한다.
    // 접합 고리/안전 깊이 초과는 complete=false이며 수명을 갱신하지 않는다. 표면 스냅샷을 읽기만 한다.
    static BridgeDecayWalk CollectDecay(const SurfaceFinder& surfaces, std::uint16_t root, std::size_t capacity=100);
    // 원본 00421770 ↔ CD 00449e30. 방향 연결이 있고 이웃 목록에 해당 표면 번호가 없으면 열린 방향이다.
    // surface는 256×256 ushort 표면 지도다. 이웃 단정도 좌표에 원본 0.9999f를 더한 뒤 0 방향으로 잘라 칸을 고른다.
    static bool IsOpen(char side, int direction, float x, float y,
        std::span<const std::uint16_t> surface, std::span<const int> neighbors);
    // 원본 004217f0. CD판에서는 같은 판단이 00449250 내부에 인라인되어 별도 함수 대응을 두지 않는다.
    // 세 갈래는 두 번째 열린 방향, 모퉁이/판자는 첫 열린 방향, 끝 칸은 연결 반대 방향을 돌려준다.
    static int OpenDirection(char side, float x, float y,
        std::span<const std::uint16_t> surface, std::span<const int> neighbors);
    // 원본 00421c30 ↔ CD 0044a1c0의 첫 외부 효과 호출 전까지: 수명 감소/0 제한·비트 보존·제거 플래그.
    // 금 간 그림·화면 갱신·수송 유닛 낙하·실제 객체 삭제를 실행하지 않는다.
    static BridgeLifeChange ReduceLife(std::uint16_t word, int reduction, bool battle, char side);
};
}
