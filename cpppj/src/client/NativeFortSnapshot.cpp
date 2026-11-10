//원문Fort저장의순서/현재값재조회와부분효과를같은장기데이터/실제파일장치에연결한다.
#include "client/NativeFortSnapshot.h"
#include "client/NativeFortSaveVirtual.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientTimeSource.h"
#include "client/CampaignTemplateStorage.h"
#include "client/CurrentAllocationBytes.h"
#include "client/EditorFiles.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawCursorPosition.h"
#include "client/RawCarrierDropPoint.h"
#include "o/RawFortSnapshotParent.h"
#include "o/RawFortSnapshotBasics.h"
#include "o/RawFortContentSave.h"
#include "o/RawFortObjectSave.h"
#include "o/RawFortObjectsSave.h"
#include "o/RawFortRegionSave.h"
#include "o/RawFortTerritorySave.h"
#include "o/RawFortMoneySave.h"
#include "o/RawFortBadgesSave.h"
#include "o/RawFortBadgesGlobals.h"
#include "o/RawPlayerTemplateState.h"
#include "o/RawGameAuxState.h"
#include "o/RawSquidFinder.h"
#include "o/RawFortCoreWrite.h"
#include "o/RawFortSpWrite.h"
#include "o/RawGameRandomSeed.h"
#include "o/RawPlayerCoreRefresh.h"
#include "o/FortSerializationGlobals.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#include <utility>

