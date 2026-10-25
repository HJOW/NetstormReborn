// 현재Player가소유하는원본0x160BYTE Deck과기술표시내용물을같은월드에서조회한다.
#pragma once
#include "o/RawPlayerWorld.h"
#include "o/RawCarrierPreDestroy.h"
#include "o/RawContainedCreate.h"
#include <map>
#include <memory>
#include <optional>
#include <utility>

namespace netstorm::o {
// 원본Deck객체의전체물리크기와고정된66개의5BYTE항목이다.
inline constexpr std::size_t kProductionDeckBytes=0x160,kProductionEntryBytes=5,kProductionEntryCapacity=66;
// 기존목록추가의원본진단위치다.호스트물리영역을넘는손상개수는별도로거부한다.
enum class ProductionDeckAssertion { DuplicateType,Capacity,TypeTruncated,ChanceTruncated,PowerTruncated,RemainingTruncated };
// Player+68의32비트토큰에대응하는유일한Deck수명저장소다.다른Player사본은만들지않는다.
class RawProductionDeck {
    struct RetirementLifetime {}; // 같은 실제 Deck 모듈의 C++ 수명을 구별하며 Native 게임 필드가 아니다.
public:
    class PlayerLinksPass { // 같은 실제0048fcd0 전체 정상 복귀를 증명하며 외부 생성/복사를 허용하지 않는다.
    public:
        PlayerLinksPass(PlayerLinksPass&&) noexcept=default;PlayerLinksPass& operator=(PlayerLinksPass&&) noexcept=default; // 현재 모듈 반납 증빙만 이동한다.
        PlayerLinksPass(const PlayerLinksPass&)=delete;PlayerLinksPass& operator=(const PlayerLinksPass&)=delete; // 임의 완료 상태를 만들지 않는다.
    private:
        friend class RawProductionDeck;
        PlayerLinksPass(const RawProductionDeck* source,std::weak_ptr<const RetirementLifetime> lifetime) noexcept:source_(source),lifetime_(std::move(lifetime)) {} // 실제 CleanupPlayerLinks 정상 복귀 뒤에만 발급한다.
        const RawProductionDeck* source_;std::weak_ptr<const RetirementLifetime> lifetime_; // 소멸 뒤 포인터는 비교만 하며 주소 재사용은 lifetime으로 구별한다.
    };

    // 할당직후의원본미초기화바이트를공급한다.0채움관찰기본값을묵시적으로넣지않는다.
    using Fill=std::function<void(std::span<std::uint8_t>)>;
    // PATCH Player9와현재templateOwner는player자체에서읽는다.할당기/진단대상은더오래살아야한다.
    RawProductionDeck(RawPlayerWorld& player,Fill fill,std::function<void(ProductionDeckAssertion)> assertion={},std::uint32_t firstToken=0x30000000);
    // 같은현재Player기록이더오래살아있어야한다.자신이소유한현재포인터만지운다.
    ~RawProductionDeck();
    RawProductionDeck(const RawProductionDeck&)=delete; // Player포인터와실제블록의단일소유자를유지한다.
    RawProductionDeck& operator=(const RawProductionDeck&)=delete; // 다른월드의Deck수명과섞지않는다.
    // 0044f700:없으면실제저장소를할당하고5DWORD만초기화한뒤현재Player+68에부착한다.
    std::uint32_t Get(std::uint32_t owner) const;
    // 0044f970:현재templateOwner의Deck을읽는다.항목의type BYTE와전체인자DWORD를비교한다.
    bool Contains(std::uint32_t type) const;
    // 0044f200의생성초기화와0044f230의항목/가중합초기화를구분한다.
    void Initialize(std::uint32_t token) const;
    void ClearEntries(std::uint32_t token) const;
    // 0044f290/0044f3d0/0044f430의실제추가·signed power활성화·압축몸체다.
    void Append(std::uint32_t token,std::uint32_t type,std::int32_t chance) const;
    void Activate(std::uint32_t token,std::uint32_t type,std::int32_t increment) const;
    void Compact(std::uint32_t token) const;
    // 후속재계산은같은raw항목/power/count를갱신한다.관찰결과별도목록을공급하지않는다.
    std::span<std::uint8_t> Bytes(std::uint32_t token) const;
    // CRT free 경계: 같은 소유자의 한 블록만 반납한다. Player 필드/다른 블록/다음 토큰은 바꾸지 않는다.
    // 0은 원문 free(NULL)처럼 무효과이며, 미등록 비영/중복 토큰은 명시 계약 오류다. 빌린 Bytes 뷰는 즉시 무효다.
    void Release(std::uint32_t token) const;
    PlayerLinksPass CleanupPlayerLinksForState(std::function<std::span<std::uint8_t>()> currentPlayers); // 실제 부모와 이 Deck의 실제 Release만 사용한 정상 복귀 증빙이다.
    void ReturnPlayerLinksForState(PlayerLinksPass&& pass); // 같은 모듈/전체 링크0/실제 블록 반납/호출복귀 증빙을 소비한다.
    bool PlayerLinksReturnedForState() const; // 실제 보관 증빙만 조회하며 외부 성공 setter는 제공하지 않는다.

