// 힐 주문의 signed DWORD 감김/0방향 평균과 효과 사이 현재값 재조회를 보존한다.
#include "o/RawBombHeal.h"
#include "o/RawSquidHitPoints.h"
#include <bit>
#include <new> //실제 다음 Regular의 확보 실패는 생성만 생략하고 삭제를 계속한다.
#include <stdexcept>
#include <utility>

namespace netstorm::o { //원본 가상 함수 주소를 호스트 함수 포인터로 직접 실행하지 않는다.
namespace { //원문 비트와 선택한 효과 요구를 이 모듈 안에 한정한다.
constexpr std::uint32_t kAllowed=0x81574204U,kWalking=0x200000U,kNoBurst=0x1000004U; //41ebb0/41ebd0의 실제 genus 마스크다.
template<class F> F HealNeed(const F& hook) { if (!hook) throw std::logic_error("힐 주문의 선택된 실제 하위 미연결");return hook; } //선택한 함수 사본을 정상/예외 복귀까지 유지한다.
}
RawBombHeal::RawBombHeal(SidPool& pool,std::span<const RiftTypeRecord> types,BombHealHooks hooks):pool_(pool),types_(types),hooks_(std::move(hooks)) { //다른 판본의HP 폭/TYPE 배치를 혼용하지 않는다.
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("힐 주문 PATCH/188TYPE 계약 오류");
}
std::uint32_t RawBombHeal::Read(Sid sid,std::size_t offset,std::size_t width) const { //free/dead나 추가 자격 필터 없이 원문 폭으로 읽는다.
    const auto raw=pool_.Slot(sid);std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(raw[offset+byte])<<(8*byte); //지정한 바이트만 little endian으로 조합한다.
    return value;
}
const RiftTypeRecord& RawBombHeal::Type(Sid sid) const { //외부 효과가 바꾼 현재 종류를 호출 위치마다 확인한다.
    const auto type=Read(sid,10,1);if (type>=types_.size()) throw std::out_of_range("힐 주문 현재TYPE 범위 오류");return types_[type];
}
std::array<float,2> RawBombHeal::Center(Sid sid) const { //정수 발자국의 절반을 빼고 두 결과를 단정도에 저장한다.
    const auto& type=Type(sid);return {static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(sid,14)))-type.footX*0.5),static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(sid,18)))-type.footY*0.5)};
}
std::uint32_t RawBombHeal::MayTarget(Sid target) const { return Type(target).flags2&kAllowed; } //원본은HP 보유/소유자/동맹/현재HP를 추가로 검사하지 않는다.
void RawBombHeal::Apply(Sid parent,Sid target) const { //현재 대상의 각 효과가 끝난 뒤 필요한 필드를 다시 읽는다.
    if (target.value==Read(parent,12,2)) return;
    if (HealNeed(hooks_.hasHitPoints)(target)) {
        const auto health=Read(target,26);const auto maximum=HealNeed(hooks_.maximumHitPoints)(target);
        const auto sum=std::bit_cast<std::int32_t>(health+std::bit_cast<std::uint32_t>(maximum)); //원문 ADD의 DWORD 감김을 유지하고 signed 평균만0방향으로 절삭한다.
        SetRawSquidHitPoints(pool_,target,sum/2);
    }
    const auto raw=pool_.Slot(target);const_cast<std::uint8_t*>(raw.data())[40]&=0xcfU; //현재 extra의 은신/마비 두 비트만 해제한다.
    const auto kind=Read(target,10,1);if (kind==HealNeed(hooks_.fenceType)()) HealNeed(hooks_.fence)(target);
    const auto walking=(Type(target).flags2&kWalking)!=0;const auto table=Read(target,0);
    if (walking) { HealNeed(hooks_.unpop)(target,table,0);HealNeed(hooks_.repop)(target,Read(target,0),0x40); }
    else HealNeed(hooks_.refresh)(target,table);
    if (Type(target).flags2&kNoBurst) return;
    const auto burst=HealNeed(hooks_.burstType)();const auto created=HealNeed(hooks_.create)(burst,2);const auto center=Center(target);
    HealNeed(hooks_.pop)(created,Read(created,0),center[0],center[1],0);
}
const SidPool& RawBombHeal::Pool() const { return pool_; } //같은 현재 Runtime의 물리 자료를 쓰는지 확인한다.
//원문은 event를 읽지 않고 signed count와 현재 A count-1을 비교한 뒤 실제 일반 Regular2를 만든다.
float HandleHealBurstRegular(SidPool& pool,std::span<const RiftTypeFrames> frames,const SquidFrame& setter,SquidProcessHost& host,const std::function<void(Sid)>& refresh,const std::function<void(Sid,std::uint32_t)>& destroy,Sid sid,std::uint32_t count,float payload) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || frames.size()!=188 || &host.Pool()!=&pool) throw std::invalid_argument("힐 버스트 실제 풀/프레임/Host 계약 오류");
    const auto raw=pool.Slot(sid);const auto type=raw[10];if (type>=frames.size()) throw std::out_of_range("힐 버스트 현재TYPE 범위 오류");
    const auto limit=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(frames[type].Run('A').count)-1U);
    if (std::bit_cast<std::int32_t>(count)<limit) {
        std::uint32_t frame{};for (std::size_t byte=0;byte<4;++byte) frame|=std::uint32_t(raw[36+byte])<<(8*byte); //원문은 현재 프레임에1을 더하며 첫 프레임으로 다시 시작하지 않는다.
        setter.Set(sid,std::bit_cast<std::int32_t>(frame+1U),0);HealNeed(refresh)(sid);return payload;
    }
    auto next=std::unique_ptr<RegularProcess>(new(std::nothrow) RegularProcess(2,0.05F)); //실제 현재 Client Host에 붙일 같은 객체를 한 번 확보한다.
    if (next) host.AttachRegular(std::move(next),sid,false,0x50); //원문 일반496e80의 같은 생성자/부착/현재 시각 저장을 실행한다.
    HealNeed(destroy)(sid,0x100048U);return -1.0F;
}
}
