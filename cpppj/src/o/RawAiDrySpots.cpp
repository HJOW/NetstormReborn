// 원본 좌표 중복·순환 검사·압축과 효과 뒤 현재 backing 재조회를 보존한다.
#include "o/RawAiDrySpots.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kCapacity=0xadc,kCount=0xae0,kPointer=0xae4,kCursor=0xae8,kOwner=0xaec; //원본내장DrySpot목록의실제필드다.
constexpr std::size_t kRecordBytes=12; //x/y의float비트와TYPE DWORD 한개를그대로담는다.
constexpr double kQuarter=0.25,kHalf=0.5; //00501550/00500ee4의 정확한 float 상수이며 중간 합에는 float 저장을 넣지 않는다.
// 비정렬DWORD를물리범위안에서읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI DrySpot DWORD 읽기 범위");std::uint32_t value{};
    // 실제네BYTE만낮은자리부터합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);return value;
}
// 지정네BYTE만쓰고주변/미사용꼬리를보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI DrySpot DWORD 쓰기 범위");
    // 원본little endian표현을기록한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// DWORD의signed비교/나머지에서구현의존변환을피한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 실제선택된하위가없으면앞선원문쓰기를유지한채거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI DrySpot 실제 하위 미연결");return function; }
}
// 생성시게임판단/목록할당을수행하지않는다.
RawAiDrySpots::RawAiDrySpots(RawAiBrainMemory& memory,const SidPool& pool,AiDrySpotHooks hooks)
    :memory_(memory),pool_(pool),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("AI DrySpot은같은PATCH풀만지원합니다"); }
