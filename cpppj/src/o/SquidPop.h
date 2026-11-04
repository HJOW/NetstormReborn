// 원본 raw SID의 일반 Pop을 복원한다. 공통 표시·postPop 및 건물 옆면을 같은 공간에 선택 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidHash.h"
#include "o/RawSquidPopParent.h"

namespace netstorm::o {
class SquidDisplay;
class SquidPostPop;
class SquidUnpop;
class RawBuildingSpace;
class RawMetadisplayDirty;
// 패치판 overlap은 좌표와 앞선 spot 쓰기를 남기고 활성화하지 않는다.
enum class RawPopResult { Unchanged,Registered,Overlap };
// 객체를 공간 해시와 점유 지도에 등록하는 Pop 처리기이다.
class SquidPop {
public:
    // Unpop과 같은 풀/해시/spot을 받는다. 공통 표시·비용/목록/통계 후처리를 선택 연결한다.
    SquidPop(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,SquidDisplay* display=nullptr,SquidPostPop* postPop=nullptr);
    // 004b02d0 ↔ CD 004ad490. 현재 SHP 프레임 크기로 단계를 정하고 실제 raw 공간 필드를 갱신한다.
    // PATCH는 도달한 지점에서 현재 TYPE/프레임/효과를 조회하며 앞선 부분 변경을 유지한다. CD는 기존 사전 검사를 유지한다.
    RawPopResult Pop(Sid sid,const RiftTypeRecord& type,float frameWidth,float frameHeight,
        float x,float y,std::uint32_t flags=0);
    void BindDiagnostics(SquidPopDiagnostics diagnostics); // 같은 현재 TYPE/Client 진단과 선택적 지연 SHP 조회를 연결한다.
    RawPopResult PopCurrent(Sid sid,float x,float y,std::uint32_t flags=0); // 실제 PATCH World는 사전 TYPE/SHP 조회 없이 같은 현재 표와 실제 표시기를 사용한다.
    // 원본 가상 주소를 메타데이터로 비교한다. 호스트 포인터로 호출하지 않는다.
    static bool Supports(OriginalEdition edition,std::uint32_t vtable,std::uint32_t flags);
    // 프레임 변경의 제자리 갱신과 재등록이 같은 풀/표시 대상을 쓰는지 확인한다.
    void ValidateFrameBinding(const SidPool& pool,const SquidDisplay& display,const SquidUnpop& unpop) const;
    // 건물 옆면 효과를 같은 실제 공간에 연결한다. nullptr은 미연결 건물의 기존 거부 경로다.
    void SetBuildingSpace(const RawBuildingSpace* space);
    // 원본 표면/다리 통지를 같은 풀의 실제 미니맵 큐에 연결한다. nullptr은 표시 객체가 없는 기존 경로다.
    void SetMetadisplayDirty(const RawMetadisplayDirty* dirty);
    bool ReleaseMetadisplayDirty(const RawMetadisplayDirty* expected); // 같은 관측자 대여만 끊는다. 이후 연결한 새 미니맵의 대여는 유지한다.
private:
    RawPopResult PopPatch(Sid sid,float x,float y,std::uint32_t flags,const RiftTypeRecord* fallback,std::array<float,2> size); // 독립 명시 입력과 실제 현재 World를 같은 원문 부모로 실행한다.
    SquidPopDiagnostics diagnostics_; // 전체 TYPE/장치의 수명은 이 실행기보다 길어야 한다.
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // hash_: 좌표별 객체 후보를 찾는 공간 해시.
    SquidHash& hash_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<std::uint8_t> spots_;
    SquidDisplay* display_{}; // 선택한 공통 표시 효과. nullptr은 기존 표시 억제 계약이다.
    SquidPostPop* postPop_{}; // 선택한 공통 비용/목록/통계 효과. nullptr이면 이전 억제 경로다.
    const RawBuildingSpace* building_{}; // 건물 옆면의 실제 표면 WORD/진단 몸체다.
    const RawMetadisplayDirty* metadisplay_{}; // 좌표를 쓴 직후 spot보다 먼저 등록하는 원본 미니맵 통지 대상이다.
};
}