namespace netstorm::client { //모든저장데이터는기존장기주소를빌리고이호출은stream참조/커서만임시보유한다.
namespace { //NoWorld의직렬화adapter이며RawSurfaceWorld/Session/Meta의대체객체가아니다.
// bytes의 at에서 지정 폭(기본 DWORD)을 little endian으로 읽는다. 선택한 물리가 부족하면 그 위치에서 중단한다.
std::uint32_t ReadField(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>4 || width>bytes.size()-at) throw std::out_of_range("Native Fort저장의선택필드물리범위부족");std::uint32_t value{};
    // 지정된 낮은 BYTE부터 해당 폭만 합쳐 주변 필드를 섞지 않는다.
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; //필요한BYTE/WORD/DWORD만비정렬little endian으로읽는다.
}
// bytes의 at에 값의 지정 폭만 little endian으로 쓴다. 이미 수행한 앞선 원문 효과와 주변 필드를 보존한다.
void WriteField(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    if (at>bytes.size() || width>4 || width>bytes.size()-at) throw std::out_of_range("Native Fort저장의선택쓰기물리범위부족");
    // 지정된 낮은 BYTE부터 같은 폭만 저장하고 이웃 필드는 그대로 둔다.
    for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); //주변필드를유지하고원문지정폭만쓴다.
}
// 원본 MSVC char의 signed 확장을 DWORD 비교에 사용하도록 BYTE 비트를 부호형으로 돌려준다.
std::int32_t SignedByte(std::uint8_t value) { return std::bit_cast<std::int8_t>(value); } //원본MSVC char의signed확장을고정한다.
// Deck의 signed 항목 번호와 항목 내부 offset을 실제5BYTE 배열 위치로 변환한다. 음수 count 자체를 정상화하지 않는다.
std::size_t SelectedEntry(std::int32_t index,std::size_t field=0) {
    if (index<0) throw std::out_of_range("Native Deck의선택된음수주소물리범위부족");return static_cast<std::size_t>(index)*5+field;
} //signed count자체는정상화하지않고실제배열접근이선택된위치에서만거부한다.
// 매 후보의 현재 Content TYPE를 읽고 반환 전에 next를 보관하는 독립 원문 커서다.
class NativeContentCursor {
public:
    // 같은 장기 Pool/권한을 빌려 parent의 현재 종속 머리에서 첫 Content 후보까지 진행한다.
    NativeContentCursor(std::shared_ptr<RawSurfaceStorage> storage,std::shared_ptr<ClientGameplayGlobals> game,std::uint32_t parent)
        :storage_(std::move(storage)),game_(std::move(game)) { // 기존 장기 owner를 보유하고 새 Pool/Player/TYPE를 만들지 않는다.
        auto raw=Raw(parent);
        if (game_->finder.checkingDead && (raw[11]&2U) && !game_->finder.Boss())
            storage_->AssertDisplay("!p.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy","\\Ns\\O\\Squidfinder.cpp",0x13c);
        next_=ReadField(Raw(parent),6,2);Next(); // 진단 정상 복귀 뒤 현재 머리를 읽고 첫 후보까지 진행한다.
    }
    // Begin/Next가 반환한 현재 SID만 읽는다. 다음 링크를 새로 조회하거나 후보를 진행하지 않는다.
    std::uint32_t Current() const { return current_; } // 마지막 Begin/Next가 반환한 같은 DWORD다.
    void Next() { // 각 후보의 next를 먼저 저장하고 그때의 같은 Content TYPE를 비교한다.
        current_=0;
        while (next_) { // 이미 반환한 후보의 링크가 바뀌어도 저장했던 다음 링크에서 시작한다.
            const auto selected=next_;if (!visited_.insert(selected).second) throw std::logic_error("Native Fort 내용물 finder의 종속 순환");
            const auto raw=Raw(selected);next_=ReadField(raw,4,2);
            if (raw[10]==storage_->ContentType()) { current_=selected;return; }
        }
    }
private:
    std::span<const std::uint8_t> Raw(std::uint32_t sid) const { // DWORD SID를 미리 WORD로 자르지 않고 실제 접근 위치에서 검사한다.
        if (sid>=storage_->pool.Capacity() || sid>65535U) throw std::out_of_range("Native Fort 내용물 finder의 실제 SID 물리 범위");return storage_->pool.Slot(o::Sid{static_cast<std::uint16_t>(sid)});
    }
    std::shared_ptr<RawSurfaceStorage> storage_;std::shared_ptr<ClientGameplayGlobals> game_; // 현재 검색이 빌리는 실제 물리/권한 owner다.
    std::uint32_t current_{},next_{};std::unordered_set<std::uint32_t> visited_; // 중첩 커서마다 현재/다음과 손상 순환 표식을 따로 둔다.
};
// 같은 Template의 현재 State/Get·확장만 조립하는 순수 view이며 새 Player/World를 만들지 않는다.
std::unique_ptr<o::RawPlayerTemplateState> MakeNativeTemplateState(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates) {
    if (!templates) throw std::logic_error("Native State Get의 실제 Template owner 미설치");const auto storage=client.SurfaceStorage();
    auto state=std::make_unique<o::RawPlayerTemplateState>(o::OriginalEdition::Patch1078,templates->Headers(),[storage,templates](auto slot)->o::PlayerTemplateStateStream& { return templates->SelectState(slot); },[storage,templates](auto slot)->const std::string& { return templates->Name(slot); },templates->LoadingDepth(),storage->links.mission,
        [storage,templates](auto kind) {
            std::string_view expression="tm";std::uint32_t line=0x1fc;
            switch (kind) {
            case o::PlayerTemplateStateAssertion::DotFileName:expression="tm->fileName[0] != '.'";line=0x1c1;break;
            case o::PlayerTemplateStateAssertion::MineTransientFlag:expression="0";line=0x1c3;break;
            case o::PlayerTemplateStateAssertion::InMissionSave:expression="!inMission";line=0x1c6;break;
            case o::PlayerTemplateStateAssertion::MissingNotifyTemplate:line=0x1c7;break;
            default:break;
            }
            storage->AssertDisplay(expression,"\\Ns\\O\\Template.cpp",line);
        },[storage,templates](auto depth) { templates->LoadingDepth()=depth; }); //PATCH조회는불필요한깊이callback을선택하지않으며같은owner참조만보유한다.
    state->BindInitialization({[](auto,std::span<std::uint8_t,28> bytes) { CurrentAllocationBytes(bytes); },[storage,templates](auto slot,auto bytes)->o::PlayerTemplateStateStream& { return templates->InitializeState(slot,bytes); }});
    state->BindMinimumSize([storage,templates](auto slot,auto minimum)->o::PlayerTemplateStateStream& { return templates->EnsureStateSize(slot,minimum); });
    return state;
}
class SnapshotCall { //Client의장기소유자를빌리며그뒤현재child를원문순서로선택한다.
public:
    // 같은 Client/Template/옵션/허용표/Mission/작업 파일을 빌리는 동기 저장 호출을 구성한다. 생성자는 게임 전역을 초기화하지 않는다.
    SnapshotCall(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::span<std::uint8_t,87> options,std::span<std::uint32_t,256> restrictions,const o::FortMissionSaveHooks& mission,const EditorFiles& files);
    void Run(std::uint32_t slot,bool battle); //4bf9a0/4bf880부모의현재manager/모드/owner와전체자식을결합한다.
    // 별도의 전체 snapshot 없이 현재 Template의 실제 Notify/파일 저장 관문만 선택한다.
    void Notify(std::uint32_t slot) { state_->Changed(slot); } // 순수 현재 Template Notify만 같은 장기 view로 실행한다.
private:
    using Operation=std::function<void(std::span<std::uint8_t>,const o::MemStreamHooks&)>; //같은실제stream필드/버퍼를동기적으로빌리는일반writer형식이다.
    Client& client_;std::shared_ptr<CampaignTemplateStorage> templates_; //Client와같은Template9의실제수명을snapshot복귀까지보유한다.
    std::shared_ptr<RawSurfaceStorage> storage_;std::shared_ptr<ClientGameplayGlobals> game_; //같은Pool/TYPE/지도/현재전역이며Runtime은보유하지않는다.
    std::shared_ptr<ClientPlayerRecords> players_;std::span<std::uint8_t,87> options_; //실제Player할당과같은87BYTE옵션/숨김9BYTE를사용한다.
    std::span<std::uint32_t,256> restrictions_;o::FortMissionSaveHooks mission_;const EditorFiles& files_; //현재허용표/현재Missiongetter/실제작업파일장치를빌린다.
    std::shared_ptr<o::FortSerializationGlobals> serialization_; //같은Template/Storage의판본·scope·두mask와실제깊이를pin한다.
    NativeFortSaveVirtual virtual_;std::vector<TemplateWriteStream> streams_; //같은현재가상조회와각Create반환수명만유지한다.
    std::unique_ptr<o::RawPlayerTemplateState> state_; //Get/Notify의순수장기자료view이며Player/World를생성하거나정리하지않는다.
    std::uint32_t Manager(std::int32_t slot) const; //고정this슬롯의현재첫DWORD를읽는다.
    std::uint32_t Remember(TemplateWriteStream stream); //실제Create/Select가돌려준같은stream에이호출의opaque참조만부여한다.
    TemplateWriteStream Resolve(std::uint32_t token) const; //최초발급된같은stream을조회하고현재manager로대체하지않는다.
    void CheckManager(std::uint32_t slot,std::uint32_t manager) const; //실제DataManager식별자의현재슬롯별칭을확인한다.
    std::uint32_t Create(std::uint32_t slot,std::uint32_t manager,std::string_view name); //원문새섹션의실제할당/반환수명을기록한다.
    void Write(std::uint32_t token,std::span<const std::uint8_t> bytes); //캡처된같은stream에일반writer를호출한다.
    void Access(std::uint32_t token,const Operation& operation); //현재필드/버퍼를기존MemStream몸체에빌린다.
    void Assert(std::string_view expression,std::string_view file,std::uint32_t line) const; //같은Client중앙진단의정상복귀를caller에그대로전달한다.
    std::uint32_t CurrentTechnology() const; //현재PlayerKnown0은원문처럼0이며그외현재Player+9c를읽는다.
    o::Sid Sid(std::uint32_t value) const; //DWORD SID를미리WORD로자르지않고선택된실제풀범위를확인한다.
    bool IslandExists(std::int32_t island) const; //현재signed count뒤존재DWORD를읽는다.
    bool RegionAllowed(std::int32_t island,std::uint32_t owner,std::int32_t region) const; //46f200의현재editor/missionPost분기를사용한다.
    std::uint32_t HasIsland(std::uint32_t x,std::uint32_t y) const; //46f6e0/46f680의실제표면WORD/현재terrainTYPE조회다.
    void Basics(std::uint32_t slot,bool names); //TypeNames/Deck의기존순수저장부모를실제현재child에결합한다.
    void Contents(std::uint32_t parent,std::uint32_t stream); //현재contained타입/권한/Pool로전체재귀내용물을저장한다.
    o::FortObjectSaveHooks ObjectHooks(); //현재전역/가상조회/stream/내용물의원문읽기장치를만든다.
    void Chunk(std::uint32_t stream,std::int32_t x,std::int32_t y); //현재해시의독립Finder로한청크를저장한다.
    void Objects(std::uint32_t slot); //현재version/scope를공유해20Terr와조건부Chaff를저장한다.
    void Territory(std::uint32_t slot); //기존120BYTE/현재지도/누락seed와stream재조회를유지한다.
    void RefreshCore(); //0044fc00의현재Core/기술/다이스count를같은순수부모로갱신한다.
    void CoreData(std::uint32_t slot); //004beb30의현재Core/단일난수/seed/SPwriter를연결한다.
    void ReadTerritory(std::uint32_t slot,std::span<std::uint8_t,120> output); //004bd660의현재manager/stream과120BYTE읽기를그대로진행한다.
    o::ChunkEntry& SelectChunk(std::int32_t x,std::int32_t y); //00432180의원문진단뒤현재지도를선택한다.
    std::array<std::uint8_t,6> IslandRecord(std::int32_t island); //00432430의최초selector와현재높이재조회/첫일치record를사용한다.
    void NotifySave(std::uint32_t slot); //같은DataManager/현재파일/숨김표/실제writer로SaveFile한다.
    std::uint32_t GetDeck(std::uint32_t owner); //같은장기Player+68블록의getter이며소멸/반납을추가하지않는다.
    std::span<std::uint8_t> Deck(std::uint32_t token) const; //같은현재352BYTE할당을빌린다.
    std::int32_t DeckCount(std::uint32_t token) const; //음수는원문signed조건에남기고양수도선택접근까지현재값을유지한다.
    void AppendDeck(std::uint32_t token,std::uint32_t type); //44f290/44f160의현재count/진단/부분쓰기순서다.
    void ActivateDeck(std::uint32_t token,std::uint32_t type); //44f3d0의첫BYTE일치/power복구/없으면추가다.
    bool Allowed(std::int32_t type) const; //현재TYPE범위진단뒤같은장기금지DWORD를읽는다.
    void FactoryProducts(o::Sid factory,std::uint32_t deck); //실제mode0은허용표조회와Deck활성화만수행하며Contained생성을하지않는다.
    void RefreshDeck(std::uint32_t owner); //44f7c0의현재목록/TYPE/설정과현재signed count·부분효과다.
};
//현재Stream/State의실제장치를연결만하고게임전역/loaded/권한을초기화하지않는다.
SnapshotCall::SnapshotCall(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::span<std::uint8_t,87> options,std::span<std::uint32_t,256> restrictions,const o::FortMissionSaveHooks& mission,const EditorFiles& files)
    :client_(client),templates_(templates),storage_(client.SurfaceStorage()),game_(client.GameplayGlobals()),players_(client.PlayerRecords()),options_(options),restrictions_(restrictions),mission_(mission),files_(files),serialization_(templates ? templates->SerializationGlobalsOwner():nullptr),virtual_(*storage_,*game_->GameAux()) {
    if (!templates_ || !serialization_) throw std::invalid_argument("Native Fort snapshot의실제Template/직렬화장기owner없음");
    state_=MakeNativeTemplateState(client_,templates_);
    state_->BindFileSave([this](auto slot) { NotifySave(slot); });
}
// 지정 Template9 슬롯의 현재 manager DWORD를 읽는다. loaded나 게임 모드를 선행 자격으로 검사하지 않는다.
std::uint32_t SnapshotCall::Manager(std::int32_t slot) const { if (slot<0 || slot>=9) throw std::out_of_range("Native Fort Template9선택물리범위부족");return ReadField(templates_->Headers(),static_cast<std::size_t>(slot)*20); } //manager읽기전loaded/모드를검사하지않는다.
// 실제 Create/Select가 반환한 stream 수명에 이 동기 호출에서만 유효한 비영 참조를 발급한다.
std::uint32_t SnapshotCall::Remember(TemplateWriteStream stream) { if (!stream.manager || !stream.section) return 0;streams_.push_back(stream);return static_cast<std::uint32_t>(streams_.size()); } //실제반환stream의정체성만보관한다.
// 처음 발급한 같은 stream 참조를 돌려준다. manager.current 재선택으로 원래 반환 수명을 교체하지 않는다.
TemplateWriteStream SnapshotCall::Resolve(std::uint32_t token) const { if (!token || token>streams_.size()) throw std::out_of_range("Native Fort저장의선택stream물리범위부족");return streams_[token-1]; } //현재manager.current로재선택하지않는다.
// 선택 manager와 고정 this 슬롯의 현재 manager가 같은 실제 저장소인지 확인한다.
void SnapshotCall::CheckManager(std::uint32_t slot,std::uint32_t manager) const { if (manager!=Manager(static_cast<std::int32_t>(slot))) throw std::logic_error("Native Fort고정슬롯현재manager불일치"); } //실제선택manager의alias만검사한다.
// 고정 슬롯의 현재 manager에 원문 섹션 이름으로 Create하고 반환된 실제 stream을 기억한다.
std::uint32_t SnapshotCall::Create(std::uint32_t slot,std::uint32_t manager,std::string_view name) { CheckManager(slot,manager);return Remember(templates_->CreateSection(slot,name)); } //Create의부분효과/할당실패는기존장치가소유한다.
// 앞서 보관한 같은 stream에 일반 MemStream writer를 호출한다. 다른 current 섹션을 재선택하지 않는다.
void SnapshotCall::Write(std::uint32_t token,std::span<const std::uint8_t> bytes) { templates_->WriteSection(Resolve(token),bytes); } //같은발급수명의일반writer를호출한다.
// 앞서 발급한 stream의 같은 필드와 버퍼를 원본 인라인 writer에 동기적으로 빌린다.
void SnapshotCall::Access(std::uint32_t token,const Operation& operation) { templates_->AccessSection(Resolve(token),operation); } //현재16BYTE/버퍼의기존장치를직접빌린다.
// 표현식·파일·행을 같은 Client 진단 장치로 전달하고 정상 복귀 뒤의 원문 처리를 계속하게 한다.
void SnapshotCall::Assert(std::string_view expression,std::string_view file,std::uint32_t line) const { storage_->AssertDisplay(expression,file,line); } //정상복귀를새earlyreturn으로바꾸지않는다.
// 0048fc60처럼 현재 known0이면0, 비영이면 현재 battleOwner의 Player+9c DWORD를 직접 읽는다.
std::uint32_t SnapshotCall::CurrentTechnology() const { return game_->player.playerKnown ? ReadField(players_->Records(),static_cast<std::size_t>(game_->player.battleOwner)*180+0x9c):0U; } //현재known/owner를각원문호출위치에서읽는다.
// DWORD SID를 선택된 실제 Pool 범위에서 검사한 뒤 WORD 기반 호스트 SID로 변환한다.
o::Sid SnapshotCall::Sid(std::uint32_t value) const { if (value>=storage_->pool.Capacity() || value>65535U) throw std::out_of_range("Native Fort저장의DWORD SID물리범위부족");return o::Sid{static_cast<std::uint16_t>(value)}; } //원문DWORD값을검사전에WORD로자르지않는다.
//현재count와필드를이읽기위치에서조회하며선행목록snapshot은만들지않는다.
bool SnapshotCall::IslandExists(std::int32_t island) const { const auto records=std::span<const std::uint8_t>(storage_->islands.records);return island>=0 && island<std::bit_cast<std::int32_t>(ReadField(records,0)) && ReadField(records,static_cast<std::size_t>(island)*32+8)!=0; }
//typed finishState의캐시대신원문46f200이읽는현재editor/missionPost를직접사용한다.
bool SnapshotCall::RegionAllowed(std::int32_t island,std::uint32_t owner,std::int32_t region) const {
    if (!IslandExists(island)) return false;const auto records=std::span<const std::uint8_t>(storage_->islands.records);const auto at=static_cast<std::size_t>(island)*32;
    if (!game_->world.editor && !game_->world.missionPost) { if (SignedByte(records[at+0x11])!=std::bit_cast<std::int32_t>(owner)) return false; }else if (!ReadField(records,at+0xc)) return false;
    return SignedByte(records[at+0x10])==region;
}
//실제표면WORD와현재005411dc를읽으며그밖의공간/World모듈을요구하지않는다.
std::uint32_t SnapshotCall::HasIsland(std::uint32_t x,std::uint32_t y) const {
    const auto cx=o::FortSaveCoordinate(std::bit_cast<float>(x)),cy=o::FortSaveCoordinate(std::bit_cast<float>(y));if (cx<0 || cy<0 || cx>=256 || cy>=256) return 0;
    const auto sid=storage_->hash.Cell(0,cx,cy);return sid && storage_->pool.Slot(o::Sid{sid})[10]==game_->GameAux()->TerrainType();
}
// 현재 Player+68에 연결된 실제 장기 Deck 블록을 조회하거나 원문 getter의 최초 할당만 수행한다.
std::uint32_t SnapshotCall::GetDeck(std::uint32_t owner) { return players_->ProductionDeckMemory()->Get(players_->Records(),owner,[](auto bytes) { CurrentAllocationBytes(bytes); }); } //같은현재Player+68과기존할당/다섯DWORD초기화만사용한다.
// 같은 장기 Deck 토큰의 실제352BYTE를 빌린다. 임시 Deck 소유자나 반납 동작을 만들지 않는다.
std::span<std::uint8_t> SnapshotCall::Deck(std::uint32_t token) const { return players_->ProductionDeckMemory()->Bytes(token); } //반납/소멸효과가있는임시RawProductionDeck을만들지않는다.
// 선택된 같은 Deck+14c의 signed 개수를 현재 시점에 읽는다. 음수와 손상 개수를 미리0으로 바꾸지 않는다.
std::int32_t SnapshotCall::DeckCount(std::uint32_t token) const { return std::bit_cast<std::int32_t>(ReadField(Deck(token),0x14c)); } //음수를빈signed순회로남기며현재개수를미리캐시하지않는다.
//44f290은진단뒤현재count로고정entry주소를계산하고44f160의고정인자를쓴뒤현재합계/count를갱신한다.
void SnapshotCall::AppendDeck(std::uint32_t token,std::uint32_t type) {
    for (std::int32_t index=0;index<DeckCount(token);++index) { if (ReadField(Deck(token),SelectedEntry(index),1)==type) Assert("entry[i].type != type","\\Ns\\O\\Deck.cpp",100); } //중복진단정상복귀뒤현재count를다시읽는다.
    if (DeckCount(token)>=66) Assert("totalEntries < MAX_ENTRIES","\\Ns\\O\\Deck.cpp",0x66);
    const auto entry=SelectedEntry(DeckCount(token));WriteField(Deck(token),entry+2,1,1);WriteField(Deck(token),entry,type,1);WriteField(Deck(token),entry+1,1,1);WriteField(Deck(token),entry+3,255,1);WriteField(Deck(token),entry+4,0,1);
    if ((type&255U)!=type) Assert("type==_type","\\Ns\\O\\Deck.cpp",0x2d);
    if (ReadField(Deck(token),entry+1,1)!=1) Assert("chance==_chance","\\Ns\\O\\Deck.cpp",0x2e);
    if (SignedByte(static_cast<std::uint8_t>(ReadField(Deck(token),entry+2,1)))!=1) Assert("power==_power","\\Ns\\O\\Deck.cpp",0x2f);
    if (ReadField(Deck(token),entry+3,1)!=255) Assert("numRemaining==_numRemaining","\\Ns\\O\\Deck.cpp",0x30);
    WriteField(Deck(token),0x150,ReadField(Deck(token),0x150)+1U);WriteField(Deck(token),0x14c,ReadField(Deck(token),0x14c)+1U);
}
//고정type를첫일치에적용하고기존power의signed부호와BYTE증가만수행한다.
void SnapshotCall::ActivateDeck(std::uint32_t token,std::uint32_t type) {
    for (std::int32_t index=0;index<DeckCount(token);++index) { //원문현재signed count를조건마다조회한다.
        const auto at=SelectedEntry(index);if (ReadField(Deck(token),at,1)!=type) continue;
        if (SignedByte(static_cast<std::uint8_t>(ReadField(Deck(token),at+2,1)))<0) WriteField(Deck(token),at+2,0,1);WriteField(Deck(token),at+2,ReadField(Deck(token),at+2,1)+1U,1);return;
    }
    AppendDeck(token,type);
}
//원문허용표조회도진단뒤현재범위/현재값을다시확인하며mode0은결과로Deck추가를막지않는다.
bool SnapshotCall::Allowed(std::int32_t type) const {
    if (type<1 || type>=static_cast<std::int32_t>(storage_->types.size())) Assert("typeNum > 0 && typeNum < numTypes","\\Ns\\O\\Totalmade.cpp",0x1d);
    return type>0 && type<static_cast<std::int32_t>(storage_->types.size()) && restrictions_[static_cast<std::size_t>(type)]==0;
}
// 00475530→004752b0(mode0)은 제품 표시나 내용물 생성을 하지 않고 같은 Deck만 활성화한다.
void SnapshotCall::FactoryProducts(o::Sid factory,std::uint32_t deck) {
    const auto raw=storage_->pool.Slot(factory);const auto type=raw[10];
    if (!(storage_->types.at(type).flags1&0x04000000U)) return; //00475530의 실제 선행부류 관문 뒤에만 Factory+b4를 선택한다.
    const auto table=ReadField(raw,0);
    if (table!=0x509148U && table!=0x509210U) throw std::logic_error("Native Fort 생산의 현재 Factory 가상+b4 미연결");
    auto remaining=static_cast<std::int32_t>(raw[11]>>5)+2; // 실제 두 Factory의00453f60 반환이며 원문의 최종 미사용 반환도 계산한다.
    std::array<std::uint32_t,255> seen{};NativeContentCursor cursor(storage_,game_,factory.value);
    for (;cursor.Current();cursor.Next()) { // 후보의 현재 flags/kind를 읽고 반환 전에 저장했던 next로 진행한다.
        const auto item=storage_->pool.Slot(Sid(cursor.Current()));if (!(item[20]&0x10U)) continue;
        const auto kind=ReadField(item,18,2);if (kind>=storage_->types.size()) throw std::out_of_range("Native Fort 제품의 선택 TYPE 물리 범위");
        static_cast<void>(storage_->types[kind]);static_cast<void>(Allowed(std::bit_cast<std::int32_t>(kind))); // mode0도 원문의 타입/허용 조회는 수행한다.
        if (deck) ActivateDeck(deck,kind); // 금지표의 결과는 mode0 Deck 활성화를 거르지 않는다.
        if (kind>=seen.size()) throw std::out_of_range("Native Fort 제품의 원문255DWORD 중복표 물리 범위");
        if (!seen[kind]) { remaining=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(remaining)-1U);seen[kind]=1; }
    }
    static_cast<void>(remaining); // 호출자가 쓰지 않는 mode0 반환값 때문에 새 게임 효과를 추가하지 않는다.
}
// 0044f7c0의 현재 개수·목록·진단 뒤 재조회·압축 순서를 같은 실제 Player Deck에 적용한다.
void SnapshotCall::RefreshDeck(std::uint32_t owner) {
    const auto token=GetDeck(owner);
    if (!storage_->ownerMode.fort && !storage_->ownerMode.battle) Assert("inFortMode || inBattleMode","\\Ns\\O\\Deck.cpp",0x14b);
    if (!storage_->links.mission && game_->FullDeck()) return;
    for (std::int32_t index=0;index<DeckCount(token);++index) WriteField(Deck(token),SelectedEntry(index,2),255,1); // 기존 항목의 signed power만 -1로 만든다.
    WriteField(Deck(token),0x150,0);
    if (client_.Configuration().GetInt("getTeleportTechFree")) ActivateDeck(token,game_->GameAux()->FreeTechnologyType());
    auto& factories=storage_->postPopState.Factories();
    for (std::int32_t index=0;index<std::bit_cast<std::int32_t>(factories.Count());++index) { // 원문 전역 count를 반복 조건과 modulo 선택 위치에서 다시 읽는다.
        const auto count=std::bit_cast<std::int32_t>(factories.Count());std::uint32_t number{};
        if (count) {
            if (count==-1 && index==std::numeric_limits<std::int32_t>::min()) throw std::out_of_range("Native Fort Factory 원문 idiv 물리 범위");
            const auto selected=index%count;const auto physical=factories.Storage();
            if (selected<0 || static_cast<std::size_t>(selected)>=physical.size()) throw std::out_of_range("Native Fort Factory 현재 목록의 물리 범위");number=physical[static_cast<std::size_t>(selected)];
        }
        const auto raw=storage_->pool.Slot(Sid(number));if (raw[34]!=owner) continue;
        const auto type=raw[10];if (type>=storage_->types.size()) throw std::out_of_range("Native Fort Factory 현재 TYPE 물리 범위");
        if (!(storage_->types[type].flags2&0x200U)) FactoryProducts(Sid(number),token);
        else if (Allowed(std::bit_cast<std::int32_t>(game_->GameAux()->TempleProductType()))) ActivateDeck(token,game_->GameAux()->TempleProductType());
    }
    std::int32_t removed{};
    for (std::int32_t index=0;index<DeckCount(token);++index) { // 0044f430은 먼저 같은5BYTE를 목적지에 복사한 뒤 원본 power를 판정한다.
        const auto source=SelectedEntry(index),destination=SelectedEntry(index-removed);const auto bytes=Deck(token);
        if (source>bytes.size() || bytes.size()-source<5 || destination>bytes.size() || bytes.size()-destination<5) throw std::out_of_range("Native Fort Deck 압축의 항목 물리 범위");
        const std::array<std::uint8_t,5> saved{bytes[source],bytes[source+1],bytes[source+2],bytes[source+3],bytes[source+4]};
        std::copy(saved.begin(),saved.end(),bytes.begin()+destination);
        if (SignedByte(bytes[source+2])<0) ++removed;
    }
    WriteField(Deck(token),0x14c,ReadField(Deck(token),0x14c)-static_cast<std::uint32_t>(removed));
    std::uint32_t total{};const auto count=DeckCount(token);
    for (std::int32_t index=0;index<count;++index) { // 원문 최종 합산은 시작 당시 양수 count를 고정하고 각 현재 BYTE를 읽는다.
        const auto bytes=Deck(token);const auto at=SelectedEntry(index);if (at>bytes.size() || bytes.size()-at<3) throw std::out_of_range("Native Fort Deck 가중합의 항목 물리 범위");
        if (SignedByte(bytes[at+2])>0) total+=bytes[at+1];
    }
    WriteField(Deck(token),0x150,total);
}
// TypeNames와 Deck은 서로 다른 원문 부모가 자신의 진단과 저장 순서를 소유한다.
void SnapshotCall::Basics(std::uint32_t slot,bool names) {
    o::FortSnapshotBasicsHooks hooks;hooks.manager=[this](auto selected) { return Manager(selected); };
    hooks.assertion=[this](auto reason) {
        switch (reason) {
        case o::FortSnapshotAssertion::MissingNamesTemplate:Assert("tm","\\Ns\\O\\Template.cpp",0x24a);return;
        case o::FortSnapshotAssertion::NamesNotMine:Assert("isMine()","\\Ns\\O\\Template.cpp",0x24b);return;
        case o::FortSnapshotAssertion::MissingDeckTemplate:Assert("tm","\\Ns\\O\\Template.cpp",0x4c4);return;
        case o::FortSnapshotAssertion::WriteCapacity:Assert("pos+length <= alloc","c:\\ns\\o\\memstream.h",0x24);return;
        }
        throw std::logic_error("Native Fort TypeNames/Deck의 미등록 진단");
    };
    hooks.create=[this,slot](auto manager,auto name) { return Create(slot,manager,name); };hooks.write=[this](auto token,auto bytes) { Write(token,bytes); };
    hooks.access=[this](auto token,const auto& operation) { Access(token,operation); };hooks.state=[this](auto selected)->std::span<const std::uint8_t> { return state_->Get(std::bit_cast<std::uint32_t>(selected)); };
    hooks.typeName=[this](auto type)->std::string_view { if (type<0 || static_cast<std::size_t>(type)>=storage_->types.size()) throw std::out_of_range("Native Fort TypeNames의 현재 TYPE 물리 범위");return storage_->types[static_cast<std::size_t>(type)].name; };
    hooks.refreshDeck=[this](auto owner) { RefreshDeck(owner); };hooks.getDeck=[this](auto owner) { return GetDeck(owner); };hooks.deck=[this](auto token)->std::span<const std::uint8_t> { return Deck(token); };
    const auto count=static_cast<std::int32_t>(storage_->types.size());
    if (names) o::SnapshotFortTypeNames(static_cast<std::int32_t>(slot),game_->player.templateOwner,count,hooks);else o::SnapshotFortDeck(static_cast<std::int32_t>(slot),hooks);
}
// 현재 Technology/객체 내용물의 실제 독립 커서와 원본 가상60을 기존 직렬화 부모에 빌린다.
void SnapshotCall::Contents(std::uint32_t parent,std::uint32_t stream) {
    o::FortContentSaveHooks hooks;
    hooks.begin=[this](auto selected) { const auto cursor=std::make_shared<NativeContentCursor>(storage_,game_,selected);return o::FortContentSaveCursor{[cursor] { return cursor->Current(); },[cursor] { cursor->Next(); }}; };
    hooks.poolLimit=[this] { return storage_->pool.Capacity(); };hooks.robust=[this](auto selected) { return virtual_.ContentRobust(Sid(selected))!=0; };
    hooks.form=[this](auto selected)->std::span<const std::uint8_t> { return storage_->pool.Slot(Sid(selected)); };
    hooks.type=[this](auto kind) { if (kind>=storage_->types.size()) throw std::out_of_range("Native Fort 내용물의 현재 TYPE 물리 범위");const auto& type=storage_->types[kind];return o::FortContentSaveType{type.flags1,type.contentListFlags,type.containerListFlags}; };
    hooks.parentType=[this](auto selected) { const auto kind=storage_->pool.Slot(Sid(selected))[10];if (kind>=storage_->types.size()) throw std::out_of_range("Native Fort 내용물 부모의 현재 TYPE 물리 범위");const auto& type=storage_->types[kind];return o::FortContentSaveType{type.flags1,type.contentListFlags,type.containerListFlags}; };
    hooks.assertion=[this](auto reason) {
        if (reason==o::FortContentSaveAssertion::ListFlags) Assert("(f.s().getListFlags() & lf) == lf","\\Ns\\O\\Template.cpp",0x116);
        else Assert("pos+length <= alloc","c:\\ns\\o\\memstream.h",0x24);
    };
    hooks.access=[this](auto token,const auto& operation) { Access(token,operation); };o::SaveFortContents(parent,stream,hooks);
}
// 같은 현재 Pool/TYPE/장기 저장 전역만 공급하고 객체 필터/필드 쓰기는 원문 부모가 수행한다.
o::FortObjectSaveHooks SnapshotCall::ObjectHooks() {
    o::FortObjectSaveHooks hooks;
    hooks.global=[this](auto key)->std::uint32_t {
        switch (key) {
        case o::FortObjectSaveGlobal::Battle:return storage_->ownerMode.battle;
        case o::FortObjectSaveGlobal::Mission:return storage_->links.mission;
        case o::FortObjectSaveGlobal::Editor:return game_->world.editor;
        case o::FortObjectSaveGlobal::Scope:return serialization_->scope;
        case o::FortObjectSaveGlobal::Version:return serialization_->version;
        case o::FortObjectSaveGlobal::LocalOwner:return game_->player.battleOwner;
        case o::FortObjectSaveGlobal::BridgeType:return storage_->links.bridgeType;
        case o::FortObjectSaveGlobal::LocalOnlyType:return game_->GameAux()->DaisType();
        case o::FortObjectSaveGlobal::BattleMask:return serialization_->battleSavedMask;
        case o::FortObjectSaveGlobal::OwnerMask:return serialization_->ownerSavedMask;
        }
        throw std::logic_error("Native Fort 객체 저장의 미등록 전역");
    };
    hooks.form=[this](auto selected)->std::span<const std::uint8_t> { return storage_->pool.Slot(Sid(selected)); };
    hooks.flags1=[this](auto selected) { const auto type=storage_->pool.Slot(Sid(selected))[10];return storage_->types.at(type).flags1; };
    hooks.flags2=[this](auto selected) { const auto type=storage_->pool.Slot(Sid(selected))[10];return storage_->types.at(type).flags2; };
    hooks.robust=[this](auto selected) { return virtual_.ObjectRobust(Sid(selected)); };hooks.hasLevel=[this](auto selected) { return virtual_.HasLevel(Sid(selected)); };hooks.level=[this](auto selected) { return virtual_.Level(Sid(selected)); };
    hooks.island=[this](auto x,auto y) { return HasIsland(x,y); };hooks.write=[this](auto token,auto bytes) { Write(token,bytes); };hooks.contents=[this](auto selected,auto token) { Contents(selected,token); };
    hooks.assertion=[this](auto reason) {
        if (reason==o::FortObjectSaveAssertion::XOffset) Assert("cx>=0 && cx<16","\\Ns\\O\\Template.cpp",0x29b);
        else if (reason==o::FortObjectSaveAssertion::YOffset) Assert("cy>=0 && cy<16","\\Ns\\O\\Template.cpp",0x29c);
        else Assert("playerId>=INVALID_PLAYER_ID && playerId < MAX_PLAYERS","\\Ns\\O\\Template.cpp",0x2c3);
    };return hooks;
}
// 각 청크마다 현재 Hash를 읽는 독립 Finder를 만들고 기존 단일 객체 writer를 재사용한다.
void SnapshotCall::Chunk(std::uint32_t stream,std::int32_t x,std::int32_t y) {
    struct Cursor { // 청크 한 번의 Finder와 현재 SID만 보관하며 후보 목록을 미리 복제하지 않는다.
        o::RawSquidFinder finder;o::Sid current; // 같은 실제 Pool/Hash/TYPE를 읽는 한정 커서다.
        Cursor(RawSurfaceStorage& storage,const o::FortChunkSaveArea& area):finder(storage.pool,storage.hash,storage.types) { //0041def0의 lower 절삭/upper bias 절삭을 같은 기존 함수로 실행한다.
            const auto lower=RawCursorPosition::TruncatePoint({area.left,area.top}),upper=RawCarrierDropPoint::Snap({area.right,area.bottom});
            current=finder.Begin({o::FortSaveCoordinate(lower[0]),o::FortSaveCoordinate(lower[1]),o::FortSaveCoordinate(upper[0]),o::FortSaveCoordinate(upper[1])},0);
        }
    };
    const auto objects=ObjectHooks();o::FortChunkSaveHooks hooks;
    hooks.begin=[this](const auto& area) { const auto cursor=std::make_shared<Cursor>(*storage_,area);return o::FortChunkSaveCursor{[cursor] { return cursor->current.value; },[cursor] { cursor->current=cursor->finder.Next(); }}; };
    hooks.poolLimit=[this] { return storage_->pool.Capacity(); };hooks.object=[&objects](auto token,auto selected,auto ox,auto oy) { return o::SaveFortObject(token,selected,ox,oy,objects); };hooks.access=[this](auto token,const auto& operation) { Access(token,operation); };
    o::SaveFortChunk(stream,x,y,hooks);
}
// 청크 selector의 두 원문 진단은 현재 지도에 접근하기 직전에 실행한다.
o::ChunkEntry& SnapshotCall::SelectChunk(std::int32_t x,std::int32_t y) {
    if (!storage_->chunks.Mode()) Assert("isValid()","\\Ns\\O\\Chunkmap.cpp",0xb7);
    if (x<0 || y<0 || x>=storage_->chunks.XLength() || y>=storage_->chunks.YLength()) Assert("cc.x>=0 && cc.y>=0 && cc.x<xLen && cc.y<yLen","\\Ns\\O\\Chunkmap.cpp",0xb8);
    return storage_->chunks.At(x,y); // 진단 복귀 뒤 현재 크기를 사용하는 기존 selector의 clamp/물리 정책을 유지한다.
}
// 최초(0,0) 조회는 높이0에서도 먼저 하며 각 selector 복귀 뒤 현재 높이를 다시 읽는다.
std::array<std::uint8_t,6> SnapshotCall::IslandRecord(std::int32_t island) {
    std::int32_t x{},y{};auto* entry=&SelectChunk(0,0);auto height=storage_->chunks.YLength();
    if (height>0) {
        do { // 행 우선 pointer 진행과 현재 폭/높이의 원문 읽기 순서를 유지한다.
            if (SignedByte(entry->island)==island) return entry->record;
            x=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(x)+1U);
            if (storage_->chunks.XLength()<=x) { x=0;y=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(y)+1U); }
            entry=y<height ? &SelectChunk(x,y):nullptr;height=storage_->chunks.YLength();
            if (y<height && !entry) throw std::logic_error("Native Fort record의 NULL selector 뒤 현재 지도 확장"); // 원문 NULL 역참조를 별도 정상 셀로 바꾸지 않는다.
        } while (y<height);
    }
    return {}; // 같은 원문의 첫일치 부재6BYTE0 반환이다.
}
// 기존 Terr/Temp/Chaff 섹션과 현재 지도/영역 판정·scope를 원문 전체 Objects 부모에 결합한다.
void SnapshotCall::Objects(std::uint32_t slot) {
    o::FortObjectsSaveHooks hooks;hooks.manager=[this](auto selected) { return Manager(selected); };
    hooks.assertion=[this](auto reason) {
        if (reason==o::FortObjectsSaveAssertion::MissingTemplate) Assert("tm","\\Ns\\O\\Template.cpp",0x3bd);
        else if (reason==o::FortObjectsSaveAssertion::NotMine) Assert("isMine()","\\Ns\\O\\Template.cpp",0x3be);
        else Assert("pos+length <= alloc","c:\\ns\\o\\memstream.h",0x24);
    };
    hooks.find=[this,slot](auto manager,auto name) { CheckManager(slot,manager);return Remember(templates_->FindSection(slot,name)); };
    hooks.create=[this,slot](auto manager,auto name) { return Create(slot,manager,name); };
    hooks.select=[this,slot](auto manager,auto name,auto writable) { CheckManager(slot,manager);return Remember(templates_->SelectSection(slot,name,writable)); };
    hooks.rename=[this,slot](auto manager,auto destination,auto source) { CheckManager(slot,manager);static_cast<void>(templates_->RenameSection(slot,destination,source)); };hooks.access=[this](auto token,const auto& operation) { Access(token,operation); };
    o::FortRegionSaveHooks regions;regions.chunksX=[this] { return storage_->chunks.XLength(); };regions.chunksY=[this] { return storage_->chunks.YLength(); };
    regions.chunkRegion=[this](auto x,auto y) { return SelectChunk(x,y).island; };regions.allowed=[this](auto island,auto owner,auto region) { return RegionAllowed(island,owner,region) ? 1U:0U; };regions.chunk=[this](auto stream,auto x,auto y) { Chunk(stream,x,y); };
    hooks.territory=[this,&regions](auto stream,auto owner,auto region) { o::SaveFortTerritoryChunks(stream,owner,region,serialization_->scope,regions); };
    hooks.chaff=[this,&regions](auto stream) { o::SaveFortChaffChunks(stream,serialization_->scope,regions); };
    o::SaveFortObjects(static_cast<std::int32_t>(slot),{game_->player.templateOwner,serialization_->version,[this] { return storage_->ownerMode.fort ? 1U:0U; }},hooks);
}
// 004bd660은 기존 Territory를 먼저 읽고 그 뒤 pos/end 진단을 수행한다.
void SnapshotCall::ReadTerritory(std::uint32_t slot,std::span<std::uint8_t,120> output) {
    if (!Manager(static_cast<std::int32_t>(slot))) Assert("tm","\\Ns\\O\\Template.cpp",0x22e);
    auto& stream=templates_->SelectTerritory(slot);const auto position=stream.position;
    if (position>stream.bytes.size() || stream.bytes.size()-position<output.size()) throw std::out_of_range("Native Fort Territory 읽기의 현재120BYTE 물리 범위");
    std::copy_n(stream.bytes.begin()+position,output.size(),output.begin());stream.position=position+120U;
    if (std::bit_cast<std::int32_t>(stream.end)<std::bit_cast<std::int32_t>(stream.position)) Assert("pos<=end","c:\\ns\\o\\memstream.h",0x2e);
}
// 기존 기록·현재 섬/지도·빈 seed·현재 stream 재조회 순서를 기존 부모에 맡긴다.
void SnapshotCall::Territory(std::uint32_t slot) {
    o::FortTerritorySaveHooks hooks;hooks.load=[this](auto selected,auto output) { ReadTerritory(std::bit_cast<std::uint32_t>(selected),output); };hooks.special=[this] { return game_->world.editor; };
    hooks.island=[this](auto owner,auto region) { return o::FindFortTerritoryIsland(storage_->islands.records,storage_->ownerMode.fort,owner,region); };hooks.anyIsland=[this](auto region) { return o::FindAnyFortTerritoryIsland(storage_->islands.records,region); };
    hooks.noRegion=[] { return 127; }; // 보호 PE0050c038의 읽기 전용 DWORD7f이며 InvalidTerritory와 구별한다.
    hooks.record=[this](auto island) { return IslandRecord(island); };
    hooks.seed=[this](auto record,auto index) { o::SeedFortTerritoryRecord(record,index,[this](auto limit) { return client_.RandomGlobals()->Game().Next(limit); }); };
    hooks.manager=[this](auto selected) { return Manager(selected); };hooks.create=[this,slot](auto manager) { static_cast<void>(Create(slot,manager,"Territory")); };
    hooks.current=[this,slot](auto manager) { CheckManager(slot,manager);return Remember(templates_->CurrentWriteStream(slot)); };hooks.access=[this](auto token,const auto& operation) { Access(token,operation); };
    hooks.assertion=[this](auto reason) {
        if (reason==o::FortTerritorySaveAssertion::MissingManager) Assert("tm","\\Ns\\O\\Template.cpp",0x209);else Assert("pos+length <= alloc","c:\\ns\\o\\memstream.h",0x24);
    };o::SnapshotFortTerritory(static_cast<std::int32_t>(slot),hooks);
}
// 현재 CoreData와 기술 목록·자기 다이스의 실제 count를 기존0044fc00 부모로 갱신한다.
void SnapshotCall::RefreshCore() {
    constexpr std::uint32_t kPoolAddress=0x12000000U; // 같은 실제 Pool의50BYTE 상대 주소 식별값이며 별도 메모리/가짜 객체가 아니다.
    o::PlayerCoreRefreshHooks hooks;
    hooks.core=[this] { const std::uint32_t metamap=storage_->ownerMode.challenge ? 1U:0U;return client_.CoreDataGlobals()->Read(game_->player,metamap,{[this](auto line) { Assert("_myCoreData == &lastKnownCoreData","\\Ns\\O\\Player.cpp",line); }}); };
    hooks.technologyParent=[this] { return CurrentTechnology(); };
    hooks.begin=[this](auto parent) { const auto cursor=std::make_shared<NativeContentCursor>(storage_,game_,parent);return o::PlayerCoreRefreshCursor{[cursor] { return cursor->Current(); },[cursor] { cursor->Next(); }}; };
    hooks.pool=[this] { return o::PlayerCoreRefreshPool{storage_->pool.Bytes(),kPoolAddress,storage_->pool.Capacity()}; };hooks.technologyBit=[this](auto type) { return storage_->types.at(type).techBitIndex; };
    hooks.playerType=[this] { return game_->GameAux()->DaisType(); };hooks.owner=[this] { return game_->player.battleOwner; };
    hooks.level=[this](o::PlayerCoreRefreshForm form) { const auto bytes=storage_->pool.Bytes();if (form.address<kPoolAddress || (form.address-kPoolAddress)%50U) throw std::out_of_range("Native Fort Core 갱신의 실제 객체 주소 범위");
        const auto number=(form.address-kPoolAddress)/50U;if (number>=storage_->pool.Capacity() || form.bytes.size()!=50 || form.bytes.data()!=bytes.data()+number*50U) throw std::out_of_range("Native Fort Core 갱신의 실제 객체 정체성");return std::bit_cast<std::int32_t>(virtual_.Level(Sid(number))); };
    hooks.assertion=[this](auto line) { Assert("techBitIndex >= 0 && techBitIndex < 32","\\Ns\\O\\Ch.cpp",line); };o::RefreshPlayerCore(hooks);
}
// Battle의 CoreData 저장도 원문 post/editor 자연 반환 뒤에만 같은 Core/RNG/seed/SP writer를 선택한다.
void SnapshotCall::CoreData(std::uint32_t slot) {
    o::FortCoreWriteGlobals globals;globals.metamap=[this] { return storage_->ownerMode.challenge ? 1U:0U; };globals.missionPost=[this] { return game_->world.missionPost; };globals.editor=[this] { return game_->world.editor; };
    globals.players=[this] { return client_.CoreDataGlobals()->Players(); };globals.playerOwner=[this] { return game_->player.battleOwner; };globals.seed=[this]()->std::uint32_t& { return client_.RandomGlobals()->FortCoreSeed(); };
    o::FortCoreWriteHooks hooks;hooks.manager=[this](auto selected) { return Manager(selected); };
    hooks.assertion=[this](auto reason) {
        if (reason==o::FortCoreWriteAssertion::MissingTemplate) Assert("tm","\\Ns\\O\\Template.cpp",0x45f);
        else if (reason==o::FortCoreWriteAssertion::InMetamap) Assert("!inMetamapMode","\\Ns\\O\\Template.cpp",0x460);
        else Assert("getPlayerId() == myPlayerId()","\\Ns\\O\\Template.cpp",0x467);
    };
    hooks.create=[this,slot](auto manager,auto name) { return Create(slot,manager,name); };hooks.refresh=[this] { RefreshCore(); };hooks.write=[this](auto stream,auto bytes) { Write(stream,bytes); };
    hooks.prepareRandom=[this] { const auto random=client_.RandomGlobals();o::RawGameRandomSeed seed(random->Game(),random->CrtState(),random->GameMode(),ClientUnixSeconds);static_cast<void>(seed.Reseed()); }; // 같은 실제 owner를 빌리는 임시 순수 실행기이며 새 난수 상태는 없다.
    hooks.nextRandom=[this](auto limit) { return client_.RandomGlobals()->Game().Next(limit); };
    hooks.sp=[this](auto stream,float value) { Access(stream,[this,value](auto fields,const auto& memory) { o::FortSpWriteHooks sp;sp.nextRandom=[this](auto limit) { return client_.RandomGlobals()->Game().Next(limit); };sp.stream=memory;o::WriteFortSp(fields,value,sp); }); };
    o::WriteFortCoreData(static_cast<std::int32_t>(slot),globals,hooks);
}
// Notify가 실제 저장을 선택한 뒤에만 현재 이미지·권한·숨김표·사용자 작업 파일을 연결한다.
void SnapshotCall::NotifySave(std::uint32_t slot) {
    o::FortImageHooks image;image.typeCount=[this] { return static_cast<std::uint32_t>(storage_->types.size()); };
    image.assertion=[this](auto reason) {
        if (reason==o::FortImageAssertion::TypeCount) Assert("numTypes < 256","\\Ns\\O\\Template.cpp",0xda);
        else if (reason==o::FortImageAssertion::SectionCapacity) Assert("destStream.pos + s->end < destStream.alloc","\\Ns\\O\\Datamanager.cpp",0xbe);
        else Assert("pos+length <= alloc","c:\\ns\\o\\memstream.h",0x24);
    };
    templates_->SaveFile(slot,{client_.ReadOnlyForts(),game_->player.battleOwner,std::span<const std::uint8_t>(options_).subspan(0x3e,9)},std::move(image),files_.Writer());
}
// 두 원문 전체 부모는 같은 child를 다른 순서로 부르므로 기존 순수 Fort/Battle 부모를 각각 선택한다.
void SnapshotCall::Run(std::uint32_t slot,bool battle) {
    o::FortSnapshotParentGlobals globals;globals.templateOwner=[this] { return game_->player.templateOwner; };globals.fort=[this] { return storage_->ownerMode.fort ? 1U:0U; };globals.battle=[this] { return storage_->ownerMode.battle ? 1U:0U; };
    o::FortSnapshotParentHooks hooks;hooks.manager=[this](auto selected) { return Manager(selected); };
    hooks.assertion=[this](auto reason) {
        const auto line=static_cast<std::uint32_t>(reason);std::string_view expression="tm"; // manager0 진단은 원문의 같은 문자열 tm을 쓴다.
        switch (reason) {
        case o::FortSnapshotParentAssertion::NotFortMode:expression="inFortMode";break;
        case o::FortSnapshotParentAssertion::NotBattleMode:expression="inBattleMode";break;
        case o::FortSnapshotParentAssertion::FortNotMine:case o::FortSnapshotParentAssertion::BattleNotMine:expression="isMine()";break;
        default:break;
        }
        Assert(expression,"\\Ns\\O\\Template.cpp",line);
    };
    hooks.create=[this,slot](auto manager,auto name) { return Create(slot,manager,name); };hooks.typeNames=[this](auto selected) { Basics(std::bit_cast<std::uint32_t>(selected),true); };hooks.technologyParent=[this] { return CurrentTechnology(); };hooks.technology=[this](auto parent,auto stream) { Contents(parent,stream); };
    hooks.objects=[this](auto selected) { Objects(std::bit_cast<std::uint32_t>(selected)); };hooks.territory=[this](auto selected) { Territory(std::bit_cast<std::uint32_t>(selected)); };hooks.deck=[this](auto selected) { Basics(std::bit_cast<std::uint32_t>(selected),false); };
    hooks.money=[this](auto stream) { o::SaveFortMoney(stream,client_.RandomGlobals()->Game(),[this](auto token,auto bytes) { Write(token,bytes); }); };
    hooks.mission=[this](auto stream) { auto mission=mission_;mission.write=[this](auto token,auto bytes) { Write(token,bytes); };mission.assertion=[this](auto) { Assert("s","\\Ns\\O\\Template.cpp",0x9b); };o::SaveFortMission(stream,mission); };
    hooks.badges=[this](auto selected) {
        const auto slot=std::bit_cast<std::uint32_t>(selected);o::FortBadgesSaveHooks badges;badges.manager=[this](auto current) { return Manager(current); };badges.create=[this,slot](auto manager,auto name) { return Create(slot,manager,name); };badges.access=[this](auto token,const auto& operation) { Access(token,operation); };
        badges.count=[this](auto type) { return o::ReadFortBadgeCount(client_.FortBadges()->Records(),type); };badges.assertion=[this](auto reason) { if (reason==o::FortBadgesSaveAssertion::MissingTemplate) Assert("tm","\\Ns\\O\\Template.cpp",0x412);else Assert("pos+length <= alloc","c:\\ns\\o\\memstream.h",0x24); };
        o::SaveFortBadges(selected,badges);
    };
    hooks.coreData=[this](auto selected) { CoreData(std::bit_cast<std::uint32_t>(selected)); };hooks.notify=[this](auto selected) { state_->Changed(std::bit_cast<std::uint32_t>(selected)); };
    if (battle) o::SnapshotBattleTemplate(std::bit_cast<std::int32_t>(slot),globals,hooks);else o::SnapshotFortTemplate(std::bit_cast<std::int32_t>(slot),globals,hooks);
}
} // 저장 호출의 한정 실행기·커서만 끝내며 실제 장기 자료는 반납하거나 초기화하지 않는다.

