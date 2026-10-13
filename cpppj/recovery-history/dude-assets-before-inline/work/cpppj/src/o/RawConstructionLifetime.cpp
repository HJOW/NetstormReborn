// 표시 목록의 빈 칸 재사용·40칸 확장과 건설 표시 버퍼의 반납 순서를 보존한다.
#include "o/RawConstructionLifetime.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 목록 확장 단위/포인터 폭과 base 소멸자의 부모/form 표식이다.
constexpr std::size_t kGrowth=40,kPointerBytes=4;
constexpr std::int32_t kDestroyed=-12345;
// 부착은 client 번호를 할당하는 flags 0x10이다. 깊이 BYTE의 판본별 raw 위치도 보존한다.
constexpr std::uint32_t kAttachFlags=0x10;
constexpr std::size_t kPatchZ=35,kCdZ=33;
}
// 경계 누락과 원본 signed DWORD 범위를 넘는 목록은 초기화 때 거부한다.
RawRenderRegistry::RawRenderRegistry(const SidPool& pool,RenderRegistryHooks hooks,std::vector<std::uint32_t> initial)
    :pool_(pool),hooks_(std::move(hooks)),slots_(std::move(initial)) {
    if (!hooks_.reserve || !hooks_.releaseArray || !hooks_.attach) throw std::invalid_argument("표시 목록 경계 누락");
    if (slots_.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())/kPointerBytes)
        throw std::length_error("표시 목록 크기 오류");
}
// 일반 게임 객체 외에 등록 생성자가 처음 읽는 SID 0의 BYTE도 같은 부호로 확장한다.
std::int32_t RawRenderRegistry::Depth(Sid parent) const {
    return std::bit_cast<std::int8_t>(pool_.Slot(parent)[pool_.Edition()==OriginalEdition::Patch1078 ? kPatchZ : kCdZ]);
}
// 등록을 먼저 보이게 한 뒤 부모 부착을 요청한다. 부착 경계가 바꾼 진입 부모의 깊이를 다시 읽는다.
void RawRenderRegistry::Construct(RenderProcessFields& fields,std::uint32_t token,Sid parent,std::uint32_t type) {
    if (!token || std::find(slots_.begin(),slots_.end(),token)!=slots_.end()) throw std::invalid_argument("표시 목록 토큰 중복/0");
    if (fields.token && fields.slot>=0 && static_cast<std::size_t>(fields.slot)<slots_.size() && slots_[fields.slot]==fields.token)
        throw std::logic_error("살아 있는 표시 등록 필드 재사용");
    static_cast<void>(pool_.Slot(parent));fields.token=token;fields.form=fields.parent=0;
    fields.left=fields.top=fields.right=fields.bottom=0;
    auto free=std::find(slots_.begin(),slots_.end(),0U);
    if (free==slots_.end()) {
        const auto oldSize=slots_.size();
        if (oldSize>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())/kPointerBytes-kGrowth)
            throw std::length_error("표시 목록 확장 범위 오류");
        const auto newSize=oldSize+kGrowth;
        if (!hooks_.reserve(static_cast<std::uint32_t>(newSize*kPointerBytes))) throw std::bad_alloc();
        std::vector<std::uint32_t> next(newSize,0);std::copy(slots_.begin(),slots_.end(),next.begin());
        if (oldSize) hooks_.releaseArray();slots_.swap(next);free=slots_.begin()+static_cast<std::ptrdiff_t>(oldSize);
    }
    fields.slot=static_cast<std::int32_t>(free-slots_.begin());*free=token;fields.zOrder=Depth({});
    if (parent.value) { hooks_.attach(fields,type,parent,kAttachFlags);fields.zOrder=Depth(parent); }
}
// 원본 두 assert의 번호/동일 객체 조건을 안전한 예외로 옮기며 실패 때 슬롯은 보존한다.
void RawRenderRegistry::Destroy(RenderProcessFields& fields) {
    if (fields.slot<0 || static_cast<std::size_t>(fields.slot)>=slots_.size() || !fields.token || slots_[fields.slot]!=fields.token)
        throw std::logic_error("표시 목록 삭제 등록 불일치");
    slots_[fields.slot]=0;fields.form=fields.parent=kDestroyed;
}
// 원본 시스템 배열은 구멍을 압축하거나 소멸 때 줄이지 않는다.
std::span<const std::uint32_t> RawRenderRegistry::Slots() const { return slots_; }
// 호스트 어댑터의 풀 연결 검사용 참조다.
const SidPool& RawRenderRegistry::Pool() const { return pool_; }
// free는 카운터/팔레트를 고치지 않는다. 호스트 벡터만 실제로 반납하며 외부 효과의 원본 순서를 유지한다.
void DestroyConstructionVisual(ConstructionVisualFields& visual,RenderProcessFields& registration,
    RawRenderRegistry& registry,std::uint32_t flags,const std::function<void(ConstructionRelease)>& release) {
    if (!release) throw std::invalid_argument("건설 표시 반납 경계 누락");
    // 원본 포인터가 있는 버퍼만 차례로 반납한다. swap은 capacity까지 해제한다.
    if (!visual.image.pixels.empty()) { release(ConstructionRelease::Source);std::vector<std::uint8_t>().swap(visual.image.pixels); }
    if (!visual.output.empty()) { release(ConstructionRelease::Output);std::vector<std::uint8_t>().swap(visual.output); }
    if (!visual.particles.empty()) { release(ConstructionRelease::Particles);std::vector<ConstructionVisualParticle>().swap(visual.particles); }
    registry.Destroy(registration);
    if (flags&1U) release(ConstructionRelease::Object);
}
// Create가 공급한 부모와 현재 타입으로 등록하고 상위 부착의 현재 부모/초기 사각형을 되돌려준다.
ConstructionVisualHooks MakeConstructionRegistryHooks(RawRenderRegistry& registry,RenderProcessFields& registration,
    std::uint32_t token,ConstructionVisualHooks hooks) {
    if (!token) throw std::invalid_argument("건설 표시 등록 토큰 0");
    // 기존 부착 경계의 책임을 registry.Construct 안의 BaseProcess 경계로 넘긴다.
    hooks.attach=[&registry,&registration,token](ConstructionVisualFields& visual,std::uint32_t type) {
        registry.Construct(registration,token,visual.parent,type);
        if (registration.parent<0 || registration.parent>std::numeric_limits<std::uint16_t>::max())
            throw std::out_of_range("건설 표시 부착 부모 범위 오류");
        visual.parent={static_cast<std::uint16_t>(registration.parent)};
        visual.left=registration.left;visual.top=registration.top;visual.right=registration.right;visual.bottom=registration.bottom;
        visual.zOrder=registration.zOrder;
    };return hooks;
}
}
