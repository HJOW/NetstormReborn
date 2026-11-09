// 원래ownerBYTE/currentownerDWORD·원본32바이트기록과 실제owner변경 순서를 보존한다.
#include "o/RawIslandRuntime.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 기록의 실제stride와Reset가대입하는 -10000float비트다.
constexpr std::size_t kStride=32;
constexpr std::uint32_t kUnsetBounds=0xc61c4000U;
// owner변경 몸체가 Player 첫 지역과 직접 비교하는 두 판본의 고정DWORD다.
constexpr std::int32_t kUnassignedFirstRegion=127;
// MSVC signed char를 같은 비트의signedDWORD로 확장한다.
std::int32_t SignedByte(std::uint32_t value) { return static_cast<std::int32_t>(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(value))); }
// CD isValid의초기검사는float변환이 아닌원본DWORD의signed양수검사다.
bool PositiveBits(float value) { return std::bit_cast<std::int32_t>(value)>0; }
// 실제 필드의 낮은 BYTE부터 쓰며 정렬·구조체 padding을 가정하지 않는다.
void WriteIslandField(std::span<std::uint8_t> records,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    for (std::size_t byte=0;byte<width;++byte) records[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 해당 원문 필드의 폭만 쓴다.
}
// 원본의 정렬되지 않은 필드 폭만 little endian으로 읽는다.
std::uint32_t ReadIslandField(std::span<const std::uint8_t> records,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(records[offset+byte])<<(byte*8); //지정 필드의 BYTE만 순서대로 결합한다.
    return value;
}
// Delete와 전체 Reset은 같은 한 기록 몸체를 사용하며 물리 밖 입력은 쓰기 전에 거부한다.
void ResetIslandRecord(std::span<std::uint8_t> records,std::int32_t island) {
    if (records.size()!=RawIslandStorage::kBytes || island<0 || island>=100) throw std::out_of_range("Island Reset 실제3204BYTE/기록 범위 오류");
    const auto record=4+static_cast<std::size_t>(island)*kStride; // count 다음의 현재32BYTE 기록 시작이다.
    WriteIslandField(records,record,0);WriteIslandField(records,record+4,0);WriteIslandField(records,record+8,0);
    WriteIslandField(records,record+12,255,1);WriteIslandField(records,record+13,0,1);
    WriteIslandField(records,record+20,kUnsetBounds);WriteIslandField(records,record+16,kUnsetBounds);
    WriteIslandField(records,record+28,kUnsetBounds);WriteIslandField(records,record+24,kUnsetBounds);
}
}
// Session 없는 cleanup도 같은 물리의100행 지정 필드와 마지막 count를 직접 초기화한다.
void ResetRawIslandRecords(std::span<std::uint8_t> records) {
    if (records.size()!=RawIslandStorage::kBytes) throw std::invalid_argument("Island Reset 실제3204BYTE 범위 오류");
    for (std::int32_t island=0;island<100;++island) ResetIslandRecord(records,island); // 실제100행을 모두 쓰며 padding2BYTE는 유지한다.
    WriteIslandField(records,0,0);
}
// 원문의 signed 논리 범위가 먼저이며 그 관문을 통과한 물리100기록 밖 접근은 기존 호스트처럼 거부한다.
std::uint32_t RawIslandExists(std::span<const std::uint8_t> records,std::int32_t island) {
    if (records.size()!=RawIslandStorage::kBytes) throw std::invalid_argument("Island Exists 실제3204BYTE 범위 오류");
    if (island<0 || island>=std::bit_cast<std::int32_t>(ReadIslandField(records,0))) return 0;
    if (island>=100) throw std::out_of_range("IslandRuntime 물리기록범위오류");
    return ReadIslandField(records,4+static_cast<std::size_t>(island)*kStride+4);
}
// 현재존재관문과진단뒤재조회는World없는Caller와기존섬실행기가같은몸체를사용한다.
std::uint32_t RawIslandOwner(std::span<const std::uint8_t> records,std::int32_t island,const std::function<void()>& missing) {
    if (!RawIslandExists(records,island)) { if (!missing) throw std::logic_error("Island Owner 실제 부재 진단 미결합");missing(); }
    return RawIslandExists(records,island) ? ReadIslandField(records,4+static_cast<std::size_t>(island)*kStride+8):0;
}
// 원문Bounds는부재진단뒤에도현재네float를읽으며물리100기록밖만호스트경계로거부한다.
FortRandomRectangle RawIslandBounds(std::span<const std::uint8_t> records,std::int32_t island,const std::function<void()>& missing) {
    if (!RawIslandExists(records,island)) { if (!missing) throw std::logic_error("Island Bounds 실제 부재 진단 미결합");missing(); }
    if (island<0 || island>=100) throw std::out_of_range("IslandRuntime 물리기록범위오류");
    const auto record=4+static_cast<std::size_t>(island)*kStride;
    return {std::bit_cast<float>(ReadIslandField(records,record+16)),std::bit_cast<float>(ReadIslandField(records,record+20)),std::bit_cast<float>(ReadIslandField(records,record+24)),std::bit_cast<float>(ReadIslandField(records,record+28))};
}
// 실제3204바이트저장소와필수하위효과·기존모드/Player/목록을확인한다.
RawIslandRuntime::RawIslandRuntime(const SidPool& pool,const SquidHash& hash,std::span<std::uint8_t> records,
    FortFinishOwnerState& modes,SquidPostPopList& residences,std::uint32_t& worldRedraw,std::uint32_t& mapRevision,
    IslandRuntimeHooks hooks,std::int32_t worldExtent)
    :pool_(pool),hash_(hash),records_(records),modes_(modes),residences_(residences),worldRedraw_(worldRedraw),mapRevision_(mapRevision),hooks_(std::move(hooks)),worldExtent_(worldExtent) {
    if (records.size()!=kBytes || !hooks_.setResidenceOwner || (!hooks_.assertion && !hooks_.assertionAt)) throw std::invalid_argument("IslandRuntime 실제기록/필수효과누락");
}
// raw필드의낮은바이트부터결합한다.
std::uint32_t RawIslandRuntime::Read(std::size_t offset,std::size_t width) const {
    return ReadIslandField(records_,offset,width); //NoWorld 조회와 같은 실제 필드 읽기 몸체다.
}
// 원본지정폭의낮은바이트부터대입한다.
void RawIslandRuntime::Write(std::size_t offset,std::uint32_t value,std::size_t width) {
    // 필드이외의바이트는그대로남긴다.
    for (std::size_t index=0;index<width;++index) records_[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 범위밖원본포인터산술을호스트배열언더플로로수행하지않는다.
std::size_t RawIslandRuntime::Record(std::int32_t island) {
    if (island<0 || island>=100) throw std::out_of_range("IslandRuntime 물리기록범위오류");
    return 4+static_cast<std::size_t>(island)*kStride;
}
// 원본은DWORD3개/BYTE2개/float4개만초기화한다.
void RawIslandRuntime::ResetRecord(std::int32_t island) {
    ResetIslandRecord(records_,island);
}
// count와100개기록의실제Reset이며배치축약IslandList를새로만들지않는다.
void RawIslandRuntime::Reset() {
    ResetRawIslandRecords(records_);
}
// 실제unsignedDWORD count를진단/일반소비자에전달한다.
std::uint32_t RawIslandRuntime::Count() const { return Read(0); }
// 원본signed범위검사와존재DWORD반환을그대로보존한다.
std::uint32_t RawIslandRuntime::Exists(std::int32_t island) const {
    return RawIslandExists(records_,island); //Pool/효과 없이 같은 현재 count/존재DWORD를 조회한다.
}
// missing진단뒤현재Exists를다시읽어callback의원본상태변경도보존한다.
bool RawIslandRuntime::Require(std::int32_t island,bool bothEditions,std::uint32_t line) const {
    if (!Exists(island) && (bothEditions || pool_.Edition()==OriginalEdition::Patch1078)) Assert(IslandRuntimeAssertion::MissingIsland,line);
    return Exists(island)!=0;
}
// PATCH의원문위치를전달하고기존두판본fixture는원래enum소비자를유지한다.
void RawIslandRuntime::Assert(IslandRuntimeAssertion where,std::uint32_t line) const {
    if (hooks_.assertionAt) hooks_.assertionAt(where==IslandRuntimeAssertion::InvalidBounds ? "bcr.isValid()":"exists( islandId )",line);
    else hooks_.assertion(where);
}
// 원본Add의EAX는양판본모두이전count이며나머지Temple/bounds/padding은유지한다.
std::uint32_t RawIslandRuntime::Add(std::uint32_t owner,std::uint8_t territory) {
    const auto index=Count();if (index>=100) throw std::out_of_range("IslandRuntime Add 원본100기록경계");
    const auto record=Record(static_cast<std::int32_t>(index));Write(0,index+1);Write(record+8,owner);Write(record+13,owner,1);Write(record+4,1);Write(record+12,territory,1);return index;
}
// 삭제는기록만초기화하고count/원래번호재사용정책을변경하지않는다.
void RawIslandRuntime::Delete(std::int32_t island) { if (Require(island,false,0x285)) ResetRecord(island); }
// currentowner가바뀌어도일반모드의원래ownerBYTE는유지된다.
bool RawIslandRuntime::Matches(std::int32_t island,std::int32_t owner,std::int32_t territory) const {
    if (!Exists(island)) return false;const auto record=Record(island);
    if (!modes_.special && !modes_.chaffMode) { if (SignedByte(Read(record+13,1))!=owner) return false; }
    else if (!Read(record+8)) return false;
    return SignedByte(Read(record+12,1))==territory;
}
// 신전getter는패치진단뒤기록이여전히없으면0이다.
std::uint32_t RawIslandRuntime::Temple(std::int32_t island) const { return Require(island,false,0x2dd) ? Read(Record(island)) : 0; }
// 현재record신전DWORD를원본그대로대입한다.
void RawIslandRuntime::Temple(std::int32_t island,std::uint32_t temple) { if (Require(island,false,0x2f2)) Write(Record(island),temple); }
// ownergetter는원래ownerBYTE를반환하지않는다.
std::uint32_t RawIslandRuntime::Owner(std::int32_t island) const { return RawIslandOwner(records_,island,[this] { if (pool_.Edition()==OriginalEdition::Patch1078) Assert(IslandRuntimeAssertion::MissingIsland,0x2fb); }); }
// 논리부재진단뒤에도물리100기록안의원본float4개는읽는다.
FortRandomRectangle RawIslandRuntime::Bounds(std::int32_t island) const {
    return RawIslandBounds(records_,island,[this] { Assert(IslandRuntimeAssertion::MissingIsland,0x2ba); });
}
// 실제float비트만대입하며경계유효성검사는owner변경시별도실행한다.
void RawIslandRuntime::Bounds(std::int32_t island,const FortRandomRectangle& bounds) {
    if (!Require(island,false,0x2c0)) return;const auto record=Record(island);const std::array values{bounds.left,bounds.top,bounds.right,bounds.bottom};
    // 전달받은네float를그대로대입한다.
    for (std::size_t index=0;index<values.size();++index) Write(record+16+index*4,std::bit_cast<std::uint32_t>(values[index]));
}
// patch는두유효Point/포함rectangle,CD는양수DWORD비트와부분unordered수용조건을사용한다.
bool RawIslandRuntime::ValidBounds(const FortRandomRectangle& bounds) const {
    const auto extent=static_cast<float>(worldExtent_);
    if (pool_.Edition()==OriginalEdition::Patch1078)
        return bounds.left>0 && bounds.left<extent && bounds.top>0 && bounds.top<extent && bounds.right>0 && bounds.right<extent && bounds.bottom>0 && bounds.bottom<extent && bounds.left<=bounds.right && bounds.top<=bounds.bottom;
    return PositiveBits(bounds.left) && PositiveBits(bounds.top) && PositiveBits(bounds.right) && PositiveBits(bounds.bottom) && bounds.left<extent &&
        !(bounds.top>=extent) && !(bounds.right>=extent) && !(bounds.bottom>=extent) && bounds.right>=bounds.left && !(bounds.top>bounds.bottom);
}
// 실제후보검사는bounds의양끝을포함하며CD left/top의unordered만추가로허용한다.
bool RawIslandRuntime::Contains(const FortRandomRectangle& bounds,float x,float y) const {
    if (pool_.Edition()==OriginalEdition::Patch1078) return bounds.left<=x && x<=bounds.right && bounds.top<=y && y<=bounds.bottom;
    return !(bounds.left>x) && x<=bounds.right && !(bounds.top>y) && y<=bounds.bottom;
}
// owner변경의각전역쓰기/현재목록/가상효과/Player첫지역을원본순서로처리한다.
void RawIslandRuntime::Owner(std::int32_t island,std::uint32_t owner) {
    if (!Require(island,false,0x28f) || (modes_.fort && !modes_.special)) return;
    const auto record=Record(island);Write(record+8,owner);++worldRedraw_;if (pool_.Edition()==OriginalEdition::Patch1078) mapRevision_=1;
    const FortRandomRectangle bounds{std::bit_cast<float>(Read(record+16)),std::bit_cast<float>(Read(record+20)),std::bit_cast<float>(Read(record+24)),std::bit_cast<float>(Read(record+28))};
    if (!ValidBounds(bounds)) Assert(IslandRuntimeAssertion::InvalidBounds,0x2a4);
    const auto ownerOffset=pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U;
    // 원본은목록count/SID/type/좌표와표면WORD를매반복다시읽고free/dead필터를추가하지않는다.
    for (std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(residences_.count);++index) {
        if (index>=residences_.entries.size()) throw std::out_of_range("IslandRuntime 현재residences저장소부족");
        const auto number=residences_.entries[index];if (number>=pool_.Capacity() || number>65535) throw std::out_of_range("IslandRuntime residence SID범위오류");
        const auto sid=Sid{static_cast<std::uint16_t>(number)};const auto raw=pool_.Slot(sid);if (raw[10]!=modes_.residenceType) continue;
        const auto read=[raw](std::size_t offset) {
            std::uint32_t value=0;
            // 비정렬좌표DWORD의지정4바이트만결합한다.
            for (std::size_t byte=0;byte<4;++byte) value|=static_cast<std::uint32_t>(raw[offset+byte])<<(byte*8);
            return std::bit_cast<float>(value);
        };
        const float x=read(14),y=read(18);if (!Contains(bounds,x,y)) continue;
        const auto support=RawPathSurfaceAt(hash_,{x,y});hooks_.setResidenceOwner(sid,pool_.Slot(support)[ownerOffset]);
    }
    if (owner) {
        if (owner>=modes_.playerRegions.size()) throw std::out_of_range("IslandRuntime Player첫지역범위오류");
        if (FirstRegion(owner)==kUnassignedFirstRegion) FirstRegion(owner,island);
    }
}
// 같은mutable100개기록을외부소비자에게제공한다.
std::span<const std::uint8_t> RawIslandRuntime::Bytes() const { return records_; }
// 실제pool참조를월드바인딩검사에전달한다.
const SidPool& RawIslandRuntime::Pool() const { return pool_; }
// 잘못된 물리크기를 쓰기 전에 거부하고 같은 Player 주소만 빌린다.
void RawIslandRuntime::BindPlayerRecords(std::span<std::uint8_t> players) {
    const auto stride=pool_.Edition()==OriginalEdition::Patch1078 ? 0xb4U : 0xacU; // 실제 판본별 한 기록의 크기다.
    if (players.size()!=9*stride) throw std::invalid_argument("IslandRuntime 실제9 Player 크기 오류");players_=players;currentPlayers_={};
}
// 실제 Native owner는 장기 본체보다 오래 살며 결합 때 Player를 만들지 않는다.
void RawIslandRuntime::BindCurrentPlayers(std::function<std::span<std::uint8_t>()> players) { if (!players) throw std::invalid_argument("IslandRuntime 현재 Player getter 미결합");currentPlayers_=std::move(players); }
// 별도 Player가 없는 기존 독립 입력만 빈 span을 허용하고 Native는 현재9기록을 요구한다.
std::span<std::uint8_t> RawIslandRuntime::CurrentPlayers() const {
    if (!currentPlayers_) return players_;const auto current=currentPlayers_();const auto stride=pool_.Edition()==OriginalEdition::Patch1078 ? 0xb4U:0xacU; //원문 현재 base와 판본별9기록이다.
    if (current.size()!=9*stride) throw std::invalid_argument("IslandRuntime 실제 현재9 Player 크기 오류");return current;
}
// 외부 residence callback 뒤의 현재 Player DWORD를 다시 읽는다.
std::int32_t RawIslandRuntime::FirstRegion(std::uint32_t owner) const {
    const auto players=CurrentPlayers();if (players.empty()) return modes_.playerRegions.at(owner);
    if (owner>8) throw std::out_of_range("IslandRuntime 실제 Player 첫 지역 범위 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078; // 첫 지역의 판본별 위치를 선택한다.
    const auto field=owner*(patch ? 0xb4U : 0xacU)+(patch ? 0x78U : 0x70U); // 현재owner의 물리DWORD 주소다.
    std::uint32_t value=0; // 현재 기록에서 읽은 원본 비트다.
    // 정확한 네BYTE만 조합한다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(players[field+index])<<(index*8);
    return std::bit_cast<std::int32_t>(value);
}
// 공유기록 모드에서는 첫 지역 필드만 저장한다. 다른 Player 메모리와 독립 상태는 보존한다.
void RawIslandRuntime::FirstRegion(std::uint32_t owner,std::int32_t region) {
    const auto players=CurrentPlayers();if (players.empty()) { modes_.playerRegions.at(owner)=region;return; }
    if (owner>8) throw std::out_of_range("IslandRuntime 실제 Player 첫 지역 범위 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078; // 실제 저장폭은 두 판본 모두DWORD다.
    const auto field=owner*(patch ? 0xb4U : 0xacU)+(patch ? 0x78U : 0x70U); // 현재owner의 첫 지역 주소다.
    const auto value=std::bit_cast<std::uint32_t>(region); // 부호와 무관하게 원본 비트를 저장한다.
    // 인접 신전/거주 구역 필드를 바꾸지 않는다.
    for (std::size_t index=0;index<4;++index) players[field+index]=static_cast<std::uint8_t>(value>>(index*8));
}
}
