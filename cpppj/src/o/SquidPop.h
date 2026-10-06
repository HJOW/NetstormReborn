// 원본 raw SID의 일반 Pop을 복원한다. 공통 표시 선택 연결·postPop 효과 억제·비전투 단계다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidHash.h"

namespace netstorm::o {
class SquidDisplay;
// 패치판 overlap은 좌표와 앞선 spot 쓰기를 남기고 활성화하지 않는다.
enum class RawPopResult { Unchanged,Registered,Overlap };
class SquidPop {
public:
    // Unpop과 같은 풀/해시/spot을 받는다. 공통 표시는 선택 연결하며 postPop 효과는 아직 억제한다.
    SquidPop(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,SquidDisplay* display=nullptr);
    // 004b02d0 ↔ CD 004ad490. 현재 SHP 프레임 크기로 단계를 정하고 실제 raw 공간 필드를 갱신한다.
    // 일반 공통 firstPop/postPop만 지원하며 영역·건물 부착·파생 후처리는 변경 전에 거부한다.
    RawPopResult Pop(Sid sid,const RiftTypeRecord& type,float frameWidth,float frameHeight,
        float x,float y,std::uint32_t flags=0);
    // 원본 가상 주소를 메타데이터로 비교한다. 호스트 포인터로 호출하지 않는다.
    static bool Supports(OriginalEdition edition,std::uint32_t vtable,std::uint32_t flags);
private:
    SidPool& pool_;
    SquidHash& hash_;
    std::span<std::uint8_t> spots_;
    SquidDisplay* display_{}; // 선택한 공통 표시 효과. nullptr은 기존 표시 억제 계약이다.
};
}