// 같은 실제 Client/Template 장치로 Fort 부모 한 번을 동기 실행한다.
void SnapshotNativeFort(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::span<std::uint8_t,87> options,std::span<std::uint32_t,256> restrictions,const o::FortMissionSaveHooks& mission,const EditorFiles& files,std::uint32_t slot) {
    SnapshotCall(client,templates,options,restrictions,mission,files).Run(slot,false);
}
// 같은 실제 Client/Template 장치로 Battle 부모 한 번을 동기 실행한다.
void SnapshotNativeBattle(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::span<std::uint8_t,87> options,std::span<std::uint32_t,256> restrictions,const o::FortMissionSaveHooks& mission,const EditorFiles& files,std::uint32_t slot) {
    SnapshotCall(client,templates,options,restrictions,mission,files).Run(slot,true);
}
// manager0 선행 관문 유무와 첫 Battle 반환 뒤의 owner 재조회를 원문 두 wrapper대로 구별한다.
void SnapshotNativeCurrent(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::span<std::uint8_t,87> options,std::span<std::uint32_t,256> restrictions,const o::FortMissionSaveHooks& mission,const EditorFiles& files,bool onlyIfPresent) {
    const auto game=client.GameplayGlobals();const auto storage=client.SurfaceStorage();
    if (onlyIfPresent) {
        auto owner=game->player.templateOwner;if (!templates) throw std::logic_error("Native 현재맵 저장의 실제 Template owner 미설치");
        if (owner>=9) throw std::out_of_range("Native 현재맵 저장의 현재 Template9 물리 범위");
        if (!ReadField(templates->Headers(),static_cast<std::size_t>(owner)*20)) return;
        if (storage->ownerMode.battle) { SnapshotNativeBattle(client,templates,options,restrictions,mission,files,owner);owner=game->player.templateOwner; }
        if (storage->ownerMode.fort) SnapshotNativeFort(client,templates,options,restrictions,mission,files,owner);
    } else {
        if (storage->ownerMode.battle) SnapshotNativeBattle(client,templates,options,restrictions,mission,files,game->player.templateOwner);
        if (storage->ownerMode.fort) SnapshotNativeFort(client,templates,options,restrictions,mission,files,game->player.templateOwner);
    }
}
// 이름·편집·State caller가 요구하는 원래 Notify만 수행하고 전체 저장을 앞당기지 않는다.
void NotifyNativeFortTemplate(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::span<std::uint8_t,87> options,std::span<std::uint32_t,256> restrictions,const o::FortMissionSaveHooks& mission,const EditorFiles& files,std::uint32_t slot) {
    SnapshotCall(client,templates,options,restrictions,mission,files).Notify(slot);
}
// 임시 view가 끝나도 반환 바이트는 caller가 보유한 같은 실제 Template 섹션에 남는다.
std::span<std::uint8_t> GetNativeFortTemplateState(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::uint32_t slot) {
    return MakeNativeTemplateState(client,templates)->Get(slot);
}
} // 실제 게임 모드/PlayerKnown/권한·World 수명은 caller가 계속 소유한다.
