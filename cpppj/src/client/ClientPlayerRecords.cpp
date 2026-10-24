// 원문 부모의 물리 장치만 제공하며 동맹/Chat 계산은 기존 실제 자식에 맡긴다.
#include "client/ClientPlayerRecords.h"
#include "client/CurrentAllocationBytes.h"
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
ClientPlayerRecords::ClientPlayerRecords(AllocationBytes allocationBytes)
    :allocationBytes_(allocationBytes ? std::move(allocationBytes) : CurrentAllocationBytes) {} // 제품 기본은 현재 CRT 입력이다.
ClientPlayerRecords::Use::Use(ClientPlayerRecords& records):records_(records) {
    if (records_.inUse_) throw std::logic_error("Player 고정뷰를 사용하는 이전 세션이 아직 살아 있음");records_.inUse_=true;
} // 두 번째 세션은 기존 소비자의 물리를 변경하기 전에 실패한다.
ClientPlayerRecords::Use::~Use() { records_.inUse_=false; } // 실제 Player/목록 내용은 보존한다.
std::unique_ptr<ClientPlayerRecords::Use> ClientPlayerRecords::Acquire() { return std::unique_ptr<Use>(new Use(*this)); } // 유일한 대여를 발급한다.
// 원문이 선택하는 순서에 따라 실제 할당/해제와 현재 base store를 수행한다.
void ClientPlayerRecords::Use::Initialize(std::function<void(std::uint32_t,std::uint32_t)> alliance) {
    if (attempted_) throw std::logic_error("살아 있는 Player 고정뷰 대여에서 초기화를 반복할 수 없음");attempted_=true;
    o::PlayerInitializationHooks hooks; // 복원된 부모가 직접 사용하는 실제 메모리 장치다.
    hooks.memory=[this](auto address,auto bytes) { return records_.Memory(address,bytes); };
    hooks.freeFactory=[this](auto token) { records_.Free(token,false); };hooks.freePlayers=[this](auto token) { records_.Free(token,true); };
    hooks.mallocPlayers=[this](auto bytes) { return records_.Allocate(bytes,true); };hooks.newFactory=[this](auto bytes) { return records_.Allocate(bytes,false); };
    hooks.alliance=std::move(alliance);
    o::InitializePlayerRecords({records_.fortCount_,records_.lastLoginPlayer_,[this] { return records_.base_; },[this](auto base) { records_.base_=base; }},hooks);
    initialized_=true;
}
// 현재 owner 번호만 캡처한다. Player 주소/포인터/용량/개수는 호출마다 실제 기록에서 읽는다.
void ClientPlayerRecords::Use::BindFactory(o::SquidPostPopList& list,std::uint32_t owner) {
    if (!initialized_ || owner>=o::kInitializedPlayerCount) throw std::logic_error("초기화되지 않은 Player factory 연결");
    auto* records=&records_; // Use보다 오래 사는 실제 Client 저장소다.
    list.BindCurrent({[records,owner] { return records->Factory(owner); },[records,owner] { return records->Read(owner,4); },
        [records,owner] { return records->Read(owner,8); },[records,owner](auto value) { records->Write(owner,8,value); }});
}
// 실제 부모의 두 할당 크기만 받으며 factory의 미초기화 입력은 임의0으로 치환하지 않는다.
std::uint32_t ClientPlayerRecords::Allocate(std::uint32_t bytes,bool player) {
    if (bytes!=(player ? o::kInitializedPlayerBytes : o::kInitializedFactoryBytes)) throw std::logic_error("Player 실제 할당 크기 오류");
    if (next_>std::numeric_limits<std::uint32_t>::max()-0x1000U) throw std::overflow_error("Player 물리 주소 소진");
    Block block{std::vector<std::uint32_t>(bytes/4),player}; // DWORD의 실제 수명을 시작하고 정확한 크기를 확보한다.
    if (!player) allocationBytes_({reinterpret_cast<std::uint8_t*>(block.words.data()),bytes});
    const auto token=next_;blocks_.emplace(token,std::move(block));next_+=0x1000U;return token;
}
// free(NULL)도 도달하지만 물리를 지우지 않는다. 그 외 주소는 정확한 생존 할당이어야 한다.
void ClientPlayerRecords::Free(std::uint32_t token,bool player) {
    if (!token) return;const auto found=blocks_.find(token);
    if (found==blocks_.end() || found->second.player!=player) throw std::logic_error("Player 실제 반납 주소/종류 오류");blocks_.erase(found);
}
// DWORD 주소를 호스트 포인터로 변환하지 않고 발급한 실제 블록의 현재 범위만 빌린다.
std::span<std::uint8_t> ClientPlayerRecords::Memory(std::uint32_t address,std::uint32_t bytes) {
    auto found=blocks_.upper_bound(address);if (found==blocks_.begin()) throw std::out_of_range("Player 물리 주소 없음");--found;
    auto& words=found->second.words;const auto offset=std::uint64_t(address)-found->first,available=words.size()*4ULL;
    if (offset>available || bytes>available-offset) throw std::out_of_range("Player 물리 요청폭 범위 오류");
    return {reinterpret_cast<std::uint8_t*>(words.data())+static_cast<std::size_t>(offset),bytes};
}
std::span<std::uint8_t> ClientPlayerRecords::Records() { return base_ ? Memory(base_,o::kInitializedPlayerBytes) : std::span<std::uint8_t>{}; } // 현재 실제 base다.
std::span<const std::uint8_t> ClientPlayerRecords::Records() const { return const_cast<ClientPlayerRecords*>(this)->Records(); } // 같은 물리의 읽기뷰다.
std::uint32_t ClientPlayerRecords::Base() const { return base_; } // 원문의 현재 슬롯을 읽는다.
std::uint32_t& ClientPlayerRecords::FortCount() { return fortCount_; } // 같은 로그인 카운터다.
std::uint32_t& ClientPlayerRecords::LastLoginPlayer() { return lastLoginPlayer_; } // 같은 마지막 번호다.
std::array<std::uint32_t,81>& ClientPlayerRecords::Alliances() { return alliances_; } // 사본 없는 방향별 표다.
// 현재 Player base를 매번 조회하여 재할당 이전 행을 캡처하지 않는다.
std::uint32_t ClientPlayerRecords::Read(std::uint32_t owner,std::uint32_t offset) {
    if (owner>=o::kInitializedPlayerCount || offset>o::kInitializedPlayerStride-4) throw std::out_of_range("현재 Player DWORD 범위 오류");
    const auto bytes=Memory(base_+owner*o::kInitializedPlayerStride+offset,4);std::uint32_t value{}; // 현재 little endian DWORD다.
    for (std::uint32_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[byte])<<(byte*8);return value; // 정확한4BYTE만 읽는다.
}
void ClientPlayerRecords::Write(std::uint32_t owner,std::uint32_t offset,std::uint32_t value) {
    static_cast<void>(Read(owner,offset));const auto bytes=Memory(base_+owner*o::kInitializedPlayerStride+offset,4);
    for (std::uint32_t byte=0;byte<4;++byte) bytes[byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 같은 현재 DWORD를 쓴다.
}
// 배열 접근에 도달한 경우에만 실제 포인터/용량의 물리와 DWORD 정렬을 확인한다.
std::span<std::uint32_t> ClientPlayerRecords::Factory(std::uint32_t owner) {
    const auto capacity=Read(owner,4),token=Read(owner,0); // 원문의 현재 배열 헤더다.
    if (capacity>std::numeric_limits<std::uint32_t>::max()/4) throw std::out_of_range("Player factory 용량 넘침");
    if (!capacity) return {};
    const auto bytes=Memory(token,capacity*4);
    if (reinterpret_cast<std::uintptr_t>(bytes.data())%alignof(std::uint32_t)) throw std::logic_error("Player factory 정렬 오류");
    return {reinterpret_cast<std::uint32_t*>(bytes.data()),capacity};
}
}
