// 원본 CanonDecoder.cpp: 패턴 표(여러 칸으로 이루어진 모양)를 회전을 반영해 한 칸씩 풀어 주는 반복자.
// 영역(puzzlePiece)·다리(bridge) 패턴을 옮겼다.
// 섬(00531410)·그 밖의 타입(005314a0, 005314e8) 패턴은 아직 옮기지 않았다.
#pragma once
#include "o/RiftType.h"
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::o {
// 패턴 레코드(원본 72바이트): 정수 셋과 4바이트 셀 15개. 셀은 줄 우선으로 폭 × 높이만큼 쓴다.
struct CanonPattern {
    std::int32_t first{};   // 다리의 추첨 가중치. 영역 패턴은 모두 1.
    std::int32_t width{};   // +4
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
    CanonDecoder(const RiftTypeFrames& frames, const CanonPattern& pattern, int direction, float x, float y);
    // 패턴 없는 타입은 한 칸이다. 기본 프레임 또는 명시 프레임 인자를 선택하며 CD는 홀수 direction을 assert한다.
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
    const RiftTypeFrames* frames_;
    const CanonPattern* pattern_;
    int rotation_{};        // +8
    bool mirrored_{};       // +0xc (영역 패턴에서는 항상 거짓)
    int frame_{-1};         // +0x10
    bool valid_{true};      // +0x14
    int label_{};           // +0x18
    int step_{1};           // +0x1c
    float x_{}, y_{};       // +0x20, +0x24
    float originX_{}, originY_{}; // +0x2c, +0x30
    int outerCount_{};      // +0x38
    int innerCount_{};      // +0x3c
    int patternX_{};        // +0x40
    int patternY_{};        // +0x44
    int outer_{};           // +0x48
    int inner_{};           // +0x4c
    int defaultFrame_{},argument_{}; // 패턴 없는 타입의 기본 프레임과 원본 첫 인자다.
    bool explicitFrame_{}; // +0x50: 첫 인자를 프레임 번호로 사용할지 결정한다.
};
}
