// 10.78 Badges 저장 부모와 같은28개 실제 장부의 첫일치 count getter를 복원한다.
#pragma once
#include "o/RawMemStream.h"
#include <string_view>

namespace netstorm::o {
inline constexpr std::size_t kFortBadgeRecordCount=28; // 00593160..00594970의 실제 기록 수다.
inline constexpr std::size_t kFortBadgeRecordStride=220; // type와count 사이212BYTE를 포함하는 실제 기록 폭이다.
inline constexpr std::size_t kFortBadgeCountOffset=216; // 각 기록에서 전체 count DWORD가 있는 위치다.
// 기존 RawFortMetaLoad와 같은 장기 span을 직접 읽는다. 첫 type 일치의 DWORD 또는 미일치0을 반환한다.
std::uint32_t ReadFortBadgeCount(std::span<const std::uint8_t> badges,std::uint32_t type);
// 원본 Template.cpp0x412와 인라인 MemStream0x24를 구별한다.
enum class FortBadgesSaveAssertion : std::uint32_t {
    MissingTemplate=0x412, // 새 Badges 섹션 생성 전에 같은 this의 manager0을 진단한다.
    WriteCapacity=0x24 // 초기 count BYTE를 제외한 각 type/count BYTE의 용량 진단이다.
};
// 실제 manager/Create/getter와 명시 stream16BYTE/실제 버퍼를 연결한다.
struct FortBadgesSaveHooks {
    std::function<std::uint32_t(std::int32_t)> manager; // 고정 this 슬롯의 현재 첫 DWORD를 매 원문 위치에서 읽는다.
    std::function<void(FortBadgesSaveAssertion)> assertion; // 진단 복귀 뒤 현재 data/pos/end/capacity 변경을 허용한다.
    std::function<std::uint32_t(std::uint32_t,std::string_view)> create; // 0044d1b0(manager,"Badges")가 반환한 같은 stream이다.
    std::function<std::uint32_t(std::uint32_t)> count; // 0047d7c0(type)의 실제 전체 count DWORD다. 새 getter에는 같은 live 장부 span을 준다.
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; // 같은16BYTE/버퍼의 동기 빌림이다. 최초 buffer 할당은 저장이 끝날 때까지 살아 있어야 한다.
};
// 004be7f0: 최초 count 포인터를 유지하고 type0..254의 비0 DWORD만 두 BYTE로 쓴다.
void SaveFortBadges(std::int32_t slot,const FortBadgesSaveHooks& hooks);
}
