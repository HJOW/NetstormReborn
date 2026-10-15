// 원본의실제미초기화byte보존/17목록할당/필드쓰기/지역·방향별동맹과역순소멸을유지한다.
#include "o/RawAiBrain.h"
#include "o/RawMissionAiSettings.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kUnset=0xc61c4000; //원본-10000.0f의비트다.
constexpr std::array<std::pair<std::size_t,std::uint32_t>,16> kLists{{{0x284,10},{0x2dc,500},{0x2e8,500},{0x2f4,500},{0x300,4},{0x30c,5},{0x318,5},{0x324,40},{0x330,500},{0x33c,500},{0x348,500},{0x354,500},{0x360,500},{0x36c,500},{0x378,40},{0x384,2}}}; //실제16개pointer/capacity/count필드순서다.
// 정렬되지않은원본DWORD를현재소유메모리에서읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 실제네바이트만합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);return value;
}
// 원본DWORD를같은실제소유메모리에쓴다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    // 주변미초기화byte는보존한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 선택된실제지면/metadata효과가없으면명시오류로보고한다.
template<class Function> const Function& Need(const Function& function,const char* name) { if (!function) throw std::logic_error(name);return function; }
// 원본TYPE요구에너지문자열ss의ASCII대소문자무시비교다.
bool DoubleSun(std::string_view value) { value=value.substr(0,value.find('\0'));return value.size()==2 && (value[0]=='s' || value[0]=='S') && (value[1]=='s' || value[1]=='S'); }
}

// 할당storage의초기byte입력과opaque32주소공간을명시한다.
RawAiBrainMemory::RawAiBrainMemory(std::uint32_t firstToken,Fill fill):next_(firstToken),fill_(std::move(fill)) { if (!firstToken || !fill_) throw std::invalid_argument("AI실제메모리핸들/미초기화byte입력누락"); }
// 등록에성공한block만다음핸들과요청이력에추가한다.
std::uint32_t RawAiBrainMemory::Allocate(std::size_t size) {
    const auto step=(static_cast<std::uint64_t>(size)+7U)&~7ULL;if (!size || step>std::numeric_limits<std::uint32_t>::max()-next_) throw std::out_of_range("AI실제소유메모리주소공간소진");
    std::vector<std::uint8_t> block(size);fill_(block);const auto token=next_;const auto inserted=blocks_.emplace(token,std::move(block));if (!inserted.second) throw std::logic_error("AI메모리핸들중복");next_+=static_cast<std::uint32_t>(step);allocations_.push_back({token,size});return token;
}
// 해제직전까지같은byte저장소를조회할수있고이후에는재조회할수없다.
void RawAiBrainMemory::Free(std::uint32_t token) { if (!token) return;if (!blocks_.erase(token)) throw std::logic_error("AI실제소유자없는메모리해제");frees_.push_back(token); }
// 가짜표식/해제토큰을실제메모리처럼다루지않는다.
std::span<std::uint8_t> RawAiBrainMemory::Bytes(std::uint32_t token) { const auto found=blocks_.find(token);if (found==blocks_.end()) throw std::out_of_range("AI살아있는소유메모리누락");return found->second; }
// readonly소비자도같은살아있는원문을읽는다.
std::span<const std::uint8_t> RawAiBrainMemory::Bytes(std::uint32_t token) const { const auto found=blocks_.find(token);if (found==blocks_.end()) throw std::out_of_range("AI살아있는소유메모리누락");return found->second; }
// 실제할당수명조회다.
bool RawAiBrainMemory::Contains(std::uint32_t token) const { return blocks_.contains(token); }
// 관찰된실제할당순서와크기를반환한다.
const std::vector<AiBrainAllocation>& RawAiBrainMemory::Allocations() const { return allocations_; }
// 관찰된실제해제순서를반환한다.
const std::vector<std::uint32_t>& RawAiBrainMemory::Frees() const { return frees_; }

