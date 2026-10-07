// 원본 next/상태/좌표를 직접 사용하며 잘못된 체인·미복원 효과는 변경 전에 거부한다.
#include "o/SquidUnpop.h"
#include "o/Squid.h"
#include "o/SquidDisplay.h"
#include "o/RawGraph.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// raw 위치와 상태/매몰 비트. dead는 Unpop을 막지 않는다.
constexpr std::size_t kNext=4,kType=10,kState=11,kX=14,kY=18;
constexpr std::uint8_t kFree=1,kVoid=4,kContained=8,kBuried=8;
// 원본 건물군/spot 마스크다. 일반 섬/다리 통지 helper는 비전투 null 큐에서 자연 반환한다.
constexpr std::uint32_t kBuildingMask=0x50444200,kSpotMask=kBuildingMask|0x400ff;
// 선택한 가상 경로의 지원 비트. update8c가 update88로 꼬리 호출하는 것까지 확인했다.
struct DisplayVtable { std::uint32_t vtable; std::uint8_t mask; };
#include "UnpopVtables.inc"
// 정렬되지 않은 little endian raw 값을 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    std::uint32_t value=0;
    // 필요한 폭만 조합하여 CD 이웃 필드를 읽지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
// raw next는 ushort이며 제거 대상 자신의 next는 보존한다.
void Next(std::span<std::uint8_t> bytes,std::uint16_t next) {
    bytes[kNext]=static_cast<std::uint8_t>(next); bytes[kNext+1]=static_cast<std::uint8_t>(next>>8);
}
}
// 범위가 정확한 공유 지도만 받아 부분 지도 밖 쓰기를 막는다.
SquidUnpop::SquidUnpop(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,SquidDisplay* display)
    :pool_(pool),hash_(hash),spots_(spots),display_(display) {
    if (spots_.size()!=kWorldCells*kWorldCells) throw std::invalid_argument("Unpop spot 지도 크기 오류");
}
// 잘못된 SID는 풀 밖 접근 전에 거부한다. 예측 머리도 원본 Take/Unpop 범위에서는 허용된다.
std::span<std::uint8_t> SquidUnpop::Bytes(Sid sid) {
    return std::span(pool_.bytes_).subspan(pool_.Offset(sid),pool_.layout_.stride);
}
// 코드 주소는 비교용 기록값이다. 실제 공통 표시 연결도 이 지원 표를 따른다.
bool SquidUnpop::SupportsDisplay(OriginalEdition edition,std::uint32_t vtable,std::uint32_t flags) {
    const auto values=edition==OriginalEdition::Patch1078 ? std::span<const DisplayVtable>(kPatchDisplayVtables) :
        std::span<const DisplayVtable>(kCdDisplayVtables);
    const auto mask=(flags&0x2000)!=0 ? 2U : 1U;
    // 확인한 가상 함수가 아니면 미복원 override를 적용하지 않는다.
    for (const auto& value:values) if (value.vtable==vtable) return (value.mask&mask)!=0;
    return false;
}
// raw 풀 소유자를 읽기 전용으로 확인한다.
const SidPool& SquidUnpop::Pool() const { return pool_; }
// 크기뿐 아니라 동일한 공간 인스턴스의 연결을 요구한다.
void SquidUnpop::ValidateGraph(const RawGraph& graph) const { graph.ValidateSpace(hash_,spots_); }
// 일반 공간 경로의 상태·spot·체인 쓰기 뒤 선택 연결한 공통 표시를 원본 순서로 적용한다.
void SquidUnpop::Unpop(Sid sid,const RiftTypeRecord& type,std::uint32_t flags) {
    auto bytes=Bytes(sid);
    if (!sid.value || bytes[kType]<kFirstAssetTypeNumber || (bytes[kState]&kContained))
        throw std::logic_error("Unpop 자산/SID 상태 오류");
    if ((bytes[kState]&(kFree|kVoid))!=0) return;
    const bool buried=(bytes[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&kBuried)!=0;
    if (!buried && (type.flags2&kBuildingMask))
        throw std::logic_error("Unpop 건물 부착 표면 효과는 아직 지원하지 않습니다");
    if (!SupportsDisplay(pool_.Edition(),Read(bytes,0,4),flags))
        throw std::logic_error("Unpop의 파생 표시 갱신 효과는 아직 지원하지 않습니다");
    const auto x=std::bit_cast<float>(Read(bytes,kX,4)),y=std::bit_cast<float>(Read(bytes,kY,4));
    // 버킷 조회에서 유한 좌표·지도 범위를 검증한다. 제거는 저장된 level을 신뢰하지 않는다.
    static_cast<void>(SquidHash::BucketIndex(0,x,y));
    const bool writesSpots=!buried && ((type.flags2&kSpotMask)!=0 || (type.flags1&TypeFlag1::kSurface)!=0);
    const int right=static_cast<int>(x),bottom=static_cast<int>(y);
    int left=right,top=bottom;
    if (writesSpots) {
        if (type.footX<1 || type.footY<1 || type.footX>kWorldCells || type.footY>kWorldCells)
            throw std::logic_error("Unpop 발자국 크기 오류");
        left=right-type.footX+1; top=bottom-type.footY+1;
        if (left<0 || top<0) throw std::out_of_range("Unpop 지도 밖 발자국");
    }
    std::uint16_t previous=0; std::uint16_t* bucket=nullptr; bool found=false;
    // 새 보호 경로는 원본 assert 앞의 부분 변경을 피하도록 체인을 먼저 검사한다.
    for (int level=0;level<4 && !found;++level) {
        auto& head=hash_.Bucket(level,x,y); auto current=head; previous=0; std::size_t visited=0;
        // 유효 SID 밖 next와 순환을 변경 전에 거부한다.
        while (current) {
            if (++visited>pool_.Capacity()) throw std::logic_error("Unpop 순환 체인");
            const auto entry=pool_.Slot(Sid{current}); const auto next=static_cast<std::uint16_t>(Read(entry,kNext,2));
            if (current==sid.value) {
                if (next==current || (previous && next==previous)) throw std::logic_error("Unpop 제거 뒤 자기 next");
                if (next>=pool_.Capacity()) throw std::out_of_range("Unpop next SID 범위 오류");
                bucket=&head; found=true; break;
            }
            if (next==current) throw std::logic_error("Unpop 자기 next");
            previous=current; current=next;
        }
    }
    if (!found && pool_.Edition()==OriginalEdition::Patch1078) throw std::logic_error("Unpop 해시에서 SID를 찾지 못했습니다");
    if (display_) display_->Validate(pool_.Edition(),bytes,flags);
    bytes[kState]|=kVoid;
    if (writesSpots) {
        // 원본 y/x 순서로 low byte만 해제하고 다른 spot 비트는 보존한다.
        for (int cy=top;cy<=bottom;++cy) {
            // 발자국 루프의 정수 절삭과 EffectiveGenus의 소수 편향 계산을 구별한다.
            for (int cx=left;cx<=right;++cx) {
                const auto genus=static_cast<std::uint8_t>(Squid::EffectiveGenus(type.flags2,x,y,type.footX,type.footY,cx,cy));
                auto& spot=spots_[static_cast<std::size_t>(cy*kWorldCells+cx)]; spot=static_cast<std::uint8_t>(spot&~genus);
            }
        }
    }
    if (found) {
        const auto next=static_cast<std::uint16_t>(Read(bytes,kNext,2));
        if (!previous) *bucket=next; else Next(Bytes(Sid{previous}),next);
    }
    // 원본은 void/spot/체인 제거 뒤 이전 위치를 갱신한다.
    if (display_) display_->Update(bytes,flags);
}
}
