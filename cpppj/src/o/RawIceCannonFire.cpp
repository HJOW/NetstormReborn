// 원본 아이스 파생의 현재 필드 재조회와 냉각 전환 순서를 보존한다.
#include "o/RawIceCannonFire.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // 원본 미정렬 필드를 호스트 정렬과 구별한다.
namespace { // 이 파생의 작은 raw 필드 접근만 공유한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) { // 현재DWORD만 little endian으로 읽는다.
    std::uint32_t value{};for (std::size_t i=0;i<4;++i) value|=std::uint32_t(bytes[offset+i])<<(8*i);return value; // 이웃 padding은 읽지 않는다.
}
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) { // 지정한DWORD 밖의 padding을 보존한다.
    for (std::size_t i=0;i<4;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i)); // 실제 하위 바이트부터 쓴다.
}
double Deadline(const IceCannonFireFields& fields) { return std::bit_cast<double>(std::uint64_t(Read(fields.extra,8))|(std::uint64_t(Read(fields.extra,12))<<32)); } //+48 double을 원본 비트로 읽는다.
void Deadline(IceCannonFireFields& fields,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.extra,8,static_cast<std::uint32_t>(bits));Put(fields.extra,12,static_cast<std::uint32_t>(bits>>32)); } //+44 padding과 공통 dirty는 건드리지 않는다.
}
RawIceCannonFire::RawIceCannonFire(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const PriestPlainCanonType> frames,
    CannonFireState& state,GunAnimationHooks common,IceCannonFireHooks hooks)
    :pool_(pool),frames_(frames),state_(state),hooks_(std::move(hooks)),common_(pool,types,frames,state,std::move(common)) {} // 판본/범위 검사는 같은 공통 생성자가 수행한다.
void RawIceCannonFire::Initialize(IceCannonFireFields& fields,Sid gunForm,Sid target) const { //004685e0은 공통 ctor가 끝난 뒤 파생 쓰기를 수행한다.
    common_.Initialize(fields.common,gunForm,target);Put(fields.common.bytes,0,0x50b3f0);Put(fields.common.bytes,32,0x50b3ec);Put(fields.extra,0,1);Deadline(fields,state_.now+4.0);
}
std::uint32_t RawIceCannonFire::Step(IceCannonFireFields& fields,std::int32_t frame) const { //00468660의 fcomp/test C0는 unordered 때 전환을 건너뛴다.
    if (state_.now>=Deadline(fields)) {
        if (Read(fields.extra,0)) {
            Put(fields.extra,0,0);if (!hooks_.cooling) throw std::logic_error("아이스 캐논의 실제 냉각 표시004a8770 미연결");hooks_.cooling(fields.common.Parent());Deadline(fields,state_.now+6.0);return std::bit_cast<std::uint32_t>(frame);
        }
        Put(fields.extra,0,1);Deadline(fields,state_.now+4.0);
    }
    const auto parent=fields.common.Parent();if (!parent.value || parent.value>=pool_.Capacity()) throw std::out_of_range("아이스 캐논 현재 부모 범위");const auto raw=pool_.Slot(parent);
    if (raw[10]!=state_.type) throw std::logic_error("아이스 캐논 현재 부모 TYPE");const auto current=Read(raw,36);const auto& table=frames_[raw[10]].frames;const auto codes=table.Codes();if (current>=codes.size()) throw std::out_of_range("아이스 캐논 현재 프레임 코드");const auto run=table.Run(codes[current].side);
    const auto last=std::bit_cast<std::int32_t>(std::uint32_t(run.first)+std::uint32_t(run.count)-1);const auto next=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(frame)+1);
    if (next>last) return static_cast<std::uint32_t>(run.first);
    if (!hooks_.context) throw std::logic_error("아이스 캐논 실제8DWORD 문맥 미연결");const auto context=hooks_.context(fields.common.Parent()); // context 효과 뒤의 현재 target을 읽는다.
    if (!hooks_.mayTarget) throw std::logic_error("아이스 캐논 실제 MayTarget 미연결");if (!hooks_.mayTarget(context,fields.common.Target())) return std::bit_cast<std::uint32_t>(next)|0x8000U;
    return std::bit_cast<std::uint32_t>(next)|((next==last && Read(fields.extra,0)) ? 0x4000U : 0U);
}
void RawIceCannonFire::Frame(IceCannonFireFields& fields) const { common_.FrameWithStep(fields.common,[this,&fields](auto frame) { return Step(fields,frame); }); } // 기존 공통 추적/공간/완료를 그대로 사용한다.
bool RawIceCannonFire::Launch(IceCannonFireFields& fields) const { return common_.Launch(fields.common); } // 현재 display130/effect0/속도 getter는 같은 상태를 읽는다.
const SidPool& RawIceCannonFire::Pool() const { return pool_; } // factory가 같은 실제 풀을 검사한다.
std::uint32_t RawIceCannonFire::Type() const { return state_.type; } // 현재 원본 부모 TYPE다.
IceCannonFire::IceCannonFire(const RawIceCannonFire& logic,IceCannonFireFields fields):logic_(logic),fields_(std::move(fields)) {} // 장기 논리와 실제 필드만 보유한다.
void IceCannonFire::RunFrame() { logic_.Frame(fields_); } // 같은 Kernel의 현재 시각을 사용한다.
void IceCannonFire::OnAttached() { Put(fields_.common.bytes,4,Form().value);Put(fields_.common.bytes,8,Parent().value); } // Host가 확정한 두DWORD다.
IceCannonFireFields& IceCannonFire::Fields() { return fields_; } // 후속 관찰은 같은 객체를 읽는다.
GunAnimationHooks MakeIceCannonFireHostHooks(SquidProcessHost& host,GunAnimationHooks hooks) { // 원문 완료/삭제를 실제 클래스에 연결한다.
    hooks.notify=[&host](Sid form,std::uint32_t event,std::uint32_t info,std::uint32_t value) { auto* gun=dynamic_cast<GunProcess*>(host.FromForm(form));if (!gun) throw std::logic_error("아이스 발사 완료 GunProcess 없음");gun->Notify(event,info,value); };
    hooks.kill=[&host](Sid form,std::uint32_t flags) { auto* fire=dynamic_cast<IceCannonFire*>(host.FromForm(form));if (!fire) throw std::logic_error("아이스 발사 완료 Process 없음");host.Kill(*fire,flags); };return hooks;
}
IceCannonFire* CreateIceCannonFire(SquidProcessHost& host,const RawIceCannonFire& logic,Sid gunForm,Sid target) { // 실제 원본 부모에35/flags10을 부착한다.
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("아이스 발사 Host/Pool 불일치");auto* gun=dynamic_cast<GunProcess*>(host.FromForm(gunForm));if (!gun || host.Pool().Slot(gun->Parent())[10]!=logic.Type()) throw std::logic_error("아이스 발사 factory 부모 오류");IceCannonFireFields fields;logic.Initialize(fields,gunForm,target);return dynamic_cast<IceCannonFire*>(host.Attach(std::make_unique<IceCannonFire>(logic,std::move(fields)),35,gun->Parent(),0x10));
}
}
