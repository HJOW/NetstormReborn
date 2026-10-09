// 원본 CanonDecoder 생성자의 타입 전역/패턴 번호 선택을 공통 반복자에 연결한다.
#pragma once
#include "o/CanonDecoder.h"

namespace netstorm::o {
struct CanonTypeQuery {
    std::uint32_t type{}; // 현재 타입 번호다. 패턴 번호와 구별한다.
    int argument{},direction{}; // 패턴 번호 또는 명시 프레임과 방향 값이다.
    float x{},y{}; // 생성 시 캡처하는 시작 좌표다.
    bool explicitFrame{}; // 비패턴 경로에서만 argument를 프레임으로 선택한다.
};
// 00425c20 ↔ CD 0041fcf0: 앞 네 전역을 우선 비교하고 뒤 네 타입은 공유 표를 쓴다.
// 전역 순서: puzzlePiece, bridge, island, noIsland, thunderCannon, rainCannon, windArcher, windBlocker.
CanonDecoder DecodeCanonType(const RiftTypeFrames& frames,int defaultFrame,std::span<const std::uint32_t,8> patternTypes,
    CanonTypeQuery query,OriginalEdition edition);
// 분석/검사에 복원된 네 패턴 레코드를 제공한다. 그룹은 island=0, noIsland=1, 공유 전투=2다.
std::span<const CanonPattern> CanonSpecialPatterns(std::size_t group);
}
