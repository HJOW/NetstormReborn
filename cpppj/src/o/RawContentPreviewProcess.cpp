// 내용물 미리보기의 double 기한과 부모 삭제/ProcessForm 수명을 원본 순서대로 연결한다.
#include "o/RawContentPreviewProcess.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 정렬과 호스트 주소 폭에 의존하지 않는 원문 DWORD 읽기다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) {
    std::uint32_t value{};
    // 낮은 바이트부터 정확히 네 바이트만 합친다.
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[at+i])<<(8*i);
    return value;
}
// 지정한 DWORD만 써 +c 패딩과 만료 시각을 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value) {
    // 원본 little endian의 낮은 네 바이트를 기록한다.
    for (std::size_t i=0;i<4;++i) raw[at+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 선택된 경계만 검사한다. 미만료/NaN/빈 통지는 효과를 요구하지 않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("내용물 미리보기 프로세스의 실제 효과 미연결");
    return function;
}
}
double ContentPreviewFields::Deadline() const { return std::bit_cast<double>(std::uint64_t(Read(bytes,16))|(std::uint64_t(Read(bytes,20))<<32)); }
Sid ContentPreviewFields::Parent() const {
    const auto parent=Read(bytes,8);
    if (parent>0xffffU) throw std::out_of_range("미리보기 부모 DWORD가 SID 범위를 벗어났습니다");
    return Sid{static_cast<std::uint16_t>(parent)};
}
RawContentPreviewProcess::RawContentPreviewProcess(SidPool& pool,const double& now,const std::uint32_t& processType,ContentPreviewHooks hooks)
    :pool_(pool),now_(now),processType_(processType),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("내용물 미리보기 프로세스는 10.78 몸체입니다");
}
void RawContentPreviewProcess::Construct(ContentPreviewFields& fields,Sid parent,float duration) const {
    Put(fields.bytes,8,0);Put(fields.bytes,4,0);Put(fields.bytes,0,0x503c1cU);
    Put(fields.bytes,16,0);Put(fields.bytes,20,0); // 500460의 원문 double0 초기화 뒤 실제 만료 시각을 덮어쓴다.
    const auto deadline=std::bit_cast<std::uint64_t>(static_cast<double>(duration)+now_);
    Put(fields.bytes,16,static_cast<std::uint32_t>(deadline));Put(fields.bytes,20,static_cast<std::uint32_t>(deadline>>32));
    Need(hooks_.attach)(fields,parent,processType_,0x10U);
}
void RawContentPreviewProcess::Run(ContentPreviewFields& fields) const {
    if (!(now_>=fields.Deadline())) return;
    const auto parent=fields.Parent();const auto raw=pool_.Slot(parent); // 41bfb0은 진단 전에 부모 포인터를 고정한다.
    if (!(raw[6]|raw[7])) Need(hooks_.assertion)();
    Need(hooks_.destroy)(parent,0x400000U);
}
void RawContentPreviewProcess::Notify(ContentPreviewFields&,std::uint32_t,std::uint32_t,std::uint32_t) {} // 실제 480260은 상태를 읽거나 쓰지 않는다.
void RawContentPreviewProcess::Destruct(ContentPreviewFields& fields,std::uint32_t flags) const {
    Put(fields.bytes,0,0x5019ecU);Put(fields.bytes,4,0xffffcfc7U);Put(fields.bytes,8,0xffffcfc7U);
    if (flags&1U) Need(hooks_.release)(fields);
}
const SidPool& RawContentPreviewProcess::Pool() const { return pool_; }
ContentPreviewProcess::ContentPreviewProcess(const RawContentPreviewProcess& logic,ContentPreviewFields fields):logic_(logic),fields_(std::move(fields)) {}
ContentPreviewProcess::~ContentPreviewProcess() { logic_.Destruct(fields_,0); }
void ContentPreviewProcess::RunFrame() { if (Host()) logic_.Run(fields_); }
void ContentPreviewProcess::OnAttached() {
    if (!Host() || &Host()->Pool()!=&logic_.Pool()) throw std::logic_error("미리보기 프로세스와 Host의 풀 불일치");
    Put(fields_.bytes,4,Form().value);Put(fields_.bytes,8,Parent().value);
}
void ContentPreviewProcess::Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) { RawContentPreviewProcess::Notify(fields_,event,info,value); }
ContentPreviewFields& ContentPreviewProcess::Fields() { return fields_; }
}
