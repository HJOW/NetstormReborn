// 다이스·제단의 고유 삭제 순서를 같은 현재 물리와 실제 선택된 부모에 공유한다.
#pragma once
#include "o/RawPlayerWorld.h"
#include "o/RawSquidNeighbors.h"
#include "o/SquidProcess.h"

namespace netstorm::o { // 독립 수명 객체와 Native 장기 owner가 같은 삭제 본문을 사용한다.
struct DaisDeletionHooks { // 게임 상태를 복제하지 않고 선택된 실제 하위만 호출하는 경계다.
    std::function<Sid(Sid)> findDais,findAltar,findPriest; // 현재 위치의 typed 다이스/제단과 강한 같은 owner 사제를 조회한다.
    std::function<void(Sid,Sid)> finishForms; // 현재 제단 Form의 Regular/Shared24마다 전체 희생 완료를 호출한다.
    std::function<void(Sid)> stopPriest; // 같은 Host의 첫604 제거와 현재 E 프레임 지정이다.
    std::function<void(std::uint32_t)> refreshOwner; // 원문495060의 현재1..8 참여 상태 갱신이다.
    std::function<bool()> boss; // 룬 후보를 선택할 때의 실제 현재 권한이다.
    std::function<void(Sid,std::uint32_t)> destroy,pre,post; // 실제 가상10·전체 Damageable 전처리/후처리다.
    std::function<PlayerRecordAccess()> players; // 현재 실제 Player base와 수명을 주소 소비 동안만 빌린다.
};
class RawDaisDeletion { // 상태 없는 고유 삭제 실행기로 Pool/표/역할/효과의 owner는 더 오래 살아야 한다.
public:
    RawDaisDeletion(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,const RawSquidNeighbors& footprint,
        const std::uint32_t& altarType,const std::uint32_t& runeType,DaisDeletionHooks hooks); // 연결 시 게임 전역·필드·효과를 읽지 않는다.
    void AltarPre(Sid sid,std::uint32_t flags) const; // 다이스 가상 삭제→현재 추상 룬 정리→전체 Damageable이다.
    void DaisPre(Sid sid,std::uint32_t flags) const; // extra9 관문→제단24 완료→604 중지→제단48→참여 갱신→부모다.
    void DaisPost(Sid sid,std::uint32_t flags) const; // PATCH의 현재 Player SID 쌍 정리 뒤 전체 Damageable에 직접 위임한다.
    const SidPool& Pool() const; // 장기 owner와 독립 객체의 실제 물리 정체성을 확인한다.
    static Sid FindTypeAt(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,Sid source,std::uint32_t type); // 원문4b1fa0의 현재 좌표/typed 점 조회이며 추가 상태/owner 필터가 없다.
    static Sid FindTypeAt(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,float x,float y,std::uint32_t type); // 이미 저장한 좌표 인자로 같은 실제 typed 검색 본문을 사용한다.
    static void FinishForms(const SidPool& pool,SquidProcessHost& host,Sid dais,Sid altar,const std::uint32_t& regularType,
        const std::uint32_t& sharedType,const std::function<void(Sid)>& finish); // 저장한 next와 현재 전체 역할/실제 Kernel을 원문 순서로 읽는다.
private:
    const SidPool& pool_; // 원문 물리 슬롯을 현재 조회한다.
    const SquidHash& hash_; // 주변 룬/제단의 현재 일반 해시다.
    std::span<const RiftTypeRecord> types_; // 현재 발자국을 복사하지 않고 빌린다.
    const RawSquidNeighbors& footprint_; // 같은 현재 TYPE/프레임의 발자국이다.
    const std::uint32_t& altarType_;const std::uint32_t& runeType_; // 매 후보에서 읽는 현재 전체 DWORD 역할이다.
    DaisDeletionHooks hooks_; // 실제 선택된 함수는 호출 동안 사본으로 수명을 유지한다.
};
}
