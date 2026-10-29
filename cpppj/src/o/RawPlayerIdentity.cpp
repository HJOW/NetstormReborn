// 실제 PlayerFinder의 부분완료·우선순위와 물리 바이트 보존을 구현한다.
#include "o/RawPlayerIdentity.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 도달한 현재 전역을 가짜0/이름으로 대체하지 않는다.
template<class Function> const Function& Required(const Function& function) {
    if (!function) throw std::logic_error("실제 Player 식별자 전역 미연결");return function;
}
// PlayerFinder의 물리 이름/주소/모드와 공유 목록 크기다.
constexpr std::size_t kName=8,kNameBytes=40,kAddress=48,kMode=52,kChatStride=48,kChatCount=100,kMetaCount=40;
constexpr std::size_t kTerminatedNameBytes=kNameBytes+1; //채팅 캡처의40BYTE 뒤NUL은 조회레코드의주소 첫BYTE/Chat행의flags 첫BYTE다. 할당을 넓히지 않고 실제 인접 한BYTE까지 읽는다.
}
// 저장소 크기는 생성 시 확인하고 실제 레코드 검색/효과는 호출 때 실행한다.
RawPlayerIdentity::RawPlayerIdentity(OriginalEdition edition,std::span<const std::uint8_t> players,std::span<const std::uint8_t> meta,std::span<const std::uint8_t> chat,PlayerIdentityReads reads)
    :edition_(edition),players_(players),meta_(meta),chat_(chat),reads_(std::move(reads)) {
    const auto playerStride=edition==OriginalEdition::Patch1078 ? 180U : 172U,metaStride=edition==OriginalEdition::Patch1078 ? 460U : 296U;
    if (players.size()!=9*playerStride || (!meta.empty() && meta.size()!=kMetaCount*metaStride) || chat.size()!=kChatCount*kChatStride)
        throw std::invalid_argument("Player 식별자 공유 저장소 크기 오류");
}
// 원문 little endian DWORD를 부호 변경 없이 읽는다.
std::uint32_t RawPlayerIdentity::Read(std::span<const std::uint8_t> bytes,std::size_t offset) {
    if (offset+4>bytes.size()) throw std::out_of_range("Player 식별자 필드 범위 오류");std::uint32_t value=0;
    // 낮은 바이트부터 원본 폭만 조립한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[offset+index])<<(8*index);return value;
}
// 현재 필드 밖의 표식 바이트를 보존한다.
void RawPlayerIdentity::Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    // 원본DWORD의 낮은 바이트부터 쓴다.
    for (std::size_t index=0;index<4;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// NUL 없는 원본 버퍼 초과는 호스트 예외로 거부한다.
std::string_view RawPlayerIdentity::Text(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t capacity) {
    if (offset+capacity>bytes.size()) throw std::out_of_range("Player 식별자 문자열 범위 오류");const auto text=bytes.subspan(offset,capacity);const auto end=std::find(text.begin(),text.end(),std::uint8_t{});
    if (end==text.end()) throw std::length_error("Player 식별자 NUL 없는 문자열");return {reinterpret_cast<const char*>(text.data()),static_cast<std::size_t>(end-text.begin())};
}
// 원본 strcpy처럼 최대40BYTE 이름과 종결자만 쓴다.40BYTE의NUL은인접주소첫BYTE이며기존60BYTE물리안이다.원본근거는단위571문서다.
void RawPlayerIdentity::SetName(PlayerIdentityRecord& record,std::string_view name) {
    const auto nul=name.find('\0');if (nul!=std::string_view::npos) name=name.substr(0,nul);if (name.size()>kNameBytes) throw std::length_error("Player 식별자40바이트 이름 초과");
    std::copy(name.begin(),name.end(),record.begin()+kName);record[kName+name.size()]=0;
}
// 원본 C locale의 ASCII 알파벳만 접으며 확장 바이트는 같은 값끼리만 일치한다.
bool RawPlayerIdentity::Equal(std::string_view first,std::string_view second) {
    if (first.size()!=second.size()) return false;
    // 각 원문 바이트를 같은 규칙으로 비교한다.
    for (std::size_t index=0;index<first.size();++index) {
        const auto fold=[](unsigned char value) { return value>='A' && value<='Z' ? value+('a'-'A') : value; }; // C locale의 소문자 변환이다.
        if (fold(static_cast<unsigned char>(first[index]))!=fold(static_cast<unsigned char>(second[index]))) return false;
    }return true;
}
// 원본 생성자는 전체 레코드를 memset하지 않는다.
void RawPlayerIdentity::Initialize(PlayerIdentityRecord& record,std::uint32_t mode) { Put(record,0,0);Put(record,4,0);record[kName]=0;Put(record,kAddress,0);Put(record,kMode,mode); }
// 원본 로컬 match는 네 독립 필드 중 하나만 맞아도 같은 현재 정보로 덮어쓴다.채팅의40BYTE조회는인접주소첫BYTE의NUL까지읽으며서로다른owner/subscriber/주소를합치지않는다.
bool RawPlayerIdentity::Resolve(PlayerIdentityRecord& record) const {
    if ((Read(record,0) && Read(record,0)==Required(reads_.localOwner)()) || (Read(record,4) && Read(record,4)==Required(reads_.subscriber)()) ||
        (record[kName] && Equal(Text(record,kName,kTerminatedNameBytes),Required(reads_.name)())) || (Read(record,kAddress) && Read(record,kAddress)==Required(reads_.address)())) {
        SetName(record,Required(reads_.name)());Put(record,kAddress,Required(reads_.address)());Put(record,4,Required(reads_.subscriber)());Put(record,0,Required(reads_.localOwner)());return true;
    }
    const bool patch=edition_==OriginalEdition::Patch1078;const std::size_t playerStride=patch ? 180 : 172,metaStride=patch ? 460 : 296;
    if (Required(reads_.battle)() || Required(reads_.editor)()) {
        // Player0은 검색하지 않고 실제1..8의 첫 OR 일치를 사용한다.
        for (std::uint32_t owner=1;owner<9;++owner) {
            const auto player=players_.subspan(owner*playerStride,playerStride);
            if ((Read(record,0) && Read(record,0)==owner) || (Read(record,4) && Read(record,4)==Read(player,0x2c)) ||
                (record[kName] && Equal(Text(record,kName,kTerminatedNameBytes),Text(player,0xc,32)))) {
                Put(record,0,owner);Put(record,4,Read(player,0x2c));SetName(record,Text(player,0xc,32));Put(record,kAddress,0);break;
            }
        }
        if (Read(record,0) && Read(record,4) && record[kName]) return true;
    }
    if (Required(reads_.metamap)()) {
        if (meta_.empty()) throw std::logic_error("실제 metamap 식별자 기록 미연결");
        // 실제40개의 활성 기록만 subscriber 또는 이름으로 비교한다. owner는 match 조건이 아니다.
        for (std::size_t index=0;index<kMetaCount;++index) {
            const auto entry=meta_.subspan(index*metaStride,metaStride);const bool active=patch ? (entry[0x64]&1)!=0 : (Read(entry,0x48)&1)!=0;const auto nameOffset=patch ? 0x26U : 8U;
            if (active && ((Read(record,4) && Read(record,4)==Read(entry,4)) || (record[kName] && Equal(Text(record,kName,kTerminatedNameBytes),Text(entry,nameOffset,40))))) {
                Put(record,0,Read(entry,0));Put(record,4,Read(entry,4));SetName(record,Text(entry,nameOffset,40));Put(record,kAddress,0);break;
            }
        }
    }
    if (Read(record,4) && record[kName]) return true;
    // 채팅은100개 전체의 첫 일치이며 활성 flag나 subscriber 비영 필터를 추가하지 않는다.
    for (std::size_t index=0;index<kChatCount;++index) {
        const auto entry=chat_.subspan(index*kChatStride,kChatStride);
        if ((Read(record,4) && Read(record,4)==Read(entry,0x2c)) || (record[kName] && Equal(Text(record,kName,kTerminatedNameBytes),Text(entry,0,kTerminatedNameBytes)))) {
            Put(record,0,0);Put(record,4,Read(entry,0x2c));SetName(record,Text(entry,0,kTerminatedNameBytes));break;
        }
    }
    return Read(record,4) && record[kName];
}
// 실제 getter는 Resolve의 bool을 별도 성공 관문으로 사용하지 않는다.
std::uint32_t RawPlayerIdentity::SubscriberForOwner(std::uint32_t owner) const { PlayerIdentityRecord record{};Initialize(record,2);Put(record,0,owner);Resolve(record);return Read(record,4); }
// 원본 subscriber→owner getter의 현재 부분완료 결과도 그대로 반환한다.
std::uint32_t RawPlayerIdentity::OwnerForSubscriber(std::uint32_t subscriber) const { PlayerIdentityRecord record{};Initialize(record,2);Put(record,4,subscriber);Resolve(record);return Read(record,0); }
// 주소 getter도 같은 실제 Resolve 몸체를 사용한다.
std::uint32_t RawPlayerIdentity::AddressForSubscriber(std::uint32_t subscriber) const { PlayerIdentityRecord record{};Initialize(record,2);Put(record,4,subscriber);Resolve(record);return Read(record,kAddress); }
// 이름 실패의 mutable SOMEONE 문자열만 명시 경계로 읽는다.
std::string RawPlayerIdentity::NameForSubscriber(std::uint32_t subscriber) const { PlayerIdentityRecord record{};Initialize(record,0);Put(record,4,subscriber);return std::string(Resolve(record) ? Text(record,kName,kTerminatedNameBytes) : Required(reads_.unknownName)()); }
// owner와subscriber를구분하며부분완료레코드의이름을성공으로승격하지않는다.
std::string RawPlayerIdentity::NameForOwner(std::uint32_t owner) const { PlayerIdentityRecord record{};Initialize(record,0);Put(record,0,owner);return std::string(Resolve(record) ? Text(record,kName,kTerminatedNameBytes) : Required(reads_.unknownName)()); }
// 실제 조회 몸체와 같은 원본 판본이다.
OriginalEdition RawPlayerIdentity::Edition() const { return edition_; }
// 호출자가 같은 현재9 Player를 사용하게 한다.
std::span<const std::uint8_t> RawPlayerIdentity::PlayerRecords() const { return players_; }
// 메뉴 metadata는 같은40개 기록을 빌린다.
std::span<const std::uint8_t> RawPlayerIdentity::MetaRecords() const { return meta_; }
// 현재 모드/로컬 식별자 getter를 다른 실제 소비자와 공유한다.
const PlayerIdentityReads& RawPlayerIdentity::CurrentInputs() const { return reads_; }
}
