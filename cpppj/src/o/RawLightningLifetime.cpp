// 생성 중 부모 재진입과 경로부터 시작하는 실제 반납 순서를 유지한다.
#include "o/RawLightningLifetime.h"
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// unavailable의 raw 주소 의미를0 실패와 합치지 않고 생존 검사 뒤에만 확보한다.
LightningCreateResult CreateLightning(const SidPool& pool,Sid parent,Sid target,std::optional<std::string> sound,
    const std::function<LightningVisualProcess*(Sid,Sid,std::optional<std::string>)>& create) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || target.value>=pool.Capacity()) throw std::invalid_argument("빔 wrapper 판본/표적 오류");
    if (pool.Slot(target)[11]&3) return {LightningCreateStatus::TargetUnavailable,target,nullptr};
    if (!create) throw std::invalid_argument("빔 wrapper 실제 생성 경계 누락");const auto process=create(parent,target,std::move(sound));
    return {process ? LightningCreateStatus::Created : LightningCreateStatus::AllocationFailed,{},process};
}
// 카메라는호출시점값이며확보효과가바꿔도이미계산한정수양끝을다시계산하지않는다.
LightningVisualProcess* CreateLightningAtScreen(Sid parent,std::array<std::int32_t,2> from,std::array<std::int32_t,2> to,float life,std::int32_t cameraX,std::int32_t cameraY,
    const std::function<LightningVisualProcess*(Sid,std::array<std::int32_t,2>,std::array<std::int32_t,2>,float,std::string)>& create) {
    if (!create) throw std::invalid_argument("화면빔 실제확보/생성 경계누락");
    const auto add=[](std::int32_t value,std::int32_t camera) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+std::bit_cast<std::uint32_t>(camera)); }; // 원본add의32비트감김이다.
    from={add(from[0],cameraX),add(from[1],cameraY)};to={add(to[0],cameraX),add(to[1],cameraY)};return create(parent,from,to,life,"shortZap.wav");
}
namespace {
// 보호10.78의 실제 process19·120입자·stride24·공유 색 객체/경로 확보량이다.
constexpr std::uint32_t kProcessType=19,kParticleCount=120,kParticleBytes=24,kPaletteBytes=8,kPathBytes=64;
// 원본 생성자가 쓰는 깊이와 거리 배율의 정확한 binary32다.
constexpr std::int32_t kDepth=-25;
// 원본0050c264의 흔들림 배율이다.
constexpr float kJitterScale=std::bit_cast<float>(std::uint32_t{0x3ccccccd});
// 실제 경로의 비정렬 little endian float만 읽는다.
float Distance(const LightningPathFields& fields) {
    std::uint32_t bits=0;
    // 원본+18의 네 바이트 위치를 유지한다.
    for (std::size_t index=0;index<4;++index) bits|=static_cast<std::uint32_t>(fields.bytes[24+index])<<(8*index);
    return std::bit_cast<float>(bits);
}
}
// 현재 판본/같은 풀/필수 확보 경계를 등록 전에 확인한다.
RawLightningLifetime::RawLightningLifetime(const SidPool& pool,RawRenderRegistry& registry,const RawLightningPath& path,
    LightningLifetimeState& state,LightningLifetimeHooks hooks):pool_(pool),registry_(registry),path_(path),state_(state),hooks_(std::move(hooks)) {
    if (pool_.Edition()!=OriginalEdition::Patch1078 || &pool_!=&registry_.Pool() || !hooks_.reserve) throw std::invalid_argument("빔 생성 월드/확보 경계 오류");
}
// 공통 등록/부착이 끝나야 파생 생성 필드를 설정한다. 초기화 중 Run/Draw는 상위 ready가 막는다.
void RawLightningLifetime::Initialize(LightningFrameFields& fields,std::uint32_t token,Sid parent,Sid target,std::optional<std::string> sound) const {
    InitializeBase(fields,token,parent);
    if (fields.registration.parent<0 || fields.registration.parent>65535) throw std::out_of_range("빔 생성 현재 부모 SID 오류");
    const Sid current{static_cast<std::uint16_t>(fields.registration.parent)};const auto raw=pool_.Slot(current);
    if (!raw[6] && !raw[7]) throw std::logic_error("빔 생성 현재 부모 head 원본 assert");
    path_.Initialize(fields.path,current,target,token);fields.jitter=static_cast<float>(static_cast<double>(Distance(fields.path))*kJitterScale);fields.sound=std::move(sound);
}
// target형과직접좌표형이같은원본생성접두를공유한다.
void RawLightningLifetime::InitializeBase(LightningFrameFields& fields,std::uint32_t token,Sid parent) const {
    registry_.Construct(fields.registration,token,parent,kProcessType);
    fields.particles.values.clear();fields.particles.total=0;fields.particles.owner=token;fields.lastTime=fields.elapsed=0;fields.first=false;
    if (!hooks_.reserve(kParticleCount*kParticleBytes)) throw std::bad_alloc();
    fields.particles.values.assign(kParticleCount,LightningParticle{});fields.firstLive=0;
    if (!state_.palette) {
        if (!hooks_.createPalette || !hooks_.buildPalette) throw std::logic_error("빔 첫 공유 색 경계 누락");
        if (!hooks_.reserve(kPaletteBytes)) throw std::bad_alloc();
        state_.palette=hooks_.createPalette();if (!state_.palette) throw std::bad_alloc();hooks_.buildPalette(state_.palette);
    }
    fields.palette=state_.palette;fields.registration.zOrder=kDepth;
    if (!hooks_.reserve(kPathBytes)) throw std::bad_alloc();fields.pathAllocated=true;
}
// 직접경로에는부모head/type/가짜target를추가로읽는절차가없다.
void RawLightningLifetime::InitializePoints(LightningFrameFields& fields,std::uint32_t token,Sid parent,std::array<float,2> from,std::array<float,2> to,float life,std::optional<std::string> sound) const {
    InitializeBase(fields,token,parent);path_.InitializePoints(fields.path,from,to,token);fields.jitter=static_cast<float>(static_cast<double>(Distance(fields.path))*kJitterScale);
    if (life!=0.0F) {
        const auto bits=std::bit_cast<std::uint32_t>(life); // NaN도원본비교처럼수명필드에그대로쓴다.
        // 실제4바이트수명만쓰고다른경로필드는보존한다.
        for (std::size_t i=0;i<4;++i) fields.path.bytes[32+i]=static_cast<std::uint8_t>(bits>>(8*i));
    }
    fields.sound=std::move(sound);
}
// native free는 필드의 포인터/개수 자체를 지우지 않는다. 호스트 입자 vector 저장소만 반납한다.
void RawLightningLifetime::Destroy(LightningFrameFields& fields,std::uint32_t flags,const std::function<void(LightningRelease)>& release) const {
    if (!release) throw std::invalid_argument("빔 반납 경계 누락");
    if (fields.pathAllocated) release(LightningRelease::Path);
    if (!fields.particles.values.empty()) { release(LightningRelease::Particles);std::vector<LightningParticle>().swap(fields.particles.values); }
    registry_.Destroy(fields.registration);if (flags&1U) release(LightningRelease::Object);
}
}
