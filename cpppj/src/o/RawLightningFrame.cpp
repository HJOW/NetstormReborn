// 원본의 저장 폭·전체 용량 갱신·굵기 선 순서를 유지한다.
#include "o/RawLightningFrame.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제 타이머 최대 조각/시간 배율과 공통 난수의 간격이다.
constexpr double kMaximumStep=.5*.1,kDeltaScale=10,kRandomUnit=1.0/65536.0;
// 원본 Draw가 직접 전달하는 중심/보조선 색이다. 공유 팔레트를 대신 조회하지 않는다.
constexpr std::uint32_t kMainColor=195,kSideColor=85;
// 원본 CRT처럼 유한한 signed32 범위만0방향으로 절삭한다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("빔 표시 정수화 범위 오류");
    return static_cast<std::int32_t>(value);
}
// 비정렬 raw float를 실제 little endian DWORD로 읽는다.
float Float(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t result=0;
    // 원본 네 바이트의 위치를 유지한다.
    for (std::size_t index=0;index<4;++index) result|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return std::bit_cast<float>(result);
}
// 실제004239f0의 오른쪽/아래쪽1픽셀과 ordered 비교를 보존한다.
void Include(float x,float y,float& left,float& top,float& right,float& bottom) {
    left=left<x ? left : x;top=top<y ? top : y;
    const auto nextRight=static_cast<float>(static_cast<double>(x)+1),nextBottom=static_cast<float>(static_cast<double>(y)+1);
    right=nextRight<right ? right : nextRight;if (bottom<=nextBottom) bottom=nextBottom;
}
// 잘못된 저장소/꼬리/첫 위치는 상태나 출력을 바꾸기 전에 거부한다.
void Validate(const LightningFrameFields& fields) {
    if (fields.particles.values.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) || fields.particles.total<0 ||
        static_cast<std::size_t>(fields.particles.total)>fields.particles.values.size() || fields.firstLive<0 ||
        static_cast<std::size_t>(fields.firstLive)>fields.particles.values.size()) throw std::invalid_argument("빔 입자 저장소/꼬리 오류");
}
}
// 빈 성공 경계를 허용하지 않고 현재10.78 본체만 선택한다.
RawLightningFrame::RawLightningFrame(const SidPool& pool,GameRandom& random,const RawLightningPath& path,
    const LightningFrameState& state,LightningFrameHooks hooks):pool_(pool),random_(random),path_(path),state_(state),hooks_(std::move(hooks)) {
    if (pool_.Edition()!=OriginalEdition::Patch1078 || !hooks_.invalidate || !hooks_.line || !hooks_.sound || !hooks_.removeForm)
        throw std::invalid_argument("빔 원본/출력 경계 미연결");
}
// 검사에는 넓은 시간 차이, 다음 연산에는 먼저 저장한 float delta를 사용한다.
bool RawLightningFrame::Clock(LightningFrameFields& fields) const {
    const double difference=state_.now-fields.lastTime;fields.delta=static_cast<float>(difference);const bool finished=!(difference>kMaximumStep);
    if (!finished) { fields.delta=static_cast<float>(kMaximumStep);fields.lastTime+=kMaximumStep; }
    else fields.lastTime=state_.now;
    const double wide=static_cast<double>(fields.delta)*kDeltaScale;
    fields.delta=static_cast<float>(wide);fields.scaledDelta=static_cast<float>(wide*state_.rate);fields.elapsed+=static_cast<double>(fields.delta);return finished;
}
// 넓은 간격 계산은 임의 float 중간 저장을 추가하지 않는다.
double RawLightningFrame::Random(float low,float high) const { return static_cast<double>(random_.Next(65536))*kRandomUnit*std::abs(static_cast<double>(high)-low)+low; }
// 최초에는 delta/흔들림/dirty 없이 Seed와 선택 파일명 소리만 수행한다.
void RawLightningFrame::Step(LightningFrameFields& fields) const {
    Validate(fields);
    if (fields.registration.parent<0 || fields.registration.parent>65535) throw std::out_of_range("빔 부모 SID 범위 오류");
    const auto raw=pool_.Slot(Sid{static_cast<std::uint16_t>(fields.registration.parent)});
    if (!raw[6] && !raw[7]) throw std::logic_error("빔 부모 head 원본 assert");
    const auto rawX=Float(raw,14),rawY=Float(raw,18);const auto x=static_cast<float>(static_cast<double>(rawX)*state_.pixelScaleX),y=static_cast<float>(static_cast<double>(rawY)*state_.pixelScaleY);
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(state_.now) || !std::isfinite(fields.lastTime) || !std::isfinite(fields.jitter)) throw std::invalid_argument("빔 좌표/시각/흔들림 오류");
    auto& rect=fields.registration;
    if (!fields.first) {
        fields.lastTime=state_.now;fields.elapsed=0;rect.left=rect.right=x;rect.top=rect.bottom=y;fields.first=true;path_.Seed(fields.path,fields.particles);
        if (fields.sound) hooks_.sound(rawX,rawY,*fields.sound);
        return;
    }
    float left=x,top=y,right=x,bottom=y;bool finished;
    // 영역은 모든 시간 조각에 걸쳐 누적한다. 시간 후퇴와0간격도 원문 그대로 처리한다.
    do {
        finished=Clock(fields);path_.Step(fields.path,fields.particles,static_cast<float>(fields.elapsed),fields.delta);
        fields.firstLive=static_cast<std::int32_t>(fields.particles.values.size());
        // 사용 꼬리가 아닌 용량 전체에서 죽은/미사용 입자의 수명까지 줄인다.
        for (std::size_t index=0;index<fields.particles.values.size();++index) {
            auto& particle=fields.particles.values[index];const double remaining=static_cast<double>(particle.life)-fields.delta;particle.life=static_cast<float>(remaining);
            if (!(remaining>0)) continue;
            particle.x=static_cast<float>(((Random(-fields.jitter,fields.jitter)*particle.jitterWeight)*fields.scaledDelta)+particle.x);
            particle.y=static_cast<float>(((Random(-fields.jitter,fields.jitter)*particle.jitterWeight)*fields.scaledDelta)+particle.y);
            Include(particle.x,particle.y,left,top,right,bottom);fields.firstLive=std::min(fields.firstLive,static_cast<std::int32_t>(index));
        }
        if (static_cast<std::size_t>(fields.firstLive)>=fields.particles.values.size()) { hooks_.removeForm();return; }
        Invalidate(rect.left,rect.top,rect.right,rect.bottom);Invalidate(left,top,right,bottom);rect.left=left;rect.top=top;rect.right=right;rect.bottom=bottom;
    } while (!finished);
}
// 실제487ba0의 카메라/여유는 중간 float 저장 없이 __ftol에 들어간다.
void RawLightningFrame::Invalidate(float left,float top,float right,float bottom) const {
    hooks_.invalidate({Ftol((static_cast<double>(left)-1)-state_.cameraX),Ftol((static_cast<double>(top)-1)-state_.cameraY),
        Ftol((static_cast<double>(right)+1)-state_.cameraX),Ftol((static_cast<double>(bottom)+1)-state_.cameraY)});
}
// 카메라 차이를 float로 먼저 저장하며 보조선의+1/-1은 그 값에서 넓게 계산한다.
void RawLightningFrame::Draw(const LightningFrameFields& fields) const {
    Validate(fields);if (static_cast<std::size_t>(fields.firstLive)>=fields.particles.values.size()) return;
    // 첫 생존점 다음부터 실제 사용 꼬리까지만 선을 만든다. 이전 점의 생존 여부는 검사하지 않는다.
    for (std::int32_t index=fields.firstLive+1;index<fields.particles.total;++index) {
        const auto& particle=fields.particles.values[static_cast<std::size_t>(index)];if (!(particle.life>0)) continue;
        if (!particle.previous || particle.previous>fields.particles.values.size()) throw std::out_of_range("빔 이전 점 토큰 오류");
        const auto& previous=fields.particles.values[particle.previous-1];
        const auto x=static_cast<float>(static_cast<double>(particle.x)-state_.cameraX),y=static_cast<float>(static_cast<double>(particle.y)-state_.cameraY);
        const auto px=static_cast<float>(static_cast<double>(previous.x)-state_.cameraX),py=static_cast<float>(static_cast<double>(previous.y)-state_.cameraY);
        const auto ix=Ftol(x),iy=Ftol(y),ipx=Ftol(px),ipy=Ftol(py);hooks_.line({ipx,ipy,ix,iy,kMainColor});
        if (particle.strength>=.25F) {
            hooks_.line({Ftol(static_cast<double>(px)+1),ipy,Ftol(static_cast<double>(x)+1),iy,kSideColor});
            if (particle.strength>.5F) hooks_.line({ipx,Ftol(static_cast<double>(py)-1),ix,Ftol(static_cast<double>(y)-1),kSideColor});
        }
    }
}
}