    const SidPool& Pool() const; // 같은실제월드조립을확인한다.
    RawPlayerWorld& Player() const; // 같은현재9Player/두owner를후속부모가재사용한다.
private:
    // 현재Player의정확한DWORD68필드범위를계산한다.
    std::size_t PointerOffset(std::uint32_t owner) const;
    // 정확한원본진단을전달하며없으면명시예외를낸다.
    void Assert(ProductionDeckAssertion where) const;
    // 실제접근할항목개수의물리한계를검사한다.signed음수는원본처럼빈순회다.
    std::int32_t Count(std::span<const std::uint8_t> raw) const;
    void RequireLiveLinks() const; // 반납한 Player 뷰로 새 Deck을 만들거나 메모리를 조회/반납하지 못하게 한다.
    std::shared_ptr<const RetirementLifetime> retirementLifetime_{std::make_shared<RetirementLifetime>()}; // 실제 모듈이 사라진 뒤 이전 pass의 재사용을 거부한다.
    std::optional<PlayerLinksPass> returnedLinks_; // 실제 정상 자식에서 이동한 증빙일 때만 oldPlayer 소멸 재조회를 생략한다.
    std::size_t activeLinkCalls_{}; // 실제 부모의 free/후속store가 복귀하기 전 반납 증빙을 소비하지 않는다.
    RawPlayerWorld& player_; // 원본Player기록과templateOwner의장기저장소다.
    Fill fill_; // 실제할당직후바이트공급경계이며할당성공판정을대신하지않는다.
    std::function<void(ProductionDeckAssertion)> assertion_; // 원본assert를분기성공으로바꾸지않는진단이다.
    mutable std::map<std::uint32_t,std::array<std::uint8_t,kProductionDeckBytes>> blocks_; // 각현재토큰의유일한실제블록이다.
    mutable std::uint32_t next_; // 새블록마다원본물리크기만큼진행하는opaque32비트주소다.
};
// 004929e0의현재기술부모/반환kindWORD/실제contained생성사슬이다.타입assert를임의로추가하지않는다.
class RawIndicateTechnology {
public:
    // 실제CurrentTechnology·ContainedFinder·Factory/부착기를같은풀로연결한다.
    RawIndicateTechnology(RawPlayerWorld& player,const RawCarrierPreDestroy& carrier,const RawContainedCreate& create);
    // kindWORD조회결과가0이면현재부모를다시읽고(type,0,4,10,0)으로생성한다.
    void Indicate(std::uint32_t type) const;
    const SidPool& Pool() const; // 현재세실제몸체의풀동일성검사용이다.
private:
    // 현재getter의DWORD를WORD로미리자르지않고실제풀범위를확인한다.
    Sid Parent() const;
    RawPlayerWorld& player_; // 현재PlayerKnown/battleOwner의실제TechnologyDWORD다.
    const RawCarrierPreDestroy& carrier_; // 기존004ac9f0과같은typed조건순회를재사용한다.
    const RawContainedCreate& create_; // 같은실제Factory/ContentForm부착의전체하위몸체다.
};
}
