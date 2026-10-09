// 원본 raw SID의 일반 Pop을 복원한다. 공통 표시·postPop 일부 효과를 선택 연결하는 비전투 단계다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidHash.h"

namespace netstorm::o {
class SquidDisplay;
class SquidPostPop;
class SquidUnpop;
// 패치판 overlap은 좌표와 앞선 spot 쓰기를 남기고 활성화하지 않는다.
enum class RawPopResult { Unchanged,Registered,Overlap };
class SquidPop {
public:
    // Unpop과 같은 풀/해시/spot을 받는다. 공통 표시·비용/목록/통계 후처리를 선택 연결한다.
    SquidPop(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,SquidDisplay* display=nullptr,SquidPostPop* postPop=nullptr);
    // 004b02d0 ↔ CD 004ad490. 현재 SHP 프레임 크기로 단계를 정하고 실제 raw 공간 필드를 갱신한다.
    // 공통 firstPop/postPop과 명시 연결한 파생 후처리를 지원한다. 건물 부착·미연결 파생은 사전 거부한다.
    RawPopResult Pop(Sid sid,const RiftTypeRecord& type,float frameWidth,float frameHeight,
        float x,float y,std::uint32_t flags=0);
    // 원본 가상 주소를 메타데이터로 비교한다. 호스트 포인터로 호출하지 않는다.
    static bool Supports(OriginalEdition edition,std::uint32_t vtable,std::uint32_t flags);
    // 프레임 변경의 제자리 갱신과 재등록이 같은 풀/표시 대상을 쓰는지 확인한다.
    void ValidateFrameBinding(const SidPool& pool,const SquidDisplay& display,const SquidUnpop& unpop) const;
private:
    SidPool& pool_;
    SquidHash& hash_;
    std::span<std::uint8_t> spots_;
    SquidDisplay* display_{}; // 선택한 공통 표시 효과. nullptr은 기존 표시 억제 계약이다.
    SquidPostPop* postPop_{}; // 선택한 공통 비용/목록/통계 효과. nullptr이면 이전 억제 경로다.
};
}
