// 현재 자산의 실제 VT+14/+18/+34를 같은 장기 삭제 부모에 연결한다.
#include "client/RawSurfaceWorld.h"
#include <array> // 보호 PE에서 읽은83개 생성자 VT를 고정 메타데이터로 보관한다.
#include <stdexcept>
#include <string>

namespace netstorm::client { // 실제 Pool/전역만 사용하며 임시 World나 Session을 생성하지 않는다.
namespace { // 주소는 가상 선택용 정수이며 호스트 함수 포인터로 실행하지 않는다.
struct NativeAssetDeletionEntry {
    std::uint32_t table; // 현재 raw 슬롯의 VT DWORD다.
    std::uint32_t pre; // 원본 VT+14의 실제 전처리 진입점이다.
    std::uint32_t post; // 원본 VT+18의 실제 후처리 진입점이다.
    std::uint32_t notice; // 원본 VT+34의 실제 부모 삭제 통지 진입점이다.
};
#include "AssetDeletionVtables.inc"

// 효과를 호출하는 시점의 VT를 읽으며 TYPE/owner/상태로 가상 선택을 바꾸지 않는다.
const NativeAssetDeletionEntry& CurrentAssetDeletionEntry(const o::SidPool& pool,o::Sid sid) {
    const auto raw=pool.Slot(sid);
    const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);
    for (const auto& entry:kNativeAssetDeletionEntries) if (entry.table==table) return entry; // 실제 생성자83개만 허용한다.
    throw std::logic_error("장기 자산 삭제의 미지 VT: "+std::to_string(table));
}
}

// 공통/표면/피해가능 객체/간헐천은 기존 실제 몸체를 재사용하고 전용 미연결은 선택 위치에서 알린다.
void RawSurfaceStorage::EmitDefaultAssetEffect(const o::SquidDestroyEvent& event) {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 자산 가상 효과의 Native 판본 계약");
    const auto& entry=CurrentAssetDeletionEntry(pool,event.sid);
    if (event.effect==o::SquidDestroyEffect::ReleaseDependent) {
        if (entry.notice==0x0041c100U) return; // 실제77VT의 빈 ret8이며 상태·TYPE·통지 인자를 추가로 읽지 않는다.
        throw std::logic_error("장기 자산의 전용 부모 삭제 통지 미연결: "+std::to_string(entry.notice));
    }
    const bool pre=event.effect==o::SquidDestroyEffect::PreDestroy;
    if (!pre && event.effect!=o::SquidDestroyEffect::PostDestroy) throw std::logic_error("장기 자산의 가상 삭제 효과 종류 오류");
    const auto method=pre ? entry.pre:entry.post;
    switch (method) {
    case 0x0041f910U: //실제오벨리스크의단일jmp는현재공통Pre와동일한인자/순서를사용한다.
    case 0x004b0950U:CommonDeletion().PreDestroy(event.sid,event.flags);return;
    case 0x004b0840U:CommonDeletion().PostDestroy(event.sid,event.flags);return;
    case 0x0044b4b0U: // 실제 Damageable의 가상 전처리다. 내부44afd0과 구별한다.
    case 0x00484f80U:DamageablePre().PreDestroy(event.sid,event.flags);return; // Nugget의 실제 단일 점프 thunk다.
    case 0x0044b9e0U:DamageablePost().PostDestroy(event.sid,event.flags);return;
    case 0x00426890U:CarrierPre().PreDestroy(event.sid,event.flags);return; //실제다섯Carrier VT가같은장기finder/보충부모를사용한다.
    case 0x004221b0U: // 다리의 고유 전처리다.
    case 0x00422300U: // 다리의 고유 후처리다.
    case 0x00442240U: // 섬의 고유 전처리다.
    case 0x00442640U:SurfaceLifetime().Hooks().emit(event);return; // noIsland의 고유 후처리다.
    case 0x00485580U:GeyserLifetime().PreDestroy(event.sid,event.flags);return;
    case 0x00485540U:GeyserLifetime().PostDestroy(event.sid,event.flags);return;
    default:throw std::logic_error("장기 자산의 전용 삭제 몸체 미연결: "+std::to_string(method));
    }
}
}
