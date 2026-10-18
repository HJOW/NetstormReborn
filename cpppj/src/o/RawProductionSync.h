// 생산 Deck과 현재 기술 부모의 CFF_TELEPORT 내용물을 원본43df80 순서로 동기화한다.
#pragma once
#include "o/RawContainedFinder.h"
#include <functional>

namespace netstorm::o {
// 실제 원본 전역을 같은 장기 저장소에서 읽는다.
struct ProductionSyncState {
    const std::uint32_t& localOwner; // 00540c70: TYPEbit4 내용물의 신전 존재를 조회할 owner다.
    const std::uint32_t& fullDeck; // 0054db14: 비활성/누락 생산 내용물의 제거를 억제한다.
};
// 현재 조회와 실제 삭제·커서·생성 경계다. 고정 성공값으로 대신하지 않는다.
struct ProductionSyncHooks {
    std::function<std::span<const std::uint8_t>()> deck; // 44f760의 현재 templateOwner Deck 원문이다.
    std::function<std::uint32_t()> technology; // 48fc60의 현재 기술 부모다. 순회 전/후 각각 조회한다.
    std::function<bool(std::uint32_t)> temple; // 실제 Player+7c 포인터의 비0 여부다.
    std::function<void()> cancelCursor; // 선택 중인 내용물 제거 전446e70의 실제 취소다.
    std::function<void(Sid,std::uint32_t)> destroy; // 내용물 가상+10(0)의 실제 수명이다.
    std::function<std::uint32_t(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> findKind; // 4acac0의 저장kind WORD 반환이다. SID가 아니다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> create; // 4afa70의 실제 ContentForm 생성·부착이다.
    std::function<void(std::string_view)> assertion; // 원본 중복 진단 뒤에도 생성은 계속한다.
};
// 같은 Pool/188TYPE/내용물 검색 정책과 현재 Deck을 사용한다.
class RawProductionSync {
public:
    // 모든 참조와 효과 대상은 동기화 호출보다 오래 살아야 한다.
    RawProductionSync(const SidPool& pool,std::span<const RiftTypeRecord> types,const ContainedFinderState& finder,ProductionSyncState state,ProductionSyncHooks hooks);
    // 실제43df80 전체다. 일반기술·비활성teleport·신전·선택취소/생성 순서를 보존한다.
    void Synchronize() const;
    const SidPool& Pool() const; // 같은 실제 월드의 조립 검사용 참조다.
private:
    Sid Parent(std::uint32_t value) const; // 전체DWORD의 물리 범위를 확인한 뒤에만 SID로 바꾼다.
    const SidPool& pool_; // 현재 기술 내용물의 유일한 저장소다.
    std::span<const RiftTypeRecord> types_; // 각 현재kind의 원본flags2다.
    const ContainedFinderState& finder_; // 같은 현재TYPE6/권한·진단 전역이다.
    ProductionSyncState state_; // 현재 owner/fullDeck의 안정 참조다.
    ProductionSyncHooks hooks_; // 선택된 실제 하위의 소비자다.
};
}
