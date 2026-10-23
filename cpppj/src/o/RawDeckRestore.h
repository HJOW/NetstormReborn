// Deck로드가실제로도달하는Open/저장네필드추가/FullDeck과그하위추가를복원한다.
#pragma once
#include "o/SpStore.h"
#include <functional>
#include <span>

namespace netstorm::o {
// Entry/두Append가선택하는실제Deck.cpp진단행이다.복귀하면현재메모리에서계속한다.
enum class DeckRestoreAssertion : std::uint32_t {
    TypeTruncated=0x2d,ChanceTruncated=0x2e,PowerTruncated=0x2f,RemainingTruncated=0x30, //0044f160의네저장필드진단이다.
    FullDuplicate=0x64,FullCapacity=0x66, //0044f290의기본두인자추가진단이다.
    RestoredDuplicate=0x6f,RestoredCapacity=0x71 //0044f330의저장네인자추가진단이다.
};
// FullDeck의두실제TYPE조회에서그때읽을두필드다.첫조회는group,두번째조회는flags2만쓴다.
struct FullDeckType {
    std::int32_t group; //TYPE+9c의현재signedDWORD다.
    std::uint32_t flags2; //TYPE+ec이며bit0x100을검사한다.기존RiftTypeRecord.flags2와같은필드다.
};
// 실제현재TYPE수와group10의비선택조건에서예외로포함하는두타입DWORD를같은소유자에서빌린다.
struct FullDeckGlobals {
    const std::int32_t& typeCount; //00541348이며최초/매반복끝에서현재값을읽는다.
    const std::uint32_t& nuggetType; //005412c0의현재너겟타입이다.이타입은group10/flags2조건에서도포함한다.
    const std::uint32_t& geyserType; //00541240의현재간헐천타입이다.너겟비교가불일치할때읽고같으면포함한다.
};
// 실제현재TYPE와선택진단만공급하며선택/공유난수/Entry직접쓰기는몸체가소유한다.
struct DeckRestoreHooks {
    std::function<FullDeckType(std::uint32_t)> type; //0049a840의실제TYPE필드조회다.동일index도필요하면두번호출한다.
    std::function<void(DeckRestoreAssertion)> assertion; //동일Deck메모리/현재전역을바꾼뒤복귀할수있다.누락은도달할때만거부한다.
};
//0044f250:같은352BYTE/공유난수의기존RawDeckDraw::Open몸체를재사용한다.
void OpenRestoredDeck(std::span<std::uint8_t> deck,GameRandom& random,std::uint32_t seed);
//0044f330/0044f160:네입력DWORD를그대로받아현재count/중복/용량진단뒤실제5BYTEEntry를쓴다.
void AppendRestoredDeckEntry(std::span<std::uint8_t> deck,std::uint32_t type,std::uint32_t chance,std::uint32_t power,std::uint32_t remaining,const DeckRestoreHooks& hooks);
//0044f680:세DWORD초기화→현재TYPE조건→같은random.Next(5)→실제0044f290/0044f160추가다.
void LoadFullDeck(std::span<std::uint8_t> deck,GameRandom& random,const FullDeckGlobals& globals,const DeckRestoreHooks& hooks);
}
