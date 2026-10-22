// 현재필드재조회·목록중복/꼬리·기한NaN·좌표별칭과실제하위순서를보존한다.
#include "o/RawAiBridgeStrategies.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kPointBias=0.99999F; //0041d7a0의00502310 실제float상수다.
// 비정렬DWORD의주변을읽지않는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 다리 전략 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //네BYTE만합친다.
// 현재지정DWORD만쓴다.
void Write(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 다리 전략 쓰기 범위");for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //주변바이트를보존한다.
// 원문DWORD의signed비교다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 실제__ftol signed64하위DWORD이며indefinite하위는0이다.
std::uint32_t Truncate(double value) { if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(value)); }
// 선택된하위누락을빈성공으로바꾸지않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 다리 전략 실제 하위 미연결");return function; }
//0041d610은사각형을수정하지않고고정256경계에서margin5를엄격히검사한다.
bool Interior(const AiBuildSearchBounds& bounds) { return bounds.left>5.0F && bounds.top>5.0F && bounds.right<251.0F && bounds.bottom<251.0F; }
}
// 생성은게임효과/난수/필드변경을실행하지않는다.
RawAiBridgeStrategies::RawAiBridgeStrategies(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,GameRandom& random,AiBridgeStrategyGlobals globals,AiBridgeStrategyHooks hooks):memory_(memory),pool_(pool),types_(types),random_(random),globals_(std::move(globals)),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("AI 다리 전략 실제 PATCH188 누락"); }
// 현재살아있는같은본체를다시조회한다.
std::span<std::uint8_t> RawAiBridgeStrategies::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 다리 전략 실제0xb08 본체 누락");return raw; }
// 외부효과뒤의현재DWORD다.
std::uint32_t RawAiBridgeStrategies::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 원본이쓰는네BYTE만바꾼다.
void RawAiBridgeStrategies::Field(std::uint32_t token,std::size_t at,std::uint32_t value) const { Write(Body(token),at,value); }
// NaN/부호0도비트그대로읽는다.
RawPathPoint RawAiBridgeStrategies::Point(std::uint32_t token,std::size_t at) const { return {std::bit_cast<float>(Field(token,at)),std::bit_cast<float>(Field(token,at+4))}; }
// 같은실제본문의두축만갱신한다.
void RawAiBridgeStrategies::Point(std::uint32_t token,std::size_t at,RawPathPoint point) const { Field(token,at,std::bit_cast<std::uint32_t>(point.x));Field(token,at+4,std::bit_cast<std::uint32_t>(point.y)); }
// 잘못된번호를다른정상WORD로감지않는다.
Sid RawAiBridgeStrategies::Object(std::uint32_t value) const { if (value>=pool_.Capacity() || value>65535) throw std::out_of_range("AI 다리 전략 실제 SID 범위");return Sid{static_cast<std::uint16_t>(value)}; }
// free/owner필터를genus조회에추가하지않는다.
std::uint32_t RawAiBridgeStrategies::Genus(std::uint32_t value) const { const auto kind=pool_.Slot(Object(value))[10];if (kind>=types_.size()) throw std::out_of_range("AI 다리 전략 TYPE 범위");return types_[kind].flags2; }
// 목록외부의호스트메모리접근만명시거부한다.
std::uint32_t RawAiBridgeStrategies::Item(std::uint32_t token,std::size_t list,std::int32_t index) const { if (index<0) throw std::out_of_range("AI 다리 전략 음수 목록 인덱스");return Read(memory_.Bytes(Field(token,list)),static_cast<std::size_t>(index)*4); }
// native0040e9b0과같이count/용량을먼저읽고full이면포인터를읽지않는다.
void RawAiBridgeStrategies::Append(std::uint32_t token,std::size_t list,std::uint32_t value) const { const auto count=Signed(Field(token,list+8));if (count>=Signed(Field(token,list+4))) return;if (count<0) throw std::out_of_range("AI 다리 전략 음수 추가 위치");Write(memory_.Bytes(Field(token,list)),static_cast<std::size_t>(count)*4,value);Field(token,list+8,static_cast<std::uint32_t>(count)+1U); }
// 네이티브안정압축과미사용꼬리보존을유지한다.
void RawAiBridgeStrategies::Remove(std::uint32_t token,std::size_t list,std::uint32_t value) const {
    std::uint32_t removed{};
    for (std::int32_t index=0;index<Signed(Field(token,list+8));++index) { const auto item=Item(token,list,index);if (item==value) ++removed;else Write(memory_.Bytes(Field(token,list)),(static_cast<std::size_t>(index)-removed)*4,item); } //현재active전체에서모든동일값을제거한다.
    Field(token,list+8,Field(token,list+8)-removed);
}
// 네case는refresh뒤현재신전/graph/기한을다시검사하며unordered는실행하지않는다.
bool RawAiBridgeStrategies::Ready(std::uint32_t token,std::size_t deadline) const { if (!Field(token,0x294) || Field(token,0x298)==Need(globals_.invalidGraph)()) return false;const auto due=std::bit_cast<double>((std::uint64_t(Field(token,deadline+4))<<32)|Field(token,deadline));return globals_.now>=due; }
// 원본지도상한은float로저장한다.
bool RawAiBridgeStrategies::Valid(RawPathPoint point) const { const auto limit=static_cast<float>(globals_.mapLimit);return point.x>0 && point.x<limit && point.y>0 && point.y<limit; }
// 실제중심float를더정밀한중간합으로거의올림한다.
RawPathPoint RawAiBridgeStrategies::RoundedCenter(std::uint32_t value) const { const auto point=Need(hooks_.center)(Object(value));return {static_cast<float>(Signed(Truncate(static_cast<double>(point.x)+kPointBias))),static_cast<float>(Signed(Truncate(static_cast<double>(point.y)+kPointBias)))}; }
// 원본필드쓰기순서를유지한다.
void RawAiBridgeStrategies::ResetPoint(std::uint32_t token) const { Field(token,0x3d4,0xc61c4000U);Field(token,0x3d0,0xc61c4000U); }
// 유효한기존적신전을진행한호출에서는성공값으로추가상태를만들지않는다.
void RawAiBridgeStrategies::Temple(std::uint32_t token) const {
    if (!Ready(token,0x5a8)) return;const auto target=Field(token,0x3cc);
    if (target && !(pool_.Slot(Object(target))[11]&2) && (Genus(target)&0x200U) && Need(hooks_.graph)(Object(Field(token,0x3cc)))!=Field(token,0x298)) { const auto point=RoundedCenter(Field(token,0x3cc));static_cast<void>(Need(hooks_.advance)(token,point));return; }
    Field(token,0x3cc,Field(token,0x2a8));ResetPoint(token);
}
// 현재대상연결→포기목록재등록→진행/실패또는새대상선택의원래순서다.
void RawAiBridgeStrategies::Geysers(std::uint32_t token) const {
    if (!Ready(token,0x5a8)) return;const auto target=Field(token,0x3cc);const bool valid=target && !(pool_.Slot(Object(target))[11]&2) && (Genus(target)&0x10000000U);bool connected{};
    if (valid && (connected=Need(hooks_.connected)(token,Object(Field(token,0x3cc))))) { Remove(token,0x348,Field(token,0x3cc));Append(token,0x330,Field(token,0x3cc)); }
    const auto due=std::bit_cast<double>((std::uint64_t(Field(token,0x5d4))<<32)|Field(token,0x5d0));
    if (Signed(Field(token,0x35c))>0 && globals_.now>=due) { const auto retry=Item(token,0x354,0);Need(hooks_.delay)(token);Remove(token,0x354,retry);Append(token,0x348,retry);Need(hooks_.log)(token,globals_.geyserLog,AiBridgeStrategyLog::Requeued,retry); }
    if (valid && !connected) {
        const auto point=RoundedCenter(Field(token,0x3cc));if (Need(hooks_.advance)(token,point)) return;const auto abandoned=Field(token,0x3cc);Append(token,0x354,abandoned);Need(hooks_.delay)(token);Remove(token,0x348,abandoned);Field(token,0x3cc,0);ResetPoint(token);Field(token,0x3d8,1);Need(hooks_.log)(token,globals_.geyserLog,AiBridgeStrategyLog::Abandoned,abandoned);return;
    }
    const auto home=Need(hooks_.home)(token);Need(hooks_.reclassify)(token);const auto invalid=Need(globals_.invalidGraph)();const auto selected=Need(hooks_.select)(token,home,invalid,invalid);Field(token,0x3cc,selected.value);if (selected.value) Need(hooks_.log)(token,globals_.geyserLog,AiBridgeStrategyLog::Trying,selected.value);ResetPoint(token);
}
// 주변대상사각형검사가실패해도오른쪽아래좌표를유지해같은유효성분기로진행한다.
void RawAiBridgeStrategies::Expand(std::uint32_t token) const {
    if (!Ready(token,0x5b0) || !Field(token,0x2a8) || Signed(Field(token,0x3c8))<1) return;const auto index=Signed(random_.Next(Field(token,0x2f0)+1U));RawPathPoint source{};
    if (index<Signed(Field(token,0x2f0))) {
        const auto target=Item(token,0x2e8,index);const auto raw=pool_.Slot(Object(target));const auto x=std::bit_cast<float>(Read(raw,14)),y=std::bit_cast<float>(Read(raw,18));const AiBuildSearchBounds bounds{static_cast<float>(static_cast<double>(x)-3.0F),static_cast<float>(static_cast<double>(y)-3.0F),static_cast<float>(static_cast<double>(x)+3.0F),static_cast<float>(static_cast<double>(y)+3.0F)};
        source={bounds.right,bounds.bottom};if (Interior(bounds)) source=Need(hooks_.randomStart)(token,bounds);else Remove(token,0x2e8,target);
    } else { const AiBuildSearchBounds bounds{std::bit_cast<float>(Field(token,0x270)),std::bit_cast<float>(Field(token,0x274)),std::bit_cast<float>(Field(token,0x278)),std::bit_cast<float>(Field(token,0x27c))};source=Need(hooks_.randomStart)(token,bounds); }
    if (!Valid(source)) return;const auto dy=Signed(random_.Next(16))-8,dx=Signed(random_.Next(16))-8;const RawPathPoint target{static_cast<float>(static_cast<double>(source.x)+static_cast<float>(dx)),static_cast<float>(static_cast<double>(source.y)+static_cast<float>(dy))};const auto next=Need(hooks_.extend)(token,source,target);Point(token,0x3d0,next);const auto current=Point(token,0x3d0);if (!(current.x==source.x && current.y==source.y)) Field(token,0x3c8,Field(token,0x3c8)-1U);
}
// 진단뒤현재좌표를다시읽어inclusive사각형과실제Advance를호출한다.
void RawAiBridgeStrategies::Coordinate(std::uint32_t token) const {
    if (!Ready(token,0x5b0)) return;if (!Valid(Point(token,0xaf0))) Need(hooks_.assertion)("sendBridgeHere.isValid()",0xa3a);const auto x=Truncate(std::bit_cast<float>(Field(token,0xaf0))),y=Truncate(std::bit_cast<float>(Field(token,0xaf4)));
    if (!Need(hooks_.rectangle)(Signed(x-1U),Signed(y-1U),Signed(x+1U),Signed(y+1U)) || !Need(hooks_.advance)(token,Point(token,0xaf0))) { Field(token,0xaf4,0xc61c4000U);Field(token,0xaf0,0xc61c4000U); }
}
// 현재목표는재고갱신의효과뒤조회한다.미지원목표도선행재고효과는보존한다.
void RawAiBridgeStrategies::Update(std::uint32_t token) const {
    static_cast<void>(Body(token));if (!Field(token,0x294) || Field(token,0x298)==Need(globals_.invalidGraph)() || !(Field(token,0x248)&0x1000U) || (Need(globals_.checking)() && Need(globals_.disabled)())) return;const auto parent=Need(hooks_.technology)(Field(token,0x260));Need(hooks_.refresh)(token,parent);
    switch (Field(token,0x2d4)) { case 1:Temple(token);break;case 2:Geysers(token);break;case 3:Expand(token);break;case 4:Coordinate(token);break;default:break; }
}
// 실제월드조립확인용같은풀이다.
const SidPool& RawAiBridgeStrategies::Pool() const { return pool_; }
}
