// 다리의 연결·열린 끝·붕괴 방문 목록·수명 비트 갱신·한 칸 붕괴 처리·붕괴 스캔 커서를 복원한다.
// 전역 표면 지도/Squid 번호를 명시적 입력으로 받는다. 배치·실제 객체 삭제(Unpop/그래프 분할/반납)·낙하는 후속이다.
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
// 0052f960 ↔ CD 0051f05c: 수명이 이 값 아래로 처음 내려가면 금 간 프레임으로 바꾼다.
inline constexpr int kBridgeCrackLife = 5;
// 0052f95c ↔ CD 0051f058: 수명이 이 값 이하가 되면 칸 위 이동체를 확인한다.
inline constexpr int kBridgeCarrierLife = 5;
// 한 칸 처리(004227e0)가 그래프 표면 수가 이 값보다 작은 다리를 곧바로 없앤다.
inline constexpr int kBridgeMinimumGraphSurfaces = 5;
// 프레임 코드 플래그: 금 간 프레임과 단단한 프레임.
inline constexpr std::uint8_t kBridgeCrackedFrame = 0x20, kBridgeHardFrame = 0x40;
struct BridgeLifeChange {
    std::uint16_t word{}; // 제거 경로에서는 원본과 같이 이전 단어를 그대로 둔다.
    int previous{}, remaining{};
    bool remove{};
    std::uint32_t removalFlags{};
    bool crack{}; // 제거가 아니고 이전 수명 ≥ 5, 새 수명 < 5: 금 간 프레임 전환(00421b60)을 시도한다.
    bool carriers{}; // 제거가 아니고 새 수명 ≤ 5: 칸 위 이동체 확인(004202f0 이후)을 한다.
};
// 원본 실행 상태 전역. 기본값은 싱글 플레이(로컬이 서버) 전투 화면이다.
struct BridgeDecayMode {
    bool server{true}; // 00540bc0 ↔ CD 00540a28. 꺼져 있으면 수명 0에서도 제거하지 않고 비트만 쓴다.
    bool authority{true}; // 00540bc4 ↔ CD 00540a2c. 스캔 실행과 destroy 재정의의 조건이다.
    bool editor{}; // 005c85a4 ↔ CD 00518904. 편집기에서는 스캔을 돌리지 않는다.
    bool debugRedraw{}; // 005453ac ↔ CD 0051f064. 금이 가지 않은 수명 변화에도 화면을 갱신한다.
    bool debugKeep{}; // 0054db80 ↔ CD 0052e928. 큰 그래프의 다리 destroy를 막는다.
};
// 한 칸 처리에서 읽고 바꾸는 다리 객체의 상태다.
struct BridgeDecayState {
    std::uint16_t id{};
    std::uint16_t word{}; // Squid +0xc. 수명 비트 외의 비트는 보존한다.
    int frame{}; // 다리 타입 프레임 번호(패치 +36 DWORD, CD +34 byte).
    std::int16_t graphSurfaces{}; // 이 객체가 속한 그래프 레코드의 표면 수(00462c80).
    bool graphValid{true}; // 그래프 번호가 254면 원본은 NULL 레코드를 읽는다. 그 경우 false.
    std::uint8_t extra{}; // 패치 +40 / CD +35. abstract 1, buried 8.
    bool dead{}; // state의 dead 비트(2).
};
// 한 칸 처리가 밖으로 내는 효과다. 실제 삭제·그리기·소리·낙하는 호출자가 수행한다.
enum class BridgeDecayEventKind {
    Destroy, // 기본 destroy(004af780)로 진행했다. flags는 넘긴 인자다.
    Redraw, // 공통 화면 갱신(vtable +0x88).
    Carriers, // 그 칸 위의 이동체 확인.
    CrackSound, // 시작 칸 위치에서 bridgeCrack.wav.
};
struct BridgeDecayEvent {
    BridgeDecayEventKind kind{};
    std::uint16_t id{};
    std::uint32_t flags{};
};
enum class BridgeDecayOutcome {
    InvalidGraph, // 그래프 번호 254: 원본은 NULL 레코드를 읽어 예외가 나고 스캔의 catch가 삼킨다(실행 검증 없음).
    SmallGraph, // 표면 수 5 미만: 그 칸을 destroy(0) 한다.
    Hard, // 단단한 프레임: 아무 일도 하지 않는다.
    Isolated, // 이웃 없음: 수명 1 감소.
    Closed, // 열린 방향 없음.
    Blocked, // 방문 뒤 붕괴 진행 플래그가 꺼져 있음.
    Decayed, // 방문 목록의 수명을 목표 값으로 맞췄다.
    Incomplete, // 접합 고리 등으로 방문 목록이 불완전하다(새 코드의 안전 중단). 상태를 바꾸지 않는다.
    Faulted, // 제거할 칸의 그래프가 254: 원본 예외 지점에서 중단한다(실행 검증 없음).
};
struct BridgeDecayCell {
    BridgeDecayOutcome outcome{};
    int lowest{}, target{}; // Decayed에서만 뜻이 있다. 0이 아닌 수명이 없으면 lowest는 0이다.
    std::vector<BridgeDecayEvent> events; // 원본 호출 순서.
    std::vector<BridgeDecayState> states; // 입력 순서 그대로, 바뀐 값을 반영한다.
};
// 프레임 전환 보조 결과. changed가 거짓이면 프레임은 그대로다.
struct BridgeFrameChange {
    std::uint16_t word{};
    int frame{};
    bool changed{};
};
// 00422490/00422bc0 ↔ CD 00449200/00449250의 스캔 상태. first..last는 양 끝을 포함한다.
struct BridgeDecayScanState {
    std::int32_t first{}, last{}, cursor{};
    double next{}; // 다음 주기가 시작되는 게임 시각.
};
// 이번 프레임에 훑는 번호 구간 [begin, end).
struct BridgeDecayScanStep {
    std::int32_t begin{}, end{};
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
    // 원본 00421c30 ↔ CD 0044a1c0: 수명 감소/0 제한·비트 보존·제거 플래그와 뒤따르는 효과의 조건.
    // server는 00540bc0 ↔ CD 00540a28이다(이전 이름 battle은 잘못된 해석이었다. SID 할당기가 서버 범위를 고르는 그 전역이다).
    // 금 간 그림·화면 갱신·이동체 확인·실제 객체 삭제 자체는 실행하지 않고 조건만 돌려준다.
    static BridgeLifeChange ReduceLife(std::uint16_t word, int reduction, bool server, char side);
    // 원본 00421b60 ↔ CD 0044a1c0 인라인: 같은 방향·변형 P·번호+10·금 간 플래그의 첫 프레임. 없거나 이미 금 간/단단하면 -1.
    // force가 참이면 단단한 프레임(번호 20 이상)을 번호 1로 보고 금 간 프레임을 찾는다(약화 00421db0의 규칙).
    static int CrackedFrame(const RiftTypeFrames& frames, int frame, bool force);
    // 원본 00421bd0: 금 간 번호는 10을 빼고 단단한 번호는 1로 바꿔 같은 방향·변형 P의 첫 프레임을 찾는다. 없으면 -1.
    static int NormalFrame(const RiftTypeFrames& frames, int frame);
    // 원본 00421db0: 수명을 금 간 기준 - 1(4)로 쓰고 금 간 프레임을 force 규칙으로 찾는다.
    static BridgeFrameChange Weaken(const RiftTypeFrames& frames, std::uint16_t word, int frame);
    // 원본 00421e60: 수명을 0으로 쓰고 보통 프레임으로 되돌린다.
    static BridgeFrameChange Restore(const RiftTypeFrames& frames, std::uint16_t word, int frame);
    // 원본 004220f0 ↔ CD 00449820(다리 vtable +0x10): 기본 destroy로 진행하면 참.
    // 편집기가 아니고 권한이 있고 abstract/buried가 아니고 그래프 표면 수가 5 이상일 때, 단단한 프레임이나 디버그 유지는 destroy를 막는다.
    static bool DestroyProceeds(const BridgeDecayMode& mode, std::uint8_t extra, std::int16_t graphSurfaces, std::uint8_t frameFlags);
    // 원본 00421c30 ↔ CD 0044a1c0 전체를 객체 하나에 적용한다: 제거(destroy 재정의 포함) 또는 수명 비트 갱신 →
    // 금 간 프레임 전환/디버그 갱신 → 이동체 확인. 효과는 events 끝에 원본 순서로 붙인다.
    // 반환값이 거짓이면 그래프 254인 칸의 destroy에서 원본 예외 지점에 닿은 것이다(상태는 그대로다).
    static bool ApplyLife(const RiftTypeFrames& frames, BridgeDecayState& state, int reduction, const BridgeDecayMode& mode,
        std::vector<BridgeDecayEvent>& events);
    // 원본 004227e0 ↔ CD 00449250 인라인: 스캔이 다리 한 칸에 하는 처리.
    // surfaces는 처리 시작 시점의 표면 스냅샷, states는 방문될 수 있는 모든 다리의 가변 상태다(시작 칸 포함).
    // 실제 삭제는 하지 않는다. 기본 destroy로 넘어간 칸은 dead로 표시하고 사건으로 알린다.
    static BridgeDecayCell DecayCell(const SurfaceFinder& surfaces, const RiftTypeFrames& frames,
        std::span<const BridgeDecayState> states, std::uint16_t root, const BridgeDecayMode& mode = {});
};
// 다리 붕괴 스캔의 커서와 주기. 칸 처리는 호출자가 구간의 번호마다 Eligible을 확인해 DecayCell로 한다.
class BridgeDecayScan {
public:
    // 0052f968 ↔ CD 0051f068: 스캔 주기(초).
    static constexpr float kPeriod = 10.0f;
    // 원본 00422490 ↔ CD 00449200: 범위는 서버 SID 영역 + 예측 머리다(패치 15000..23001, CD 6000..14000).
    static BridgeDecayScanState Reset(OriginalEdition edition, double now);
    // 원본 00422bc0 ↔ CD 00449250의 바깥 루프. 정지·편집기·권한 없음이면 아무것도 하지 않는다.
    // 남은 시간이 양수면 trunc(delta / 주기 × 범위)개, 아니면 범위 전체만큼 커서를 전진시키고(끝 번호까지),
    // 주기가 지났으면 커서를 처음으로 되돌리고 다음 시각을 now + 주기로 잡는다.
    static BridgeDecayScanStep Advance(BridgeDecayScanState& state, double now, double delta, bool paused,
        const BridgeDecayMode& mode = {});
    // 스캔이 한 칸 처리를 부르는 조건: free/dead/void가 아니고 다리이며 abstract가 아니다.
    static bool Eligible(std::uint8_t state, std::uint32_t flags2, std::uint8_t extra);
};
}
