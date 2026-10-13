// 실제 두 판본의 x87 저장 위치와 단계별 연소 프레임 순서를 보존한다.
#include "o/RawBurningFrame.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 timer10배, 난수 단위, visible 여백과 프레임 경계의 실제 상수다.
constexpr double kTickScale=10.0,kRandomUnit=1.0/65536.0;
constexpr std::int32_t kInset=100,kRowsDivisor=5;
constexpr float kEmitMaximum=1.4f,kLifeMinimum=4,kLifeMaximum=9,kRise=-2;
// 원본 ftol의 정상 유한 범위를 확인한 뒤0 방향으로 정수화한다.
std::int32_t FrameInteger(double value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0)
        throw std::out_of_range("Burning frame 원본 정수화 범위 오류");
    return static_cast<std::int32_t>(value);
}
// 비정렬 raw DWORD는 호스트 포인터/정렬을 사용하지 않고 읽는다.
std::uint32_t FrameDword(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 실제 little endian 저장 순서다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(8*index);
    return value;
}
// Patch helper와 CD inline의 ordered 비교 차이를 보존한다. 좌표는 정상 유한 입력이다.
void IncludePoint(bool patch,float x,float y,float& left,float& top,float& right,float& bottom) {
    if (patch) {
        left=left<x ? left : x;top=top<y ? top : y;
        const auto nextRight=static_cast<float>(static_cast<double>(x)+1),nextBottom=static_cast<float>(static_cast<double>(y)+1);
        right=nextRight<right ? right : nextRight;
        if (bottom<=nextBottom) bottom=nextBottom;
    } else {
        left=left<=x ? left : x;top=top<=y ? top : y;
        const auto nextRight=static_cast<float>(static_cast<double>(x)+1),nextBottom=static_cast<float>(static_cast<double>(y)+1);
        right=nextRight<=right ? right : nextRight;bottom=nextBottom<=bottom ? bottom : nextBottom;
    }
}
}
// 기존 생성자·source·Append를 같은 풀에 연결하고 현재 캡처 소비자를 요구한다.
RawBurningFrame::RawBurningFrame(const SidPool& pool,std::span<const RiftTypeRecord> types,
    const RawBurningVisual& visual,const RawBurningSources& sources,GameRandom& random,
    const BurningFrameState& state,SquidDisplaySink& sink,BurningFrameHooks hooks)
    :pool_(pool),types_(types),visual_(visual),sources_(sources),random_(random),state_(state),sink_(sink),hooks_(std::move(hooks)) {
    if (&visual.Pool()!=&pool_ || &sources.Pool()!=&pool_) throw std::invalid_argument("Burning frame/생성/연소점 SID 풀 불일치");
    if (!hooks_.capture) throw std::invalid_argument("Burning frame 실제 ShapeToBuffer 캡처 미연결");
}
// 원본 HasHP는 일반 flags1 bit10이며 Dais genus400000에서는 extra bit1이다.
bool RawBurningFrame::HasHitPoints(std::span<const std::uint8_t> raw) const {
    const auto type=static_cast<std::size_t>(raw[10]);
    if (type>=types_.size()) throw std::out_of_range("Burning frame 현재 타입 표 범위 오류");
    const auto& record=types_[type];
    return record.flags2&0x400000U ? (raw[pool_.Edition()==OriginalEdition::Patch1078 ? 40U : 35U]&1U)!=0 : (record.flags1&0x10U)!=0;
}
// float 난수는 상위16비트·실제float 상수1/65536·넓은 x87 반환값을 사용한다.
double RawBurningFrame::Random(float minimum,float maximum) const {
    return static_cast<double>(random_.Next(65536))*kRandomUnit*std::abs(static_cast<double>(maximum)-minimum)+minimum;
}
// 이전 버퍼의 해제 뒤 원본은 일부 메타데이터를 유지한다. 정상 valid 캡처만 새 버퍼로 대체한다.
void RawBurningFrame::Capture(BurningVisualFields& fields) const {
    if (fields.total>fields.capacity || fields.particles.size()!=fields.capacity) throw std::invalid_argument("Burning capture 현재 입자 배열 오류");
    fields.image.pixels.clear();fields.rows.clear();fields.total=0;
    // 실제 particle container clear는 용량만큼 모든24바이트 의미 필드를0으로 만든다.
    for (auto& particle:fields.particles) particle={};
    if (!fields.victim.value || fields.victim.value==65535) return;
    const auto raw=pool_.Slot(fields.victim);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (!FrameDword(raw,0)) { if (patch) throw std::logic_error("Burning capture 가상표0 원본 assert");return; }
    if (!HasHitPoints(raw)) { if (patch) throw std::logic_error("Burning capture hasHitPoints 원본 assert");return; }
    auto image=hooks_.capture(fields.victim);
    const auto sum=static_cast<std::int64_t>(image.width)+image.height;
    if (image.width<0 || image.height<0 || sum>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("Burning capture 행 표식 크기 오류");
    fields.image=std::move(image);fields.rows.assign(static_cast<std::size_t>(sum/kRowsDivisor),0);fields.selected=0;
}
// 현재 프레임의 이전 범위를 먼저 정수화하고 camera를 빼서 다시 원본 ftol을 수행한다.
bool RawBurningFrame::Visible(const RenderProcessFields& registration) const {
    const auto left=FrameInteger(static_cast<float>(static_cast<double>(static_cast<float>(FrameInteger(registration.left)))-state_.cameraX));
    const auto top=FrameInteger(static_cast<float>(static_cast<double>(static_cast<float>(FrameInteger(registration.top)))-state_.cameraY));
    const auto right=FrameInteger(static_cast<float>(static_cast<double>(static_cast<float>(FrameInteger(registration.right)))-state_.cameraX));
    const auto bottom=FrameInteger(static_cast<float>(static_cast<double>(static_cast<float>(FrameInteger(registration.bottom)))-state_.cameraY));
    const auto margin=state_.insetViewport ? kInset : 0;
    const auto& view=state_.viewport;
    const auto viewLeft=static_cast<std::int64_t>(view.left)+margin,viewTop=static_cast<std::int64_t>(view.top)+margin;
    const auto viewRight=static_cast<std::int64_t>(view.right)-margin,viewBottom=static_cast<std::int64_t>(view.bottom)-margin;
    return viewLeft<=viewRight && viewTop<=viewBottom && left<=right && top<=bottom && viewLeft<right && left<viewRight && viewTop<bottom && top<viewBottom;
}
// SetRect는 양끝을1픽셀 확장하고 카메라를 뺀 넓은 값을 정수화한 뒤 viewport로 제한한다.
void RawBurningFrame::Invalidate(float left,float top,float right,float bottom) const {
    const auto& view=state_.viewport;
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078; // 패치와CD의x87 감산 순서를 구별한다.
    // 패치는 여백→카메라, CD는 카메라→여백 순서로 넓은 좌표를 계산한다.
    const auto coordinate=[patch](float value,float camera,double edge) {
        return FrameInteger(patch ? (static_cast<double>(value)+edge)-camera : (static_cast<double>(value)-camera)+edge);
    };
    SquidDisplayRect rect{std::clamp(coordinate(left,state_.cameraX,-1),view.left,view.right),
        std::clamp(coordinate(top,state_.cameraY,-1),view.top,view.bottom),
        std::clamp(coordinate(right,state_.cameraX,1),view.left,view.right),
        std::clamp(coordinate(bottom,state_.cameraY,1),view.top,view.bottom)};
    if (rect.left<rect.right && rect.top<rect.bottom) sink_.Invalidate(rect,0);
}
// capacity0・초기캡처・첫좌표・visible timer・source와 모든capacity입자의 실제 순서다.
void RawBurningFrame::Step(BurningVisualFields& fields,BurningFrameFields& frame) const {
    if (!fields.capacity) return;
    if (fields.particles.size()!=fields.capacity || fields.total>fields.capacity) throw std::invalid_argument("Burning frame 현재 입자 배열 오류");
    if (fields.stage==0) { fields.lastTime=state_.now;fields.elapsed=0;Capture(fields);++fields.stage;return; }
    if (!fields.victim.value || fields.victim.value==65535) return;
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto raw=pool_.Slot(fields.victim);
    if (!HasHitPoints(raw)) { if (patch) throw std::logic_error("Burning frame hasHitPoints 원본 assert");return; }
    const auto x=std::bit_cast<float>(FrameDword(raw,14)),y=std::bit_cast<float>(FrameDword(raw,18));
    const auto centerX=static_cast<float>(static_cast<double>(x)*state_.pixelScaleX),centerY=static_cast<float>(static_cast<double>(y)*state_.pixelScaleY);
    fields.registration.zOrder=static_cast<std::int8_t>(raw[patch ? 35U : 33U])-1;
    if (fields.stage==1) { fields.registration.left=centerX;fields.registration.top=centerY;fields.registration.right=centerX;fields.registration.bottom=centerY;fields.off=true;fields.stage=2;return; }
    if (!Visible(fields.registration)) {
        if (!fields.off && fields.sound) { if (!hooks_.stopSound) throw std::logic_error("Burning frame 실제 nonzero 소리 정지 미연결");hooks_.stopSound(fields.sound); }
        fields.off=true;return;
    }
    if (fields.off) { fields.lastTime=state_.now;fields.off=false; }
    const auto wide=(state_.now-fields.lastTime)*kTickScale;frame.delta=static_cast<float>(wide);frame.scaledDelta=static_cast<float>(wide*state_.deltaMultiplier);fields.lastTime=state_.now;fields.elapsed+=static_cast<double>(frame.delta);
    sources_.Select(fields);
    float left=centerX,top=centerY,right=centerX,bottom=centerY;
// 원본은 total이 아니라 capacity 전체를 검사하므로 새 후방입자를 같은프레임에 처리할 수 있다.
    for (std::size_t index=0;index<fields.capacity;++index) {
        auto& particle=fields.particles[index];const bool active=patch ? particle.life>0 : std::bit_cast<std::int32_t>(particle.life)>0;
        if (!active) continue;
        if (particle.source) {
            if (!(Random(0,kEmitMaximum)<frame.delta)) continue;
            const auto px=patch ? static_cast<float>((static_cast<double>(centerX)-fields.image.hotspotX)+particle.x) : static_cast<float>((static_cast<double>(particle.x)-fields.image.hotspotX)+centerX);
            const auto py=patch ? static_cast<float>((static_cast<double>(centerY)-fields.image.hotspotY)+particle.y) : static_cast<float>((static_cast<double>(particle.y)-fields.image.hotspotY)+centerY);
            const BurningParticle emitted{px,py,0,kRise,static_cast<float>(Random(kLifeMinimum,kLifeMaximum)),false};
            if (visual_.Append(fields,emitted)) IncludePoint(patch,emitted.x,emitted.y,left,top,right,bottom);
        } else {
            particle.life=static_cast<float>(static_cast<double>(particle.life)-frame.delta);
            const auto velocity=static_cast<double>(particle.velocityX)+state_.wind;particle.velocityX=static_cast<float>(velocity);
            particle.x=static_cast<float>((patch ? velocity : static_cast<double>(particle.velocityX))*frame.delta+particle.x);
            particle.y=static_cast<float>(static_cast<double>(particle.velocityY)*frame.delta+particle.y);
            IncludePoint(patch,particle.x,particle.y,left,top,right,bottom);
        }
    }
    right=static_cast<float>(static_cast<double>(right)+2);bottom=static_cast<float>(static_cast<double>(bottom)+1);
    const auto minimum=static_cast<float>(static_cast<double>(right)-17);left=patch ? (minimum<=left ? minimum : left) : (left<=minimum ? left : minimum);top=static_cast<float>(static_cast<double>(top)-2);
    Invalidate(fields.registration.left,fields.registration.top,fields.registration.right,fields.registration.bottom);Invalidate(left,top,right,bottom);
    fields.registration.left=left;fields.registration.top=top;fields.registration.right=right;fields.registration.bottom=bottom;
}
// 현재 피해·캡처·실제 효과 소비자가 같은 pool을 쓰는지 확인한다.
const SidPool& RawBurningFrame::Pool() const { return pool_; }
}
