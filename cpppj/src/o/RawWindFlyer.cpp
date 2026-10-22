// 고정0.25초 공격과0.08초 그림 갱신을 서로 다른 타이머로 보존한다.
#include "o/RawWindFlyer.h"
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr double kAnimationDelay=0.07999999821186066; // 원본509c60의double 상수다.
// 비정렬 원문DWORD를 계산 없이 조합한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) {
    std::uint32_t value{};
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[at+i])<<(8*i); // 원문 네 바이트만 읽는다.
    return value;
}
// 선택한DWORD/double 폭만 쓰고 주변 미정 바이트를 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint64_t value,std::size_t width=4) {
    for (std::size_t i=0;i<width;++i) raw[at+i]=static_cast<std::uint8_t>(value>>(8*i)); // 지정한 폭의바이트만 쓴다.
}
// double을 원본 비트 순서로 읽는다.
double Time(const FlyerProcessFields& fields,std::size_t at) { return std::bit_cast<double>(std::uint64_t(Read(fields.bytes,at))|(std::uint64_t(Read(fields.bytes,at+4))<<32)); }
// 실제 double 저장만 수행하며 별도dirty를 더하지 않는다.
void Time(FlyerProcessFields& fields,std::size_t at,double value) { Put(fields.bytes,at,std::bit_cast<std::uint64_t>(value),8); }
// 원본CRT의정상signed64 절삭 뒤lowDWORD를 보존한다.
std::int32_t Truncate(double value) {
    if (!(value>=-9223372036854775808.0 && value<9223372036854775808.0)) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 원본41d7a0의거의올림→float 저장→절삭이다.
int Cell(float value) { return Truncate(static_cast<float>(Truncate(static_cast<double>(value)+0.9999899864196777F))); }
// 선택된 하위 누락은 앞선 효과를 유지하며 명시적으로 거부한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("더스트 데빌 실제 하위 미연결");return hook; }
}
RawWindFlyer::RawWindFlyer(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<std::uint32_t> soundFrames,std::span<const std::uint32_t> activeSounds,const double& now,const bool& boss,const std::uint32_t& frame,WindFlyerHooks hooks)
    :pool_(pool),hash_(hash),types_(types),soundFrames_(soundFrames),activeSounds_(activeSounds),now_(now),boss_(boss),frame_(frame),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || soundFrames.size()!=188 || activeSounds.size()!=188) throw std::invalid_argument("더스트 데빌 PATCH188 자료 불일치");
}
void RawWindFlyer::Specialize(FlyerProcessFields& fields) { Put(fields.bytes,0,0x509f98);Put(fields.bytes,48,0x509f78);Time(fields,96,0);Put(fields.bytes,84,0); } // 기본 생성자의부착 뒤 실제 전용 표만 쓴다.
void RawWindFlyer::Initialize(FlyerProcessFields& fields) const { Time(fields,96,now_);Put(fields.bytes,80,0);Put(fields.bytes,88,0);Put(fields.bytes,84,1);Time(fields,72,now_+20.0); } // 원본45e230의저장 순서다.
std::int32_t RawWindFlyer::Animation(FlyerProcessFields& fields) const {
    // 실제41bfb0은각부모조회마다종속head가0인지진단하며반환한뒤에도다시호출된다.
    const auto parent=[this,&fields] { const auto raw=pool_.Slot(fields.Parent());if (!raw[6] && !raw[7]) Need(hooks_.assertion)(0xa4);return raw; };
    if ((parent()[40]&0x20U) || !(Time(fields,96)<=now_)) return std::bit_cast<std::int32_t>(Read(parent(),36));
    Time(fields,96,now_+kAnimationDelay);
    const auto current=Read(parent(),36);
    const auto count=Need(hooks_.runCount)(fields.Parent());
    const auto next=std::bit_cast<std::int32_t>(current+1U);
    if (!count || (next==INT32_MIN && count==-1)) throw std::domain_error("더스트 데빌 원본idiv 범위 오류");
    return next%count;
}
float RawWindFlyer::Handle(Sid sid,std::uint32_t event,std::uint32_t count,float period) const {
    if (event) return Need(hooks_.damageable)(sid,event,count,period);
    const auto signedCount=std::bit_cast<std::int32_t>(count);
    if (signedCount<80) {
        if (period!=0.25) Need(hooks_.assertion)(0x19e);
        Attack(sid,0.25f);
    } else if (boss_) { Need(hooks_.destroy)(sid,0);return -1.0f; }
    if (signedCount%40==0) {
        const auto type=pool_.Slot(sid)[10];
        if (soundFrames_[type]!=frame_) {
            soundFrames_[pool_.Slot(sid)[10]]=frame_;
            const auto x=std::bit_cast<float>(Read(pool_.Slot(sid),14)),y=std::bit_cast<float>(Read(pool_.Slot(sid),18));
            Need(hooks_.sound)(x,y,activeSounds_[pool_.Slot(sid)[10]]);
        }
    }
    return 0.25f;
}
void RawWindFlyer::Attack(Sid sid,float period) const {
    const auto x=std::bit_cast<float>(Read(pool_.Slot(sid),14)),y=std::bit_cast<float>(Read(pool_.Slot(sid),18));
    const auto cx=Cell(x),cy=Cell(y);
    RawSquidFinder finder(pool_,hash_,types_);
    auto candidate=finder.Begin({cx,cy,cx,cy},0); // 원본422230의반경0/flags0으로표면까지포함한다.
    std::array<Sid,10> victims{};std::size_t size{}; // 원본Buffer(10)는가득차면추가하지않는다.
    for (;candidate.value;candidate=finder.Next()) { // 효과를시작하기전에실제finder 순서의후보목록을완성한다.
        if (Need(hooks_.mayTarget)(sid,candidate) && !(types_[pool_.Slot(candidate)[10]].flags2&0x120000U) && size<victims.size()) victims[size++]=candidate;
        if ((types_[pool_.Slot(candidate)[10]].flags2&4U) && size<victims.size()) victims[size++]=candidate;
    }
    for (std::size_t i=0;i<size;++i) { // 각피해효과뒤후보의현재free/genus와공격자의현재owner를다시읽는다.
        const auto target=victims[i];if (pool_.Slot(target)[11]&1U) continue;
        if (types_[pool_.Slot(target)[10]].flags2&4U) Need(hooks_.weaken)(target);
        else {
            const auto rate=Need(hooks_.damageRate)(sid,target);
            Need(hooks_.damage)(target,Truncate(static_cast<double>(rate)*period),pool_.Slot(sid)[34],sid);
        }
    }
}
}
