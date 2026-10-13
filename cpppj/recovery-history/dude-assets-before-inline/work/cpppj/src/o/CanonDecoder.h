// 원본 CanonDecoder.cpp: 패턴 표(여러 칸으로 이루어진 모양)를 회전을 반영해 한 칸씩 풀어 주는 반복자.
// 영역(puzzlePiece)·다리(bridge) 패턴을 옮겼다.
// 타입별 선택과 섬·받침·공유 전투 패턴은 CanonTypeDecoder에서 제공한다.
#pragma once
#include "o/RiftType.h"
#include <array>
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
// 패턴 레코드(원본 72바이트): 정수 셋과 4바이트 셀 15개. 셀은 줄 우선으로 폭 × 높이만큼 쓴다.
struct CanonPattern {
    std::int32_t first{};   // 다리의 추첨 가중치. 영역 패턴은 모두 1.
    // width: 가로 크기.
    std::int32_t width{};   // +4
    // height: 세로 크기.
    std::int32_t height{};  // +8
    // 셀: [0] 변형 숫자 글자('1'), [1] 방향 글자('A'~'P', 빈 칸은 '.'), [2] 미확정, [3] 번호 글자('a'~, 없으면 0).
    std::array<std::array<std::uint8_t, 4>, 15> cells{};
};
// 영역 패턴 표(패치 005300f0, 68개). `Territory` 레코드의 모양 번호(하위 6비트)로 고른다.
std::span<const CanonPattern> TerritoryPatterns();
// 다리 패턴 표(패치 0052f998, CD 00514a80), 가중치 합은 각각 287/306이다.
std::span<const CanonPattern> BridgePatterns(OriginalEdition edition = OriginalEdition::Patch1078);
// 원본 004257c0 ↔ CD 0041fc70: 누적 가중치와 signed 나머지를 <=로 비교한다.
std::size_t SelectBridgePattern(std::int32_t random, OriginalEdition edition = OriginalEdition::Patch1078);

// 원본 반복자 객체(0x54바이트).
class CanonDecoder {
public:
    // 원본 FUN_00425c20 ↔ CD 0041fcf0의 영역·다리 패턴 경로. direction은 방향 값이며 회전은 direction / 2다.
    // frames는 그 타입의 프레임 코드 표다. (x, y)는 첫 칸의 좌표다.
    CanonDecoder(const RiftTypeFrames& frames, const CanonPattern& pattern, int direction, float x, float y,
        std::function<void()> missingFrameDiagnostic={});
    // 패턴 없는 타입은 한 칸이고 방향의 크기/부호는 출력에 영향을 주지 않는다. 기본 프레임 또는 명시 프레임을 선택하며 CD는 홀수를 assert한다.
    CanonDecoder(const RiftTypeFrames& frames,int defaultFrame,int argument,int direction,float x,float y,
        bool explicitFrame=false,OriginalEdition edition=OriginalEdition::Patch1078);
    // 00425b90 / CD 004202e0: 현재 좌표 절삭에서 순회 개수×현재 발자국을 뺀 미리보기 사각형이다.
    std::array<int,4> Bounds(int footX,int footY) const;
    // 아직 칸이 남았는가(원본 +0x14).
    bool Valid() const;
    // 원본 FUN_00425860 ↔ CD 0041feb0: 다음 유효 칸으로 간다. 빈 칸과 프레임을 찾지 못한 칸은 건너뛴다.
    void Advance();
    // 현재 칸의 프레임 번호(+0x10).
    int Frame() const;
    // 현재 칸의 좌표(+0x20, +0x24).
    float X() const;
    float Y() const;
    // 현재 칸의 번호 글자에서 'a'를 뺀 값(+0x18). 번호가 없으면 -0x61이다.
    int Label() const;
    // 원본 FUN_00425c00: 현재 프레임 코드의 방향 글자.
    char Side() const;
private:
    // frames_: 타입 프레임의 코드와 방향별 번호 구간을 조회하는 표 (RiftTypeFrames).
    const RiftTypeFrames* frames_;
    // pattern_: 칸별 방향·변형·라벨을 펼칠 Canon 패턴 자료.
    const CanonPattern* pattern_;
    // missingFrameDiagnostic_: 실제 FindNumber 실패의 보고 후 계속 진행을 요청한다. 빈 값은 기존 호출 동작이다.
    std::function<void()> missingFrameDiagnostic_;
    // rotation_: 패턴을 회전할 방향 값.
    int rotation_{};        // +8
    bool mirrored_{};       // +0xc (영역 패턴에서는 항상 거짓)
    // frame_: 타입에서 사용할 그림 프레임 번호.
    int frame_{-1};         // +0x10
    // valid_: 현재 자료나 순회 항목을 읽을 수 있는지.
    bool valid_{true};      // +0x14
    // label_: 화면에 표시할 항목 문구 또는 패턴 번호.
    int label_{};           // +0x18
    // step_: 단계.
    int step_{1};           // +0x1c
    // x_: X 좌표.
    // y_: Y 좌표.
    float x_{}, y_{};       // +0x20, +0x24
    // originX_: 좌표 계산의 기준 X 원점.
    // originY_: 좌표 계산의 기준 Y 원점.
    float originX_{}, originY_{}; // +0x2c, +0x30
    // outerCount_: 패턴 바깥 반복의 칸 개수.
    int outerCount_{};      // +0x38
    // innerCount_: 패턴 안쪽 반복의 칸 개수.
    int innerCount_{};      // +0x3c
    // patternX_: 현재 Canon 패턴의 X 인덱스.
    int patternX_{};        // +0x40
    // patternY_: 현재 Canon 패턴의 Y 인덱스.
    int patternY_{};        // +0x44
    // outer_: 현재 패턴 바깥 반복 인덱스.
    int outer_{};           // +0x48
    // inner_: 현재 패턴 안쪽 반복 인덱스.
    int inner_{};           // +0x4c
    int defaultFrame_{},argument_{}; // 패턴 없는 타입의 기본 프레임과 원본 첫 인자다.
    bool explicitFrame_{}; // +0x50: 첫 인자를 프레임 번호로 사용할지 결정한다.
};
}
