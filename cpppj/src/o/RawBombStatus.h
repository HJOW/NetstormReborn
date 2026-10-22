// 117/118 상태 주문의 실제 대상 자격과 가상c0 효과를 같은 물리 풀에 적용한다.
#pragma once
#include "o/RiftType.h"
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 현재 TYPE 전역과 동맹을 외부 효과 뒤 다시 읽기 위한 장기 상태다.
struct BombStatusState {
    std::uint32_t altar{164},outpost{126},priest{158},fence{136},flare{106}; // 5412e8/250/2d0/278/200의 현재 종류다.
    std::span<const std::uint32_t> alliances; // 현재9×9 동맹 DWORD 표다.
};
// 비트 쓰기 이후 선택한 실제 가상 표시/배치/연결/플레어만 소비한다.
struct BombStatusHooks {
    std::function<void(Sid)> refresh; // 현재 가상88의4ad500이다.
    std::function<void(Sid,std::uint32_t)> unpop,repop; // 현재 가상48(0)/4c(40)이다.
    std::function<void(Sid)> fence; // 457e20→4578f0의 현재 썬 바리케이드 연결 정리다.
    std::function<void(Sid,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t)> flare; // 45a480(parent,count,event,type,direction)의 실제 방향 지정 호출이다.
};
// 큰 하위 호출은 같은 월드의 현재 구현으로 연결하고 자격/효과 순서는 원본대로 실행한다.
class RawBombStatus {
public:
    RawBombStatus(SidPool& pool,std::span<const RiftTypeRecord> types,BombStatusState& state,BombStatusHooks hooks); // PATCH 풀/188TYPE와 현재 전역의 수명을 빌린다.
    bool MayTarget(Sid parent,Sid target) const; // 117의41eca0과118의41fdb0을 정확한 현재 VT로 분배한다.
    void Apply(Sid parent,Sid target) const; // 41fc50/41fdd0의 운반체 제외·상태·현재 가상 효과를 실행한다.
private:
    std::uint32_t Kind(Sid parent) const; // 정확한 은신/마비 적용 표와 같은41fc50인 두 Wind 적용 표만 구분한다.
    std::uint32_t Genus(Sid sid) const; // 효과 뒤 바뀐 현재 TYPE의 genus를 읽는다.
    SidPool& pool_; // 원본 extra를 직접 쓰는 단일 물리 저장소다.
    std::span<const RiftTypeRecord> types_; // 현재 종류/자격의 같은188TYPE다.
    BombStatusState& state_; // 동맹과 현재 종류 번호를 사본으로 고정하지 않는다.
    BombStatusHooks hooks_; // 실제 선택된 큰 하위만 필수로 요구한다.
};
}
