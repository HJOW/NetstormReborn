// 원문 전역 수신/관찰/버전/길이 표를 실제 여러 backend가 공유하고 짧은 Session만 대여한다.
#pragma once
#include "o/RawPacketDispatch.h"
#include <bitset>
#include <memory>

namespace netstorm::o { // backend의 now/close/free hooks는 공유 표 안으로 옮기지 않는다.
class PacketRegistrationLease; // 현재 Session과 실행 중 콜백의 독립 수명 대여다.
//호스트는256슬롯을보유하지만원문실제할당/정상조회범위는0..Last다. 기본길이는판본별로한번설정한다.
class PacketRegistrationTable final:public std::enable_shared_from_this<PacketRegistrationTable> {
public:
    explicit PacketRegistrationTable(OriginalEdition edition); // receiver/observer/version은0, 원문 길이만 초기화한다.
    PacketRegistrationTable(const PacketRegistrationTable&)=delete; // 활성 세대/원문 표를 값 복제로 분리하지 않는다.
    PacketRegistrationTable& operator=(const PacketRegistrationTable&)=delete; // 같은 표를 빌린 backend의 고정 주소를 유지한다.
    PacketRegistrationTable(PacketRegistrationTable&&)=delete; // currentIdentity와 공유 owner의 주소를 이동하지 않는다.
    PacketRegistrationTable& operator=(PacketRegistrationTable&&)=delete; // 생존 Session 대여를 다른 표로 옮기지 않는다.
    const PacketRegistration& Registration(std::uint32_t type) const; //원문복제의현재observer를0..Last에서읽는다.0은정상NULL슬롯이고호스트256물리폭으로범위를넓히지않는다.
    std::uint32_t LastType() const; // 다른 판본 표의 잘못된 공유를 생성 시 거부하는 한도다.
    void ResetCallbacks(); // R00482b60처럼 지원 슬롯0..last의 함수/버전을0으로 만들고 같은 길이표/owner는 보존한다. Session 대여/실행 중에는 거부한다.
    bool ConnectionCallbackActive() const noexcept; // 활성/반납 세대의 콜백 중에는 전체 Runtime 정리를 시작하지 않는다.
    std::shared_ptr<PacketRegistrationLease> BindSession(const std::shared_ptr<void>& owner); // 실제 Session 전체 owner를 약하게 빌린다. 진행 중/중복 대여는 거부한다.
private:
    friend class RawPacketDispatch; // 기존 등록 API만 실제 슬롯을 수정한다.
    friend class PacketRegistrationLease; // 자기 세대가 바꾼 슬롯만 noexcept 복원한다.
    void Touch(std::uint32_t type); // 현재 대여의 변경 슬롯과 receiver/observer pin을 기록한다.
    std::array<PacketRegistration,256> entries_; // Local/TCP/Editor가 실제 함께 조회하는 원문 표 한 개다.
    std::uint32_t lastType_; // PATCH137/CD126의 기존 한도다.
    std::weak_ptr<PacketRegistrationLease> current_,retiring_; // 활성 등록 세대와 아직 실행 중인 반납 세대다.
    PacketRegistrationLease* currentIdentity_{}; // 마지막 shared 해제 중에도 자기 세대인지 비교만 한다. 역참조하지 않는다.
};
// 약한 Session owner 때문에 Session→lease→Session의 강한 순환을 만들지 않는다.
class PacketRegistrationLease final:public std::enable_shared_from_this<PacketRegistrationLease> {
public:
    ~PacketRegistrationLease(); // 정상/예외 파괴에서도 자기 활성 세대만 반납한다.
    PacketRegistrationLease(const PacketRegistrationLease&)=delete; // 한 세대의 snapshot/반납 권한을 복제하지 않는다.
    PacketRegistrationLease& operator=(const PacketRegistrationLease&)=delete; // callback이 빌린 세대 주소를 유지한다.
    PacketRegistrationLease(PacketRegistrationLease&&)=delete; // shared callback pin이 가리키는 대여를 이동하지 않는다.
    PacketRegistrationLease& operator=(PacketRegistrationLease&&)=delete; // 활성 세대와 실행 수를 대입하지 않는다.
    void Detach() noexcept; // 새 세대나 다른 전역 표를 지우지 않고 변경 슬롯만 원래 값으로 복원한다.
    std::shared_ptr<void> Pin(); // 콜백과 실제 Session 전체 owner를 최외곽 복귀까지 유지한다.
    bool Active() const; // 종료된 대여를 현재 수신기로 다시 사용하지 않는다.
private:
    friend class PacketRegistrationTable; // 표만 snapshot/활성 상태를 처음 만들 수 있다.
    struct CallbackPin; // Session 전체와 이 대여를 함께 유지하고 실행 수를 반납한다.
    PacketRegistrationLease(std::shared_ptr<PacketRegistrationTable> table,const std::shared_ptr<void>& owner);
    std::shared_ptr<PacketRegistrationTable> table_; // 표가 backend/Session보다 먼저 사라지지 않는 owner다.
    std::weak_ptr<void> owner_; // 현재 Session/World 전체의 실제 수명이며 단순 raw this 표식이 아니다.
    std::array<PacketRegistration,256> previous_; // 등록 시작 전 표이며 생성 실패 시 활성 표를 건드리지 않는다.
    std::bitset<256> touched_; // 이 세대의 실제 등록 API가 바꾼 슬롯만 복원한다.
    std::size_t calls_{}; // detached 콜백이 끝나기 전 새 세대 등록/수정의 재진입을 거부한다.
    bool active_{true}; // Detach는 한 번만 현재 표를 복원한다.
};
}
