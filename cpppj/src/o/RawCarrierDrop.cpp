// 원본의 현재 재조회와 조기 반환/계속 진행 순서를 보존한다.
#include "o/RawCarrierDrop.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 선택된 실제 하위가 없으면 그 호출 위치에서 오류를 보고한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("Cast/Drop 실제 하위 미연결");return hook; }
// 호출자 소유 패킷의 비정렬 WORD/DWORD를 원본 폭으로 읽는다.
std::uint32_t Bytes(std::span<const std::uint8_t> data,std::size_t offset,std::size_t width) {
    std::uint32_t value{};
    // 낮은 바이트부터 조합해 호스트 정렬에 의존하지 않는다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(data[offset+index])<<(8*index);return value;
}
// 지정 폭 밖의 패킷 바이트를 보존한다.
void Put(std::span<std::uint8_t> data,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 원본 little endian 바이트만 기록한다.
    for (std::size_t index=0;index<width;++index) data[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 헤더 작성기는 bit7만 지우고 선언 길이를4 이상으로 고치지 않는다.
void Header(std::span<std::uint8_t> data,const FormPacketHeader& header) { data[0]=header.protocol&0x7fU;data[1]=header.message;Put(data,2,header.length,2); }
}
// 실제 현재 상태를 빌리며 생성 시 외부 효과를 실행하지 않는다.
RawCarrierDrop::RawCarrierDrop(SidPool& pool,std::span<const RiftTypeRecord> types,const ContainedFinderState& contained,CarrierDropState& state,CarrierDropHooks hooks)
    :pool_(pool),types_(types),contained_(contained),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("Cast/Drop 판본/TYPE 오류");
}
std::uint32_t RawCarrierDrop::Read(Sid sid,std::size_t offset,std::size_t width) const { return Bytes(pool_.Slot(sid),offset,width); } //같은 물리 슬롯을 현재 위치에서 읽는다.
std::uint32_t RawCarrierDrop::Genus(Sid sid) const { const auto type=Read(sid,10,1);if (type>=types_.size()) throw std::out_of_range("Cast/Drop 자산 TYPE");return types_[type].flags2; } //현재 부모의 genus다.
std::uint32_t RawCarrierDrop::Kind(Sid form) const { return Read(form,18,2); } //내용물의 종류는 BYTE 자산 타입과 다르다.
std::uint32_t RawCarrierDrop::KindGenus(Sid form) const { const auto type=Kind(form);if (type>=types_.size()) throw std::out_of_range("Cast/Drop 내용물 TYPE");return types_[type].flags2; } //외부 효과 뒤 종류를 다시 읽는다.
void RawCarrierDrop::Mark(Sid form,std::uint8_t value) const { pool_.bytes_[pool_.Offset(form)+22]=value; } //원본이 허용하는 물리 free 슬롯에도 같은 BYTE 쓰기를 유지한다.
// 원본의 주문 전용 관문을 일반 내려놓기에 추가하지 않는다.
void RawCarrierDrop::Request(Sid parent,Sid content) const {
    if (!content.value) {
        RawContainedFinder finder(pool_,contained_);content=finder.Begin(parent);
        // 주문을 건너뛰고 현재 첫 비주문을 선택한다.
        while (content.value && (KindGenus(content)&0x100U)) content=finder.Next();
        if (!content.value) return;
    }
    if (KindGenus(content)&0x100U) {
        if (Read(parent,40,1)&0x20U) return;
        if (Need(hooks_.immobile)(parent)) return;
        if (!Need(hooks_.afford)(static_cast<float>(Need(hooks_.cost)(Kind(content))))) return;
        const auto cost=Need(hooks_.cost)(Kind(content)); //승인 효과 뒤의 현재 비용을 다시 읽는다.
        const auto negative=std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(cost)); //원본 neg의 signed32 감김이다.
        Need(hooks_.subtract)(state_.localOwner,static_cast<float>(negative));
        const auto point=Need(hooks_.project)({std::bit_cast<float>(Read(parent,14)),std::bit_cast<float>(Read(parent,18))});
        Need(hooks_.popup)(cost,point,0);Need(hooks_.regular)(parent,0.1f);
        Need(hooks_.attached)(parent,[this](Sid process) { if (Read(process,10,1)==state_.notifyProcessType) Need(hooks_.notify)(process,3,0,0); });
        Need(hooks_.select)(0,0,1);
    }
    if (KindGenus(content)&0x8000U) Mark(content,1);
    std::array<std::uint8_t,9> packet{};Header(packet,state_.request);Put(packet,4,parent.value,2);Put(packet,6,content.value,2);packet[8]=Read(content,22,1)!=0;Need(hooks_.send)(9,packet);
}
// 검색 결과와 요청 대상이 다를 수 있는 원본 메뉴의 최초 SID 보관을 유지한다.
std::uint32_t RawCarrierDrop::Command(Sid parent,std::uint32_t code,Sid listParent) const {
    if (Read(parent,34,1)==state_.localOwner) {
        const auto command=code&0x03ffffffU;
        if (command==state_.selectCommand) { Need(hooks_.select)(parent.value,0,1);return 1; }
        if (command==state_.castCommand) {
            RawContainedFinder finder(pool_,contained_);auto current=finder.Begin(listParent);const auto initial=current;
            // 현재 플래그를 찾더라도 실제 요청에는 처음 보관한 SID를 전달한다.
            while (current.value && !(Read(current,20,1)&1U)) current=finder.Next();
            Request(parent,initial);return 1;
        }
        if (command==state_.dropCommand) { Request(parent,{});return 1; }
        if (command==state_.pickupCommand) { Need(hooks_.pickup)(parent);return 1; }
    }
    return Need(hooks_.baseMenu)(parent,code,listParent);
}
// 비용 부족 통지 뒤에도 생성 분기를 계속하는 원본 동작을 별도 성공 반환으로 바꾸지 않는다.
void RawCarrierDrop::Receive(std::uint32_t sender,std::span<const std::uint8_t> packet) const {
    if (packet.size()<9) throw std::invalid_argument("Cast/Drop 요청 패킷 길이 부족");
    if (!state_.server) Need(hooks_.assertion)(0x4cf);
    const Sid parent{static_cast<std::uint16_t>(Bytes(packet,4,2))};
    if (Read(parent,11,1)&3U) return;
    const Sid content{static_cast<std::uint16_t>(Bytes(packet,6,2))};if (!content.value) return;
    if ((Read(content,11,1)&7U) || Read(content,10,1)!=state_.contentType) return;
    const auto kind=Kind(content);if (state_.checking && (!kind || static_cast<std::int32_t>(kind)>=std::bit_cast<std::int32_t>(state_.numTypes))) Need(hooks_.assertion)(0x4e8);
    if (!kind || static_cast<std::int32_t>(kind)>=std::bit_cast<std::int32_t>(state_.numTypes)) return;
    auto point=Need(hooks_.dropPoint)(parent,kind);const auto limit=static_cast<float>(state_.mapLimit);
    if (!(0.0f<point[0] && point[0]<limit && 0.0f<point[1] && point[1]<limit)) return;
    point={static_cast<float>(static_cast<std::int32_t>(point[0])),static_cast<float>(static_cast<std::int32_t>(point[1]))};
    Need(hooks_.queue)(state_.queueCommand,kind,0);
    if (kind>=types_.size()) throw std::out_of_range("Cast/Drop 현재 TYPE 물리 범위");
    if (types_[kind].flags2&0x100U) {
        const auto owner=Read(parent,34,1);if (sender!=owner) { Need(hooks_.badOwner)(kind,sender);return; }
        auto cost=Need(hooks_.cost)(kind);if (owner==state_.localOwner) cost=0;
        if (Need(hooks_.money)(owner)<static_cast<double>(cost)) Need(hooks_.refund)(kind,Read(parent,34,1),{-10000.0f,-10000.0f},0,0);
        const auto movement=Need(hooks_.movement)(Read(parent,10,1));Need(hooks_.removeMovement)(parent,movement,0,1);Need(hooks_.regular)(parent,0.1f);
        Need(hooks_.detach)(parent,0);Need(hooks_.unpop)(parent,0);
        const auto born=Need(hooks_.create)(kind,0);if (!born.value) throw std::bad_alloc();
        Need(hooks_.owner)(born,Read(parent,34,1));Need(hooks_.argument)(born,Bytes(packet,4,2));Need(hooks_.pop)(born,point,0);
        std::array<std::uint8_t,6> activation{};Header(activation,state_.activation);Put(activation,4,born.value,2);
        Need(hooks_.activate)(9,0,activation);Need(hooks_.broadcast)(activation);return;
    }
    if (Need(hooks_.place)(kind,point,Read(parent,34,1),Read(content,12,2),0,1,0.0,0)) {
        Mark(content,packet[8]!=0);Need(hooks_.destroy)(content,0);
        if (!(Genus(parent)&0x20000U) && !Need(hooks_.standing)(parent)) Need(hooks_.escape)(parent,1.0f,-1);
    }
}
const SidPool& RawCarrierDrop::Pool() const { return pool_; } //같은 저장소를 사용하는지 연결 단계에서 확인한다.
}
