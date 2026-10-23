// Deck의현재Player포인터와5BYTE항목·signed power를원본필드폭/순서대로보존한다.
#include "o/RawProductionDeck.h"
#include "o/RawDeckDraw.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본Deck의활성개수/가중합/남은수/seed/openDWORD필드다.
constexpr std::size_t kCount=0x14c,kChance=0x150,kRemaining=0x154,kSeed=0x158,kOpen=0x15c;
// 비정렬little endian DWORD/WORD를필요한폭만읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 인접필드나호스트정렬을섞지않는다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
// 원본DWORD필드만쓰고항목/미초기화꼬리는보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    // 낮은바이트부터정확히4BYTE를쓴다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
}
// Player의실제opaqueDWORD와할당블록을단일소유하며PATCH배치만허용한다.
RawProductionDeck::RawProductionDeck(RawPlayerWorld& player,Fill fill,std::function<void(ProductionDeckAssertion)> assertion,std::uint32_t firstToken)
    :player_(player),fill_(std::move(fill)),assertion_(std::move(assertion)),next_(firstToken) {
    if (player.Pool().Edition()!=OriginalEdition::Patch1078 || !fill_ || firstToken==0 || firstToken>std::numeric_limits<std::uint32_t>::max()-kProductionDeckBytes)
        throw std::invalid_argument("실제Deck의PATCH/할당바이트/토큰연결누락");
}
// 다른모듈이교체한Player포인터를지우지않고자신의실제소유블록만해제한다.
RawProductionDeck::~RawProductionDeck() {
    // 동일9기록의현재포인터와자신이소유한토큰을확인한다.
    for (std::uint32_t owner=0;owner<9;++owner) { auto records=player_.Records();const auto offset=PointerOffset(owner);if (blocks_.contains(Read(records,offset))) Put(records,offset,0); }
}
// 원본은Player0도사용할수있지만물리9기록밖주소는호스트계약에서거부한다.
std::size_t RawProductionDeck::PointerOffset(std::uint32_t owner) const {
    if (owner>=9 || player_.Records().size()!=9*0xb4) throw std::out_of_range("실제Deck의Player범위오류");
    return static_cast<std::size_t>(owner)*0xb4+0x68;
}
// opaque토큰을호스트포인터로해석하지않고이저장소의실제블록만반환한다.
std::span<std::uint8_t> RawProductionDeck::Bytes(std::uint32_t token) const {
    const auto found=blocks_.find(token);if (found==blocks_.end()) throw std::out_of_range("현재PlayerDeck의실제할당블록누락");return found->second;
}
// 0048fcd0의 선택된 free 인자를 동일한 할당 소유자에 연결한다. Player+68의 후속0쓰기는 부모가 수행한다.
void RawProductionDeck::Release(std::uint32_t token) const {
    if (!token) return;
    const auto found=blocks_.find(token); // 다른 월드/이미 반납한 비영 토큰을 성공으로 처리하지 않는다.
    if (found==blocks_.end()) throw std::out_of_range("현재PlayerDeck의실제반납블록누락");
    blocks_.erase(found);
}
// 새할당의미초기화영역은fill입력을보존하고원본다섯DWORD만쓴다.
std::uint32_t RawProductionDeck::Get(std::uint32_t owner) const {
    const auto offset=PointerOffset(owner);const auto current=Read(player_.Records(),offset);if (current) { static_cast<void>(Bytes(current));return current; }
    if (next_>std::numeric_limits<std::uint32_t>::max()-kProductionDeckBytes) throw std::overflow_error("실제Deck토큰주소고갈");
    const auto token=next_;std::array<std::uint8_t,kProductionDeckBytes> bytes;fill_(bytes);blocks_.emplace(token,std::move(bytes));next_+=static_cast<std::uint32_t>(kProductionDeckBytes);
    Initialize(token);Put(player_.Records(),offset,token);return token;
}
// 원본의호출가능한진단은그대로전달하고누락은성공으로넘기지않는다.
void RawProductionDeck::Assert(ProductionDeckAssertion where) const { if (!assertion_) throw std::logic_error("실제Deck원본진단경계누락");assertion_(where); }
// signed개수를유지하고호스트블록을넘는손상된양수개수만거부한다.
std::int32_t RawProductionDeck::Count(std::span<const std::uint8_t> raw) const {
    const auto count=std::bit_cast<std::int32_t>(Read(raw,kCount));if (count>static_cast<std::int32_t>(kProductionEntryCapacity)) throw std::out_of_range("실제Deck의66항목물리범위오류");return count;
}
// 생성자는count/seed/chance/remaining/open의원본다섯DWORD만0으로쓴다.
void RawProductionDeck::Initialize(std::uint32_t token) const { RawDeckDraw::Initialize(Bytes(token)); }
// 새생산재계산은목록개수/가중합/남은수만초기화하고seed/open을유지한다.
void RawProductionDeck::ClearEntries(std::uint32_t token) const { auto raw=Bytes(token);Put(raw,kCount,0);Put(raw,kChance,0);Put(raw,kRemaining,0); }
// 현재templateOwner와현재Deck개수를읽으며typeDWORD를BYTE로미리자르지않는다.
bool RawProductionDeck::Contains(std::uint32_t type) const {
    const auto token=Get(player_.State().templateOwner);const auto raw=Bytes(token);
    // 원본처럼첫일치뒤에는나머지항목을읽지않는다.
    for (std::int32_t index=0;index<Count(raw);++index) if (static_cast<std::uint32_t>(raw[static_cast<std::size_t>(index)*kProductionEntryBytes])==type) return true;
    return false;
}
// 중복진단을먼저수행한뒤원본5BYTE초기화와DWORD합계를기록한다.
void RawProductionDeck::Append(std::uint32_t token,std::uint32_t type,std::int32_t chance) const {
    RawDeckDraw::Append(Bytes(token),type,chance,[this](ProductionDeckAssertion where) { Assert(where); });
}
// 기존항목의signed power가음수이면0으로바꾸고increment가비영일때만1을더한다.
void RawProductionDeck::Activate(std::uint32_t token,std::uint32_t type,std::int32_t increment) const {
    auto raw=Bytes(token);
    // 최초BYTE타입일치만변경하며없으면원본chance1/power1항목을추가한다.
    for (std::int32_t index=0;index<Count(raw);++index) if (static_cast<std::uint32_t>(raw[static_cast<std::size_t>(index)*kProductionEntryBytes])==type) { auto& power=raw[static_cast<std::size_t>(index)*kProductionEntryBytes+2];if (std::bit_cast<std::int8_t>(power)<0) power=0;if (increment) ++power;return; }
    Append(token,type,1);
}
// 원본은제외할항목도먼저앞으로복사한뒤signed power가음수이면삭제개수를늘린다.
void RawProductionDeck::Compact(std::uint32_t token) const {
    auto raw=Bytes(token);const auto count=Count(raw);std::int32_t removed=0;
    // 원본의앞방향겹침복사와남은물리꼬리보존을유지한다.
    for (std::int32_t index=0;index<count;++index) { const auto from=static_cast<std::size_t>(index)*kProductionEntryBytes,to=static_cast<std::size_t>(index-removed)*kProductionEntryBytes;const auto word=Read(raw,from);const auto last=raw[from+4];Put(raw,to,word);raw[to+4]=last;if (std::bit_cast<std::int8_t>(raw[from+2])<0) ++removed; }
    Put(raw,kCount,static_cast<std::uint32_t>(count-removed));
}
// 월드조립에서풀동일성을확인한다.
const SidPool& RawProductionDeck::Pool() const { return player_.Pool(); }
// 후속재계산이현재원본두owner/Player9를다시읽는다.
RawPlayerWorld& RawProductionDeck::Player() const { return player_; }
// 기술표시의실제조회/생성은반드시같은풀이어야한다.
RawIndicateTechnology::RawIndicateTechnology(RawPlayerWorld& player,const RawCarrierPreDestroy& carrier,const RawContainedCreate& create):player_(player),carrier_(carrier),create_(create) {
    if (&player.Pool()!=&carrier.Pool() || &player.Pool()!=&create.Pool()) throw std::invalid_argument("기술표시의실제Player/Carrier/생성풀이서로다릅니다");
}
// CurrentTechnology의원본PlayerKnown0반환과호스트풀범위계약을구분한다.
Sid RawIndicateTechnology::Parent() const { const auto sid=player_.CurrentTechnology();if (sid>std::numeric_limits<std::uint16_t>::max() || sid>=player_.Pool().Capacity()) throw std::out_of_range("기술표시의현재TechnologySID범위오류");return {static_cast<std::uint16_t>(sid)}; }
// 원본004acac0은SID가아닌저장kindWORD를돌려주므로kind0일치는여전히생성으로진행한다.
void RawIndicateTechnology::Indicate(std::uint32_t type) const {
    const auto found=carrier_.FindContained(Parent(),4,type,0);const auto kind=found.value ? Read(player_.Pool().Slot(found),18,2) : 0;
    if (!kind) create_.Create(Parent(),type,0,4,0x10,0);
}
// 실제같은월드어댑터검사용이다.
const SidPool& RawIndicateTechnology::Pool() const { return player_.Pool(); }
}
