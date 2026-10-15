// PATCH0041c100/CD00401c20의 실제 빈 ret8과 파생 VT+24를 구별한다.
#include "o/RawContentParentNotice.h"
#include <array>
#include <stdexcept>
#include <string>
#include <utility>

namespace netstorm::o {
namespace {
// 실제 PE에서 읽은 가상 표 주소와 +24 진입점이다. 호스트 함수 포인터로 사용하지 않는다.
struct ContentNoticeEntry {
    std::uint32_t table; // 현재 부모 raw DWORD와 비교할 원본 VT다.
    std::uint32_t entry; // 실제 부모 내용물 통지의 원본 코드 주소다.
};
#include "ContentNoticeVtables.inc"
}
// 분배기는 현재 풀을 빌리며 부모 상태나 목록을 별도로 복사하지 않는다.
RawContentParentNotice::RawContentParentNotice(const SidPool& pool,ContentFormNotice derived)
    :pool_(pool),derived_(std::move(derived)) {}
// 현재 VT를 기준으로 찾으며 owner/type/dead/void 필터를 원본 빈 몸체 앞에 추가하지 않는다.
std::uint32_t RawContentParentNotice::NativeEntry(Sid parent) const {
    const auto raw=pool_.Slot(parent); // 가상 호출 시점의 현재 부모 슬롯이다.
    std::uint32_t table=0; // 비정렬 DWORD의 little endian 조립 결과다.
    // 원본 32비트 VT 주소 네 바이트만 읽는다.
    for (std::size_t i=0;i<4;++i) table|=std::uint32_t(raw[i])<<(i*8);
    const auto entries=pool_.Edition()==OriginalEdition::Patch1078 ? std::span<const ContentNoticeEntry>(kPatchContentNotices) :
        std::span<const ContentNoticeEntry>(kCdContentNotices); // 판본별 실제 생성자 표다.
    // 생성자에서 실제 사용할 수 있는 VT만 허용한다.
    for (const auto& candidate:entries) if (candidate.table==table) return candidate.entry;
    throw std::logic_error("내용물 부모의 미복원 VT: "+std::to_string(table));
}
// 원본 기본 가상 통지는 아무 자료도 읽거나 쓰지 않는다.
void RawContentParentNotice::Notice(Sid parent,Sid form,std::uint32_t reason) const {
    const auto entry=NativeEntry(parent); // 원본 VT+24의 실제 선택 결과다.
    if (entry==(pool_.Edition()==OriginalEdition::Patch1078 ? 0x41c100U : 0x401c20U)) return;
    if (!derived_) throw std::logic_error("실제 내용물 부모 파생 통지 미연결: "+std::to_string(entry));
    derived_(parent,form,reason);
}
}
