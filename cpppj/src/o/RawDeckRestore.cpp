// 진단뒤현재count/필드를다시읽고기존352BYTE안의metadata겹침까지원문쓰기순서를유지한다.
#include "o/RawDeckRestore.h"
#include "o/RawDeckDraw.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::size_t kRestoreDeckBytes=352,kRestoreCount=332,kRestoreChance=336,kRestoreRemaining=340; //원본0x160블록과실제세집계DWORD위치다.
// 실제할당범위만확인하고항목개수66진단을호스트사전검사로바꾸지않는다.
void CheckRestoreDeck(std::span<const std::uint8_t> deck) { if (deck.size()!=kRestoreDeckBytes) throw std::invalid_argument("Deck 복원의 실제352BYTE 누락"); }
// 비정렬DWORD의현재네BYTE만읽는다.
std::uint32_t RestoreU(std::span<const std::uint8_t> deck,std::size_t at) {
    if (at>deck.size() || deck.size()-at<4) throw std::out_of_range("Deck 복원 DWORD 실제물리범위");std::uint32_t value{}; //현재DWORD조립값이다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(deck[at+byte])<<(byte*8);return value; //이웃필드를섞지않는다.
}
// 지정DWORD만쓰며현재항목/seed/open을보존한다.
void RestorePut(std::span<std::uint8_t> deck,std::size_t at,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) deck[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //호출위치는현재352BYTE안의고정DWORD다.
// 선택장치가없으면이미쓴Entry/이전진단효과를보존한채실패한다.
template<class Function> const Function& NeedRestore(const Function& function) { if (!function) throw std::logic_error("Deck 복원의 실제 경계 미연결");return function; }
// 현재BYTE조회만물리한계에서거부하며양수count가66을넘었다고앞서순회를생략하지않는다.
std::uint8_t RestoreByte(std::span<const std::uint8_t> deck,std::size_t at) { if (at>=deck.size()) throw std::out_of_range("Deck 복원 항목 실제물리범위");return deck[at]; }
// 두Append의공통현재count/실제Entry쓰기이며두함수의진단행만구분한다.
void AppendRestore(std::span<std::uint8_t> deck,std::uint32_t type,std::uint32_t chance,std::uint32_t power,std::uint32_t remaining,bool restored,const DeckRestoreHooks& hooks) {
    CheckRestoreDeck(deck);std::int32_t index{}; //중복순회는현재signedcount를매반복다시읽는다.
    if (std::bit_cast<std::int32_t>(RestoreU(deck,kRestoreCount))>0) {
        do { //진단이count를바꾸어도최초타입인자와현재이동한항목주소는유지한다.
            if (RestoreByte(deck,static_cast<std::size_t>(index)*5)==type) NeedRestore(hooks.assertion)(restored ? DeckRestoreAssertion::RestoredDuplicate : DeckRestoreAssertion::FullDuplicate);
            ++index;
        } while (index<std::bit_cast<std::int32_t>(RestoreU(deck,kRestoreCount)));
    }
    if (std::bit_cast<std::int32_t>(RestoreU(deck,kRestoreCount))>=66) NeedRestore(hooks.assertion)(restored ? DeckRestoreAssertion::RestoredCapacity : DeckRestoreAssertion::FullCapacity);
    const auto count=std::bit_cast<std::int32_t>(RestoreU(deck,kRestoreCount)); //용량진단뒤실제다음Entry주소를위해다시읽는다.
    if (count<0 || static_cast<std::uint64_t>(count)*5+5>deck.size()) throw std::out_of_range("Deck 복원 다음Entry 실제물리범위");
    const auto at=static_cast<std::size_t>(count)*5; //count66..69도352BYTE안이면원문metadata겹침을그대로보존한다.
    deck[at+2]=static_cast<std::uint8_t>(power);deck[at]=static_cast<std::uint8_t>(type);deck[at+1]=static_cast<std::uint8_t>(chance);deck[at+3]=static_cast<std::uint8_t>(remaining);deck[at+4]=0;
    if (std::uint32_t(deck[at])!=type) NeedRestore(hooks.assertion)(DeckRestoreAssertion::TypeTruncated);
    if (std::uint32_t(deck[at+1])!=chance) NeedRestore(hooks.assertion)(DeckRestoreAssertion::ChanceTruncated);
    if (std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<std::int8_t>(deck[at+2])))!=power) NeedRestore(hooks.assertion)(DeckRestoreAssertion::PowerTruncated);
    if (std::uint32_t(deck[at+3])!=remaining) NeedRestore(hooks.assertion)(DeckRestoreAssertion::RemainingTruncated);
    const auto sum=RestoreU(deck,kRestoreChance)+chance,next=RestoreU(deck,kRestoreCount)+1U; //Entry진단이바꾼현재두집계에캡처한chance/1을더한다.
    RestorePut(deck,kRestoreChance,sum);RestorePut(deck,kRestoreCount,next);
}
}
// 이미원문관측된Open을같은실제공유난수와물리Deck에직접연결한다.
void OpenRestoredDeck(std::span<std::uint8_t> deck,GameRandom& random,std::uint32_t seed) { RawDeckDraw(deck,random,{}).Open(seed); }
// 타입/찬스/power/remaining을미리절삭하거나다른기본추가부모로대체하지않는다.
void AppendRestoredDeckEntry(std::span<std::uint8_t> deck,std::uint32_t type,std::uint32_t chance,std::uint32_t power,std::uint32_t remaining,const DeckRestoreHooks& hooks) { AppendRestore(deck,type,chance,power,remaining,true,hooks); }
// TYPEgetter는큰효과경계이므로동일index의두조회와그뒤현재전역을원래위치에서읽는다.
void LoadFullDeck(std::span<std::uint8_t> deck,GameRandom& random,const FullDeckGlobals& globals,const DeckRestoreHooks& hooks) {
    CheckRestoreDeck(deck);RestorePut(deck,kRestoreCount,0);RestorePut(deck,kRestoreChance,0);RestorePut(deck,kRestoreRemaining,0);std::int32_t index{}; //원문세DWORD초기화와현재타입순회시작이다.
    if (globals.typeCount<=0) return;
    do { //최초양수확인뒤에는하위호출이typeCount를0으로바꾸어도현재한타입은끝까지처리한다.
        const auto type=static_cast<std::uint32_t>(index);const auto first=NeedRestore(hooks.type)(type);bool append=true; //첫조회는group만읽는다.
        if (first.group==10) { const auto second=NeedRestore(hooks.type)(type);if (!(second.flags2&0x100U) && type!=globals.nuggetType && type!=globals.geyserType) append=false; } //두번째TYPE의현재flags2와비선택조건의현재두예외타입을짧게평가한다.
        if (append) { const auto chance=random.Next(5);AppendRestore(deck,type,chance,1,255,false,hooks); } //실제0044f290/Entry쓰기까지본문에포함한다.
        ++index;
    } while (index<globals.typeCount);
}
}
