// 비행 프로세스의 실제 원문 폭과 생성·전송·후처리 순서를 보존한다.
#include "o/RawFlyerProcess.h"
#include "o/RawWindFlyer.h" // 수신용 Wind leaf도 기존의 정확한 전용 VT/추가 필드 쓰기를 재사용한다.
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kDirty=8,kHome=10,kInner=48,kForm=52,kParent=56; // 원본 outer/inner의 실제 바이트 위치다.
constexpr std::array<std::size_t,7> kDataOffsets{10,12,16,20,24,32,40}; // dirty2..80의 실제 전송 필드 순서다.
constexpr std::array<std::size_t,7> kDataWidths{2,2,4,4,4,8,8}; // 두 WORD/세 DWORD/두8BYTE의 독립 폭이다.
// 원본 비정렬 WORD/DWORD를 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4) { std::uint32_t value=0;for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[at+i])<<(8*i);return value; }
// 선택한 폭만 써 padding과 인접 필드를 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value,std::size_t width=4) { for (std::size_t i=0;i<width;++i) raw[at+i]=static_cast<std::uint8_t>(value>>(8*i)); }
// 실제 double 저장의 낮은/높은 DWORD다.
void Time(FlyerProcessFields& fields,std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(fields.bytes,at,static_cast<std::uint32_t>(bits));Put(fields.bytes,at+4,static_cast<std::uint32_t>(bits>>32)); }
// 실제 선택된 누락만 앞선 쓰기를 보존하고 거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("비행체 실제 하위 몸체 미연결");return function; }
// 현재 raw 좌표의 float 비트를 읽는다.
float Position(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<float>(Read(raw,at)); }
// 프레임 덧셈의 low DWORD 감김을 보존한다.
std::uint32_t FrameSum(std::int32_t frames,std::span<const std::uint8_t> raw) { return std::bit_cast<std::uint32_t>(frames)+Read(raw,36); }
void ObserveFlyerData(const FlyerProcessFields& fields) { // 네 실제 outer VT0은 모두480260의 ret12다. 미등록 가상을 같은 빈 몸체로 대체하지 않는다.
    const auto table=Read(fields.bytes,0);if (table!=0x509cc8U && table!=0x509f24U && table!=0x509f60U && table!=0x509f98U) throw std::logic_error("비행 데이터의 현재 가상0 observer 미연결");
}
}
Sid FlyerProcessFields::Form() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kForm))}; } // inner Form의 현재 하위WORD다.
Sid FlyerProcessFields::Parent() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kParent))}; } // inner 부모의 현재 하위WORD다.
Sid FlyerProcessFields::Home() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kHome,2))}; } // 홈은 원문 WORD다.
RawFlyerProcess::RawFlyerProcess(SidPool& pool,std::span<const RiftTypeRecord> types,FlyerProcessGlobals globals,FlyerProcessHooks hooks):pool_(pool),types_(types),globals_(globals),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("비행 프로세스는 PATCH188 타입만 복원했습니다");
}
// 기본 생성자의 실제 부착은 맨 오 워 전용 표 쓰기보다 먼저다.
void RawFlyerProcess::Construct(FlyerProcessFields& fields,Sid parent,Sid home,std::uint32_t speed,float parameter) const {
    Time(fields,32,0);Time(fields,40,0);fields.bytes[kDirty]=0xff;Put(fields.bytes,kInner,0x5019ec);Put(fields.bytes,kParent,0);Put(fields.bytes,kForm,0);
    Put(fields.bytes,0,0x509cc8);Put(fields.bytes,kInner,0x509ca8);Time(fields,64,0);Time(fields,72,0);Put(fields.bytes,84,0);
    if (!globals_.server) Need(hooks_.assertion)(0x2e3);
    Put(fields.bytes,kHome,home.value,2);fields.bytes[kDirty]|=0x1e;Put(fields.bytes,16,speed);Put(fields.bytes,12,0,2);Put(fields.bytes,20,std::bit_cast<std::uint32_t>(parameter));
    fields.bytes[kDirty]|=0x60;Time(fields,32,globals_.now);Put(fields.bytes,24,0);
    Need(hooks_.attach)(fields,parent,globals_.processType,8);
}
void RawFlyerProcess::SpecializeRain(FlyerProcessFields& fields) { Put(fields.bytes,0,0x509f60);Put(fields.bytes,kInner,0x509f40);Time(fields,96,0);Time(fields,104,0);Put(fields.bytes,84,0); } // 생성·부착 뒤의 실제 추가 쓰기다.
// 원본은 생성자가 건드리지 않는+80/+84를 초기화하지 않는다. 전용 첫Frame에서 쓴다.
void RawFlyerProcess::SpecializeSun(FlyerProcessFields& fields) {
    Put(fields.bytes,0,0x509f24);Put(fields.bytes,kInner,0x509f04);Time(fields,104,0);
    std::fill_n(fields.sunTail.begin(),16,std::uint8_t{});Put(fields.bytes,84,0);
}
void RawFlyerProcess::InitializeReceived(FlyerProcessFields& fields,std::int32_t theme) { // 기본 ctor는 미작성 home/위치/속도와 padding을 보존한다.
    Time(fields,32,0);Time(fields,40,0);fields.bytes[kDirty]=0xff;Put(fields.bytes,kInner,0x5019ec);Put(fields.bytes,kParent,0);Put(fields.bytes,kForm,0);
    Put(fields.bytes,0,0x509cc8);Put(fields.bytes,kInner,0x509ca8);Time(fields,64,0);Time(fields,72,0);Put(fields.bytes,12,0,2);Put(fields.bytes,84,0);
    if (theme==1) SpecializeRain(fields);else if (!theme) RawWindFlyer::Specialize(fields);else SpecializeSun(fields); // 각 leaf의 추가 타이머만 초기화하며 첫Frame 초기화는 호출하지 않는다.
}
std::uint32_t RawFlyerProcess::ReadData(FlyerProcessFields& fields,std::span<const std::uint8_t> packet) {
    std::uint32_t parent{};if (packet.size()<9) throw std::out_of_range("비행 수신의 실제9BYTE 머리 부족");const auto dirty=packet[8];std::size_t offset=9;ObserveFlyerData(fields); // before의 sign-extend dirty/1/0 인자는 빈 observer에서 소비하지 않는다.
    const auto require=[&](std::size_t width) { if (offset>packet.size() || width>packet.size()-offset) throw std::out_of_range("비행 수신의 선택된 dirty 필드 부족"); }; // 앞선 부분 쓰기는 예외에서도 유지한다.
    if (dirty&1U) { require(4);parent=Read(packet,offset);offset+=4; }
    for (std::size_t index=0;index<kDataOffsets.size();++index) if (dirty&(2U<<index)) { require(kDataWidths[index]);std::copy_n(packet.begin()+offset,kDataWidths[index],fields.bytes.begin()+kDataOffsets[index]);offset+=kDataWidths[index]; } // 반복은 원문 WORD/DWORD/8BYTE의 순서/비트만 복원한다.
    ObserveFlyerData(fields);return parent; // after observer도 현재 VT를 다시 선택하며 dirty/inner 소유권은 보존한다.
}
void RawFlyerProcess::DestructFields(FlyerProcessFields& fields) { Put(fields.bytes,kInner,0x5019ec);Put(fields.bytes,kForm,0xffffcfc7U);Put(fields.bytes,kParent,0xffffcfc7U); } // Native−12345 두DWORD는 WORD로 줄이지 않는다.
// 첫 실제 nondead Form만 갱신하며 없으면 원문처럼 자연 반환한다.
void RawFlyerProcess::Bind(Sid flyer,Sid home) const {
    const auto type=pool_.Slot(flyer)[10];if (type>=types_.size()) throw std::out_of_range("비행 홈 연결 TYPE 범위 오류");
    if (!(types_[type].flags2&0x100000U)) Need(hooks_.assertion)(0x904);
    const auto form=Need(hooks_.findProcess)(flyer,globals_.processType);if (!form.value) return;
    auto& fields=Need(hooks_.fields)(form);fields.bytes[kDirty]|=2;Put(fields.bytes,kHome,home.value,2);
}
// genus 진단은 무조건 선택되며 dead/free 판단과 finder 뒤 checking은 정상 복귀 이후의 현재값을 읽는다.
Sid RawFlyerProcess::Home(Sid flyer) const {
    const auto type=pool_.Slot(flyer)[10];if (type>=types_.size()) throw std::out_of_range("비행 홈 조회 TYPE 범위 오류");
    if (!(types_[type].flags2&0x100000U)) Need(hooks_.assertion)(0x8ee);
    if (pool_.Slot(flyer)[11]&3U) return Sid{};
    const auto form=Need(hooks_.findProcess)(flyer,globals_.processType); // 실제 첫 타입 조회 결과는 진단 중 새 Form이 생겨도 다시 찾지 않는다.
    if (!form.value) { if (globals_.checking) Need(hooks_.assertion)(0x8f5);return Sid{}; }
    return Need(hooks_.fields)(form).Home(); // 원문 NULL inner의 주소+a 접근은 호스트 장치 오류로 남기며 가짜 홈0으로 성공시키지 않는다.
}
void RawFlyerProcess::Frame(FlyerProcessFields& fields) const { Need(hooks_.frame)(fields); } // 아직 미복원인 큰 Frame을 조용히 생략하지 않는다.
void RawFlyerProcess::Notify(FlyerProcessFields& fields,std::uint32_t message,std::uint32_t info1,std::uint32_t info2) const { Need(hooks_.notify)(fields,message,info1,info2); } // 실제 선택 통지를 유지한다.
void RawFlyerProcess::ParentDying(FlyerProcessFields& fields) const { Need(hooks_.parentDying)(fields); } // 홈 파괴 통지를 유지한다.
// 파편 효과 뒤 부모가 바뀌면 원본처럼 그 현재 부모를 다시 읽어 삭제한다.
void RawFlyerProcess::Finish(FlyerProcessFields& fields) const {
    if (!globals_.server) Need(hooks_.assertion)(0x41d);Need(hooks_.pieces)(fields.Parent());Need(hooks_.destroy)(fields.Parent(),8);
}
// 파생 writer는 기본 직렬화를 사용한다. 추가96/104 타이머는 이 패킷에 포함되지 않는다.
std::uint16_t RawFlyerProcess::WriteProcess(FlyerProcessFields& fields,std::span<std::uint8_t> packet,bool includeParent,bool force,std::uint8_t protocol,std::uint8_t message) {
    const auto parent=(includeParent || force) ? Read(fields.bytes,kParent) : 0U;const auto dirty=force ? 0xffU : fields.bytes[kDirty];
    std::size_t required=9+(parent ? 4U : 0U); // 실제 선택 필드들의 총 길이를 쓰기 전에 검사한다.
    for (std::size_t index=0;index<kDataWidths.size();++index) if (dirty&(2U<<index)) required+=kDataWidths[index]; // 수신과 같은 원문의 WORD2/DWORD3/8BYTE2 폭이다.
    if (packet.size()<required) throw std::out_of_range("비행 프로세스 상태 패킷 크기 오류");if (force) fields.bytes[kDirty]=0xff;
    packet[0]=protocol;packet[1]=message;Put(packet,4,Read(fields.bytes,kForm));std::size_t at=9;
    if (parent) { fields.bytes[kDirty]|=1;Put(packet,at,parent);at+=4; }
    for (std::size_t index=0;index<kDataOffsets.size();++index) if (fields.bytes[kDirty]&(2U<<index)) { std::copy_n(fields.bytes.begin()+static_cast<std::ptrdiff_t>(kDataOffsets[index]),kDataWidths[index],packet.begin()+static_cast<std::ptrdiff_t>(at));at+=kDataWidths[index]; } // 같은 메타데이터의 바이트 복사로 NaN/미정8BYTE도 보존한다.
    Put(packet,2,static_cast<std::uint32_t>(at),2);packet[8]=fields.bytes[kDirty];fields.bytes[kDirty]=0;return static_cast<std::uint16_t>(at);
}
const SidPool& RawFlyerProcess::Pool() const { return pool_; } // 실제 Host와 같은 풀이다.
FlyerProcess::FlyerProcess(const RawFlyerProcess& logic,FlyerProcessFields fields):logic_(logic),fields_(std::move(fields)) {} // 아직 생성하지 않은 caller 이미지다.
void FlyerProcess::RunFrame() { logic_.Frame(fields_); } // Kernel이 실제 시각에 부른다.
void FlyerProcess::OnAttached() { Put(fields_.bytes,kForm,Form().value);Put(fields_.bytes,kParent,Parent().value); } // 실제 부착 결과를 반영한다.
void FlyerProcess::Notify(std::uint32_t message,std::uint32_t info1,std::uint32_t info2) { logic_.Notify(fields_,message,info1,info2); } // 실제 의존 통지다.
void FlyerProcess::OnParentDying(Sid) { logic_.ParentDying(fields_); } // 원문은 전달된 부모 인자를 사용하지 않는다.
FlyerProcessFields& FlyerProcess::Fields() { return fields_; } // 같은 현재 이미지다.
const FlyerProcessFields& FlyerProcess::Fields() const { return fields_; } // 같은 현재 이미지다.
std::uint16_t FlyerProcess::WritePacket(std::span<std::uint8_t> packet,bool parentChanged,bool force) { OnAttached();return RawFlyerProcess::WriteProcess(fields_,packet,parentChanged,force); } // 부착 전 writer에도 현재 Host SID를 쓴다.
RawFlyerPostPop::RawFlyerPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const FlyerPostPopType> details,const bool& server,FlyerPostPopHooks hooks):pool_(pool),types_(types.begin(),types.end()),details_(details.begin(),details.end()),server_(server),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || details.size()!=188) throw std::invalid_argument("비행 후처리는 PATCH188 타입만 복원했습니다");
}
bool RawFlyerPostPop::Handles(Sid sid) const { const auto table=Read(pool_.Slot(sid),0);return table==0x50a388U || table==0x50a1e8U || table==0x50a050U; } // 세실제 전용 가상 표다.
// 실제0045e6b0은 TYPE 포인터를 처음 snapshot하고 효과 뒤에도 그 TYPE의 현재 프레임 수를 사용한다.
void RawFlyerPostPop::PostPop(Sid sid,std::uint32_t flags) const {
    if (!Handles(sid)) throw std::invalid_argument("다른 비행 파생 PostPop 미복원");
    const bool sun=Read(pool_.Slot(sid),0)==0x50a1e8U; // 효과가 가상 표를 바꿔도 진입한 전용 몸체를 유지한다.
    const bool wind=Read(pool_.Slot(sid),0)==0x50a050U; // 더스트 데빌은생산기가비행프로세스를따로부착한다.
    if (wind && (flags&1U) && !Need(hooks_.hasRegular)(sid,0) && Need(hooks_.allocateWindRegular)()) Need(hooks_.createWindRegular)(sid,0,0.25f);
    if (sun && (flags&1U)) {
        const auto& original=details_.at(pool_.Slot(sid)[10]); // 홈/확보/부착 전에 읽은 TYPE 포인터다.
        if (server_) {
            const Sid home{static_cast<std::uint16_t>(Read(pool_.Slot(sid),12,2))}; // 두 가상 공간 효과가 공유하는 최초 홈이다.
            Need(hooks_.unpop)(home,0);Put(pool_.AllocatedBytes(home),36,10);Need(hooks_.repop)(home,0);
            if (Need(hooks_.allocateSunProcess)()) Need(hooks_.createSunProcess)(sid,Sid{static_cast<std::uint16_t>(Read(pool_.Slot(sid),12,2))},original.speed,0.6f);
        }
        if (Need(hooks_.allocateSound)()) Need(hooks_.createSound)(sid,original.impactSound,original.activeSound,1);
    }
    if (!sun && !wind && (flags&1U) && server_ && Need(hooks_.allocateRainProcess)()) { const auto home=Sid{static_cast<std::uint16_t>(Read(pool_.Slot(sid),12,2))};const auto speed=details_.at(pool_.Slot(sid)[10]).speed;Need(hooks_.createRainProcess)(sid,home,speed,0.5f); }
    const auto type=pool_.Slot(sid)[10];const auto& details=details_.at(type); // 프로세스 생성 효과가 끝난 현재 TYPE다.
    if (flags&1U) {
        if (std::bit_cast<std::int32_t>(Read(pool_.Slot(sid),36))<details.frames && (types_.at(type).flags1&0x400000U)) {
            const auto shadow=Need(hooks_.create)(pool_.Slot(sid)[10],2);auto raw=pool_.AllocatedBytes(shadow);Put(raw,36,FrameSum(details.frames,pool_.Slot(sid)));raw[40]|=8;Put(pool_.AllocatedBytes(sid),8,shadow.value,2);
            const auto parent=pool_.Slot(sid);Need(hooks_.pop)(shadow,Position(parent,14),Position(parent,18),0);
        }
        Need(hooks_.initializeUnit)(sid);
    }
    const auto shadow=Sid{static_cast<std::uint16_t>(Read(pool_.Slot(sid),8,2))};
    if (shadow.value) {
        Put(pool_.AllocatedBytes(shadow),36,FrameSum(details.frames,pool_.Slot(sid)));Need(hooks_.unpop)(shadow,0);
        const auto x=Position(pool_.Slot(sid),14),y=Position(pool_.Slot(sid),18);const auto offset=Need(hooks_.shadowOffset)(sid); // 원문은+c8 전에 현재 좌표 두 개를 snapshot한다.
        Need(hooks_.pop)(shadow,static_cast<float>(static_cast<double>(x)+offset[0]),static_cast<float>(static_cast<double>(y)+offset[1]),0);
    }
    Need(hooks_.damageable)(sid,flags);
}
const SidPool& RawFlyerPostPop::Pool() const { return pool_; } // 현재 공통 장부와 같은 풀이다.
}