// 해제토큰/작은가짜본체를거부한다.
std::span<std::uint8_t> RawAiDrySpots::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI DrySpot 실제0xb08본체누락");return raw; }
// 하위효과뒤현재본체를다시빌린다.
std::uint32_t RawAiDrySpots::Field(std::uint32_t token,std::size_t offset) const { return Read(Body(token),offset); }
// 선택필드만쓴다.
void RawAiDrySpots::Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const { Put(Body(token),offset,value); }
// 포인터를미리복사한별도목록으로대체하지않는다.
std::span<std::uint8_t> RawAiDrySpots::Records(std::uint32_t token) const { return memory_.Bytes(Field(token,kPointer)); }
// 원본정상범위밖메모리접근을다른항목/자동clamp로바꾸지않는다.
std::size_t RawAiDrySpots::Offset(std::span<const std::uint8_t> raw,std::int32_t index) const { if (index<0 || static_cast<std::size_t>(index)>raw.size()/kRecordBytes || raw.size()-static_cast<std::size_t>(index)*kRecordBytes<kRecordBytes) throw std::out_of_range("AI DrySpot 기록 물리 범위");return static_cast<std::size_t>(index)*kRecordBytes; }
// 용량이찼으면포인터/중복검색을읽지않고나간다.현재TYPE가달라도같은좌표면중복이다.
bool RawAiDrySpots::Add(std::uint32_t token,float x,float y,std::uint32_t type) const {
    const auto count=Signed(Field(token,kCount));if (count>=Signed(Field(token,kCapacity))) return false;
    // float동등비교로signed0을같게보고NaN좌표는중복으로제거하지않는다.
    for (std::int32_t index=0;index<Signed(Field(token,kCount));++index) { const auto raw=Records(token);const auto at=Offset(raw,index);if (x==std::bit_cast<float>(Read(raw,at)) && y==std::bit_cast<float>(Read(raw,at+4))) return false; }
    auto raw=Records(token);const auto at=Offset(raw,count);Put(raw,at,std::bit_cast<std::uint32_t>(x));Put(raw,at+4,std::bit_cast<std::uint32_t>(y));Put(raw,at+8,type);Field(token,kCount,Field(token,kCount)+1U);return true;
}
// 원본은index가끝이상이면복사없이count만감소하며cursor/미사용꼬리를초기화하지않는다.
void RawAiDrySpots::Remove(std::uint32_t token,std::int32_t index) const {
    // 각기록을x→y→TYPE순서로옮기고현재포인터/count를재조회한다.
    while (index<Signed(Field(token,kCount)-1U)) { auto raw=Records(token);const auto target=Offset(raw,index),source=Offset(raw,Signed(static_cast<std::uint32_t>(index)+1U));Put(raw,target,Read(raw,source));Put(raw,target+4,Read(raw,source+4));Put(raw,target+8,Read(raw,source+8));index=Signed(static_cast<std::uint32_t>(index)+1U); }
    Field(token,kCount,Field(token,kCount)-1U);
}
// 한 번에 한 기록만 검사하며 제거 뒤 원본 cursor를 보정하지 않는다.
std::uint32_t RawAiDrySpots::Scan(std::uint32_t token) const {
    const auto count=Signed(Field(token,kCount));if (!count) return 0;const auto numerator=Signed(Field(token,kCursor)+1U);if (count==-1 && numerator==std::numeric_limits<std::int32_t>::min()) throw std::overflow_error("AI DrySpot signed 나눗셈 넘침");const auto index=numerator%count;
    const auto pointer=Field(token,kPointer),owner=Field(token,kOwner);Field(token,kCursor,static_cast<std::uint32_t>(index));std::string text;const auto raw=memory_.Bytes(pointer);const auto at=Offset(raw,index);
    if (!Need(hooks_.energy)(text,std::bit_cast<float>(Read(raw,at)),std::bit_cast<float>(Read(raw,at+4)),Read(raw,at+8),owner)) return 0;
    const auto current=Signed(Field(token,kCursor));const auto records=Records(token);const auto type=Read(records,Offset(records,current)+8);Remove(token,current);return type;
}
// 반환TYPE가0이면로그를내지않고현재fake종류와디버그값을읽는다.
std::uint32_t RawAiDrySpots::Maintain(std::uint32_t token) const { const auto type=Scan(token);if (type) { const auto fake=type==Field(token,0x280);if (Field(token,0xb00)) Need(hooks_.log)(token,fake); }return type; }
// 전체 합/곱 뒤 네 float를 저장하고, 난수 한 번 뒤 원본 순서로 두 좌표를 기록한다.
void RawAiDrySpots::Seed(std::uint32_t token,GameRandom& random) const {
    if (!(Field(token,0x248)&0x20U)) return;
    // 이 조회는 현재 본체 float를 넓혀 읽고 계산 중간의 임의 float 반올림을 피한다.
    const auto number=[this,token](std::size_t at) { return static_cast<double>(std::bit_cast<float>(Field(token,at))); };
    const auto combinedX=static_cast<float>((((number(0x2c0)+number(0x2b8))+number(0x278))+number(0x270))*kQuarter);
    const auto combinedY=static_cast<float>((((number(0x2c4)+number(0x2bc))+number(0x27c))+number(0x274))*kQuarter);
    const auto ownX=static_cast<float>((number(0x278)+number(0x270))*kHalf),ownY=static_cast<float>((number(0x27c)+number(0x274))*kHalf);
    switch (random.Next(8)) {
    case 0:Field(token,0xaf4,Field(token,0x274));Field(token,0xaf0,std::bit_cast<std::uint32_t>(combinedX));break;
    case 1:Field(token,0xaf4,Field(token,0x27c));Field(token,0xaf0,std::bit_cast<std::uint32_t>(combinedX));break;
    case 2:Field(token,0xaf0,Field(token,0x270));Field(token,0xaf4,std::bit_cast<std::uint32_t>(combinedY));break;
    case 3:Field(token,0xaf4,std::bit_cast<std::uint32_t>(combinedY));Field(token,0xaf0,Field(token,0x278));break;
    case 4:Field(token,0xaf4,Field(token,0x274));Field(token,0xaf0,std::bit_cast<std::uint32_t>(ownX));break;
    case 5:Field(token,0xaf4,Field(token,0x27c));Field(token,0xaf0,std::bit_cast<std::uint32_t>(ownX));break;
    case 6:Field(token,0xaf0,Field(token,0x270));Field(token,0xaf4,std::bit_cast<std::uint32_t>(ownY));break;
    case 7:Field(token,0xaf4,std::bit_cast<std::uint32_t>(ownY));Field(token,0xaf0,Field(token,0x278));break;
    }
    const auto type=Field(token,0x280),y=Field(token,0xaf4),x=Field(token,0xaf0);
    static_cast<void>(Add(token,std::bit_cast<float>(x),std::bit_cast<float>(y),type));
}
// 같은 실제 월드의 풀인지 확인한다.
const SidPool& RawAiDrySpots::Pool() const { return pool_; }
}
