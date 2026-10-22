// 원본의 float 저장 지점과 픽셀 색 주소만 offset+1로 정규화한다.
#include "o/RawDissolveFrame.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr double kRandomUnit=1.0/65536.0,kPoleY=36.3,kTimeScale=10.0; //원본 게임 난수/508c40/418430의 값이다.
constexpr float kPoleX=20.799999237060547F,kMaximumRandom=1.2999999523162842F,kVelocity=10.0F; //508c48과4500a0의 두 float 상수다.
// 원본 CRT의 signed64 절삭 후 하위 DWORD를 사용한다.
std::int32_t Truncate(double value) {
    constexpr double kLimit=9223372036854775808.0; //원본 _ftol의 불확정 반환 경계다.
    if (!std::isfinite(value) || value<-kLimit || value>=kLimit) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 현재 비정렬 float 비트 네 바이트를 읽는다.
float Float(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t word{};
    for (std::size_t index=0;index<4;++index) word|=std::uint32_t(raw[offset+index])<<(8*index); //주변 raw 필드는 포함하지 않는다.
    return std::bit_cast<float>(word);
}
// 메모리/픽셀을 만지기 전에 같은 원본 이미지와 실제 사용 꼬리 범위를 확인한다.
void Validate(const DissolveFrameFields& fields) {
    const auto& value=fields.common;const auto size=static_cast<std::int64_t>(value.image.width)*value.image.height;
    if (value.image.width<1 || value.image.height<1 || size>std::numeric_limits<std::int32_t>::max() || static_cast<std::size_t>(size)!=value.image.pixels.size() || value.total<0 || static_cast<std::size_t>(value.total)>value.particles.size())
        throw std::invalid_argument("Dissolve 캡처/입자 범위 오류");
}
}
// 빈 출력/삭제 효과로 정상 소진을 대신하지 않는다.
RawDissolveFrame::RawDissolveFrame(const SidPool& pool,GameRandom& random,const ShrapnelFrameState& state,ShrapnelFrameHooks hooks)
    :pool_(pool),random_(random),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || !hooks_.reserve || !hooks_.release || !hooks_.invalidate || !hooks_.pixel || !hooks_.removeForm)
        throw std::invalid_argument("Dissolve 판본/실제 출력 경계 미연결");
}
// 더미/폴은 하위 생성 이후 현재 슬롯을 다시 읽는다.
std::array<float,2> RawDissolveFrame::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {Float(raw,14),Float(raw,18)}; }
// 원본 float 끝점의 차이를 double로 계산하며 중간 float 저장은 호출자가 수행한다.
double RawDissolveFrame::Random() const { return static_cast<double>(random_.Next(65536))*kRandomUnit*(static_cast<double>(kMaximumRandom)-1.0)+1.0; }
// 최소 폭은 원본 max(1,half/2)가 보장한다.
std::int32_t RawDissolveFrame::Jitter(std::int32_t width) const { return static_cast<std::int32_t>(random_.Next(static_cast<std::uint32_t>(width*2)))-width; }
// 원점은 저장한 사각형 float가 아닌 원래 배율 곱에서 hotspot을 뺀다.
void RawDissolveFrame::Initialize(DissolveFrameFields& fields,Sid first,Sid second,std::int32_t xOriented) const {
    Validate(fields);auto& value=fields.common;const Sid parent{static_cast<std::uint16_t>(value.registration.parent)};
    const auto point=Position(parent);const double x=static_cast<double>(point[0])*state_.pixelScaleX,y=static_cast<double>(point[1])*state_.pixelScaleY;
    value.registration.zOrder=std::bit_cast<std::int8_t>(pool_.Slot(parent)[35]);value.registration.left=value.registration.right=static_cast<float>(x);value.registration.top=value.registration.bottom=static_cast<float>(y);
    Prepare(fields,Truncate(x-value.image.hotspotX),Truncate(y-value.image.hotspotY),first,second,xOriented);
}
// 같은 캡처의 불투명 픽셀마다 한 입자이며 열 바깥/행 안쪽 순서를 바꾸지 않는다.
void RawDissolveFrame::Prepare(DissolveFrameFields& fields,std::int32_t originX,std::int32_t originY,Sid first,Sid second,std::int32_t xOriented) const {
    Validate(fields);auto& value=fields.common;const auto a=Position(first),b=Position(second);
    const auto ax=static_cast<float>(static_cast<double>(a[0])*state_.pixelScaleX-kPoleX);
    const double ay=static_cast<double>(a[1])*state_.pixelScaleY-kPoleY;
    const double bx=static_cast<double>(static_cast<float>(static_cast<double>(b[0])*state_.pixelScaleX))-kPoleX;
    const auto by=static_cast<float>(static_cast<double>(static_cast<float>(static_cast<double>(b[1])*state_.pixelScaleY))-kPoleY);
    const auto gapA=static_cast<float>(xOriented ? static_cast<double>(originX)-ax : static_cast<double>(originY)-ay);
    const auto gapB=static_cast<float>(xOriented ? bx-originX : static_cast<double>(by)-originY);
    const auto transverse=Truncate(xOriented ? ay-originY : static_cast<double>(ax)-originX);
    fields.opaqueCount=static_cast<std::int32_t>(std::count_if(value.image.pixels.begin(),value.image.pixels.end(),[](auto color) { return color!=255; }));
    const auto capacity=fields.opaqueCount;if (fields.particlesAllocated) hooks_.release(false);
    if (!hooks_.reserve(static_cast<std::uint32_t>(capacity)*24U)) throw std::bad_alloc();value.particles.assign(static_cast<std::size_t>(capacity),ShrapnelParticle{});value.total=0;fields.particlesAllocated=true;
    const auto halfX=value.image.width/2,halfY=value.image.height/2;const auto jitterX=std::max(1,halfX/2),jitterY=std::max(1,halfY/2);
    for (std::int32_t x=0;x<value.image.width;++x) { //원본은 열 단위로 게임 난수를 소비한다.
        for (std::int32_t y=0;y<value.image.height;++y) { //투명색은 난수를 소비하지 않는다.
            const auto pixel=static_cast<std::size_t>(y*value.image.width+x);if (value.image.pixels[pixel]==255) continue;
            const auto noise=Random();float vx{},vy{},life{};
            if (xOriented) {
                const auto speed=static_cast<float>((static_cast<double>(std::abs(x-halfX))/halfX+noise)*kVelocity);
                if (Jitter(jitterX)+x<halfX) { vx=-speed;life=static_cast<float>((static_cast<double>(x)+gapA)/speed); }
                else { vx=speed;life=static_cast<float>(((static_cast<double>(value.image.width-x)+gapB)-value.image.width)/speed); }
                vy=static_cast<float>(static_cast<double>(transverse-y)/life);
            } else {
                const auto speed=static_cast<float>((static_cast<double>(std::abs(y-halfY))/halfY+noise)*kVelocity);
                if (Jitter(jitterY)+y<halfY) { vy=-speed;life=static_cast<float>((static_cast<double>(y)+gapA)/speed); }
                else { vy=speed;life=static_cast<float>(((static_cast<double>(value.image.height-y)+gapB)-value.image.height)/speed); }
                vx=static_cast<float>(static_cast<double>(transverse-x)/life);
            }
            value.particles[static_cast<std::size_t>(value.total++)]={static_cast<float>(originX+x),static_cast<float>(originY+y),vx,vy,life,static_cast<std::uint32_t>(pixel+1)};
        }
    }
}
// 최초 Frame에서는 생성 시각과 무관하게 timer를 현재 시각으로 초기화하여 delta0을 적용한다.
void RawDissolveFrame::Step(DissolveFrameFields& fields) const {
    Validate(fields);auto& value=fields.common;const auto parent=Position(Sid{static_cast<std::uint16_t>(value.registration.parent)});
    float left=static_cast<float>(static_cast<double>(parent[0])*state_.pixelScaleX),top=static_cast<float>(static_cast<double>(parent[1])*state_.pixelScaleY),right=left,bottom=top;
    if (!fields.clockStarted) { value.lastTime=state_.now;value.elapsed=0;fields.clockStarted=true; }
    const auto wide=(state_.now-value.lastTime)*kTimeScale;value.delta=static_cast<float>(wide);value.scaledDelta=static_cast<float>(wide*state_.rate);value.lastTime=state_.now;value.elapsed+=value.delta;bool alive{};
    for (auto& particle:value.particles) { //원본 Frame은 사용 꼬리가 아닌 용량 전체를 검사한다.
        if (!(particle.life>0)) continue;alive=true;particle.life=static_cast<float>(static_cast<double>(particle.life)-value.delta);
        particle.x=static_cast<float>(static_cast<double>(particle.velocityX)*value.scaledDelta+particle.x);const auto nextY=static_cast<double>(particle.velocityY)*value.scaledDelta+particle.y;particle.y=static_cast<float>(nextY);
        const auto x=static_cast<float>(Truncate(particle.x)),y=static_cast<float>(Truncate(nextY));left=std::min(left,x);top=std::min(top,y);right=std::max(right,static_cast<float>(static_cast<double>(x)+1));bottom=std::max(bottom,static_cast<float>(static_cast<double>(y)+1));
    }
    if (!alive) { hooks_.removeForm();return; }
    auto& rect=value.registration;Invalidate(rect.left,rect.top,rect.right,rect.bottom);Invalidate(left,top,right,bottom);rect.left=left;rect.top=top;rect.right=right;rect.bottom=bottom;
}
// 공통 dirty는 먼저 이전 영역, 이어 새 영역을 같은 카메라로 변환한다.
void RawDissolveFrame::Invalidate(float left,float top,float right,float bottom) const {
    hooks_.invalidate({Truncate((static_cast<double>(left)-1)-state_.cameraX),Truncate((static_cast<double>(top)-1)-state_.cameraY),Truncate((static_cast<double>(right)+1)-state_.cameraX),Truncate((static_cast<double>(bottom)+1)-state_.cameraY)});
}
// Draw는 원본 현재 수명/현재 bitmap 색과 x만 중간 float에 저장하는 순서를 유지한다.
void RawDissolveFrame::Draw(const DissolveFrameFields& fields) const {
    Validate(fields);const auto& value=fields.common;
    for (std::int32_t index=0;index<value.total;++index) { //각 살아 있는 원본 픽셀의 정수 요청이다.
        const auto& particle=value.particles[static_cast<std::size_t>(index)];if (!(particle.life>0)) continue;
        if (!particle.pixel || particle.pixel>value.image.pixels.size()) throw std::out_of_range("Dissolve 색 주소 토큰 오류");
        const auto x=static_cast<float>(static_cast<double>(Truncate(particle.x))-state_.cameraX);const auto y=static_cast<double>(Truncate(particle.y))-state_.cameraY;
        hooks_.pixel({Truncate(x),Truncate(y),value.image.pixels[particle.pixel-1]});
    }
}
}
