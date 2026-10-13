// Rune304와 Dais305/306/307, 단계 진행/다음 목표 명령/희생 시작·종료의 원본 순서를 복원한다.
#pragma once
#include "o/RawCarrierPreDestroy.h"
#include "o/RawPathStart.h"
#include "o/RawRunePostPop.h"
#include "o/RawTempleRitual.h"

namespace netstorm::o {
class RawAssetRegular; // Dais의 나머지 사건을 기존 Damageable Handle에 연결한다.
inline constexpr std::uint32_t kPatchDaisVtable=0x00507950,kCdDaisVtable=0x00504090; // 실제165 생성자의 최종 가상표다.
// 기존 의식/시간/호스트 전역을 공유하며 player 필드를 매 효과 뒤 현재 값으로 읽는다.
struct DaisRegularState {
    TempleRitualState& ritual; // 같은 boss·priestType·RuneType을 TempleRitual과 공유한다.
    RunePostPopState& rune; // 같은 altarType·daisType·억제 전역을 Rune 조회와 공유한다.
    SquidProcessState& clock; // 공유 Regular의 현재 게임 시각이다. 소리 벽시계와 구분한다.
    SquidRewardState& sp; // 기존 SP 저장 전역이다. CD는 정확한 Get/Set 분기를 사용한다.
    std::uint32_t& localOwner; // 현재 로컬 플레이어 번호다. callbacks 뒤 다시 읽는다.
    std::uint32_t sacrificeType{169},nuggetType{154},noIslandType{93}; // 희생 코드표/획득 SP/로컬 특수 분기의 현재 타입 전역이다.
    std::array<std::int32_t,(kPlayerCount+1)*(kPlayerCount+1)> alliances{}; // 패치DWORD/CD int의 현재 방향별 동맹표다.
    std::array<std::uint32_t,kPlayerCount+1> playerResults{}; // patchPlayer+a0/CDPlayer+98의 마지막 개수/획득 타입이다.
    std::array<std::uint8_t,kPlayerCount+1> grantFlags{}; // 획득 시 지우는 patchPlayer+71/CDPlayer+69 BYTE다.
    std::uint32_t selectedType{},voiceCue{}; // 로컬 획득 타입 선택과 희생 종료에서 멈추는 전역 cue다.
    bool debug{}; // 패치의 사제 genus/깊이/경로 생성 assert를 확인한다. CD는 읽지 않는다.
};
// 원본 chat 인자는 문장으로 추정하지 않고 요청 종류/현재 owner/원래 owner/상태값으로 넘긴다.
struct DaisChat {
    enum class Kind { Upgrade,Stormpower,LocalPriestDead,OtherPriestDead }; // 실제 AltarUpgrade/Sacrifice/판본별 희생 안내 요청이다.
    Kind kind{}; // 실제 호출된 요청 종류다.
    std::uint32_t originalOwner{},currentOwner{},status{}; // 저장한 altar 원래owner와 요청 시 현재 owner 및 상태 helper 반환값이다.
};
// 생략할 수 없는 하위 효과다. 실제 기본 몸체는 MakeDaisRegularHooks가 연결하고 UI/장치 경계는 caller가 공급한다.
struct DaisRegularHooks {
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 같은 SquidFrame.Set의 실제 프레임/공간/표시 갱신이다.
    std::function<void(Sid,std::uint32_t)> destroy; // 실제 파생 가상+10이며 Rune48/포획 내용물400048을 구분한다.
    std::function<bool()> reserveShared; // collapse/sacrifice/altar 이탈의 new0x28 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> addShared; // 같은 Host의305/306/24 예약이다.
    std::function<void(float,float,std::string_view,std::uint32_t)> playAt; // itIsDone2/altarBurnCollapse 위치 음향이다.
    std::function<bool()> reserveSound; // 각 지연 소리 프로세스의 독립 new0x28 성공 여부다.
    std::function<std::uint32_t(std::string_view,std::uint32_t,std::uint32_t)> loadSound; // 원본 이름/0/0xc의 실제 소리 조회다.
    std::function<std::uintptr_t(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> attachSound; // 같은 SoundProcessSystem.Add이며 실패는0이다.
    std::function<double()> wallSeconds; // 지연 소리에만 사용하는 실제 벽시계00460d70/CD004011b0이다.
    std::function<void(std::uintptr_t,double)> setSoundStart; // 붙인 소리 프로세스 +20의 실제 지연 시각을 쓴다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> stopVoice; // 희생 종료의 전역 cue/0/0 정지 요청이다.
    std::function<std::uint32_t(std::uint32_t)> processType; // 기존 공유 client Path mapper를 실제 현재 타입에 적용한다.
    std::function<RawPathStartType(Sid)> pathMetadata; // 마지막 점 교체 뒤 Prepare가 읽는 현재 speed/turningSpeed다.
    std::function<float(Sid,std::uint32_t,std::uint32_t,float)> damageable; // 같은 기존 DamageableHandle 전체 몸체다.
    std::function<void(float)> addLocalSp; // 패치Reward.AddSp, CD Get/Set/표시의 정확한 local5000이다.
    std::function<void(std::uint32_t)> normalizeForeignSp; // 패치의 비로컬 너겟 획득은 현재 SP를 절삭해 동일 저장소에 다시 쓴다.
    std::function<void(std::uint32_t,std::uint32_t)> grantForeignType; // 패치만 비로컬 다른 타입의 실제 허용 목록을 바꾼다.
    std::function<void(std::uint32_t)> localSpecial; // noIsland 획득 시 로컬의 실제 특수 요청을 원본 인수0으로 호출한다.
    std::function<void(std::uint32_t)> localType; // 나머지 로컬 타입 획득의 실제 선택 초기화를 수행한다.
    std::function<void(std::uint32_t)> refreshOwner; // 희생 종료에서 현재1..8 플레이어의 실제 갱신을 순서대로 호출한다.
    std::function<void()> refreshLocal; // 현재 로컬의 실제 메뉴/목록/표시 갱신이다.
    std::function<bool(std::uint32_t)> priestExists; // 희생 후 SP 안내가 현재 각 플레이어의 실제 사제 존재를 읽는다.
    std::function<std::uint32_t()> multiplayerStatus; // 원본의 희생 안내 상태 helper다. 패치/CD의 차이를 실제 caller가 연결한다.
    std::function<void(const DaisChat&)> chat; // 원본 문자열 key/판본별 인자의 실제 UI 소비자다.
};
// 기존 조회·프레임·Host·PathStart를 같은 풀에 재사용하는 Dais 실제 상위 몸체다.
class RawDaisRegular {
public:
    // 모든 참조/필수 효과는 이 객체보다 오래 살아야 하며 서로 다른 SID 풀은 구성에서 거부한다.
    RawDaisRegular(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
        const RawTempleRitual& ritual,const RawRunePostPop& rune,const RawCarrierPreDestroy& carrier,SquidProcessHost& host,
        const RawContainedLookup& lookup,RawPathStart& path,DaisRegularState& state,DaisRegularHooks hooks);
    bool Handles(Sid sid) const; // 실제 Rune/Dais 타입과 각각의 실제 가상표를 함께 확인한다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const; // Rune304/Dais305·306·307 및 기존 fallback의 전체 분배다.
    void Progress(Sid dais) const; // 현재 stage/사제604 취소→다음 목표/collapse 또는 희생 음향/시작·altar 이탈이다.
    void StopPriestRitual(Sid priest) const; // 현재K..O이면 첫604를 실제로 지우고 현재 타입E 첫 프레임을 지정한다.
    void Collapse(Sid dais) const; // new/Shared305 이후 현재 단계 목표점의 위치 음향이다.
    void StartSacrifice(Sid dais) const; // extra2/altarM/개수별Dais0·45/Shared306을 원본 순서대로 실행한다.
    float SacrificeAdvance(Sid dais) const; // 현재 sacrificeType169 코드 구간에서 다음 프레임/0.12 또는 종료0이다.
    void FinishSacrifice(Sid dais) const; // cue/altarP/개수·내용물/획득/owner/UI/SP 안내의 실제 상위 순서다.
    void GrantCapturedType(std::uint32_t type,std::uint32_t owner) const; // 획득 flags/너겟/noIsland/로컬·비로컬 판본 분기를 보존한다.
    void AltarEscape(Sid altar) const; // 기준점Dais조회→altarL→new/Shared24 payload0.12의 실제 요청이다.
    bool CanBridge(Sid dais,Sid priest) const; // stage4 Rune/altar방향/사제genus·owner/포획 사제·패치동맹 조건이다.
    void BridgeCommand(Sid dais,Sid priest,std::uint32_t currentStage) const; // 칸일치의mode0 또는 깊이<4의실제PathStart/끝점/Prepare를 실행한다.
    std::int32_t MoveAwayDepth(Sid priest) const; // 현재 mapper/종속/Host로 실제 기존 Path 객체의 깊이를 읽는다.
    const SidPool& Pool() const; // 후속 GameWorld/등록의 동일 풀 검증이다.
private:
    std::int32_t CurrentFrame(Sid sid) const; // 패치signedDWORD/CD BYTE의 현재 프레임이다.
    TempleRitualPoint Position(Sid sid) const; // 원본 저장 폭의 현재 float 좌표다.
    std::uint32_t Owner(Sid sid) const; // 현재 owner BYTE를 효과 뒤 다시 읽는다.
    std::int32_t First(Sid sid,std::uint8_t letter) const; // 현재 raw타입의 실제 first-table을 기존 Run으로 읽는다.
    Sid Contained(Sid dais,std::uint32_t mask,std::uint32_t kind,bool allowDead=true) const; // 기존 Carrier 선택 몸체/실제 finder의 dead 허용을 공유한다.
    std::uint32_t CapturedWord(Sid dais) const; // mask4/kind0의 첫 포획 Form kind WORD다.
    Sid FindDaisAt(Sid sid) const; // altar 이탈의 결과를 버리는 실제 typed Dais 조회도 실행한다.
    void DelaySound(Sid dais,std::string_view name,double delay) const; // 독립 확보/조회/부착 성공 뒤에만 벽시계+지연을 쓴다.
    void Player(std::uint32_t owner) const; // 원본 중립0과 플레이어1..8 저장 범위를 손상 입력 전에 확인한다.
    SidPool& pool_; // 유일한 현재 raw 저장소다.
    const SquidHash& hash_; // 기존 typed finder와 같은 네 해시다.
    std::span<const RiftTypeRecord> types_; // 현재 사제 genus와 typed 발자국이다.
    std::span<const RiftTypeFrames> frames_; // 현재 direction/first와 전역169 희생 구간이다.
    const RawTempleRitual& ritual_; // 기존 count/Stage/목표점/사제·룬/의식 몸체다.
    const RawRunePostPop& rune_; // 기존 같은ownerDais와 공유AltarAt 몸체다.
    const RawCarrierPreDestroy& carrier_; // 기존 contained 조건/순서 몸체다.
    SquidProcessHost& host_; // 현재604 조회/실제삭제/PathFromForm의같은Host다.
    const RawContainedLookup& lookup_; // 기존현재Path form을조회하는같은종속저장소다.
    RawPathStart& path_; // 실제검색/flags0 Init/Prepare/서버부착의같은시작몸체다.
    DaisRegularState& state_; // 공유 원본 전역의 참조와 현재 player 필드다.
    DaisRegularHooks hooks_; // 실제 프레임/삭제/음향/예약/장치/UI 효과다.
};
// 실제 Frame/Host/Damageable/SP 저장소를 연결한다. 나머지 UI·음향·삭제/mapper·메타는 caller가 실제 연결한다.
DaisRegularHooks MakeDaisRegularHooks(const SidPool& pool,const SquidFrame& frame,SquidProcessHost& host,const RawAssetRegular& assets,
    SquidReward& reward,ScrambledSpStore* spStore,DaisRegularState& state,std::function<void()> refreshSp,DaisRegularHooks hooks);
}
