// 실제 기준점 난수 분포·signed 깊이·주기·후속 플레어와 종료 순서를 보존한다.
#include "o/RawFlare.h"
#include "o/RawPathAnimation.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kFlareVtable=0x005096a0U; // 원본106 생성자의 실제 가상 표다.
constexpr float kYScale=0.09090909361839294F; // 원본504194의float 비트3dba2e8c다.
// 필수 효과를 실제 호출할 때만 검사한다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("플레어 실제 하위 미연결");return callback; }
// 실제fadd0.5→CRT ftol→하위signedDWORD→float 저장 순서다. invalid의 하위DWORD는0이다.
float Integer(float input) {
    const double value=double(input)+0.5;if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;
    return static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value))));
}
// 실제 x87 unordered 분기는 NaN을 유지하며 유한 좌표에만1..255를 적용한다.
float Clamp(float value) { if (value<1.0F) value=1.0F;if (255.0F<value) value=255.0F;return value; }
}
// 같은 PATCH 자료의188개 타입/프레임만 허용하며 생성 시 게임 효과는 실행하지 않는다.
RawFlare::RawFlare(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,GameRandom& random,FlareState state,FlareHooks hooks)
    :pool_(pool),types_(types),frames_(frames),random_(random),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=188) throw std::invalid_argument("플레어 PATCH 풀/TYPE/프레임 오류");
}
// 가상 표를 확인해 이름만 같은 다른 파생에 처리기를 적용하지 않는다.
bool RawFlare::Handles(Sid sid) const { return Read(sid,0)==kFlareVtable; }
// 원본 DWORD만 낮은 바이트부터 조합한다.
std::uint32_t RawFlare::Read(Sid sid,std::size_t offset) const {
    const auto raw=pool_.Slot(sid);std::uint32_t value{};
    // 인접한 필드로 읽기 폭을 넓히지 않는다.
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[offset+i])<<(i*8);return value;
}
// 원본의 직접 frame 쓰기는 SquidFrame 통지를 호출하지 않는다.
void RawFlare::Put(Sid sid,std::size_t offset,std::uint32_t value) const {
    auto raw=pool_.AllocatedBytes(sid);
    // 네 바이트만 덮어써 깊이/owner/extra를 유지한다.
    for (std::size_t i=0;i<4;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
// 원본은 입력float 구간 차이를 넓힌 뒤 절댓값과 현재 상위16비트를 곱한다.
double RawFlare::Random(float low,float high) const { return double(random_.Next(65536))*(1.0/65536.0)*std::abs(double(high)-double(low))+double(low); }
// TYPE의 실제 코드 방향으로부터 first/count를 얻고 잘못된 물리 프레임은 거부한다.
RiftTypeFrames::LetterRun RawFlare::Run(Sid sid) const {
    const auto type=pool_.Slot(sid)[10];if (type>=frames_.size()) throw std::out_of_range("플레어 현재 TYPE 범위");
    const auto direction=RawPathAnimation::Direction(frames_[type],std::bit_cast<std::int32_t>(Read(sid,36)));
    if (direction<0 || direction>=16) throw std::out_of_range("플레어 현재 프레임 방향 범위");return frames_[type].Run(static_cast<std::uint8_t>('A'+direction));
}
// 각 생성 후 현재 부모 좌표/기준점을 다시 읽고 로컬 Kernel 개수만 하나씩 증가시킨다.
void RawFlare::Spawn(Sid parent,std::int32_t count,std::uint32_t event) const {
    if (state_.suppressedTime!=0 || state_.loadingDepth) return;
    const auto effectType=state_.type;const auto depth=pool_.Slot(parent)[35];auto size=Need(hooks_.processCount)();
    // 음수/0개는 생성하지 않으며30000개일 때는 마지막 하나만 더 만든다.
    for (std::int32_t i=0;i<count;++i) {
        if (size>30000) return;
        const auto child=Need(hooks_.create)(effectType,2);const auto x=std::bit_cast<float>(Read(parent,14)),y=std::bit_cast<float>(Read(parent,18));
        const auto hotspot=Need(hooks_.sourceHotspot)(parent);const auto type=pool_.Slot(parent)[10];if (type>=types_.size()) throw std::out_of_range("플레어 부모 현재 TYPE 범위");
        const bool shifted=(types_[type].flags2&0x30000U)!=0;const float offsetX=shifted ? -0.7F : 0.0F,offsetY=shifted ? -0.1F : 0.0F;
        const auto highX=static_cast<float>(double(hotspot[0])*0.0625+double(offsetX)),highY=static_cast<float>(double(hotspot[1])*double(kYScale)+double(offsetY));
        const auto randomX=Random(offsetX,highX),randomY=Random(offsetY,highY);const auto currentType=pool_.Slot(child)[10];if (currentType>=frames_.size()) throw std::out_of_range("플레어 생성 현재 TYPE 범위");
        Put(child,36,static_cast<std::uint32_t>(frames_[currentType].Run('A').first));Need(hooks_.owner)(child,0);
        auto raw=pool_.AllocatedBytes(child);raw[40]|=8U;raw[35]=static_cast<std::uint8_t>(depth-1U);
        const float spawnX=Clamp(static_cast<float>(double(x)-randomX)),spawnY=Clamp(static_cast<float>(double(y)-randomY));Need(hooks_.pop)(child,spawnX,spawnY,0);
        if (Need(hooks_.reserveRegular)()) Need(hooks_.addRegular)(child,event,static_cast<float>(Random(0,0.07F)+0.05));
        ++size;
    }
}
// count/event은 원본signed int 비교이며 실제 Kernel의uint32 비트 입력을 유지한다.
float RawFlare::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const {
    if (!Handles(sid)) throw std::invalid_argument("플레어 현재 가상 표 오류");
    const auto run=Run(sid);const auto step=std::bit_cast<std::int32_t>(count),kind=std::bit_cast<std::int32_t>(event);
    if (!step) payload=static_cast<float>(Random(0.05F,0.13F));
    if (kind>1 && step==run.count-1) {
        if (Need(hooks_.processCount)()>30000) return 0.0F;
        const auto child=Need(hooks_.create)(pool_.Slot(sid)[10],2);const auto x=Integer(std::bit_cast<float>(Read(sid,14))),y=Integer(std::bit_cast<float>(Read(sid,18)));
        const auto randomX=Random(-0.4F,0.4F),randomY=Random(-0.4F,0.4F);const auto childY=static_cast<float>(randomY+double(y));Put(child,36,static_cast<std::uint32_t>(run.first));
        Need(hooks_.pop)(child,static_cast<float>(randomX+double(x)),childY,0);
        if (Need(hooks_.reserveRegular)()) Need(hooks_.addRegular)(child,event-1U,0);
    }
    if (step>=run.count) { Need(hooks_.update)(sid);Need(hooks_.destroy)(sid,0);return -1.0F; }
    Need(hooks_.frame)(sid,std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(run.first)+count),0);
    if (step==run.count/2 && RawPathAnimation::Direction(frames_[pool_.Slot(sid)[10]],std::bit_cast<std::int32_t>(Read(sid,36)))==1)
        Need(hooks_.sound)(std::bit_cast<float>(Read(sid,14)),std::bit_cast<float>(Read(sid,18)),"powerup.wav",0);
    return payload;
}
// 나머지 부모는 기존 실제 처리기로 전달한다.
RegularHandler RawFlare::Handler(RegularHandler fallback) const {
    return [this,fallback=std::move(fallback)](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
        if (Handles(sid)) return Handle(sid,event,count,payload);if (!fallback) throw std::logic_error("플레어 외 Regular 미연결");return fallback(sid,event,count,payload);
    };
}
// 효과 생성/수명 조립의 동일성을 확인한다.
const SidPool& RawFlare::Pool() const { return pool_; }
}
