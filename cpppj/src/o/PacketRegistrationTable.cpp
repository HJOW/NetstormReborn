// 단일 등록표와 실제 콜백 pin은 호스트 수명 경계이며 원문 분배를 두 번 호출하지 않는다.
#include "o/PacketRegistrationTable.h"
#include "o/RawPacketLengths.h"
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace netstorm::o { // 원문 receiver/observer는 기존 RawPacketDispatch 호출 위치에서만 실행한다.
// 실제 전체 owner를 먼저 보관하고 마지막 콜백 복귀에 실행 수를 줄인다.
struct PacketRegistrationLease::CallbackPin {
    std::shared_ptr<PacketRegistrationLease> lease; // 실행 중 대여의 고정 주소다.
    std::shared_ptr<void> owner; // 실제 Session/World/소비 모듈 전체가 들어 있는 caller owner다.
    CallbackPin(std::shared_ptr<PacketRegistrationLease> source,std::shared_ptr<void> lifetime):lease(std::move(source)),owner(std::move(lifetime)) { ++lease->calls_; } // 성공한 두 strong pin 뒤에만 실행 수를 올린다.
    ~CallbackPin() { --lease->calls_; } // owner의 최종 소멸보다 먼저 현재 callback 완료를 기록한다.
};
// 원문 정적 길이와 BSS 함수/버전 초기값을 한 번 만든다.
PacketRegistrationTable::PacketRegistrationTable(OriginalEdition edition):lastType_(edition==OriginalEdition::Patch1078 ? 137U:126U) {
    for (std::size_t index=0;index<=lastType_;++index) entries_[index].length=edition==OriginalEdition::Patch1078 ? kPatchPacketLengths[index]:kCdPacketLengths[index]; // 판본의 실제 지원 타입까지만 복사한다.
}
// 표를 공유하는 backend의 판본 검증에만 쓴다.
const PacketRegistration& PacketRegistrationTable::Registration(std::uint32_t type) const { if (type>lastType_) throw std::out_of_range("패킷 observer 표의 실제 할당 범위 밖");return entries_[type]; } //Native4dedb0의Last+1할당/0초기화범위만읽으며0은정상슬롯이다.
std::uint32_t PacketRegistrationTable::LastType() const { return lastType_; }
// 원문의 새 전역 함수/버전 표 할당을 현재 공유 owner 안에서 수행하며 정적 길이표는 바꾸지 않는다.
void PacketRegistrationTable::ResetCallbacks() {
    if (currentIdentity_ || ConnectionCallbackActive()) throw std::logic_error("Session 대여/콜백 실행 중 패킷 전역 초기화 금지");
    for (std::size_t type=0;type<=lastType_;++type) { // 원문 last+1개 receiver/observer·두 버전 BYTE를 같은 순서로 비운다.
        PacketRegistration empty;empty.length=entries_[type].length;std::swap(entries_[type],empty); // 호스트 함수 저장소는 반납하되 기존 실제 길이/표 주소를 유지한다.
    }
}
// 등록을 반납한 콜백도 실제 owner를 아직 빌리므로 상위 정리 거부에 포함한다.
bool PacketRegistrationTable::ConnectionCallbackActive() const noexcept {
    const auto current=current_.lock();const auto retiring=retiring_.lock();return (current && current->calls_) || (retiring && retiring->calls_);
}
// snapshot 복사/할당이 실패하면 기존 표와 활성 대여는 그대로 남는다.
std::shared_ptr<PacketRegistrationLease> PacketRegistrationTable::BindSession(const std::shared_ptr<void>& owner) {
    if (!owner) throw std::invalid_argument("등록 대여는 실제 Session 전체 owner가 필요합니다");
    if (currentIdentity_) throw std::logic_error("패킷 등록 Session 중복 대여");
    if (const auto retiring=retiring_.lock();retiring && retiring->calls_) throw std::logic_error("이전 Session 콜백 복귀 전 새 등록 대여 금지");
    auto lease=std::shared_ptr<PacketRegistrationLease>(new PacketRegistrationLease(shared_from_this(),owner));current_=lease;currentIdentity_=lease.get();return lease;
}
// 등록 API만 현재 세대의 수명 표식을 함께 쓴다. 분배 중 raw observer를 별도로 중계하지 않는다.
void PacketRegistrationTable::Touch(std::uint32_t type) {
    if (auto current=current_.lock();current && current->active_) { current->touched_.set(type);entries_[type].registrationLease=current;entries_[type].leased=true;return; }
    if (currentIdentity_) throw std::logic_error("등록 대여 반납 도중 전역 등록 수정 금지");
    if (const auto retiring=retiring_.lock();retiring && retiring->calls_) throw std::logic_error("반납된 Session 콜백 안에서 전역 등록 수정 금지");
    entries_[type].registrationLease.reset();entries_[type].leased=false;
}
// 실제 현재 표를 모두 snapshot하되 수정/반납은 touched 슬롯만 수행한다.
PacketRegistrationLease::PacketRegistrationLease(std::shared_ptr<PacketRegistrationTable> table,const std::shared_ptr<void>& owner):table_(std::move(table)),owner_(owner),previous_(table_->entries_) {}
// 외부에서 명시 반납했으면 아무 것도 바꾸지 않는다.
PacketRegistrationLease::~PacketRegistrationLease() { Detach(); }
// swap은 할당하지 않으므로 생성 실패/World 반납 경로에서도 예외를 만들지 않는다.
void PacketRegistrationLease::Detach() noexcept {
    if (!active_) return;active_=false;
    if (table_->currentIdentity_!=this) return; // 마지막 shared 소멸 중 weak.lock 실패도 자기 표 복원을 생략하지 않는다.
    static_assert(std::is_nothrow_swappable_v<PacketRegistration>); // callback 반납 중 새 할당/예외를 허용하지 않는다.
    for (std::size_t type=1;type<=table_->lastType_;++type) if (touched_.test(type)) std::swap(table_->entries_[type],previous_[type]); // 자기 세대가 바꾸지 않은 전역/슬롯은 보존한다.
    table_->retiring_=weak_from_this();table_->current_.reset();table_->currentIdentity_=nullptr;
}
// 함수 사본뿐 아니라 그 함수가 참조하는 실제 월드/Session을 함께 pin한다.
std::shared_ptr<void> PacketRegistrationLease::Pin() {
    if (!active_) throw std::logic_error("반납된 Session 등록 콜백 호출");auto owner=owner_.lock();if (!owner) throw std::logic_error("Session 전체 owner보다 오래 남은 패킷 등록");return std::make_shared<CallbackPin>(shared_from_this(),std::move(owner));
}
// 진단/대여 생성 관문에서만 현재 상태를 읽는다.
bool PacketRegistrationLease::Active() const { return active_; }
// 복사한 registration도 자신이 등록된 같은 세대의 owner를 pin해야 한다.
std::shared_ptr<void> PacketRegistration::Pin() const {
    if (!leased) return {};const auto lease=registrationLease.lock();if (!lease) throw std::logic_error("이미 반납된 등록 세대의 콜백");return lease->Pin();
}
}
