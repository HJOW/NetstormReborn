// 카운트는 signed 나머지이며 화면 정수 산술은 DWORD로 감긴다.
#include "o/RawManaBoltRegular.h"
#include "o/RawPathAnimation.h"
#include <bit>
#include <stdexcept>
namespace netstorm::o {
namespace {
// 정렬되지 않은 raw 필드의 지정 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result{};
    // 낮은 바이트부터 조합하여 인접 필드를 섞지 않는다.
    for (std::size_t i=0;i<width;++i) result|=std::uint32_t(raw[offset+i])<<(i*8);
    return result;
}
// 선택된 실제 효과가 없으면 생략하지 않는다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("마나 볼트 주기 효과 미연결");return value; }
// signed32 뺄셈의 실제 DWORD 감김이다.
std::int32_t Sub(std::int32_t a,std::uint32_t b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)-b); }
}
// PATCH의 실제 폭/모듈 정체성을 확인한다. CD의 다른 ABI는 임의 허용하지 않는다.
RawManaBoltRegular::RawManaBoltRegular(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    std::span<const SquidDisplayShape> shapes,const RawManaBoltArrival& arrival,ManaBoltRegularState& state,ManaBoltRegularHooks hooks)
    :pool_(pool),types_(types),frames_(frames),shapes_(shapes),arrival_(arrival),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=types.size() || shapes.size()!=types.size() || &arrival.Pool()!=&pool) throw std::invalid_argument("마나 볼트 주기 판본/같은 월드 오류");
}
// 풀/호스트의 수명 검사와 별도로 실제 가상 몸체 쌍을 선택한다.
bool RawManaBoltRegular::Handles(Sid sid) const { const auto raw=pool_.Slot(sid);return raw[10]==105 && Read(raw,0)==0x509810; }
// 실제 효과의 순서를 보존하고 가상 자기 삭제 뒤에는 raw/this를 읽지 않는다.
float RawManaBoltRegular::Run(Sid sid,std::uint32_t event,std::uint32_t count) const {
    if (!Handles(sid)) throw std::logic_error("다른 마나 볼트 주기 가상 몸체");
    const auto raw=pool_.Slot(sid);const auto type=raw[10];const auto direction=RawPathAnimation::Direction(frames_[type],std::bit_cast<std::int32_t>(Read(raw,36)));
    if (direction<0 || direction>=16) throw std::out_of_range("마나 볼트 주기 글자 구간");
    const auto run=frames_[type].Run(static_cast<std::uint8_t>('A'+direction));if (run.count<=0) throw std::logic_error("마나 볼트 주기 frame 나머지0");
    if (!event && !count) Need(hooks_.sound)(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),"influenceIcon.wav");
    Need(hooks_.setFrame)(sid,std::bit_cast<std::int32_t>(count)%run.count+run.first,0);
    if (event!=1) return state_.interval;
    const auto target=arrival_.Target(sid);
    if (!target.value || !(pool_.Slot(target)[40]&1)) { Need(hooks_.destroy)(sid,0);return -1.0f; }
    const auto currentType=pool_.Slot(sid)[10];const auto& shape=shapes_[currentType];const auto& record=types_[currentType];
    if (!shape.loaded || shape.frameCount<=0 || shape.frames.empty() || record.maxHitPoints==-0x22222223 || record.maxHitPoints==-0x32323233) throw std::logic_error("마나 볼트 자기 TYPE frame0 SHP 오류");
    const auto frame=shape.frames[0];const auto targetRaw=pool_.Slot(target);
    auto point=Need(hooks_.project)(std::bit_cast<float>(Read(targetRaw,14)),std::bit_cast<float>(Read(targetRaw,18)));
    point[0]=Sub(point[0],static_cast<std::uint32_t>(static_cast<std::int32_t>(frame.width)+frame.hotspotX)+static_cast<std::uint32_t>(state_.spacing));
    point[1]=Sub(point[1],(Read(pool_.Slot(sid),12,2)+1)*static_cast<std::uint32_t>(state_.spacing));
    Need(hooks_.unpop)(sid,0);const auto position=Need(hooks_.inverse)(point[0],point[1]);Need(hooks_.pop)(sid,position[0],position[1],0);return state_.interval;
}
// 같은 raw 월드 정체성이다.
const SidPool& RawManaBoltRegular::Pool() const { return pool_; }
}
