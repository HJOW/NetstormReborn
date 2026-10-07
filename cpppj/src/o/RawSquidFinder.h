// 원본 일반 사각형 탐색기의 4단계 해시/동적 raw 체인을 복원한다. flag 8 표면 이웃 탐색과 별개다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidHash.h"
#include <functional>
#include <unordered_set>

namespace netstorm::o {
// 원본 Begin의 정수 사각형이다. 오른쪽/아래 -1은 보드 끝이며 경계 접촉을 포함한다.
struct SquidSearchArea { int left{},top{},right{-1},bottom{-1}; };
// 원본 finder +4..+60의 상태를 호스트 주소 없이 보존한다.
struct SquidSearchState {
    SquidSearchArea area;
    int firstX{},firstY{},lastX{},lastY{},level{},x{},y{};
    Sid next{},current{};
    bool skipSurface{},extendedHeight{};
};
class RawSquidFinder {
public:
    // 풀/해시/타입은 탐색기보다 오래 살아야 한다. 높이 확장은 원본 type +1e4(CD +1c4)의 명시적 입력이다.
    RawSquidFinder(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const int> heightExtensions={});
    // 004b16d0 ↔ CD 004eae20: flag 1이면 1단계부터, flag 4이면 발자국 높이를 확장한다.
    // 기본 가상 필터는 원본 0044daa0/CD 0040f180처럼 모두 허용한다. 다른 필터는 호출자가 제공한다.
    Sid Begin(SquidSearchArea area,std::uint32_t flags=0,std::function<bool(Sid)> filter={});
    // 004b1810 ↔ CD 004eafe0: y/x·단계·next 순서로 탐색하며 이전 반환의 next는 이미 저장되어 있다.
    // 이후 후보의 extra/좌표/타입과 아직 읽지 않은 버킷 머리는 그 시점의 값을 사용한다.
    Sid Next();
    // 독립 기계어 대조와 후속 수명 연결에 현재 커서를 공개한다.
    const SquidSearchState& State() const;
private:
    // 각 단계에서 오른쪽/아래 버킷을 한 칸 확장하고 배열 끝으로 자른다.
    void SetLevel(int level);
    // 체인이 끝나면 다음 버킷/행/단계를 선택한다. 전체 소진이면 false다.
    bool AdvanceBucket();
    // 현재 후보의 소수 좌표를 절삭한 정수 발자국과 원본 OR 교차 조건을 검사한다.
    bool Intersects(Sid sid) const;
    const SidPool& pool_;
    const SquidHash& hash_;
    std::span<const RiftTypeRecord> types_;
    std::span<const int> heightExtensions_;
    SquidSearchState state_;
    std::function<bool(Sid)> filter_;
    std::unordered_set<std::uint16_t> chain_; // 같은 버킷의 순환만 거부한다. 다른 버킷의 중복은 원본처럼 허용한다.
    bool started_{};
};
}
