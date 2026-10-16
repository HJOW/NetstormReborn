// 실제53비트 x87 저장 순서와 원본 방출/시간 조각/선 요청 순서를 보존한다.
#include "o/RawFieryTailFrame.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제 timer의 최대 간격0.5×0.1과 delta 배수, 입자 영역 여백, float 난수 단위다.
constexpr double kMaximumStep=.5*.1,kDeltaScale=10,kMargin=10,kRandomUnit=1.0/65536.0;
// 실제 최초/시간 조각당 방출 수와 진단2가 쓰는 두 상수다.
constexpr std::int32_t kInitialParticles=10,kStepParticles=2,kDiagnosticColors=0x17655,kDiagnosticLife=0x315a;
// 정상 유한 좌표만 원본 CRT처럼0 방향으로 정수화한다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("Fiery Tail 정수화 범위 오류");
    return static_cast<std::int32_t>(value);
}
// 원본 비정렬 raw DWORD를 읽는다.
std::uint32_t Dword(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t result=0;
    // 네 바이트의 little endian 위치를 보존한다.
    for (std::size_t index=0;index<4;++index) result|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return result;
}
// signed 덧셈의 원본32비트 감김을 호스트 UB 없이 보존한다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b)); }
// 실제004239f0의 배타적 오른쪽/아래쪽 경계와 ordered 비교를 보존한다.
void Include(float x,float y,float& left,float& top,float& right,float& bottom) {
    left=left<x ? left : x;top=top<y ? top : y;
    const auto nextRight=static_cast<float>(static_cast<double>(x)+1),nextBottom=static_cast<float>(static_cast<double>(y)+1);
    right=nextRight<right ? right : nextRight;if (bottom<=nextBottom) bottom=nextBottom;
}
// 손상된 사용 수와 호스트 크기 넘침은 상태 변경 전에 거부한다.
void Validate(const FieryTailFrameFields& fields) {
    if (fields.particles.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) || fields.total<0 ||
        static_cast<std::size_t>(fields.total)>fields.particles.size()) throw std::invalid_argument("Fiery Tail 현재 입자 수 오류");
}
}
// 외부 출력 경계를 확인하고 원본10.78 몸체만 선택한다.
RawFieryTailFrame::RawFieryTailFrame(const SidPool& pool,GameRandom& random,const FieryTailFrameState& state,
    FieryTailFramePalette& palette,FieryTailFrameHooks hooks):pool_(pool),random_(random),state_(state),palette_(palette),hooks_(std::move(hooks)) {
    if (pool_.Edition()!=OriginalEdition::Patch1078 || !hooks_.invalidate || !hooks_.line) throw std::invalid_argument("Fiery Tail 판본/출력 경계 미연결");
}
// PATCH00487b40은 꼬리를 먼저 쓰고 꽉 찬 뒤에만 죽은 슬롯을 앞에서 찾는다.
FieryTailParticle* RawFieryTailFrame::Append(FieryTailFrameFields& fields) const {
    if (static_cast<std::size_t>(fields.total)<fields.particles.size()) return &fields.particles[static_cast<std::size_t>(fields.total++)];
    // NaN과 양수 수명은 건너뛰고0/음수 수명만 재사용한다.
    for (auto& particle:fields.particles) if (particle.life<=0) return &particle;
    return nullptr;
}
// 난수는 슬롯이 없을 때도 두 번 전진한다. 최초 방출은 offset을 더하지 않는다.
void RawFieryTailFrame::Emit(FieryTailFrameFields& fields,float x,float y,bool initial) const {
    const double noiseY=static_cast<double>(random_.Next(65536))*kRandomUnit*6-3;
    const double noiseX=static_cast<double>(random_.Next(65536))*kRandomUnit*6-3;
    const double offset=initial ? 0 : fields.offset;
    if (auto* particle=Append(fields)) {
        particle->x=static_cast<float>((noiseX+offset)+x);particle->y=static_cast<float>((noiseY+offset)+y);
        particle->life=static_cast<float>(fields.maximumLife);
    }
}
// 원본은 시간 차이 float를 먼저 저장한 뒤 넓은 값과 한도를 비교한다.
bool RawFieryTailFrame::Clock(FieryTailFrameFields& fields) const {
    const double difference=state_.now-fields.lastTime;
    fields.delta=static_cast<float>(difference);const bool finished=!(difference>kMaximumStep);
    if (!finished) { fields.delta=static_cast<float>(kMaximumStep);fields.lastTime+=kMaximumStep; }
    else fields.lastTime=state_.now;
    const double wide=static_cast<double>(fields.delta)*kDeltaScale;
    fields.delta=static_cast<float>(wide);fields.scaledDelta=static_cast<float>(wide*state_.rate);
    fields.elapsed+=static_cast<double>(fields.delta);return finished;
}
// 첫 프레임은 영역 설정과10입자만 수행하며 dirty 제출과 수명 감소가 없다.
void RawFieryTailFrame::Step(FieryTailFrameFields& fields) const {
    Validate(fields);
    if (fields.registration.parent<0 || fields.registration.parent>65535) throw std::out_of_range("Fiery Tail 부모 SID 범위 오류");
    const auto raw=pool_.Slot(Sid{static_cast<std::uint16_t>(fields.registration.parent)});
    if (!raw[6] && !raw[7]) throw std::logic_error("Fiery Tail 부모 head 원본 assert");
    const auto x=static_cast<float>(static_cast<double>(std::bit_cast<float>(Dword(raw,14)))*state_.pixelScaleX);
    const auto y=static_cast<float>(static_cast<double>(std::bit_cast<float>(Dword(raw,18)))*state_.pixelScaleY);
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(state_.now) || !std::isfinite(fields.lastTime)) throw std::invalid_argument("Fiery Tail 현재 좌표/시각 오류");
    auto& rect=fields.registration;
    if (!fields.first) {
        fields.lastTime=state_.now;fields.elapsed=0;rect.left=rect.right=x;rect.top=rect.bottom=y;
        fields.previousX=x;fields.previousY=y;fields.first=true;
        // 실제 최초 방출 횟수이며 capacity가 작아도 모든 난수를 소비한다.
        for (std::int32_t index=0;index<kInitialParticles;++index) Emit(fields,x,y,true);
        return;
    }
    float left=x,top=y,right=x,bottom=y;bool finished;
    // 표시 영역은 시간 조각 사이에 초기화하지 않고 누적한다.
    do {
        finished=Clock(fields);
        // 각 시간 조각에서 먼저 두 입자를 생성한다.
        for (std::int32_t index=0;index<kStepParticles;++index) Emit(fields,x,y,false);
        // 실제 사용 수가 아닌 용량 전체를 검사한다. 이번 조각에서 죽어도 영역에는 포함한다.
        for (auto& particle:fields.particles) {
            if (!(particle.life>0)) continue;
            particle.life=static_cast<float>(static_cast<double>(particle.life)-fields.delta);
            Include(static_cast<float>(static_cast<double>(particle.x)-kMargin),static_cast<float>(static_cast<double>(particle.y)-kMargin),left,top,right,bottom);
            Include(static_cast<float>(static_cast<double>(particle.x)+kMargin),static_cast<float>(static_cast<double>(particle.y)+kMargin),left,top,right,bottom);
        }
        Invalidate(rect.left,rect.top,rect.right,rect.bottom);Invalidate(left,top,right,bottom);
        rect.left=left;rect.top=top;rect.right=right;rect.bottom=bottom;fields.previousX=x;fields.previousY=y;
    } while (!finished);
}
// 실제00487ba0이00499310으로 넘기는 다섯 인자 중 flags는 항상0이다.
void RawFieryTailFrame::Invalidate(float left,float top,float right,float bottom) const {
    hooks_.invalidate({Ftol((static_cast<double>(left)-1)-state_.cameraX),Ftol((static_cast<double>(top)-1)-state_.cameraY),
        Ftol((static_cast<double>(right)+1)-state_.cameraX),Ftol((static_cast<double>(bottom)+1)-state_.cameraY)});
}
// 색수 float 저장과 역수 float 저장 뒤의 두 넓은 곱을 보존한다.
std::uint32_t RawFieryTailFrame::Color(float life,float maximum) const {
    if (!std::isfinite(maximum) || maximum<=0 || palette_.count<0) throw std::invalid_argument("Fiery Tail 색 표 분모/개수 오류");
    const auto reciprocal=static_cast<float>(1.0/static_cast<double>(maximum));
    const auto index=std::max(0,Ftol(static_cast<double>(static_cast<float>(palette_.count))*std::min(life,maximum)*reciprocal));
    if (static_cast<std::size_t>(index)>=palette_.readable.size()) throw std::out_of_range("Fiery Tail 색 표 인접 DWORD 미공급");
    return palette_.readable[static_cast<std::size_t>(index)];
}
// 실제 선은 입자 화면 좌표에서 호출자가 전달한 부모 화면 끝점+offset까지 그린다.
void RawFieryTailFrame::Draw(FieryTailFrameFields& fields,std::int32_t endpointX,std::int32_t endpointY) const {
    Validate(fields);endpointX=Add(endpointX,fields.offset);endpointY=Add(endpointY,fields.offset);
    if (state_.diagnostic==2) { palette_.count=kDiagnosticColors;fields.maximumLife=kDiagnosticLife; }
    // 원본 Draw는 용량 대신 사용 수까지만 순회하고 양수 수명만 선택한다.
    for (std::int32_t index=0;index<fields.total;++index) {
        const auto& particle=fields.particles[static_cast<std::size_t>(index)];if (!(particle.life>0)) continue;
        const auto color=Color(particle.life,static_cast<float>(fields.maximumLife));
        hooks_.line({Ftol(static_cast<double>(particle.x)-state_.cameraX),Ftol(static_cast<double>(particle.y)-state_.cameraY),endpointX,endpointY,color});
    }
}
}
