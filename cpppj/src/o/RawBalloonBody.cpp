// 그림자 갱신 중의 현재 부모 재독과 원본 float 저장 지점을 보존한다.
#include "o/RawBalloonBody.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kHeightToCell=0.09090909361839294F,kShadowX=0.10000000149011612F,kShadowY=0.30000001192092896F; //원본504194/5022ec/504190이다.
// 비정렬 필드를 실제 BYTE 폭으로 조립한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{};
    // 낮은 BYTE부터 읽는다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);return value;
}
// 현재 raw WORD/DWORD만 쓰고 인접 바이트는 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 지정 폭의 낮은 BYTE만 기록한다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 그림자의 동일 TYPE 내 두 번째 그림 묶음을 원본 signed DWORD 감김으로 계산한다.
std::uint32_t Frame(const SidPool& pool,Sid parent,std::int32_t count) { return static_cast<std::uint32_t>(count)+Read(pool.Slot(parent),36); }
}
// 같은 PATCH 풀/TYPE/공통 모듈과 물리 지도만 허용한다.
RawBalloonBody::RawBalloonBody(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,
    const bool& boss,const RawCarrierPostPop& post,const RawCarrierPreDestroy& pre,BalloonBodyHooks hooks)
    :pool_(pool),types_(types),spots_(spots),boss_(boss),post_(post),pre_(pre),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || spots.size()!=65536 || &post.Pool()!=&pool || &pre.Pool()!=&pool) throw std::invalid_argument("벌룬의 실제 원본 월드 연결 오류");
}
// 가상 표만 확인하며 원본의 두 동일 생성자 타입을 임의로 합성하지 않는다.
bool RawBalloonBody::Handles(Sid sid) const { return Read(pool_.Slot(sid),0)==0x5040c0; }
// 공간 변경 전 현재 부모만 검사하며 상위 공통 ValidateBase와 효과를 중복 실행하지 않는다.
void RawBalloonBody::Validate(Sid sid,std::uint32_t flags) const {
    (void)flags;const auto raw=pool_.AllocatedBytes(sid);if (!Handles(sid) || (raw[11]&9) || raw[10]<70 || raw[10]>=types_.size()) throw std::logic_error("벌룬 후처리의 현재 원시 부모 오류");
}
// 최초flag는 그림자 좌표 임시값과 별개다. 디컴파일의 같은 임시변수명으로 합치지 않는다.
void RawBalloonBody::PostPop(Sid sid,std::uint32_t flags) const {
    const auto kind=pool_.Slot(sid)[10];if (kind>=types_.size() || !hooks_.frames) throw std::logic_error("벌룬의 현재 TYPE 프레임 미연결");const auto& type=types_[kind];const bool first=(flags&1)!=0; //최초 TYPE 주소와 별도 보존한 최초flag다.
    if (first && std::bit_cast<std::int32_t>(Read(pool_.Slot(sid),36))<hooks_.frames(kind) && (type.flags1&0x400000U)) {
        const auto shadow=hooks_.create(pool_.Slot(sid)[10],2);Put(pool_.AllocatedBytes(shadow),36,Frame(pool_,sid,hooks_.frames(kind)));pool_.AllocatedBytes(shadow)[40]|=8;Put(pool_.AllocatedBytes(sid),8,shadow.value,2);
        hooks_.pop(shadow,std::bit_cast<float>(Read(pool_.Slot(sid),14)),std::bit_cast<float>(Read(pool_.Slot(sid),18)),0);
    }
    auto shadow=Sid{static_cast<std::uint16_t>(Read(pool_.Slot(sid),8,2))}; //Unpop 중 부모WORD가 바뀌어도 이 첫 그림자 주소를 유지한다.
    if (shadow.value) {
        hooks_.unpop(shadow,0);Put(pool_.AllocatedBytes(shadow),36,Frame(pool_,sid,hooks_.frames(kind)));const auto x=std::bit_cast<float>(Read(pool_.Slot(sid),14)),y=std::bit_cast<float>(Read(pool_.Slot(sid),18));
        const auto height=static_cast<double>(pool_.Slot(sid)[30])*kHeightToCell;pool_.AllocatedBytes(shadow)[35]=0xe6; //42ad64~42ad9d는 중간float 저장 없이 두 좌표의 fstp에서만 반올림한다.
        hooks_.pop(shadow,static_cast<float>(height*0.5+x+kShadowX),static_cast<float>((height+y)-kShadowY),0);
    }
    shadow=Sid{static_cast<std::uint16_t>(Read(pool_.Slot(sid),8,2))}; //첫 가상Pop 효과 뒤 현재 그림자WORD를 다시 읽는다.
    if (shadow.value) {
        auto raw=pool_.AllocatedBytes(shadow);raw[40]^=static_cast<std::uint8_t>((pool_.Slot(sid)[40]^raw[40])&0x10);raw[35]=0xe6;pool_.AllocatedBytes(sid)[35]=0xe2;
        if (!pool_.Slot(sid)[30]) {
            const auto x=std::bit_cast<float>(Read(pool_.Slot(sid),14)),y=std::bit_cast<float>(Read(pool_.Slot(sid),18));if (!std::isfinite(x) || !std::isfinite(y) || x<0 || x>=256 || y<0 || y>=256) throw std::out_of_range("벌룬 표시 깊이의 현재 칸 범위");
            pool_.AllocatedBytes(sid)[35]=static_cast<std::uint8_t>(-20-((spots_[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]&0x10)!=0));raw[35]=0xec;
        }
    }
    if (!first && !boss_) hooks_.check(sid);post_.DamageablePostPop(sid,flags);
}
// 42ae80은 그림자 삭제 뒤426890과 동일한 기본 효과 순서를 갖는다.
void RawBalloonBody::PreDestroy(Sid sid,std::uint32_t flags) const {
    const auto shadow=Sid{static_cast<std::uint16_t>(Read(pool_.Slot(sid),8,2))};if (shadow.value) hooks_.destroy(shadow,0);pre_.PreDestroy(sid,flags);
}
// 같은 유일한 슬롯 저장소다.
const SidPool& RawBalloonBody::Pool() const { return pool_; }
}