// 현재Player9/섬기록과같은Pool/방향별81DWORD만연결한다.
RawAiBrainConstructor::RawAiBrainConstructor(RawAiBrainMemory& memory,RawPlayerWorld& player,RawPlayerParticipation& participation,RawIslandRuntime& islands,std::span<std::uint32_t> allies,std::span<const RiftTypeRecord> types,AiBrainConstructorGlobals globals,AiBrainConstructorHooks hooks)
    :memory_(memory),player_(player),participation_(participation),islands_(islands),allies_(allies),types_(types),globals_(globals),hooks_(std::move(hooks)) {
    if (player.Pool().Edition()!=OriginalEdition::Patch1078 || &player.Pool()!=&islands.Pool() || player.Records().size()!=1620 || allies.size()!=81 || types.empty() || !globals_.invalidGraph) throw std::invalid_argument("AI생성자같은PATCH세계/Player9/동맹표/현재그래프조회누락");
}
// 실제0xb08본체는목록생성실패때도소유자가정리한다.
std::uint32_t RawAiBrainConstructor::Construct(std::uint32_t owner,std::span<const std::uint8_t> settings,std::size_t bytes) {
    if (bytes!=kBytes) throw std::invalid_argument("AI실제본체크기는0xb08입니다");const auto token=memory_.Allocate(kBytes);try { Initialize(token,owner,settings); } catch (...) { memory_.Free(token);throw; }return token;
}
// 미초기화영역전체를지우지않고원본이실제로쓰는field/목록만구성한다.
void RawAiBrainConstructor::Initialize(std::uint32_t token,std::uint32_t owner,std::span<const std::uint8_t> settings) {
    if (settings.size()!=RawMissionAiSettings::kSize || owner>=9 || live_.contains(token) || destructed_.contains(token)) throw std::invalid_argument("AI생성자실제설정/owner/새본체계약오류");
    auto raw=memory_.Bytes(token);if (raw.size()!=kBytes) throw std::invalid_argument("AI생성자실제0xb08소유메모리누락");std::vector<std::uint32_t> made;
    const auto allocate=[&](std::size_t bytes) { const auto block=memory_.Allocate(bytes);made.push_back(block);return block; }; //생성중예외시실제구성된목록만역순정리한다.
    try {
        std::copy(settings.begin(),settings.end(),raw.begin());Put(raw,0x274,kUnset);Put(raw,0x270,kUnset);Put(raw,0x27c,kUnset);Put(raw,0x278,kUnset);
        Put(raw,0x284,allocate(40));Put(raw,0x288,10);Put(raw,0x28c,0);Put(raw,0x2bc,kUnset);Put(raw,0x2b8,kUnset);Put(raw,0x2c4,kUnset);Put(raw,0x2c0,kUnset);
        // 첫목록뒤15개는원본capacity×DWORD크기와포인터/capacity/count순서다.
        for (std::size_t index=1;index<kLists.size();++index) { const auto [offset,capacity]=kLists[index];Put(raw,offset,allocate(capacity*4U));Put(raw,offset+4,capacity);Put(raw,offset+8,0); }
        constexpr std::array<std::pair<std::size_t,std::uint32_t>,27> firstFields{{{0x3cc,0},{0x3d4,kUnset},{0x3d0,kUnset},{0x3d8,1},{0x3dc,0},{0x3f8,0},{0x404,0},{0x3e0,0},{0x3e4,6},{0x3f0,1},{0x3f4,0},{0x3fc,0},{0x400,0},{0x3ec,0},{0x574,0},{0x5a4,0},{0x58c,0},{0x590,0},{0x5a0,0},{0x59c,0},{0x598,0},{0x594,0},{0x628,0},{0x5f4,0},{0x5f0,0},{0xadc,10},{0xae0,0}}}; //ae4할당전에원본이쓰는고정필드다.
        // 원본상수와zero필드를같은실제storage에쓴다.
        for (const auto [offset,value]:firstFields) Put(raw,offset,value);
        constexpr std::array<std::size_t,10> doubles{0x5a8,0x5b0,0x5b8,0x5c0,0x5c8,0x5d0,0x5d8,0x5e0,0x5e8,0x630}; //00500460의실제double0을쓰는위치다.
        // 원본double0의두DWORD만변경한다.
        for (const auto offset:doubles) { Put(raw,offset,0);Put(raw,offset+4,0); }
        Put(raw,0xaec,owner);Put(raw,0xae8,0);Put(raw,0xae4,allocate(120));Put(raw,0x2d8,Read(raw,0x22c));Put(raw,0x260,owner);
        constexpr std::array<std::pair<std::size_t,std::uint32_t>,30> lastFields{{{0x2d4,0},{0x264,0},{0x2a4,0},{0x258,0},{0x25c,0},{0x6d0,0},{0xad8,2},{0x390,3},{0xad4,kUnset},{0xad0,kUnset},{0xaf4,kUnset},{0xaf0,kUnset},{0x3e0,0},{0x3e4,6},{0x3f0,1},{0x3f4,0},{0x3fc,0},{0x400,0},{0x3ec,0},{0x3b0,0},{0x394,0},{0x2cc,0},{0x2c8,0},{0x2d0,0},{0x290,0},{0x3c8,3},{0xaf8,10},{0xafc,10},{0xb00,0},{0x294,0}}}; //첫지역/metadata조회전의실제고정필드다.
        // 원본고정배열255DWORD만0으로만든다.
        for (std::size_t index=0;index<255;++index) Put(raw,0x6d4+index*4,0);
        // 주변미초기화영역을보존한다.
        for (const auto [offset,value]:lastFields) Put(raw,offset,value);
        Put(raw,0x298,globals_.invalidGraph());Put(raw,0x2a8,0);Put(raw,0x2ac,globals_.invalidGraph());
        const auto records=player_.Records();
        const auto playerOffset=[&]() { const auto current=Read(raw,0x260);if (current>=9) throw std::out_of_range("AI현재owner실제9기록범위오류");return static_cast<std::size_t>(current)*180; }; //callback뒤에도현재brainowner를다시읽는다.
        if (std::bit_cast<std::int32_t>(Read(records,playerOffset()+0x78))==globals_.noRegion) {
            const auto priest=Need(hooks_.priest,"AI생성자실제사제조회누락")(Read(raw,0x260));
            if (priest.value>0 && priest.value<player_.Pool().Capacity()) {
                const auto piece=player_.Pool().Slot(priest);const auto region=Need(hooks_.regionAt,"AI생성자실제좌표지역조회누락")(std::bit_cast<float>(Read(piece,14)),std::bit_cast<float>(Read(piece,18)));
                if (islands_.Exists(region)!=0) { islands_.Owner(region,Read(raw,0x260));Put(records,playerOffset()+0x78,static_cast<std::uint32_t>(region)); }
            }
        }
        Put(raw,0x280,0);
        // 실제flags40800후요구에너지문자열ss에맞는첫타입만저장한다.
        for (std::size_t type=0;type<types_.size();++type) if (types_[type].flags2&0x40800U) if (DoubleSun(Need(hooks_.requiredEnergy,"AI생성자실제TYPE에너지문자열누락")(static_cast<std::uint32_t>(type)))) { Put(raw,0x280,static_cast<std::uint32_t>(type));break; }
        if (!(raw[0x248]&1)) { Put(raw,0x370,0);Put(raw,0x37c,0); }
        participation_.SetAlliance(Read(raw,0x260),Read(raw,0x23c));const auto current=Read(raw,0x260);if (current>=9 || globals_.localOwner>=9) throw std::out_of_range("AI현재방향별동맹범위오류");
        if (allies_[current*9+globals_.localOwner]) participation_.AddAlly(globals_.localOwner,Read(raw,0x260));live_.insert(token);
    } catch (...) { for (auto item=made.rbegin();item!=made.rend();++item) memory_.Free(*item);throw; }
}
// 소멸자는본체의현재pointerfield를읽는다. 후속전략의실제재할당도같은등록부를사용해야한다.
void RawAiBrainConstructor::Destruct(std::uint32_t token) {
    if (!live_.contains(token)) throw std::logic_error("AI실제생성자완료없는소멸요청");const auto raw=memory_.Bytes(token);if (Read(raw,0xae4)) memory_.Free(Read(raw,0xae4));
    // 원본은capacity가0으로바뀌어도실제포인터를역순해제한다.
    for (auto item=kLists.rbegin();item!=kLists.rend();++item) memory_.Free(Read(raw,item->first));live_.erase(token);destructed_.insert(token);
}
// Participation의destructBrain뒤freeBrain에분리연결한다.
void RawAiBrainConstructor::FreeBody(std::uint32_t token) { if (!destructed_.erase(token)) throw std::logic_error("AI실제목록소멸전본체해제요청");memory_.Free(token); }
// 모든후속소비자는실제살아있는동일원문을읽고쓴다.
std::span<std::uint8_t> RawAiBrainConstructor::Bytes(std::uint32_t token) { return memory_.Bytes(token); }
// 상위세계바인딩이같은Pool인지검증한다.
const SidPool& RawAiBrainConstructor::Pool() const { return player_.Pool(); }
}
