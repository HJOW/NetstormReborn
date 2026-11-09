// 원본 modulo 계산과 현재 raw 프레임 폭 및 실제 지정 호출을 보존한다.
#include "o/RawDiscRegular.h"
#include <bit>
#include <limits>
#include <stdexcept>
namespace netstorm::o {
namespace {
// little endian DWORD를 주변 슬롯 바이트 변경 없이 읽는다.
std::uint32_t ReadDisc(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 현재 필드 네 바이트를 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// x86 signed IDIV의 0분모/몫 overflow만 명시하며 음수 피연산자는 그대로 허용한다.
std::int32_t DiscDivide(std::int32_t value,std::int32_t divisor) {
    if (!divisor || (value==std::numeric_limits<std::int32_t>::min() && divisor==-1)) throw std::domain_error("Disc 원본 IDIV 오류");
    return value/divisor;
}
// 나머지 연산도 같은 IDIV 오류 조건을 먼저 확인해 C++의 정의되지 않은 동작을 피한다.
std::int32_t DiscRemainder(std::int32_t value,std::int32_t divisor) {
    static_cast<void>(DiscDivide(value,divisor));return value%divisor;
}
}
// 동일 풀/판본 표를 확인하여 다른 월드의 지정기를 거부한다.
RawDiscRegular::RawDiscRegular(const SidPool& pool,std::span<const ArcherTypeData> types,const SquidFrame& setter)
    :pool_(pool),types_(types),setter_(setter) {
    if (&pool!=&setter.Pool() || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U))
        throw std::invalid_argument("Disc Regular 풀/TYPE 판본 불일치");
}
// 원본 정상 signed32 입력에서 방향당 프레임 수의 modulo를 정확 적용한다.
std::int32_t RawDiscRegular::NextFrame(std::int32_t current,std::int32_t frameCount,std::int32_t directions) {
    if (current<0 || current==std::numeric_limits<std::int32_t>::max() || directions<=0 || frameCount<directions)
        throw std::out_of_range("Disc Regular 현재 프레임/TYPE 나눗셈 범위 오류");
    return NextFrameNative(current,frameCount,directions);
}
// 원문 LEA/SUB/ADD는 DWORD로 감기며 세 나눗셈의 signed 나머지를 그대로 사용한다.
std::int32_t RawDiscRegular::NextFrameNative(std::int32_t current,std::int32_t frameCount,std::int32_t directions) {
    const auto stride=DiscDivide(frameCount,directions);const auto next=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(current)+1U);
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(DiscRemainder(next,stride))-static_cast<std::uint32_t>(DiscRemainder(current,stride))+static_cast<std::uint32_t>(current));
}
// 원본 가상 표 기록만 확인하며 보호 PE 주소를 호스트 함수로 실행하지 않는다.
bool RawDiscRegular::Handles(Sid sid) const { return ReadDisc(pool_.Slot(sid),0)==(pool_.Edition()==OriginalEdition::Patch1078 ? 0x50b570U : 0x501440U); }
// event/count는 원본도 미사용이다. 프레임 지정 뒤 payload 전체 float를 유지한다.
float RawDiscRegular::Handle(Sid sid,std::uint32_t,std::uint32_t,float payload) const {
    if (!Handles(sid)) throw std::invalid_argument("Disc Regular 현재 가상 표 미지원");
    return HandleIn(pool_,setter_,[this](auto type) { if (type>=types_.size()) throw std::out_of_range("Disc Regular TYPE 오류");return types_[type]; },sid,payload);
}
// 실제 현재 TYPE를 먼저 선택하고 같은 Frame.Set 뒤 payload 전체 float 비트를 돌려준다.
float RawDiscRegular::HandleIn(const SidPool& pool,const SquidFrame& setter,const std::function<ArcherTypeData(std::uint32_t)>& type,Sid sid,float payload,bool nativeArithmetic) {
    if (!type) throw std::logic_error("Disc Regular 현재 TYPE getter 미연결");const auto raw=pool.Slot(sid);const auto data=type(raw[10]);
    const auto current=pool.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(ReadDisc(raw,36)) : raw[34];
    const auto next=nativeArithmetic ? NextFrameNative(current,data.frameCount,data.directions):NextFrame(current,data.frameCount,data.directions);
    setter.Set(sid,next,0);return payload;
}
// 다른 부모 처리기는 같은 입력을 유지한다. 미지정 fallback은 명시 오류다.
RegularHandler RawDiscRegular::Handler(RegularHandler fallback) const {
    return [this,fallback=std::move(fallback)](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
        if (Handles(sid)) return Handle(sid,event,count,payload);
        if (!fallback) throw std::logic_error("Disc 외부 Regular 부모 미연결");return fallback(sid,event,count,payload);
    };
}
}
