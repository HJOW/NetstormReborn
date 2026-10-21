// 초기 전략의 원본 읽기·쓰기·하위 호출 순서를 보존한다.
#include "o/RawAiStrategySetup.h"
#include "o/CanonDecoder.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kPlayerBytes=180,kBridgeSlots=8; //Player 한 기록과5f0 재고의 실제 패턴 칸 수다.
constexpr std::uint32_t kUnset=0xc61c4000U,kReserve=1200; //원본 미정 좌표-10000.0f와 목표 선택의 비용 여유다.
// 비정렬 little endian DWORD를 지정된 물리 저장소에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 전략 준비 DWORD 읽기 범위");
    std::uint32_t value{};
    // 원본 네 BYTE만 낮은 자리부터 합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
// 현재 지정 DWORD만 바꾸고 인접 필드를 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 전략 준비 DWORD 쓰기 범위");
    // 네 BYTE의 little endian 표현만 기록한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 원본 signed 분기에서 DWORD 감김을 정의된 비트 해석으로 보존한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 선택된 하위만 필요로 하며 누락된 효과를 빈 성공으로 처리하지 않는다.
template<class Function> const Function& Need(const Function& function,const char* message) { if (!function) throw std::logic_error(message);return function; }
}
// 생성은 실제 전략 실행 없이 같은 저장소의 연결만 확인한다.
RawAiStrategySetup::RawAiStrategySetup(RawAiBrainMemory& memory,RawPlayerWorld& player,const RawAiPreparation& preparation,
    const RawCarrierPreDestroy& finder,const RawContainedCreate& create,const ContainedFinderState& finderState,
    std::span<const RiftTypeRecord> types,GameRandom& random,std::span<const std::uint32_t> allies,AiStrategySetupGlobals globals,AiStrategySetupHooks hooks)
    :memory_(memory),player_(player),preparation_(preparation),finder_(finder),create_(create),finderState_(finderState),types_(types),random_(random),allies_(allies),globals_(globals),hooks_(std::move(hooks)) {
    if (player.Pool().Edition()!=OriginalEdition::Patch1078 || &player.Pool()!=&preparation.Pool() || &player.Pool()!=&finder.Pool() || &player.Pool()!=&create.Pool() || player.Records().size()!=9*kPlayerBytes || allies.size()!=81 || types.empty()) throw std::invalid_argument("AI 전략 준비 같은 PATCH/Player9/동맹81/TYPE 계약 누락");
}
// 다른 모듈과 같은 살아 있는 본체만 허용한다.
std::span<std::uint8_t> RawAiStrategySetup::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 전략 준비 실제0xb08 본체 누락");return raw; }
// 하위 효과 이후에도 본체를 다시 조회한다.
std::uint32_t RawAiStrategySetup::Field(std::uint32_t token,std::size_t offset) const { return Read(Body(token),offset); }
// 현재 본체의 선택된 필드만 갱신한다.
void RawAiStrategySetup::Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const { Put(Body(token),offset,value); }
// 축소 이전에 실제 풀 경계를 검사하며0도 원본 getter 입력으로 보존한다.
Sid RawAiStrategySetup::PhysicalSid(std::uint32_t value) const { if (value>=Pool().Capacity() || value>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("AI 전략 준비 SID 범위");return Sid{static_cast<std::uint16_t>(value)}; }
// 별도 지역 사본을 만들지 않는다.
std::int32_t RawAiStrategySetup::Region(std::uint32_t owner) const { if (owner>=9) throw std::out_of_range("AI 전략 준비 현재 Player 범위");return Signed(Read(player_.Records(),owner*kPlayerBytes+0x78)); }
// 원본 진단이 반환하면 다음 원문 연산으로 진행한다.
void RawAiStrategySetup::Assert(std::string_view text) const { Need(hooks_.assertion,"AI 전략 준비 진단 미연결")(text); }
// 범위가 잘못된 타입도 진단 장치 호출 자체는 먼저 보존한다.
void RawAiStrategySetup::AssertType(std::uint32_t type) const { if (Signed(type)<1 || types_.size()<=type) Assert("typeNum > 0 && typeNum < numTypes"); }
// 두 원본 함수는 서로 다른 이름이어도 실제 PE에서는 모두 mask8이다.
Sid RawAiStrategySetup::FindProduct(std::uint32_t token,std::uint32_t type) const {
    AssertType(type);const auto parent=PhysicalSid(player_.Technology(Signed(Field(token,0x260))));return finder_.FindContained(parent,8,type,0);
}
// 원본 생산 등록은 자금 차감이나 즉시 Pop 없이 내용물 생성만 요청한다.
void RawAiStrategySetup::AddProduct(std::uint32_t token,Sid parent,std::uint32_t type) const {
    AssertType(type);if (FindProduct(token,type).value) Assert("!haveProd( typeNum )");if (FindProduct(token,type).value) Assert("!haveTeleportable( typeNum )");
    if (Field(token,0x260)!=globals_.localOwner) {
        const auto existing=FindProduct(token,type);if (existing.value) Need(hooks_.destroy,"AI 전략 준비 현재 내용물 삭제 미연결")(existing,0);
        const auto technology=PhysicalSid(player_.Technology(Signed(Field(token,0x260))));const auto duplicate=finder_.FindContained(technology,8,type,0);
        if (duplicate.value && (static_cast<std::uint32_t>(Pool().Slot(duplicate)[18])|(static_cast<std::uint32_t>(Pool().Slot(duplicate)[19])<<8))) Assert("!s.findContainedType( CFF_TELEPORT, typeNum )");
        static_cast<void>(create_.Create(technology,type,0,8,0x10,0));
    }
    static_cast<void>(create_.Create(parent,type,0,0x10,0x10,0));
}
// 손상된 재고가 원본 고정 배열 뒤의 상태를 덮기 전에 물리 계약 오류로 알린다.
void RawAiStrategySetup::StoreBridge(std::uint32_t token,std::uint32_t pattern) const {
    const auto index=Signed(Field(token,0x628));if (index<0 || static_cast<std::size_t>(index)>=kBridgeSlots) throw std::out_of_range("AI 실제 다리 재고8칸 초과");Field(token,0x608+static_cast<std::size_t>(index)*4,pattern);
}
// 원본은 직선 조각을 강제할 때에도 난수/가중 추첨을 먼저 소비한다.
void RawAiStrategySetup::AddBridge(std::uint32_t token,Sid parent) const {
    const auto pattern=static_cast<std::uint32_t>(SelectBridgePattern(static_cast<std::int32_t>(random_.Next(10000)),OriginalEdition::Patch1078));StoreBridge(token,pattern);
    const auto serial=Field(token,0x5f4);Field(token,0x5f4,serial+1U);if (Signed(serial)%4==0) StoreBridge(token,0);
    const auto index=Signed(Field(token,0x628));static_cast<void>(create_.Create(parent,globals_.bridgeType,Field(token,0x608+static_cast<std::size_t>(index)*4),0x40,0x10,0));
    Field(token,0x628,Field(token,0x628)+1U);const auto deadline=std::bit_cast<std::uint64_t>(static_cast<double>(std::bit_cast<float>(Field(token,0x600)))+globals_.now);Field(token,0x630,static_cast<std::uint32_t>(deadline));Field(token,0x634,static_cast<std::uint32_t>(deadline>>32));
}
// 사용 중인 다리도 총수에는 포함하고, 배치 가능한 패턴만8칸 재고에 모은다.
std::uint32_t RawAiStrategySetup::ScanBridges(std::uint32_t token,Sid parent) const {
    Field(token,0x628,0);std::uint32_t total{};RawContainedFinder cursor(Pool(),finderState_);
    // 저장된 next를 이용하는 같은 원본 내용물 순회로 현재 다리 상태를 읽는다.
    for (auto sid=cursor.Begin(parent);sid.value;sid=cursor.Next()) {
        const auto raw=Pool().Slot(sid);const auto flags=static_cast<std::uint32_t>(raw[20])|(static_cast<std::uint32_t>(raw[21])<<8);
        if (!(flags&0x40)) continue;++total;if ((flags&0x140)==0x40) { StoreBridge(token,static_cast<std::uint32_t>(raw[12])|(static_cast<std::uint32_t>(raw[13])<<8));Field(token,0x628,Field(token,0x628)+1U); }
    }
    return total;
}
// 목표 개수는 첫 스캔 전에 저장하며 원본 부족분 반복만 수행한다.
void RawAiStrategySetup::SeedBridges(std::uint32_t token,Sid parent,std::uint32_t count) const {
    Field(token,0x604,count);const auto total=ScanBridges(token,parent);
    // 하위가 현재 목표 개수를 바꾸면 다음 비교에서 다시 읽으며 unsigned 감김 뒤 signed 비교한다.
    for (auto made=total;Signed(made)<Signed(Field(token,0x604));++made) AddBridge(token,parent);
}
// 원본은 신전/시간 관문에서 반환할 때에도 먼저 실제 패턴 재고를 갱신한다.
void RawAiStrategySetup::RefreshBridges(std::uint32_t token,Sid parent) const {
    const auto total=ScanBridges(token,parent);const auto owner=Pool().Slot(parent)[34];if (owner>=9) throw std::out_of_range("AI 다리 보충 현재 Player 범위");
    if (!Read(player_.Records(),owner*kPlayerBytes+0x7c) || Signed(total)>=Signed(Field(token,0x604))) return;
    const auto due=std::bit_cast<double>((std::uint64_t(Field(token,0x634))<<32)|Field(token,0x630));
    if (!(globals_.now>=due)) { if (owner!=globals_.localOwner || !Need(hooks_.demo,"AI 다리 보충 현재 데모 조회 미연결")()) return; }
    AddBridge(token,parent);
}
// 적과 양쪽 시작 지역이 준비되지 않았으면 완료 표식을 쓰지 않는다.
void RawAiStrategySetup::Initialize(std::uint32_t token) const {
    const auto enemy=Field(token,0x264);if (!enemy) preparation_.SelectEnemy(token);
    else { const auto owner=Field(token,0x260);if (owner>=9 || enemy>=9) throw std::out_of_range("AI 전략 준비 방향별 동맹 범위");if (allies_[owner*9+enemy]) preparation_.SelectEnemy(token); }
    if (!Field(token,0x264) || Region(Field(token,0x260))==globals_.noRegion || Region(Field(token,0x264))==globals_.noRegion) return;
    const auto bounds=Need(hooks_.regionBounds,"AI 초기 자기 청크 경계 미연결")(Region(Field(token,0x260)));
    // float 경계 네 값의 비트 표현을 같은 본체에 저장한다.
    for (std::size_t index=0;index<bounds.size();++index) Field(token,0x270+index*4,std::bit_cast<std::uint32_t>(bounds[index]));
    Field(token,0x268,Field(token,0x270));Field(token,0x26c,Field(token,0x274));
    const auto centerX=static_cast<float>((static_cast<double>(std::bit_cast<float>(Field(token,0x278)))-std::bit_cast<float>(Field(token,0x270)))*0.5+std::bit_cast<float>(Field(token,0x268)));Field(token,0x268,std::bit_cast<std::uint32_t>(centerX));
    const auto centerY=static_cast<float>((static_cast<double>(std::bit_cast<float>(Field(token,0x27c)))-std::bit_cast<float>(Field(token,0x274)))*0.5+std::bit_cast<float>(Field(token,0x26c)));Field(token,0x26c,std::bit_cast<std::uint32_t>(centerY));
    const auto technology=PhysicalSid(player_.Technology(Signed(Field(token,0x260))));SeedBridges(token,technology,Field(token,0x238));
    const auto temple=Need(hooks_.temple,"AI 초기 현재 신전 조회 미연결")(Region(Field(token,0x260)));
    if (temple && !FindProduct(token,globals_.templeProduct).value) { const auto sid=PhysicalSid(temple);AddProduct(token,sid,globals_.templeProduct);const auto type=Pool().Slot(sid)[10];if (type>=types_.size()) throw std::out_of_range("AI 초기 신전 BYTE TYPE 범위");Field(token,0x390,std::bit_cast<std::uint32_t>(types_[type].theme)); }
    Field(token,0x258,1);
}
// 원본 목표 변경 때만 기존 다리 탐색 상태를 초기화한다.
void RawAiStrategySetup::SelectGoal(std::uint32_t token) const {
    const auto attached=Field(token,0x2d0);std::uint32_t goal{};
    if (attached && Signed(Field(token,0x2d8))>Signed(Field(token,0x290)+kReserve) && (Field(token,0x248)&4) && Field(token,0x298)!=Field(token,0x2ac)) goal=1;
    else if (!attached && (Field(token,0x248)&0x10)) goal=2;
    else {
        const auto x=std::bit_cast<float>(Field(token,0xaf0)),y=std::bit_cast<float>(Field(token,0xaf4));const auto limit=static_cast<float>(globals_.mapLimit);
        if (x>0 && x<limit && y>0 && y<limit) goal=4;
        else goal=attached && Signed(Field(token,0x2d8))>Signed(Field(token,0x290)+kReserve) ? 3U : (Field(token,0x248)&0x10) ? 2U : 3U;
    }
    if (goal==Field(token,0x2d4)) return;Field(token,0x3cc,0);Field(token,0x3d4,kUnset);Field(token,0x3d0,kUnset);Field(token,0x3d8,1);Field(token,0x5f0,0);Field(token,0x2d4,goal);
}
// 후속 실제 월드 조립 확인용 동일 풀이다.
const SidPool& RawAiStrategySetup::Pool() const { return player_.Pool(); }
}
