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
    const auto stride=frameCount/directions;return ((current+1)%stride-current%stride)+current;
}
// 원본 가상 표 기록만 확인하며 보호 PE 주소를 호스트 함수로 실행하지 않는다.
bool RawDiscRegular::Handles(Sid sid) const { return ReadDisc(pool_.Slot(sid),0)==(pool_.Edition()==OriginalEdition::Patch1078 ? 0x50b570U : 0x501440U); }
// event/count는 원본도 미사용이다. 프레임 지정 뒤 payload 전체 float를 유지한다.
float RawDiscRegular::Handle(Sid sid,std::uint32_t,std::uint32_t,float payload) const {
    if (!Handles(sid)) throw std::invalid_argument("Disc Regular 현재 가상 표 미지원");
    const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("Disc Regular TYPE 오류");
    const auto current=pool_.Edition()==OriginalEdition::Patch1078 ? std::bit_cast<std::int32_t>(ReadDisc(raw,36)) : raw[34];
    setter_.Set(sid,NextFrame(current,types_[raw[10]].frameCount,types_[raw[10]].directions),0);return payload;
}
// 다른 부모 처리기는 같은 입력을 유지한다. 미지정 fallback은 명시 오류다.
RegularHandler RawDiscRegular::Handler(RegularHandler fallback) const {
    return [this,fallback=std::move(fallback)](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
        if (Handles(sid)) return Handle(sid,event,count,payload);
        if (!fallback) throw std::logic_error("Disc 외부 Regular 부모 미연결");return fallback(sid,event,count,payload);
    };
}
}
