// Player/Subscriber/이름/IP 조회의 실제60바이트 레코드와 두 판본의 검색 순서를 복원한다.
#pragma once
#include "o/RiftType.h"
#include <array>
#include <functional>
#include <span>

namespace netstorm::o {
// 원본 PlayerFinder의 전체 물리 입력이다. 이름 뒤 미작성 바이트와 마지막DWORD도 보존한다.
using PlayerIdentityRecord=std::array<std::uint8_t,60>;
// 현재 전역을 읽는 실제 연결이다. 월드/설정과 별도의 로컬 정보 사본을 만들지 않는다.
struct PlayerIdentityReads {
    std::function<std::uint32_t()> localOwner,subscriber,address; // PATCH540c70/540c74/5953fc·CD50f6c8/50f6cc/5490d8이다.
    std::function<std::string_view()> name; // PATCH540ca8/CD50f6d0이 가리키는 현재 원본 바이트 문자열이다.
    std::function<bool()> battle,editor,metamap; // 각 원본 모드를 호출 위치에서 현재 값으로 조회한다.
    std::function<std::string_view()> unknownName; // 이름 getter 실패의 SOMEONE 포인터이며 실제 내용은 호출자가 공급한다.
};
// PATCH004841c0/CD00401610과 실제 초기화/네 getter다. 채팅·Player·metamap 기록을 직접 빌린다.
class RawPlayerIdentity {
public:
    // Players9·채팅100개는 필수다. metamap 기록40개는 미연결 시 빈 span이며 해당 모드 진입 시 거부한다.
    RawPlayerIdentity(OriginalEdition edition,std::span<const std::uint8_t> players,std::span<const std::uint8_t> meta,
        std::span<const std::uint8_t> chat,PlayerIdentityReads reads);
    // PATCH00484190/CD004015e0: 원본이 쓴 필드만 초기화하며 이름의 뒤·마지막DWORD는 보존한다.
    static void Initialize(PlayerIdentityRecord& record,std::uint32_t mode);
    // 현재 로컬→battle/editor Player1..8→metamap40→채팅100 순서로 실제 레코드를 갱신한다.
    // 반환true는 모든 필드가 완성됐다는 뜻이 아니다. 원본의 단계별 부분완료 조건을 유지한다.
    bool Resolve(PlayerIdentityRecord& record) const;
    // PATCH004844f0/CD004019e0: owner 번호를 받아 실제 subscriber DWORD를 반환한다.
    std::uint32_t SubscriberForOwner(std::uint32_t owner) const;
    // PATCH00484550/CD00401ad0: subscriber를 받아 owner 번호를 반환한다.
    std::uint32_t OwnerForSubscriber(std::uint32_t subscriber) const;
    // PATCH00484520/CD00401aa0: subscriber를 받아 주소DWORD를 반환한다.
    std::uint32_t AddressForSubscriber(std::uint32_t subscriber) const;
    // PATCH00484410/CD00401890: subscriber 이름이며 조회 실패 시 현재 unknownName이다.
    std::string NameForSubscriber(std::uint32_t subscriber) const;
    // 실제 메뉴가 같은 판본/기록/전역을 사용하도록 원본 참조를 빌린다. 재복사하지 않는다.
    OriginalEdition Edition() const;
    std::span<const std::uint8_t> PlayerRecords() const;
    std::span<const std::uint8_t> MetaRecords() const;
    const PlayerIdentityReads& CurrentInputs() const;
private:
    // 정렬되지 않은DWORD와 NUL 바이트 문자열을 읽고 원문이 쓴 폭만 갱신한다.
    static std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset);
    static void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value);
    static std::string_view Text(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t capacity);
    static void SetName(PlayerIdentityRecord& record,std::string_view name);
    // C locale의 ASCII 대소문자 비교이며 원본 바이트를 UTF-8 변환하지 않는다.
    static bool Equal(std::string_view first,std::string_view second);
    OriginalEdition edition_; // 현재 원본 판본이다.
    std::span<const std::uint8_t> players_; // 실제9개 Player 원문이다.
    std::span<const std::uint8_t> meta_; // 실제40개 metamap 기록이며 관리자+b70부터의 span이다.
    std::span<const std::uint8_t> chat_; // 기존 실제100×48 채팅 기록이다.
    PlayerIdentityReads reads_; // 같은 현재 전역/설정 조회다.
};
}
