// 예측 삭제 뒤의 재조회, signed WORD와 BYTE의 차이, 생산 통지의 효과 순서를 유지한다.
#include "o/RawFactoryResearch.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kPacketBytes=16; // 원본58h의 실제 선언/송신 길이다.
constexpr std::uint32_t kResearch=0x03000080U; // 메뉴에서 오는 연구 명령의 수정키 제외 값이다.
// 원본이 쓰는 필드 폭만 읽고 다음 필드는 섞지 않는다.
std::uint32_t Get(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    std::uint32_t value{};
    // 낮은 바이트부터 원래 비트 위치에 조립한다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[offset+i])<<(i*8);return value;
}
// 선언 길이/SID와 double 시각을 호스트 정렬과 무관하게 기록한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint64_t value,std::size_t width) {
    // 지정한 바이트만 덮어써 뒤의 버퍼는 보존한다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
// 선택된 효과가 연결되지 않았으면 실제 호출 위치에서 거부한다.
template<class F> const F& Required(const F& effect) { if (!effect) throw std::logic_error("Factory 연구 실제 하위 미연결");return effect; }
// 원본41bce0의 client SID 판정이며 풀의 예측 영역과 구분한다.
bool Client(Sid sid) { return sid.value>4 && sid.value<15000; }
}
// 입력의 종류는 최종 BYTE만 쓰며 시각의8바이트는 현재 double 비트 그대로다.
std::size_t WriteFactoryResearchCommand(Sid parent,std::uint32_t kind,double now,std::span<std::uint8_t> packet) {
    if (packet.size()<kPacketBytes) throw std::out_of_range("Factory 연구16바이트 저장소 부족");
    packet[0]=1;packet[1]=0x58;Put(packet,2,kPacketBytes,2);Put(packet,4,parent.value,2);packet[6]=static_cast<std::uint8_t>(kind);packet[7]=0;Put(packet,8,std::bit_cast<std::uint64_t>(now),8);return kPacketBytes;
}
// 콜백 누락은 해당 분기에서 검사하며 생성 시에는 실제 효과를 실행하지 않는다.
RawFactoryResearch::RawFactoryResearch(const SidPool& pool,std::span<const RiftTypeRecord> types,FactoryResearchState state,FactoryResearchHooks hooks)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("Factory 연구 PATCH 자료 오류");
}
// 실제 VT를 사용하므로 타입 번호만으로 미복원 파생을 허용하지 않는다.
bool RawFactoryResearch::HandlesNotice(Sid parent) const { const auto vt=Read(parent,0);return vt==0x00509148U || vt==0x00509210U; }
// form0을 포함해 물리 범위 안의 원문 필드만 읽는다.
std::uint32_t RawFactoryResearch::Read(Sid sid,std::size_t offset,std::size_t width) const { return Get(pool_.Slot(sid),offset,width); }
// 첫 검색 결과는 슬롯 재조회 뒤의 원본 진단에도 사용한다.
void RawFactoryResearch::Learn(Sid parent,std::uint32_t kind,std::uint32_t attachFlags) const {
    const auto found=Required(hooks_.find)(parent,16,kind,0);
    if (Client(found)) Required(hooks_.destroy)(found,0);
    const auto remaining=Required(hooks_.slots)(parent);
    if (found.value && remaining<=0) Required(hooks_.assertion)("slotsRemaining > 0");
    if (remaining<1) return;
    if (!Required(hooks_.find)(parent,16,kind,0).value) static_cast<void>(Required(hooks_.create)(parent,kind,0,16,attachFlags,0));
}
// 패킷 시각은 Learn 전에 저장한다. Learn 효과가 현재 시각을 바꿔도 송신 원문은 유지된다.
std::uint32_t RawFactoryResearch::Input(Sid parent,std::uint32_t code,std::uint32_t packedArgument) const {
    if ((code&0x03ffffffU)!=kResearch) throw std::logic_error("Factory 연구 외 입력 몸체 미복원");
    const auto kind=static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(packedArgument>>16)));
    if (!kind) { Required(hooks_.help)("researchHelp",1);return 1; }
    if (Required(hooks_.slots)(parent)>0) {
        std::array<std::uint8_t,kPacketBytes> packet{};WriteFactoryResearchCommand(parent,static_cast<std::uint32_t>(kind),state_.now,packet);
        Learn(parent,static_cast<std::uint32_t>(kind),16);Required(hooks_.send)(9,packet);
    }
    return 1;
}
// owner BYTE와 sender DWORD를 비교하며 연구 시각/flags는 생성 판단에 쓰지 않는다.
void RawFactoryResearch::Receive(std::uint32_t sender,std::span<const std::uint8_t> packet) const {
    if (packet.size()<8) throw std::out_of_range("Factory 연구 수신 본문 부족");
    const Sid parent{static_cast<std::uint16_t>(Get(packet,4,2))};if (Read(parent,11,1)&3U) return;
    if (!packet[6]) { if (packet[7]==1) throw std::logic_error("Factory 업그레이드 수신 몸체 미복원");return; }
    const auto type=Read(parent,10,1);if (type>=types_.size()) throw std::out_of_range("Factory 연구 현재 TYPE 범위 오류");
    if (!(types_[type].flags2&0x4000U) || Read(parent,34,1)!=sender || Read(parent,10,1)==state_.outpostType) return;
    if (!state_.boss) { Required(hooks_.assertion)("boss");if (!state_.boss) return; }
    Learn(parent,packet[6],0);
}
// 플레어가 내용을 변경할 수도 있으므로 kind와 후속 flags는 효과 뒤에 읽는다.
void RawFactoryResearch::Notice(Sid parent,Sid form,std::uint32_t reason) const {
    if (reason==1 && form.value && (Read(form,20,1)&16U)) {
        Required(hooks_.flare)(parent,6,2);
        const auto found=Required(hooks_.find)(parent,16,Read(form,18,2),0);
        if (found.value && found!=form && Client(found)) Required(hooks_.destroy)(found,0);
    }
    ManaNotice(parent,form,reason);
}
// flags는 한 번 읽은 원본 지역값이며 진단 뒤 owner는 현재 값을 다시 읽는다.
void RawFactoryResearch::ManaNotice(Sid parent,Sid form,std::uint32_t reason) const {
    const auto flags=Read(form,20,1);if (!state_.loadingDepth && (flags&16U)) ++state_.productionDirty;
    if (reason==1 && !state_.loadingDepth && (flags&16U)) {
        if (Read(parent,40,1)&9U) Required(hooks_.assertion)("!isAstral() && !isAbstract()");
        Required(hooks_.refresh)(Read(parent,34,1));
    }
}
// 동일 월드의 생성/삭제 연결을 검증하는 참조다.
const SidPool& RawFactoryResearch::Pool() const { return pool_; }
// 단일 패킷은 빈 observer를 호출하지 않고 실제 Receive에 같은 원문을 전달한다.
void RegisterFactoryResearchCommand(RawPacketDispatch& dispatch,RawFactoryResearch& research,std::function<void()> prepare) {
    dispatch.Observe(0x58,[](std::uint32_t) {});dispatch.Version(0x58,1,0);dispatch.RequireLength(0x58,16);
    dispatch.Register(0x58,[&research,prepare=std::move(prepare)](std::uint32_t sender,PacketConnection&,std::span<const std::uint8_t> packet) { if (prepare) prepare();research.Receive(sender,packet); });
}
}
