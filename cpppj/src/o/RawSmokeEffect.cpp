// 원본의 전체 경과 시간·동적 입자 꼬리·난수 소비와 float 저장 위치를 보존한다.
#include "o/RawSmokeEffect.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 원본 공용 입자/표시 몸체만 구현한다.
namespace { // 원문32비트 필드/정수화의 작은 공통 연산이다.
constexpr std::uint32_t kSmokeType=21,kParticleCapacity=200,kParticleBytes=24; // 실제005410c4와 생성자 요청이다.
// 정상 좌표의 원본 __ftol 하위 DWORD를 보존하고 비유한/64비트 넘침은0이다.
std::int32_t SmokeInteger(double value) {
    if (!std::isfinite(value) || value>=9223372036854775808.0 || value< -9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 원본 비정렬 float 좌표를 little endian으로 읽는다.
float SmokeCoordinate(std::span<const std::uint8_t> raw,std::size_t at) {
    std::uint32_t value=0;
    // 지정한 네 바이트만 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[at+i])<<(i*8);
    return std::bit_cast<float>(value);
}
// signed 덧셈을 원본32비트 감김으로 처리한다.
std::int32_t SmokeAdd(std::int32_t value,std::int32_t add) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)+static_cast<std::uint32_t>(add)); }
}
// 실제 연속 팔레트 번호12개이며 색수 다음 DWORD를 임의로 덧붙이지 않는다.
std::array<std::uint32_t,12> BuildSmokePalette() {
    std::array<std::uint32_t,12> colors{};
    //00487cd0의 양끝 포함 오름차순 순서다.
    for (std::size_t i=0;i<colors.size();++i) colors[i]=77+static_cast<std::uint32_t>(i);
    return colors;
}
// 다른 판본/월드나 실제 확보 경계 누락은 등록 전에 거부한다.
RawSmokeLifetime::RawSmokeLifetime(const SidPool& pool,RawRenderRegistry& registry,SmokeLifetimeState& state,SmokeLifetimeHooks hooks)
    :registry_(registry),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || &pool!=&registry.Pool() || !hooks_.reserve) throw std::invalid_argument("냉각 연기 실제 월드/확보 경계 오류");
}
// 생성자가 쓰지 않는 delta/scaledDelta는 기존 입력으로 보존한다.
void RawSmokeLifetime::Initialize(SmokeFields& fields,std::uint32_t token,const SmokeRequest& request) const {
    registry_.Construct(fields.registration,token,request.parent,kSmokeType);fields.capacity=fields.total=0;fields.hasStorage=false;fields.particles.clear();fields.lastTime=fields.elapsed=0;fields.first=false;
    fields.maximumLife=request.maximumLife;fields.noise=request.noise;fields.spread=request.spread;fields.hotspotY=request.hotspotY;fields.velocity=request.velocity;fields.hotspotX=request.hotspotX;fields.period=request.period;fields.registration.zOrder=SmokeAdd(fields.registration.zOrder,-1);
    if (!hooks_.reserve(kParticleCapacity*kParticleBytes)) throw std::bad_alloc();
    fields.particles.assign(kParticleCapacity,FieryTailParticle{});fields.capacity=kParticleCapacity;fields.hasStorage=true;fields.due=0;fields.windX=request.windX;fields.windY=request.windY;fields.remaining=8;
    if (!state_.palette) {
        if (!hooks_.createPalette || !hooks_.buildPalette) throw std::logic_error("냉각 연기 첫 공유 색 표 미연결");
        if (!hooks_.reserve(8)) throw std::bad_alloc();state_.palette=hooks_.createPalette();if (!state_.palette) throw std::bad_alloc();hooks_.buildPalette(state_.palette);
    }
}
// 원본 wrapper는 기본 생성/공유 표 초기화가 끝나야 두 값을 바꾼다.
void RawSmokeLifetime::InitializeCooling(SmokeFields& fields,std::uint32_t token,Sid parent,std::array<std::int32_t,2> hotspot) const {
    SmokeRequest request;request.parent=parent;request.hotspotX=hotspot[0];request.hotspotY=hotspot[1];Initialize(fields,token,request);fields.remaining=10;fields.period=3;
}
// 공통 소멸은 숫자 카운터를 다시 쓰지 않는다. 실제 vector만 반납한다.
void RawSmokeLifetime::Destroy(SmokeFields& fields,std::uint32_t flags,const std::function<void(SmokeRelease)>& release) const {
    if (!release) throw std::invalid_argument("냉각 연기 실제 반납 경계 미연결");
    if (fields.hasStorage) { release(SmokeRelease::Particles);std::vector<FieryTailParticle>().swap(fields.particles);fields.hasStorage=false; }
    registry_.Destroy(fields.registration);if (flags&1U) release(SmokeRelease::Object);
}
// 출력/삭제가 실제 같은 세계에 연결되었는지 생성 시 확인한다.
RawSmokeFrame::RawSmokeFrame(const SidPool& pool,GameRandom& random,const FieryTailFrameState& state,FieryTailFramePalette& palette,SmokeFrameHooks hooks)
    :pool_(pool),random_(random),state_(state),palette_(palette),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || !hooks_.invalidate || !hooks_.line || !hooks_.kill) throw std::invalid_argument("냉각 연기 판본/실제 출력·삭제 미연결");
}
// 손상된 숫자/소유 배열은 변경 전에 거부한다.
void RawSmokeFrame::Validate(const SmokeFields& fields) const {
    if (fields.capacity<0 || fields.total<0 || fields.total>fields.capacity || static_cast<std::size_t>(fields.capacity)!=fields.particles.size()) throw std::out_of_range("냉각 연기 현재 입자 용량/꼬리 오류");
}
// capacity 꼬리를 먼저 사용하고 가득 차면 사용한 범위의 죽은 슬롯만 찾는다.
FieryTailParticle* RawSmokeFrame::Append(SmokeFields& fields) const {
    if (fields.total<fields.capacity) return &fields.particles[static_cast<std::size_t>(fields.total++)];
    //0/음수 수명만 재사용한다. NaN/양수 수명은 유지한다.
    for (std::int32_t i=0;i<fields.total;++i) if (fields.particles[static_cast<std::size_t>(i)].life<=0) return &fields.particles[static_cast<std::size_t>(i)];
    return nullptr;
}
// 원본00455930의 실제 상위16비트와 넓은 중간값이다.
double RawSmokeFrame::Random(float low,float high) const { return static_cast<double>(random_.Next(65536))*(1.0/65536.0)*std::abs(static_cast<double>(high)-low)+low; }
// 실패한 Append는 난수/기존 입자를 바꾸지 않는다. 마지막 spare DWORD도 유지한다.
void RawSmokeFrame::Emit(SmokeFields& fields,float x,float y,float life) const {
    if (!fields.hasStorage) return;Validate(fields);
    if (auto* particle=Append(fields)) {
        particle->velocityX=static_cast<float>(Random(-fields.noise,fields.noise)+fields.velocity);particle->velocityY=static_cast<float>(Random(-fields.noise,fields.noise)+fields.velocity);particle->x=x;particle->y=y;particle->life=life;
    }
}
// 같은 공통 dirty helper의 여유/카메라 계산을 사용한다.
void RawSmokeFrame::Invalidate(float left,float top,float right,float bottom) const {
    hooks_.invalidate({SmokeInteger((static_cast<double>(left)-1)-state_.cameraX),SmokeInteger((static_cast<double>(top)-1)-state_.cameraY),SmokeInteger((static_cast<double>(right)+1)-state_.cameraX),SmokeInteger((static_cast<double>(bottom)+1)-state_.cameraY)});
}
// 실제 단일 프레임의 전체 delta를 사용하며 별도 시간 조각 반복을 추가하지 않는다.
void RawSmokeFrame::Step(SmokeFields& fields) const {
    if (!fields.hasStorage) return;Validate(fields);
    if (fields.registration.parent<0 || static_cast<std::uint32_t>(fields.registration.parent)>=pool_.Capacity()) throw std::out_of_range("냉각 연기 현재 부모 SID 오류");
    const auto raw=pool_.Slot(Sid{static_cast<std::uint16_t>(fields.registration.parent)});const float parentX=static_cast<float>(static_cast<double>(SmokeCoordinate(raw,14))*state_.pixelScaleX),parentY=static_cast<float>(static_cast<double>(SmokeCoordinate(raw,18))*state_.pixelScaleY);
    if (!fields.first) { fields.lastTime=state_.now;fields.elapsed=0;fields.registration.left=fields.registration.right=parentX;fields.registration.top=fields.registration.bottom=parentY;fields.first=true;return; }
    const double difference=(state_.now-fields.lastTime)*10.0;fields.delta=static_cast<float>(difference);fields.scaledDelta=static_cast<float>(difference*state_.rate);fields.lastTime=state_.now;fields.elapsed+=static_cast<double>(fields.delta); //00418454의 FADD QWORD는 기존 누적 시각을 float로 줄이지 않는다.
    if (fields.remaining>0 || (fields.remaining==-1 && fields.due<=fields.elapsed)) {
        const float spread=static_cast<float>(static_cast<double>(fields.spread)*.5);const float y=static_cast<float>(Random(-spread,spread)+fields.hotspotY+parentY);const float x=static_cast<float>(Random(-spread,spread)+fields.hotspotX+parentX);Emit(fields,x,y,fields.maximumLife);
        const double elapsed=fields.elapsed;const auto noise=Random(0,static_cast<float>(static_cast<double>(fields.period)*.25));fields.due=static_cast<float>(static_cast<double>(fields.period)*.75+noise+elapsed);if (fields.remaining!=-1) fields.remaining=SmokeAdd(fields.remaining,-1);
    }
    bool active=false;float left=parentX,top=parentY,right=parentX,bottom=parentY;
    // total은 분열 중 늘 수 있다. 새 꼬리 입자도 같은 프레임에서 순서대로 처리한다.
    for (std::int32_t i=0;i<fields.total;++i) {
        auto& particle=fields.particles[static_cast<std::size_t>(i)];if (!(particle.life>0)) continue;particle.life=static_cast<float>(static_cast<double>(particle.life)-fields.delta);
        if (particle.life>2 && Random(0,3)<fields.delta) { Emit(fields,particle.x,particle.y,particle.life);particle.life=static_cast<float>(static_cast<double>(particle.life)*.8); }
        particle.x=static_cast<float>((static_cast<double>(particle.velocityX)+fields.windX)*fields.scaledDelta+particle.x);particle.y=static_cast<float>((static_cast<double>(particle.velocityY)+fields.windY)*fields.scaledDelta+particle.y);
        const auto x=SmokeInteger(particle.x),y=SmokeInteger(particle.y);const float lowX=static_cast<float>(SmokeAdd(x,-1)),lowY=static_cast<float>(SmokeAdd(y,-1)),highX=static_cast<float>(SmokeAdd(x,1)),highY=static_cast<float>(SmokeAdd(y,1));
        if (active) { left=std::min(left,lowX);top=std::min(top,lowY);right=std::max(right,highX);bottom=std::max(bottom,highY); }else { left=lowX;top=lowY;right=highX;bottom=highY; }active=true;
    }
    if (!active) { hooks_.kill(0);return; }
    auto& rectangle=fields.registration;Invalidate(rectangle.left,rectangle.top,rectangle.right,rectangle.bottom);Invalidate(left,top,right,bottom);rectangle.left=left;rectangle.top=top;rectangle.right=right;rectangle.bottom=bottom;
}
// 색수 다음 DWORD도 실제 공급된 span 안에서만 읽는다.
std::uint32_t RawSmokeFrame::Color(float life,float maximum) const {
    if (!std::isfinite(maximum) || maximum<=0 || palette_.count<0) throw std::invalid_argument("냉각 연기 색 표 분모/개수 오류");
    const float reciprocal=static_cast<float>(1.0/static_cast<double>(maximum));const auto index=std::max(0,SmokeInteger(static_cast<double>(static_cast<float>(palette_.count))*std::min(life,maximum)*reciprocal));
    if (static_cast<std::size_t>(index)>=palette_.readable.size()) throw std::out_of_range("냉각 연기 색 표 인접 DWORD 미공급");return palette_.readable[static_cast<std::size_t>(index)];
}
// 원문은 좌표를 먼저 정수화한 뒤 카메라/가로1픽셀을 적용한다.
void RawSmokeFrame::Draw(const SmokeFields& fields) const {
    if (!fields.hasStorage) return;Validate(fields);
    // 사용한 꼬리 범위의 현재 양수 수명만 제출한다.
    for (std::int32_t i=0;i<fields.total;++i) {
        const auto& particle=fields.particles[static_cast<std::size_t>(i)];if (!(particle.life>0)) continue;
        const float x=static_cast<float>(static_cast<double>(SmokeInteger(particle.x))-state_.cameraX);const auto y=SmokeInteger(static_cast<double>(SmokeInteger(particle.y))-state_.cameraY);const auto color=Color(particle.life,fields.maximumLife);
        hooks_.line({SmokeInteger(x),y,SmokeInteger(static_cast<double>(x)+1),y,color});
    }
}
}
