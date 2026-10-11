// 비트 패턴을 실수 계산 없이 옮기며 dirty·부모 인자·캡처된 form과 경계 뒤의 현재 타입을 보존한다.
#include "o/RawConstructionData.h"
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 헤더 길이와 부모/시작/끝/추가 바이트의 전송 비트다.
constexpr std::size_t kHeaderSize=9;
constexpr std::uint8_t kParent=1,kStart=2,kEnd=4,kAdditional=8;
// 실제 폭의 바이트만 low→high 순서로 읽는다. caller가 전체 크기를 검사한 뒤 사용한다.
std::uint64_t ReadBits(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    std::uint64_t result=0;
    // 정렬되지 않은 DWORD/double을 호스트의 정렬·엔디언과 분리한다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint64_t>(bytes[offset+i])<<(8*i);
    return result;
}
// 비트 값을 주어진 폭으로만 쓴다. 뒤의 패킷/주변 표식은 보존한다.
void WriteBits(std::span<std::uint8_t> bytes,std::size_t offset,std::uint64_t value,std::size_t width) {
    // 저장된 double/NaN도 산술 변환 없이 그대로 옮긴다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 선택된 실제 필드의 길이다. 상위 네 비트는 보존하지만 추가 payload를 만들지 않는다.
std::size_t FieldSize(std::uint8_t mask) { return ((mask&kStart) ? 8U : 0U)+((mask&kEnd) ? 8U : 0U)+((mask&kAdditional) ? 1U : 0U); }
// 원본의 범위 밖 읽기 대신 실제 버퍼 길이를 미리 요구한다. 선언 길이 WORD는 별도 전송 계층의 책임이다.
void Require(std::size_t actual,std::size_t needed) { if (actual<needed) throw std::invalid_argument("건설 상태 패킷 길이 부족"); }
}
RawConstructionData::RawConstructionData(SidPool& pool,ConstructionProcessState& state,ConstructionDataHooks hooks,SquidProcessHost* host)
    :pool_(pool),state_(state),hooks_(std::move(hooks)),host_(host) {
    if (!hooks_.allocate || !hooks_.attach || (!host && !hooks_.lookup) || (host && &host->Pool()!=&pool))
        throw std::invalid_argument("건설 상태 수신의 저장소/효과 연결 오류");
}
// 실제 새 객체 수신도 같은 월드의 건설/삭제/Kernel을 사용한다.
RawConstructionData::RawConstructionData(SidPool& pool,ConstructionProcessState& state,RawConstructionProcess& logic,SquidProcessHost& host)
    :pool_(pool),state_(state),host_(&host),logic_(&logic) {
    if (&logic.Pool()!=&pool || &host.Pool()!=&pool) throw std::invalid_argument("건설 상태 수신의 실제 풀이 다릅니다");
}
std::uint16_t RawConstructionData::Write(ConstructionProcessFields& fields,std::span<std::uint8_t> packet,
    std::uint32_t form,std::uint32_t parent,bool force,ConstructionDataHeader header) {
    const auto mask=force ? std::uint8_t{0xff} : fields.dirty;
    const auto size=kHeaderSize+(parent ? 4U : 0U)+FieldSize(mask);Require(packet.size(),size);
    if (force) fields.dirty=0xff;
    packet[0]=header.protocol;packet[1]=header.message;WriteBits(packet,4,form,4);std::size_t cursor=kHeaderSize;
    if (parent) { fields.dirty|=kParent;WriteBits(packet,cursor,parent,4);cursor+=4; }
    if (fields.dirty&kStart) { WriteBits(packet,cursor,std::bit_cast<std::uint64_t>(fields.start),8);cursor+=8; }
    if (fields.dirty&kEnd) { WriteBits(packet,cursor,std::bit_cast<std::uint64_t>(fields.end),8);cursor+=8; }
    if (fields.dirty&kAdditional) packet[cursor++]=fields.additional;
    WriteBits(packet,2,cursor,2);packet[8]=fields.dirty;fields.dirty=0;return static_cast<std::uint16_t>(cursor);
}
std::uint16_t RawConstructionData::WriteProcess(ConstructionProcess& process,std::span<std::uint8_t> packet,
    bool includeParent,bool force,ConstructionDataHeader header) {
    const auto parent=(includeParent || force) ? process.Parent().value : 0U;
    return Write(process.Fields(),packet,process.Form().value,parent,force,header);
}
std::uint32_t RawConstructionData::Read(ConstructionProcessFields* fields,std::span<const std::uint8_t> packet) {
    Require(packet.size(),kHeaderSize);const auto mask=packet[8];Require(packet.size(),kHeaderSize+((mask&kParent) ? 4U : 0U)+FieldSize(mask));
    if (!fields && (mask&(kStart|kEnd|kAdditional))) throw std::invalid_argument("건설 상태의 실제 필드가 없습니다");
    std::uint32_t parent=0;std::size_t cursor=kHeaderSize;
    if (mask&kParent) { parent=static_cast<std::uint32_t>(ReadBits(packet,cursor,4));cursor+=4; }
    if (mask&kStart) { fields->start=std::bit_cast<double>(ReadBits(packet,cursor,8));cursor+=8; }
    if (mask&kEnd) { fields->end=std::bit_cast<double>(ReadBits(packet,cursor,8));cursor+=8; }
    if (mask&kAdditional) fields->additional=packet[cursor];
    return parent;
}
void RawConstructionData::Receive(std::span<const std::uint8_t> packet) const {
    Require(packet.size(),kHeaderSize);const auto number=static_cast<std::uint32_t>(ReadBits(packet,4,4));const auto mask=packet[8];
    Require(packet.size(),kHeaderSize+((mask&kParent) ? 4U : 0U)+FieldSize(mask));
    if (!number || number>=pool_.Capacity()) throw std::out_of_range("건설 상태 form 번호 오류");
    const Sid form{static_cast<std::uint16_t>(number)};const auto raw=pool_.Slot(form);
    if (raw[11]&1) {
        if (mask!=0xff) throw std::invalid_argument("새 건설 상태의 전체 비트가 필요합니다");
        auto owned=logic_ ? std::make_unique<ConstructionProcess>(*logic_,ConstructionProcessFields{}) : nullptr;
        auto* fields=owned ? &owned->Fields() : hooks_.allocate();if (!fields) throw std::bad_alloc();
        // 원본 수신 생성자가 쓰는 필드만 초기화한다. 추가/소리의 미초기화 영역은 저장소 공급자가 정한다.
        fields->dirty=0xff;fields->start=0;fields->end=0;
        const auto parent=Read(fields,packet);
        if (owned) {
            if (!parent || parent>=pool_.Capacity()) throw std::out_of_range("수신한 건설 부모 DWORD 범위 오류");
            host_->Attach(std::move(owned),state_.processType,Sid{static_cast<std::uint16_t>(parent)},0x10,form);
        } else hooks_.attach(*fields,state_.processType,parent,number,0x10);
        return;
    }
    if (mask&kParent) throw std::invalid_argument("기존 건설 상태의 부모 변경 비트 오류");
    ConstructionProcessFields* fields=nullptr;
    if (host_) {
        auto* base=host_->FromForm(form);auto* process=dynamic_cast<ConstructionProcess*>(base);
        if (base && (!process || process->Form()!=form)) throw std::invalid_argument("건설 상태의 실제 form 연결 오류");
        if (process) fields=&process->Fields();
    } else fields=hooks_.lookup(static_cast<ProcessId>(ReadBits(raw,18,4)));
    static_cast<void>(Read(fields,packet));
}
}
