// ClientBoard의 일반 자산/그림자 수집을 실제 SID/해시/타입/물리 SHP 헤더로 복원한다.
#pragma once
#include "client/DrawSubmission.h"
#include "o/SidPool.h"
#include "o/SquidHash.h"
#include "o/SquidDisplay.h"

namespace netstorm::client {
// 모든 입력은 이 객체보다 오래 살아야 한다. 같은 수집기 안에서는 원본 단계/y/x/next 순서를 유지한다.
class ObjectCollection {
public:
    // 타입과 실제 SHP 경계 캐시를 참조한다. tokenBase+SID*stride가 32비트를 넘으면 거부한다.
    ObjectCollection(o::SquidHash& hash,const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
        std::span<const o::SquidDisplayShape> shapes,std::uint32_t tokenBase=0);
    // 1~3단계에서 각 SID의 본체/그림자를 별도로 판정한다. 정상 반환 때 해시는 2단계가 된다.
    // limit은 전체 count<limit의 엄격한 assert다. 실패 전 기록/선택 상태는 남을 수 있다.
    std::int32_t Collect(o::RenderWorldRect view,std::span<o::RenderCollectionEntry> output,std::int32_t limit) const;
    // sprite 경계가 객체 토큰을 같은 풀의 실제 SID로 되돌릴 때 사용한다.
    o::Sid Object(std::uint32_t token) const;
private:
    // 패치 frameCheck(false)/물리 범위와 CD의 실제 SHP 프레임 참조를 검사한다.
    const o::SquidDisplayFrame& Frame(std::uint32_t type,std::int32_t frame) const;
    // +0.5/CRT 하위 DWORD→단계 크기 나눗셈→상단 제한으로 확장 전 칸을 얻는다.
    static int Cell(float coordinate,int level);
    // float 저장 위치와 판본별 NaN 조건을 보존해 월드 프레임의 포함 교차를 판정한다.
    bool Visible(o::RenderWorldRect view,float x,float y,const o::SquidDisplayFrame& frame) const;
    o::SquidHash& hash_;
    const o::SidPool& pool_;
    std::span<const o::RiftTypeRecord> types_;
    std::span<const o::SquidDisplayShape> shapes_;
    std::uint32_t tokenBase_{};
};
// 공통 일반 자산/효과 슬롯을 실제 수집기에 연결한다. collector 수명은 호출자가 유지해야 한다.
void BindObjectCollection(DrawCollectionHooks& hooks,const ObjectCollection& collector);
}
