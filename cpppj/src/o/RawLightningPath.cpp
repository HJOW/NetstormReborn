// 빔 경로의 float 저장 경계·재귀 순서·두 번 뽑는 step 난수를 유지한다.
#include "o/RawLightningPath.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 float 상수의 정확한 비트다.1/3과0.9를 넓은 수학 상수로 다시 만들지 않는다.
constexpr float kMinimumStep=std::bit_cast<float>(0x3da3d70aU),kBranchThreshold=std::bit_cast<float>(0x3df5c28fU),
    kStepDecay=std::bit_cast<float>(0x3f666666U),kThird=std::bit_cast<float>(0x3eaaaaabU);
constexpr double kRandomUnit=1.0/65536.0; // 원본005092cc와 같은 상위16비트 난수 단위다.
static_assert(sizeof(LightningParticle)==24); // 원본 용량/이전 입자 토큰 변환의 고정 폭이다.
// 비정렬 경로 필드의 little endian DWORD를 읽는다.
std::uint32_t Word(const LightningPathFields& fields,std::size_t offset) {
    std::uint32_t value=0;
    // 지정한 네 바이트만 합친다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(fields.bytes[offset+index])<<(8*index);
    return value;
}
// 나머지 미사용 바이트를 건드리지 않고 DWORD 하나만 쓴다.
void Word(LightningPathFields& fields,std::size_t offset,std::uint32_t value) {
    // 원본 little endian 필드 폭을 유지한다.
    for (std::size_t index=0;index<4;++index) fields.bytes[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// float32의 비트를 별칭 규칙 위반 없이 읽는다.
float Float(const LightningPathFields& fields,std::size_t offset) { return std::bit_cast<float>(Word(fields,offset)); }
// 실제 fstp float 경계에 대응하는 한 필드 저장이다.
void Float(LightningPathFields& fields,std::size_t offset,float value) { Word(fields,offset,std::bit_cast<std::uint32_t>(value)); }
}
// 미복원 판본/잘못된 배율·중심 경계는 최초 생성 전에 거부한다.
RawLightningPath::RawLightningPath(const SidPool& pool,GameRandom& random,const LightningPathState& state,std::function<std::array<float,2>(Sid)> center)
    :pool_(pool),random_(random),state_(state),center_(std::move(center)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || !center_ || !std::isfinite(state.scaleX) || !std::isfinite(state.scaleY) || state.scaleX<=0 || state.scaleY<=0)
        throw std::invalid_argument("빔 경로 판본/배율/중심 조회 오류");
}
// source 보정과 target 보정을 구별하며 경로에는 이후 따라갈 SID를 저장하지 않는다.
void RawLightningPath::Initialize(LightningPathFields& fields,Sid source,Sid target,std::uint32_t owner) const {
    if (!owner || source.value>=pool_.Capacity() || target.value>=pool_.Capacity()) throw std::out_of_range("빔 경로 소유자/자산 번호 오류");
    std::fill(fields.bytes.begin(),fields.bytes.begin()+16,std::uint8_t{});
    const auto from=center_(source);Float(fields,0,static_cast<float>(static_cast<double>(state_.scaleX)*from[0]));Float(fields,4,static_cast<float>(static_cast<double>(state_.scaleY)*from[1]));
    const auto sourceType=pool_.Slot(source)[10];
    if (sourceType==state_.vanderType) Float(fields,4,static_cast<float>(static_cast<double>(Float(fields,4))-76));
    else if (sourceType==state_.tallType) Float(fields,4,static_cast<float>(static_cast<double>(Float(fields,4))-48));
    else if (sourceType==state_.manOWarType) { Float(fields,4,static_cast<float>(static_cast<double>(Float(fields,4))-48));Float(fields,0,static_cast<float>(static_cast<double>(Float(fields,0))-5)); }
    const auto to=center_(target);Float(fields,8,static_cast<float>(static_cast<double>(state_.scaleX)*to[0]));Float(fields,12,static_cast<float>(static_cast<double>(state_.scaleY)*to[1]));
    const auto targetType=pool_.Slot(target)[10];Float(fields,12,static_cast<float>(static_cast<double>(Float(fields,12))-(targetType==state_.tallType || targetType==state_.manOWarType ? 48 : 16)));
    const double dx=static_cast<double>(Float(fields,0))-Float(fields,8),dy=static_cast<double>(Float(fields,4))-Float(fields,12);
    Float(fields,16,static_cast<float>(dx));Float(fields,20,static_cast<float>(dy));Float(fields,24,static_cast<float>(std::sqrt(dx*dx+dy*dy)));Float(fields,28,static_cast<float>(std::atan2(-dy,dx)));
    Float(fields,32,2);Word(fields,56,0);Word(fields,60,owner);
}
// owner/현재 꼬리만 확인하고 입자의 기존 수명/미사용 필드를 고쳐 주지 않는다.
void RawLightningPath::Validate(const LightningPathFields& fields,const LightningParticles& particles) {
    if (!particles.owner || Word(fields,60)!=particles.owner || particles.total<0 || static_cast<std::size_t>(particles.total)>particles.values.size() || particles.values.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::invalid_argument("빔 경로/입자 소유자 또는 꼬리 오류");
}
// 원본이 반환하는 새 입자 주소를 고정 저장소의 비영 토큰으로 돌려준다.
std::uint32_t RawLightningPath::Append(LightningParticles& particles) {
    if (static_cast<std::size_t>(particles.total)>=particles.values.size()) throw std::length_error("빔 입자 용량 원본 assert");
    return static_cast<std::uint32_t>(++particles.total);
}
// 이전 주소가 현재 사용한 꼬리 밖이면 가짜 연결을 만들지 않는다.
LightningParticle& RawLightningPath::At(LightningParticles& particles,std::uint32_t token) {
    if (!token || token>static_cast<std::uint32_t>(particles.total)) throw std::out_of_range("빔 이전 입자 토큰 오류");
    return particles.values[token-1];
}
// 첫 점의 흔들림 가중치는0이며 기본 진행 step은 정확한 float0.15다.
void RawLightningPath::Seed(LightningPathFields& fields,LightningParticles& particles) const {
    Validate(fields,particles);const auto token=Append(particles);At(particles,token)={Float(fields,0),Float(fields,4),1,0,Float(fields,32),0};
    Float(fields,36,std::bit_cast<float>(0x3e19999aU));Float(fields,40,std::bit_cast<float>(0x3e19999aU));Word(fields,52,token);Float(fields,44,0);Word(fields,48,Word(fields,28));
}
// 원본은 float 간격을 다시 float에 저장하지 않고 넓은 차/곱/덧셈으로 반환한다.
double RawLightningPath::Random(float low,float high) const { return static_cast<double>(random_.Next(65536))*kRandomUnit*std::abs(static_cast<double>(high)-low)+low; }
// 새 주점은 이전 주점에, 재귀 가지는 방금 만든 점에 연결한다. 재귀 결과를 주점으로 교체하지 않는다.
std::uint32_t RawLightningPath::Branch(LightningPathFields& fields,LightningParticles& particles,float angle,float length,std::uint32_t previous,float strength,float lifeOffset) const {
    const auto cosine=std::cos(static_cast<double>(angle)),sine=std::sin(static_cast<double>(angle));const auto before=At(particles,previous);const auto token=Append(particles);
    At(particles,token)={static_cast<float>(static_cast<double>(before.x)-cosine*length),static_cast<float>(sine*length+before.y),strength,1,static_cast<float>(static_cast<double>(lifeOffset)+Float(fields,32)),previous};
    if (kBranchThreshold<strength && random_.Next(100)<85) {
        const float spread=Float(fields,40)<kMinimumStep ? .5F : .25F;const auto offset=Random(-spread,spread);
        static_cast<void>(Branch(fields,particles,static_cast<float>(offset*strength+angle),static_cast<float>(static_cast<double>(length)*.5),token,static_cast<float>(static_cast<double>(strength)*.5),lifeOffset));
    }
    return token;
}
// step 검사와 실제 저장에 다른 난수를 쓰는 원문 분기를 한 번의 clamp로 합치지 않는다.
void RawLightningPath::Step(LightningPathFields& fields,LightningParticles& particles,float progress,float lifeOffset) const {
    Validate(fields,particles);if (Word(fields,56)) return;
    if (!std::isfinite(progress) || !std::isfinite(lifeOffset)) throw std::invalid_argument("빔 경로 진행/수명 입력 오류");
    if (progress>1) progress=1;
    // 누적 진행은 매회 float 저장한 값으로 다시 비교한다.
    while (Float(fields,44)<progress) {
        const auto step=Float(fields,40);if (!(step>0) || !std::isfinite(step)) throw std::logic_error("빔 경로 시작점 초기화 누락");
        const float spread=step<kMinimumStep ? .5F : .25F;const auto offset=Random(-spread,spread);Float(fields,48,static_cast<float>(offset*step+Float(fields,48)));
        const auto current=Branch(fields,particles,static_cast<float>(offset+Float(fields,28)),static_cast<float>(static_cast<double>(Float(fields,24))*step),Word(fields,52),1,lifeOffset);
        Word(fields,52,current);Float(fields,44,static_cast<float>(static_cast<double>(Float(fields,44))+Float(fields,40)));
        auto maximum=static_cast<float>(static_cast<double>(Float(fields,40))*kStepDecay);if (maximum<kMinimumStep) maximum=kMinimumStep;
        Float(fields,36,maximum);const float low=static_cast<float>(static_cast<double>(maximum)*kThird);
        const auto probe=Random(low,maximum);Float(fields,40,probe<kMinimumStep ? kMinimumStep : static_cast<float>(Random(static_cast<float>(static_cast<double>(Float(fields,36))*kThird),Float(fields,36))));
    }
    if (progress==1) {
        const auto token=Append(particles);At(particles,token)={Float(fields,8),Float(fields,12),1,progress,Float(fields,32),Word(fields,52)};Word(fields,56,1);
    }
}
}
