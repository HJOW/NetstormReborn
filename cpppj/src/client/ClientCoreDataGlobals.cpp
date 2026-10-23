// 실제 동일 CoreData 저장소를 기존 원문 helper에 빌리며 숨은 reset/사본 동기화를 추가하지 않는다.
#include "client/ClientCoreDataGlobals.h"
#include "o/RawPlayerCoreRecords.h"

namespace netstorm::client {
// 실제CRT표순서대로9기록을초기화한뒤별도lastKnown10을같은16BYTE자식으로초기화한다.
ClientCoreDataGlobals::ClientCoreDataGlobals() { o::InitializePlayerCoreRecords({records_,0});o::InitializePlayerCore(cached_); }
// 같은 포인터 슬롯 자체를 직접 빌린다.
std::span<std::uint8_t>& ClientCoreDataGlobals::Current() { return current_; }
// 읽기도 실제 슬롯 주소를 유지한다.
const std::span<std::uint8_t>& ClientCoreDataGlobals::Current() const { return current_; }
// 현재 활성 DWORD의 직접 쓰기 참조다.
std::uint32_t& ClientCoreDataGlobals::Active() { return active_; }
// 현재 활성 DWORD의 읽기 참조다.
const std::uint32_t& ClientCoreDataGlobals::Active() const { return active_; }
// 같은 고정 캐시10BYTE를 빌린다.
std::span<std::uint8_t> ClientCoreDataGlobals::Cached() { return cached_; }
// 캐시를 복제하지 않는 읽기뷰다.
std::span<const std::uint8_t> ClientCoreDataGlobals::Cached() const { return cached_; }
// 외부 실제Player주소를 바꿀 수 있는 현재뷰 슬롯이다.
std::span<std::uint8_t>& ClientCoreDataGlobals::Players() { return players_; }
// 같은 현재Player뷰 슬롯을 읽는다.
const std::span<std::uint8_t>& ClientCoreDataGlobals::Players() const { return players_; }
// 같은Client수명의9기록물리전체를빌린다.
std::span<std::uint8_t> ClientCoreDataGlobals::Records() { return records_; }
// const조회도동일주소를사용한다.
std::span<const std::uint8_t> ClientCoreDataGlobals::Records() const { return records_; }
// 원문metadata44BYTE를같은144BYTE에실행하며Session교체로장부를초기화하지않는다.
void ClientCoreDataGlobals::StoreMetadata(std::uint32_t owner,std::uint32_t subscriber,std::array<std::uint8_t,10> core) { o::WritePlayerCoreMetadata({records_,0},owner,subscriber,core); }
// 모든 참조 대상은 helper 호출과 저장한 callback보다 오래 살아야 한다.
o::PlayerCoreDataGlobals ClientCoreDataGlobals::Borrow(o::PlayerWorldState& player,const std::uint32_t& metamap) {
    return {player.templateOwner,player.battleOwner,player.playerKnown,active_,metamap,current_,cached_,players_};
}
// 진단85와 callback 복귀 뒤 현재 포인터 재조회를 생략하지 않는다.
std::span<std::uint8_t> ClientCoreDataGlobals::Read(o::PlayerWorldState& player,const std::uint32_t& metamap,const o::PlayerCoreDataHooks& hooks) {
    return o::ReadPlayerCoreData(Borrow(player,metamap),hooks);
}
// 호출자가 원문 위치에서 명시 요청할 때만 실제 선택/정리 본문을 실행한다.
void ClientCoreDataGlobals::Select(o::PlayerWorldState& player,const std::uint32_t& metamap,std::uint32_t owner,std::uint32_t templateOwner) {
    o::SelectPlayerCoreData(Borrow(player,metamap),owner,templateOwner);
}
}
