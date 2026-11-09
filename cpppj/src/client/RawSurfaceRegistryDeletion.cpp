// 실제 Player 기록과 정적 SID 목록의 고유 삭제 부모를 World·Session 없이 연결한다.
#include "client/RawSurfaceWorld.h"
#include "client/ClientPlayerRecords.h" // 현재 base가 선택한 실제1620BYTE만 각 필드 접근 동안 빌린다.
#include "o/RawDudeBehavior.h" // 기존450920과 같은 목록 제거·공통 Post 본체를 공유한다.
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 실제 owner/목록만 사용하며 임시 Player·World·성공 값을 만들지 않는다.
// 같은 현재 기록 owner를 최초 결합만 하고 아직 없는 Player 배열을 읽거나 만들지 않는다.
void RawSurfaceStorage::BindPlayerRecords(std::weak_ptr<ClientPlayerRecords> records) {
    if (!UsesNativeTransmissionDevices() || borrowed_ || playerRecordsBound_ || records.expired()) throw std::invalid_argument("장기 Player 삭제의 최초 실제 기록 owner 계약");
    playerRecords_=std::move(records);playerRecordsBound_=true;
}
// 세 주소 계산을 각 현재 Player 배열에 결합하고 공통 부모 전후로 span을 보관하지 않는다.
void RawSurfaceStorage::PlayerPreDestroy(o::Sid sid,std::uint32_t flags) {
    o::RawPlayerWorld::PreDestroyNative(pool,sid,flags,{
        [this] { const auto current=playerRecords_.lock();if (!current) throw std::logic_error("장기 Player 삭제의 실제 현재 기록 owner 미연결");return o::PlayerRecordAccess{current,current->Records()}; },
        [this](auto object,auto value) { CommonDeletion().PreDestroy(object,value); }});
}
// 현재 extra9 관문은 목록을 읽기 전에 적용하며 제거 후 공통 Post의 실제 깊이/Graph를 보존한다.
void RawSurfaceStorage::ResidencePostDestroy(o::Sid sid,std::uint32_t flags) {
    if (!(pool.Slot(sid)[40]&9U)) o::RemoveStateSidList(sidLists.residences,sid.value);
    CommonDeletion().PostDestroy(sid,flags);
}
// Native 분배는 VT로 선택하고 기존450920의 같은 장기 population과 공통 부모를 사용한다.
void RawSurfaceStorage::DudePostDestroy(o::Sid sid,std::uint32_t flags) {
    o::RawDudeBehavior::PostDestroyNative(pool,sidLists.dudes,sid,flags,[this](auto object,auto value) { CommonDeletion().PostDestroy(object,value); });
}
}
