// 객체의 선택 비트와 표시 전 통지·사정거리 종류·내용물 표시 Regular 생성을 복원한다.
#pragma once
#include "o/SidPool.h"
#include "o/RiftType.h"
#include <functional>

namespace netstorm::o {
// 생성자 VT에 기록된 실제+7c 주소다. 호스트 함수 포인터로 역참조하지 않는다.
std::uint32_t SquidSelectionEntry(std::uint32_t vtable);
// 474240: 사정거리 표시를 만드는 group0/1/6/3 또는 bomb 타입의 현재 조건이다.
bool SelectionUsesRange(const RiftTypeRecord& type);
// 원본41c0b0과44b2b0의 현재 raw 쓰기를 같은 풀에서 처리한다.
class RawSquidSelection {
public:
    // refresh는 실제 가상+88이며 파생+7c에서만 필수다. 풀/대상은 이 몸체보다 오래 살아야 한다.
    RawSquidSelection(SidPool& pool,std::function<void(Sid)> refresh);
    void Mark(Sid sid,std::uint32_t value) const; // value 하위 bit0을 extra bit2로 바꾸며 Damageable은 먼저+88을 호출한다.
private:
    SidPool& pool_; // free/dead 필터를 추가하지 않는 실제 물리 슬롯의 소유자다.
    std::function<void(Sid)> refresh_; // 원본 쓰기 이전의 현재 파생 표시 갱신이다.
};
// 426080: 로딩·extra 조건과 TYPE 재검사 뒤491 내용물 표시의0.2 간격 Regular를 생성한다.
void StartSelectionContentPreview(const SidPool& pool,Sid parent,std::uint32_t loadingDepth,
    const std::function<void(Sid,std::uint32_t,float)>& regular,const std::function<void()>& assertion);
}
