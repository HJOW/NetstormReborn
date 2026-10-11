// 건설 표시의 초기 픽셀 선택 순서와 난수 소비·삭제 뒤 재초기화 필드를 보존한다.
#include "o/RawConstructionVisual.h"
#include "o/RawConstructionProcess.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <unordered_set>

namespace netstorm::o {
namespace {
// 원본 객체/색 표/입자 확보 크기와 최소 입자 수·불투명 비율의 분모다.
constexpr std::uint32_t kObjectBytes=0x88,kPaletteBytes=8,kParticleBytes=12,kMinParticles=500,kPixelRatio=6;
// 투명 팔레트 항목과 패치 부모 유효성 검사에 쓰는 raw 위치다.
constexpr std::uint8_t kTransparent=0xff,kFree=1,kDead=2,kVoid=4;
constexpr std::size_t kNext=4,kHead=6,kType=10,kState=11,kX=14,kY=18,kPatchOwner=34,kPatchZ=35,kCdZ=33;
// 실제 호출의 첫 색 띠(파랑→흰색)와 두 번째 띠(흰색→128/255 회색), 공통 보정값이다.
constexpr std::array<double,3> kBlue{0.0,0.0,1.0},kWhite{1.0,1.0,1.0},kGray{128.0/255.0,128.0/255.0,128.0/255.0};
constexpr double kNoise=2.4;
// 일반 건설의 최소 수명은 원본 float32 0x3fa66666이다. 즉시 건설은 2~9초다.
constexpr float kNormalMin=std::bit_cast<float>(0x3fa66666U),kNormalMax=2.0F,kInstantMin=2.0F,kInstantMax=9.0F;
// little endian DWORD 또는 WORD를 정렬과 무관하게 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    // 지정 바이트만 낮은 자리부터 조합한다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return result;
}
// 패치 004afd00→004ac040(1)→004ad730: owner<10·유효한 살아 있는 자산·공간/종속 form 상태를 확인한다.
void ValidatePatch(const SidPool& pool,Sid parent) {
    const auto raw=pool.Slot(parent);const auto vtable=Read(raw,0);
    if (!parent.value || raw[kPatchOwner]>=10 || !vtable || vtable==0xdddddddd || vtable==0xcdcdcdcd ||
        raw[kType]<70 || (raw[kState]&(kFree|kDead|kVoid))) throw std::logic_error("건설 표시 부모 유효성 오류");
    Sid current{static_cast<std::uint16_t>(Read(raw,kHead,2))};std::unordered_set<std::uint16_t> visited;
    // 종속은 free/void가 아닌 form이어야 한다. dead는 별도 제외하지 않으며 순환 링크는 호스트에서 거부한다.
    while (current.value) {
        if (current.value>=pool.Capacity() || !visited.insert(current.value).second) throw std::logic_error("건설 표시 종속 체인 오류");
        const auto child=pool.Slot(current);
        if (child[kType]>=70 || (child[kState]&(kFree|kVoid))) throw std::logic_error("건설 표시 종속 상태 오류");
        current={static_cast<std::uint16_t>(Read(child,kNext,2))};
    }
}
}
// 미연결 표시 경계는 new나 부모 변경 전에 거부한다.
RawConstructionVisual::RawConstructionVisual(SidPool& pool,ConstructionVisualState& state,GameRandom& random,ConstructionVisualHooks hooks)
    :pool_(pool),state_(state),random_(random),hooks_(std::move(hooks)) {
    if (!hooks_.reserve || !hooks_.attach || !hooks_.createPalette || !hooks_.gradient || !hooks_.buildPalette || !hooks_.capture || !hooks_.releaseParticles)
        throw std::invalid_argument("건설 표시 경계 누락");
}
// 원본이 실패한 포인터를 계속 사용하는 경로는 안전한 예외로 치환한다.
void RawConstructionVisual::Reserve(std::uint32_t bytes) const { if (!hooks_.reserve(bytes)) throw std::bad_alloc(); }
// 원본 두 파생 생성자의 수명 지정 이후에 캡처와 좌표/입자 초기화를 수행한다.
ConstructionVisualFields RawConstructionVisual::Create(Sid parent,bool instant) const {
    if (pool_.Edition()==OriginalEdition::Patch1078) ValidatePatch(pool_,parent);
    else {
        // CD의 isRobust는 항상 참이지만 후속 버퍼 초기화가 지원하지 않는 0/풀 밖 번호는 호스트에서 거부한다.
        if (!parent.value) throw std::out_of_range("건설 표시 부모 번호 오류");
        static_cast<void>(pool_.Slot(parent));
    }
    Reserve(kObjectBytes);ConstructionVisualFields fields;fields.parent=parent;
    hooks_.attach(fields,state_.processType);
    // 공통 생성자의 필드는 RenderProcess 부착이 끝난 뒤 초기화한다. 사각형/부모는 상위 부착의 값이다.
    fields.capacity=fields.total=0;fields.particles.clear();fields.lastTime=fields.elapsed=0;
    fields.image.pixels.clear();fields.selected=fields.opaque=fields.active=0;fields.firstDraw=false;
    if (!state_.palette) {
        Reserve(kPaletteBytes);state_.palette=hooks_.createPalette();
        if (!state_.palette) throw std::bad_alloc();
        hooks_.gradient(8,kBlue,kWhite,kNoise);hooks_.gradient(3,kWhite,kGray,kNoise);hooks_.buildPalette(state_.palette);
    }
    fields.palette=state_.palette;fields.instant=instant;
    fields.minimum=instant ? kInstantMin : kNormalMin;fields.maximum=instant ? kInstantMax : kNormalMax;
    // 버퍼 캡처의 부모 인자는 생성 진입 인자다. 동기화는 부착 경계가 바꿀 수 있는 현재 fields.parent를 읽는다.
    fields.image=hooks_.capture(parent);const auto count=PixelCount(fields);Reserve(count);
    fields.lastRow=(fields.image.height-1)*fields.image.width;fields.output.assign(count,kTransparent);
    Synchronize(fields);return fields;
}
// 잘못된 크기를 곱하거나 버퍼를 쓰기 전에 호스트의 폭/용량 계약을 검사한다.
std::uint32_t RawConstructionVisual::PixelCount(const ConstructionVisualFields& fields) const {
    const auto& image=fields.image;
    if (image.width<=0 || image.height<=0 || static_cast<std::uint64_t>(image.width)*image.height>std::numeric_limits<std::int32_t>::max() ||
        static_cast<std::uint64_t>(image.width)*image.height!=image.pixels.size()) throw std::invalid_argument("건설 표시 캡처 크기 오류");
    return static_cast<std::uint32_t>(image.pixels.size());
}
// x87 결과가 float 필드에 저장되는 경계에서 단정도 반올림을 보존한다.
void RawConstructionVisual::Synchronize(ConstructionVisualFields& fields) const {
    const auto raw=pool_.Slot(fields.parent);const auto z=raw[pool_.Edition()==OriginalEdition::Patch1078 ? kPatchZ : kCdZ];
    fields.zOrder=std::bit_cast<std::int8_t>(z);
    fields.left=fields.right=static_cast<float>(static_cast<double>(state_.scaleX)*std::bit_cast<float>(Read(raw,kX)));
    fields.top=fields.bottom=static_cast<float>(static_cast<double>(state_.scaleY)*std::bit_cast<float>(Read(raw,kY)));
    Seed(fields);
}
// 원본의 열/행 선택과 순열에 사용되는 모든 난수를 같은 공유 난수에서 차례로 소비한다.
void RawConstructionVisual::Seed(ConstructionVisualFields& fields) const {
    const auto count=PixelCount(fields);if (fields.output.size()!=count) throw std::invalid_argument("건설 표시 출력 크기 오류");
    fields.opaque=static_cast<std::uint32_t>(std::count_if(fields.image.pixels.begin(),fields.image.pixels.end(),[](auto pixel) { return pixel!=kTransparent; }));
    fields.capacity=std::max(kMinParticles,fields.opaque/kPixelRatio);
    if (!fields.particles.empty()) hooks_.releaseParticles();
    Reserve(fields.capacity*kParticleBytes);fields.particles.assign(fields.capacity,{});fields.total=0;
    fields.output.back()=fields.image.pixels.back();fields.image.pixels.back()=kTransparent;
    const auto width=fields.image.width,height=fields.image.height;std::int32_t column=(width-1)/2;
    // 중앙 열부터 0,+1,-2,+3처럼 교차하며 하단의 첫 불투명 픽셀 하나만 고른다.
    for (std::int32_t step=0;step<width && fields.total<=static_cast<std::uint32_t>(width)*2;++step) {
        column+=(step&1) ? step : -step;
        // height/3보다 아래인 행만 검사한다. 그 행 자체는 포함하지 않는다.
        for (std::int32_t row=height-1;row>height/3;--row) {
            const auto pixel=static_cast<std::uint32_t>(row*width+column);
            if (fields.image.pixels[pixel]==kTransparent) continue;
            if (fields.total<fields.capacity) {
                auto& particle=fields.particles[fields.total++];const auto fraction=static_cast<double>(random_.Next(65536))/65536.0;
                particle.life=static_cast<float>(fraction*std::fabs(static_cast<double>(fields.maximum)-fields.minimum)+fields.minimum);
                particle.pixel=pixel;particle.color=fields.image.pixels[pixel];fields.image.pixels[pixel]=kTransparent;++fields.selected;
            }
            break;
        }
    }
    if (fields.total) {
        // 열 수만큼 두 난수를 소비하고 입자 12바이트에 해당하는 값 전체를 교환한다.
        for (std::int32_t i=0;i<width;++i) {
            const auto first=random_.Next(fields.total),second=random_.Next(fields.total);std::swap(fields.particles[first],fields.particles[second]);
        }
    }
}
// 같은 월드의 시작 경계에만 연결할 수 있게 읽기 전용 풀을 제공한다.
const SidPool& RawConstructionVisual::Pool() const { return pool_; }
// 표시 필드는 호출자의 저장소에 넘기며 생성 실패는 기존 Start의 부착 단계까지 진행하지 않는다.
ConstructionProcessHooks MakeConstructionVisualHooks(const SidPool& pool,const RawConstructionVisual& visual,
    std::function<void(ConstructionVisualFields)> store,ConstructionProcessHooks hooks) {
    if (&pool!=&visual.Pool() || !store) throw std::invalid_argument("건설 표시 어댑터 연결 오류");
    hooks.effect=[&visual,store=std::move(store)](Sid parent,bool instant) { store(visual.Create(parent,instant)); };return hooks;
}
}
