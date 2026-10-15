// 원본 Early Missions3~6의 읽기 순서와 DWORD 감김·5초 double 예약을 보존한다.
#include "o/RawTutorialLater.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr std::uint32_t kEventMask=0x03ffffff; // 명령 종류 비교에서만 제거하는 원본 modifier 비트다.
constexpr double kSelectionDelay=5.0; // 실제 ROM005071e0의 double 선택 안내 지연이다.
// 원본 비정렬 DWORD 필드를 해당4바이트만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t result=0;
    // 낮은 BYTE부터 원본 little endian 비트를 결합한다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(bytes[offset+index])<<(8*index);
    return result;
}
}
// 처음 실행 전에 같은 원본 저장소와 실제 하위의 물리/수명 전제를 확인한다.
RawTutorialLater::RawTutorialLater(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> players,
    RawTotalMade& made,RawMissionFrameGate& gate,MissionStartState& mission,TutorialLaterState state,TutorialLaterHooks hooks)
    :pool_(pool),types_(types),players_(players),made_(made),gate_(gate),mission_(mission),state_(std::move(state)),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || players.size()!=9*180 || state_.battleOptions.size()<3 ||
        !state_.type || !hooks_.findPriest || !hooks_.graphFor || !hooks_.productionContains || !hooks_.indicateTechnology ||
        !hooks_.regionFor || !hooks_.nearby || !hooks_.priestCaptured || !hooks_.selected || !hooks_.gameSeconds ||
        !hooks_.resetSelection || !hooks_.clearSelection || !hooks_.tell || !hooks_.closeTransientGumps)
        throw std::invalid_argument("튜토리얼3~6의 현재 원본 자료/하위 경계 누락");
}
// SID0도 원본 물리 슬롯을 읽을 수 있으나 host 범위 밖 번호는 명시 거부한다.
std::span<const std::uint8_t> RawTutorialLater::Object(std::uint32_t sid) const {
    if (sid>=pool_.Capacity() || sid>0xffffU) throw std::out_of_range("튜토리얼3~6의 현재 사건SID 물리 범위");
    return pool_.Slot(Sid{static_cast<std::uint16_t>(sid)});
}
// 실제004ac200과 같은 현재 타입 BYTE 뒤 TYPE+ec를 읽는다.
std::uint32_t RawTutorialLater::Genus(std::uint32_t sid) const {
    const auto type=Object(sid)[10];if (type>=types_.size()) throw std::out_of_range("튜토리얼3~6 현재 부류TYPE 범위");return types_[type].flags2;
}
// 원본의 cmp동일/감긴sub결과부호를 그대로 쓰며 수학적 signed 대소비교로 바꾸지 않는다.
bool RawTutorialLater::GlobalNotAboveLocal(std::uint32_t typeGlobal) const {
    const auto total=made_.Global(std::bit_cast<std::int32_t>(state_.type(typeGlobal))),local=made_.Local(std::bit_cast<std::int32_t>(state_.type(typeGlobal)));
    return total==local || std::bit_cast<std::int32_t>(total-local)<0;
}
// Advance가 실패해도 두 후속 선택 효과는 원본대로 실행한다.
void RawTutorialLater::ClearSelection() const { hooks_.resetSelection();hooks_.clearSelection(); }
// 실제 창 생성 결과와 관계없이 원본 일회성 DWORD를1로 쓴다.
void RawTutorialLater::TellOnce(std::string_view name,std::uint32_t& seen) const {
    if (hooks_.tell(name)) { hooks_.closeTransientGumps();gate_.PauseForDialog(); }seen=1;
}
// 각 실제 판정기의0 반환과3DWORD 인자를 동일 입력 큐에서 유지한다.
std::uint32_t RawTutorialLater::Handle(std::uint32_t tutorial,std::span<const std::uint32_t> event) const {
    if (event.size()<3) throw std::invalid_argument("튜토리얼 사건DWORD3개 누락");
    switch (tutorial) { case 3:Three(event);break;case 4:Four(event);break;case 5:Five(event);break;case 6:Six(event);break;default:throw std::invalid_argument("튜토리얼3~6 밖 번호"); }return 0;
}
// Capture the Priest의 Graph 연결·배치 지역·내용물 사건은 실제 현재 객체에서 읽는다.
void RawTutorialLater::Three(std::span<const std::uint32_t> event) const {
    const auto command=event[0]&kEventMask; // switch 전에 한 번 읽는 실제 첫 입력DWORD다.
    switch (gate_.State().stage) {
    case 'A':state_.battleOptions[2]=2;if (command==state_.rangeCommand) static_cast<void>(gate_.Advance());break;
    case 'B': {
        const auto enemy=hooks_.findPriest(state_.localOwner+1U),own=hooks_.findPriest(state_.localOwner); // 원본은 적 조회 후 자기 조회를 수행한다.
        if (enemy && own) { const auto ownGraph=hooks_.graphFor(own),enemyGraph=hooks_.graphFor(enemy);if (enemyGraph!=ownGraph) return; }
        static_cast<void>(gate_.Advance());break;
    }
    case 'C':if (hooks_.productionContains(state_.type(0x541174))) static_cast<void>(gate_.Advance());break;
    case 'D':
        hooks_.indicateTechnology(state_.type(0x541184));
        if (hooks_.productionContains(state_.type(0x541188)) || hooks_.productionContains(state_.type(0x541184)) || hooks_.productionContains(state_.type(0x54118c))) static_cast<void>(gate_.Advance());break;
    case 'E':if (command==state_.placedCommand) {
        const auto raw=Object(event[1]);const auto region=hooks_.regionFor(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)));
        if (raw[10]!=state_.type(0x541174)) return;
        if (state_.localOwner>=9) throw std::out_of_range("튜토리얼3 현재 Player 지역 범위");
        if (std::bit_cast<std::uint32_t>(region)==Read(players_,state_.localOwner*180+0x78)) return;
        if (!hooks_.nearby(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),0x1000000)) static_cast<void>(gate_.Advance());
        }break;
    case 'F': { const auto enemy=hooks_.findPriest(state_.localOwner+1U);if (!enemy || hooks_.priestCaptured(enemy)) static_cast<void>(gate_.Advance());break; }
    case 'G':if (command==state_.containedCommand && (Genus(event[1])&0x200000) && !Object(event[1])[0x22]) static_cast<void>(gate_.Advance());break;
    default:break;
    }
}
// Tactical Combat의 허용표와 현재 전역/로컬 제작 비교는 같은 TotalMade를 사용한다.
void RawTutorialLater::Four(std::span<const std::uint32_t> event) const {
    const auto command=event[0]&kEventMask; // 타입 사건이 아닌 명령 비교에서만 modifier를 제거한다.
    switch (gate_.State().stage) {
    case 'A':made_.Allow(std::bit_cast<std::int32_t>(state_.type(0x5412ec)),0);state_.battleOptions[2]=2;
        if (hooks_.productionContains(state_.type(0x541174)) && hooks_.productionContains(state_.type(0x541184))) static_cast<void>(gate_.Advance());break;
    case 'B':if (GlobalNotAboveLocal(0x541258)) static_cast<void>(gate_.Advance());break;
    case 'C':hooks_.indicateTechnology(state_.type(0x541258));if (hooks_.productionContains(state_.type(0x541258))) static_cast<void>(gate_.Advance());break;
    case 'D':if (GlobalNotAboveLocal(0x541174)) static_cast<void>(gate_.Advance());break;
    case 'E':if (command==state_.containedCommand && (Genus(event[1])&0x200000) && !Object(event[1])[0x22] && Object(event[2])[10]==state_.type(0x54126c)) static_cast<void>(gate_.Advance());break;
    case 'F':made_.Allow(std::bit_cast<std::int32_t>(state_.type(0x5412ec)),1);if (std::bit_cast<std::int32_t>(made_.Local(std::bit_cast<std::int32_t>(state_.type(0x5412ec))))>0) static_cast<void>(gate_.Advance());break;
    case 'G':if (command==state_.captureCommand) static_cast<void>(gate_.Advance());break;
    case 'H':if (command==state_.ascendCommand) static_cast<void>(gate_.Advance());break;
    default:break;
    }
}
// Subtle Defense의 두 일회성 경고는 switch 전에 각각 현재 단계/생산을 다시 읽는다.
void RawTutorialLater::Five(std::span<const std::uint32_t> event) const {
    const auto command=event[0]&kEventMask;
    if (std::bit_cast<std::int32_t>(made_.Global(std::bit_cast<std::int32_t>(state_.type(0x541244))))<1 && !mission_.fieldC0) TellOnce("VortexDestroyed",mission_.fieldC0);
    if (gate_.State().stage>'B' && (!hooks_.productionContains(state_.type(0x541174)) || !hooks_.productionContains(state_.type(0x541258)) || !hooks_.productionContains(state_.type(0x541190)) || !hooks_.productionContains(state_.type(0x541184))) && !mission_.fieldC4) TellOnce("FactoryDestroyed",mission_.fieldC4);
    auto& gate=gate_.State(); // 하위 Tell/정지가 바꾼 현재 단계/타이머를 재사용한다.
    switch (gate.stage) {
    case 'A':state_.battleOptions[2]=2;if (command==state_.technologyCommand && event[1]==state_.type(0x541268) && event[2]==state_.type(0x541190)) static_cast<void>(gate_.Advance());break;
    case 'B':if (hooks_.productionContains(state_.type(0x541174)) && hooks_.productionContains(state_.type(0x541258)) && hooks_.productionContains(state_.type(0x541190)) && hooks_.productionContains(state_.type(0x541184))) static_cast<void>(gate_.Advance());break;
    case 'C':if (gate.timerSeconds==0.0) {
        std::uint32_t selectedType=0; // 실제 첫 선택이0이면 두 번째 선택/raw 슬롯 조회를 생략한다.
        if (hooks_.selected()) selectedType=Object(hooks_.selected())[10];
        if (selectedType==state_.type(0x541178) || (GlobalNotAboveLocal(0x541264) && GlobalNotAboveLocal(0x541178))) gate.timerSeconds=hooks_.gameSeconds()+kSelectionDelay;
        } else if (gate.timerSeconds<=hooks_.gameSeconds()) { static_cast<void>(gate_.Advance());ClearSelection(); }break;
    case 'D':if (GlobalNotAboveLocal(0x541264) && GlobalNotAboveLocal(0x541178)) static_cast<void>(gate_.Advance());break;
    case 'E':if (command==state_.containedCommand && (Genus(event[1])&0x100)) static_cast<void>(gate_.Advance());break;
    case 'F':if (command==state_.ascendCommand) static_cast<void>(gate_.Advance());break;
    default:break;
    }
}
// Raw Power의 현재 전역 제작수와 실제 기술 알림/생산/사건만으로 단계를 예약한다.
void RawTutorialLater::Six(std::span<const std::uint32_t> event) const {
    switch (gate_.State().stage) {
    case 'A':state_.battleOptions[2]=2;if (std::bit_cast<std::int32_t>(made_.Global(std::bit_cast<std::int32_t>(state_.type(0x54128c))))>0 && std::bit_cast<std::int32_t>(made_.Global(std::bit_cast<std::int32_t>(state_.type(0x541184))))>0) static_cast<void>(gate_.Advance());break;
    case 'B':if ((event[0]&kEventMask)==state_.rangeCommand) static_cast<void>(gate_.Advance());break;
    case 'C':hooks_.indicateTechnology(state_.type(0x541288));if (hooks_.productionContains(state_.type(0x541288))) static_cast<void>(gate_.Advance());break;
    case 'D':if (std::bit_cast<std::int32_t>(made_.Global(std::bit_cast<std::int32_t>(state_.type(0x541288))))>0) static_cast<void>(gate_.Advance());break;
    case 'E':if (!made_.Global(std::bit_cast<std::int32_t>(state_.type(0x541174))) && !made_.Global(std::bit_cast<std::int32_t>(state_.type(0x541178))) && !made_.Global(std::bit_cast<std::int32_t>(state_.type(0x541264)))) static_cast<void>(gate_.Advance());break;
    default:break;
    }
}
}
