// 44a280/4492f0/4488c0의 선택된 폭·부분 효과·빈 가상 몸체만 복원한다.
#include "o/RawDaisProcessData.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o { // wire DWORD와 C++ 전체 포인터·Kernel PID를 구별한다.
namespace { // 물리 접근은 선택된 field에서만 보호한다.
constexpr std::size_t kForm=4,kParent=8,kInner=0x10,kDirty=0x18,kDeadline=0x20; // FormSID·부모SID·내장VT·dirtyBYTE·기한8BYTE를 원문 outer40BYTE에서 찾는 실제 오프셋이다.
// 정렬이나 호스트 엔디언과 무관하게 현재 원문 필드를 읽는다.
std::uint64_t DaisRead(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("DaisProcess의 선택된 입력 field 물리 부족");
    std::uint64_t result=0; // 선택한 폭의 little endian 비트다.
    for (std::size_t index=0;index<width;++index) result|=std::uint64_t(bytes[offset+index])<<(index*8); // 원문 field 밖을 읽지 않는다.
    return result;
}
// 선택된 store만 검사하며 앞서 기록한 dirty/패킷 접두는 실패해도 롤백하지 않는다.
void DaisPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint64_t value,std::size_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("DaisProcess의 선택된 출력 field 물리 부족");
    for (std::size_t index=0;index<width;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); // 원문 폭만 쓴다.
}
}
// 생성자가 건드리지 않은 padding/앞뒤 바이트를 새 의미의 전역 값으로 초기화하지 않는다.
void InitializeDaisProcessFields(DaisProcessFields& fields) {
    DaisPut(fields.bytes,kParent,0,4);DaisPut(fields.bytes,kForm,0,4);
    DaisPut(fields.bytes,kInner,0x50782cU,4);DaisPut(fields.bytes,kDeadline,0,8);
    fields.bytes[kDirty]=0xff;DaisPut(fields.bytes,0,0x507834U,4);DaisPut(fields.bytes,kInner,0x507830U,4);
}
// 468230→4ab2c0→41bd00의 기록 표식이며 Kernel은 같은 전체 C++ 포인터를 끝까지 소유한다.
void DestructDaisProcessFields(DaisProcessFields& fields) {
    DaisPut(fields.bytes,0,0x5019ecU,4);DaisPut(fields.bytes,kForm,0xffffcfc7U,4);DaisPut(fields.bytes,kParent,0xffffcfc7U,4);
}
// mask의 다른 bit는 원문처럼 무시한다. 현재 mask1이 있어도 기존 객체의 parent는 변경하지 않는다.
std::uint32_t ReadDaisProcessData(DaisProcessFields* fields,std::span<const std::uint8_t> packet,bool newProcess) {
    const auto mask=static_cast<std::uint8_t>(DaisRead(packet,8,1));
    std::size_t cursor=9;std::uint32_t parent=0; // parent가 없을 때 원문 ESI의 초기0이다.
    if (mask&1U) { if (newProcess) parent=static_cast<std::uint32_t>(DaisRead(packet,cursor,4));cursor+=4; }
    if (mask&2U) {
        const auto deadline=DaisRead(packet,cursor,8); // 원문 fld가 먼저 선택한 입력을 읽은 뒤 실제 outer+20에 저장한다.
        if (!fields) throw std::logic_error("DaisProcess bit2 저장의 현재 NULL/타클래스 raw뷰 미지원");
        DaisPut(fields->bytes,kDeadline,deadline,8); // 유한 시각 비트는 보존하며 x87 SNaN/예외 상태까지 복제하지 않는다.
    }
    return parent;
}
// 헤더 bit7은 지우지 않으며 force/부모 payload와 dirty mask를 일반 serializer 규칙으로 정상화하지 않는다.
std::uint16_t WriteDaisProcessData(DaisProcessFields& fields,std::span<std::uint8_t> packet,std::uint32_t form,std::uint32_t parentPayload,bool force,std::uint8_t protocol,std::uint8_t message) {
    if (force) fields.bytes[kDirty]=0xff;
    DaisPut(packet,0,protocol,1);DaisPut(packet,1,message,1);DaisPut(packet,4,form,4);
    std::uint16_t size=9;std::size_t cursor=9; // 옵션이 없을 때도 반환 길이는9다.
    if (parentPayload) { fields.bytes[kDirty]|=1U;DaisPut(packet,cursor,parentPayload,4);cursor+=4;size+=4; }
    if (fields.bytes[kDirty]&2U) { const auto deadline=DaisRead(fields.bytes,kDeadline,8);DaisPut(packet,cursor,deadline,8);size+=8; }
    DaisPut(packet,2,size,2);DaisPut(packet,8,fields.bytes[kDirty],1);fields.bytes[kDirty]=0;
    return size;
}
// 실제40BYTE는 metadata이고 C++ 객체의 vptr/sizeof를 원문32비트 포인터로 해석하지 않는다.
DaisProcess::DaisProcess(DaisProcessFields fields):fields_(std::move(fields)) { InitializeDaisProcessFields(fields_); }
// 원문의 free(flags1)는 C++ 단일 unique_ptr 소유권의 해제와 대응한다.
DaisProcess::~DaisProcess() { DestructDaisProcessFields(fields_); }
// 현재 기한/dirty가 있어도 원문은 아무 사건이나 예약을 실행하지 않는다.
void DaisProcess::RunFrame() {}
// BaseProcess.Attach의 두 필드를 Host가 먼저 갱신하며 전송이 Attached보다 먼저여도 같은 값을 사용한다.
void DaisProcess::OnAttached() { DaisPut(fields_.bytes,kForm,Form().value,4);DaisPut(fields_.bytes,kParent,Parent().value,4); }
// 원문의480260은 인자를 소비하지 않는 빈RET12다.
void DaisProcess::Notify(std::uint32_t,std::uint32_t,std::uint32_t) {}
// 수신과 writer가 같은 객체를 갱신하도록 실제 이미지 참조를 반환한다.
DaisProcessFields& DaisProcess::Fields() { return fields_; }
// 현재 이미지 관찰은 복사본이나 다른 PID의 객체를 만들지 않는다.
const DaisProcessFields& DaisProcess::Fields() const { return fields_; }
// 일반 부착과 전체 동기화의 기존 wrapper 인자를 현재 outer 필드로 전달한다.
std::uint16_t DaisProcess::WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force,std::uint8_t protocol,std::uint8_t message) {
    OnAttached();const auto parent=(parentChanged || force) ? static_cast<std::uint32_t>(DaisRead(fields_.bytes,kParent,4)) : 0U;
    return WriteDaisProcessData(fields_,packet,static_cast<std::uint32_t>(DaisRead(fields_.bytes,kForm,4)),parent,force,protocol,message);
}
}
