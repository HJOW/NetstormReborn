// 생성과 삭제는 같은 분류 본문을 사용하며 외부 효과 뒤 현재 목록/본체를 다시 읽는다.
#include "o/RawAiObjectNotice.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr double kGuardDistanceSquared=180.5,kLostCarrierDelay=30.0; //00501700/00501708의 실제 QWORD다.
// 원본 비정렬 DWORD를 현재 바이트에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("AI 통지 DWORD 물리 범위");std::uint32_t value{};
    // 원본의 네 BYTE만 조합한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(8*index);return value;
}
// 주변 BYTE를 보존하고 지정 DWORD만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("AI 통지 DWORD 쓰기 물리 범위");
    // 낮은 바이트부터 지정 폭만 쓴다.
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 목록 비교의 signed DWORD를 보존한다.
std::int32_t Signed(std::span<const std::uint8_t> bytes,std::size_t at) { return std::bit_cast<std::int32_t>(Read(bytes,at)); }
// 타이머는 float로 축소하지 않고 같은 QWORD에 쓴다.
void Time(std::span<std::uint8_t> bytes,std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(bytes,at,static_cast<std::uint32_t>(bits));Put(bytes,at+4,static_cast<std::uint32_t>(bits>>32)); }
// 로그 좌표의 CRT signed64 절삭 후 낮은 DWORD다.
std::int32_t Truncate(float value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) throw std::out_of_range("AI 통지 로그 좌표 변환 범위");return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value))); }
// 도달한 실제 하위가 없으면 그 위치에서 명시 오류로 남긴다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("AI 생성/삭제 통지의 실제 하위 미연결");return effect; }
}
// 원본의 실제 저장소와 현재 조회기만 빌린다.
RawAiObjectNotice::RawAiObjectNotice(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const std::uint8_t> players,const double& now,const std::uint32_t& outpostType,std::function<bool()> disabled,AiObjectNoticeHooks hooks)
    :memory_(memory),pool_(pool),types_(types),players_(players),now_(now),outpostType_(outpostType),disabled_(std::move(disabled)),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || players.size()!=1620 || !disabled_) throw std::invalid_argument("AI 통지의 실제 PATCH188/Player9/현재 모드 누락");
}
// 실제 본체의 수명/크기를 검사한다.
std::span<std::uint8_t> RawAiObjectNotice::Body(std::uint32_t token) const { auto bytes=memory_.Bytes(token);if (bytes.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 통지의 실제0xb08 본체 누락");return bytes; }
// signed count는 원본의 비양수 자연 루프를 보존한다.
std::int32_t RawAiObjectNotice::Count(std::uint32_t token,std::size_t offset) const { return Signed(Body(token),offset+8); }
// 논리 count와 별도로 현재 실제 할당 안의 인덱스를 확인한다.
std::uint32_t RawAiObjectNotice::Item(std::uint32_t token,std::size_t offset,std::int32_t index) const { if (index<0) throw std::out_of_range("AI 통지 음수 목록 인덱스");return Read(memory_.Bytes(Read(Body(token),offset)),static_cast<std::size_t>(index)*4); }
// 실제 목록의 첫 일치에서만 검색을 끝낸다.
bool RawAiObjectNotice::Contains(std::uint32_t token,std::size_t offset,std::uint32_t value) const {
    // 외부에서 공유하는 count와 포인터를 각 방문 시 다시 읽는다.
    for (std::int32_t index=0;index<Count(token,offset);++index) if (Item(token,offset,index)==value) return true;return false;
}
// 일반 추가는 중복을 허용하고 용량이 찼으면 포인터를 조회하지 않는다.
void RawAiObjectNotice::Append(std::uint32_t token,std::size_t offset,std::uint32_t value) const {
    auto raw=Body(token);const auto count=Count(token,offset);if (count>=Signed(raw,offset+4)) return;if (count<0) throw std::out_of_range("AI 통지 음수 추가 인덱스");Put(memory_.Bytes(Read(raw,offset)),static_cast<std::size_t>(count)*4,value);Put(raw,offset+8,static_cast<std::uint32_t>(count)+1);
}
// full 판정은 중복 검색보다 먼저 수행한다.
void RawAiObjectNotice::AppendUnique(std::uint32_t token,std::size_t offset,std::uint32_t value) const { if (Count(token,offset)>=Signed(Body(token),offset+4) || Contains(token,offset,value)) return;Append(token,offset,value); }
// 모든 중복만 압축하며 count 밖의 이전 꼬리를0으로 지우지 않는다.
void RawAiObjectNotice::Remove(std::uint32_t token,std::size_t offset,std::uint32_t value) const {
    std::int32_t removed{};
    // 생존 항목의 상대 순서와 원본의 현재 count 재조회를 유지한다.
    for (std::int32_t index=0;index<Count(token,offset);++index) { const auto item=Item(token,offset,index);if (item==value) ++removed;else Put(memory_.Bytes(Read(Body(token),offset)),static_cast<std::size_t>(index-removed)*4,item); }
    Put(Body(token),offset+8,static_cast<std::uint32_t>(Count(token,offset))-static_cast<std::uint32_t>(removed));
}
// 제외 능력이 없으면 어떤 로그/목록도 선택하지 않는다.
void RawAiObjectNotice::ExcludeGeyser(std::uint32_t token,std::uint32_t geyser) const {
    if (!(Read(Body(token),0x248)&8U)) return;
    if (Count(token,0x330)>0) { Append(token,0x33c,geyser);Remove(token,0x330,geyser);Need(hooks_.log)(token,{AiObjectLogKind::MadeGeyserIneligible,geyser}); }
    else Need(hooks_.log)(token,{AiObjectLogKind::CannotExcludeGeyser,geyser});
}
// 실제10620은 fadd QWORD now 뒤 QWORD에 저장한다. 디컴파일의 float now 표기를 따르지 않는다.
void RawAiObjectNotice::Built(std::uint32_t token,Sid sid) const {
    const auto object=pool_.Slot(sid);const auto x=std::bit_cast<float>(Read(object,14)),y=std::bit_cast<float>(Read(object,18));const auto iy=Truncate(y),ix=Truncate(x);const auto type=pool_.Slot(sid)[10];
    auto raw=Body(token);const auto rate=static_cast<double>(std::bit_cast<float>(Read(raw,0x228)));Time(raw,0x5a8,rate+now_);Time(raw,0x5b0,(rate+1.0)+now_);Need(hooks_.log)(token,{AiObjectLogKind::Built,sid.value,type,ix,iy});
}
// 현재 모드/Player 포인터를 읽는 바깥 원본 몸체다.
void RawAiObjectNotice::Notice(std::uint32_t owner,Sid sid,bool added) const {
    if (disabled_() || std::bit_cast<std::int32_t>(owner)<1) return;if (owner>=9) throw std::out_of_range("AI 통지 Player9 범위");const auto token=Read(players_,static_cast<std::size_t>(owner)*180+0xb0);if (token) Update(token,sid,added);
}
// 본문은 원본 genus 한 번 읽기를 보존하고 각 하위 효과 뒤의 실제 본체/타입/목록을 다시 읽는다.
void RawAiObjectNotice::Update(std::uint32_t token,Sid sid,bool added) const {
    auto raw=Body(token);const auto type=pool_.Slot(sid)[10];const auto counter=0x6d4U+std::uint32_t(type)*4;Put(raw,counter,Read(raw,counter)+(added ? 1U : 0xffffffffU));
    if (type>=types_.size()) throw std::out_of_range("AI 통지 현재 TYPE 범위");const auto genus=types_[type].flags2;
    if (!added) {
        if (genus&0x200000U) { if (Read(raw,0x2a0)==sid.value) Put(raw,0x2a0,0);if (Read(raw,0x29c)==sid.value) Put(raw,0x29c,0); }
        if (genus&0x30000U) { const auto targets=Need(hooks_.carrierTargets)(sid);if (targets[0]) ExcludeGeyser(token,targets[0]);if (Contains(token,0x384,sid.value)) { Remove(token,0x384,sid.value);Time(Body(token),0x5e0,now_+kLostCarrierDelay); } }
        raw=Body(token);if ((genus&0x200U) && Read(raw,0x294)==sid.value) Put(raw,0x294,0);Remove(token,0x36c,sid.value);Remove(token,0x378,sid.value);
        raw=Body(token);if (Read(raw,0x3f8)==sid.value) Put(raw,0x3f8,0);if (Read(raw,0x3fc)==sid.value) Put(raw,0x3fc,0);
    }
    if (genus&0x400000U) { raw=Body(token);if (added) Put(raw,0x2a4,sid.value);else if (Read(raw,0x2a4)==sid.value) Put(raw,0x2a4,0); }
    if (genus&0x4000000U) {
        if (!added) Remove(token,0x360,sid.value);
        else { bool nearby=false;const auto temple=Read(Body(token),0x294);if (temple && Need(hooks_.distanceSquared)(sid,temple)<kGuardDistanceSquared) nearby=true;
            // 첫 근접 판정 뒤에도 원본처럼 현재 목록의 모든 거리 조회를 진행한다.
            for (std::int32_t index=0;index<Count(token,0x300);++index) if (Need(hooks_.distanceSquared)(sid,Item(token,0x300,index))<kGuardDistanceSquared) nearby=true;
            if (nearby) AppendUnique(token,0x360,sid.value);
        }
    }
    if ((genus&0x30000U) && !(genus&0x200000U)) { if (added) Append(token,0x2dc,sid.value);else Remove(token,0x2dc,sid.value);Built(token,sid); }
    if (genus&0x800U) { if (added) Append(token,0x2f4,sid.value);else Remove(token,0x2f4,sid.value);Time(Body(token),0x5b8,0.0); }
    if ((genus&0x4000U) && pool_.Slot(sid)[10]!=outpostType_) {
        if (added) Append(token,0x300,sid.value);else Remove(token,0x300,sid.value);
        if (!added) {
            Remove(token,0x30c,sid.value);raw=Body(token);Put(raw,0x3e0,0);Put(raw,0x3e4,6);Put(raw,0x3f0,1);Put(raw,0x3f4,0);Put(raw,0x3fc,0);Put(raw,0x400,0);Put(raw,0x3ec,0);Remove(token,0x318,sid.value);
            // 남은 원소 시설을 원래 순서로 방문하며 대기 목록의 중복만 막는다.
            for (std::int32_t index=0;index<Count(token,0x300);++index) { const auto item=Item(token,0x300,index);if (!Contains(token,0x30c,item)) Append(token,0x30c,item); }
        } else Append(token,0x30c,sid.value);
        Put(Body(token),0x2c8,Signed(Body(token),0x3b0)<=Count(token,0x300) ? 1U : 0U);
    }
    if (genus&0x40800U) {
        if (!added) { if (Contains(token,0x2e8,sid.value)) Remove(token,0x2e8,sid.value);return; }
        if (Signed(Body(token),0x2ec)<=Count(token,0x2e8)) Remove(token,0x2e8,Item(token,0x2e8,0));Append(token,0x2e8,sid.value);raw=Body(token);Put(raw,0x3c8,Read(raw,0x3c8)+3U);
    }
    if (added && (genus&0x4200U)) Time(Body(token),0x5c0,0.0);
}
// 실제 Pop/삭제 처리기와의 풀 혼합을 거부할 때 사용한다.
const SidPool& RawAiObjectNotice::Pool() const { return pool_; }
}
