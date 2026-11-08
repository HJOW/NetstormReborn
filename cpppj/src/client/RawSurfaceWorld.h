// 본섬 표면·noIsland·다리와 파생 받침을 실제 raw 풀/Graph/Kernel로 소유하는 새 월드 연결부다.
#pragma once
#include "o/RawBoardAngle.h"
#include "o/RawSquidContainedUnpop.h" // 같은 실제 SID 풀의 종속 해제와 UpdateContents 예약을 연결한다.
#include "o/RawAmbientWind.h"
#include "o/GameClock.h"
#include "client/RawProcessHostStorage.h"
#include "o/RawAuthorityState.h"
#include "o/RawBattleBroadcast.h" // 같은현재연결표를사용하는Squid패킷송신경계다.
#include "o/RawCheckingFlag.h" // Editor/Session이같은Client진단owner를빌린다.
#include "o/RawGameAuxState.h"
#include "o/RawAssetPostPop.h" //장기Geyser가현재TYPE/목록과같은기존원문몸체를빌린다.
#include "o/RawNuggetRegular.h" //너겟의기존왕복/낙하/현재가상삭제를장기기본Host에연결한다.
#include "o/RawNuggetPostPop.h" //실제두Regular확보와같은Host의기본부모다.
#include "o/FortSerializationGlobals.h" //실제 Template/Core/객체의005c89b8 owner를같이pin한다.
#include "o/RawBoardTarget.h"
#include "o/RawPathSearch.h" //원문거리표의한소유자를Client물리에보존한다.
#include "o/RawGraph.h"
#include "o/RawIslandRuntime.h"
#include "o/ChunkMap.h"
#include "o/RawPriestPostPop.h"
#include "o/RawBridgeConnect.h"
#include "o/SquidDisplay.h"
#include "o/RawBridgeEvents.h"
#include "o/RawBuildingSpace.h"
#include "o/RawIslandPostPop.h"
#include "o/RawSurfaceLifecycle.h"
#include "o/RawSquidDestroyDispatch.h"
#include "o/SquidFactory.h"
#include "o/SquidFormDispatch.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/SquidOwner.h"
#include "o/SquidReward.h" // Native HP/보상은 같은 Client 물리와 전역을 장기 공유한다.
#include "o/RawStateSidLists.h"
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <optional>

namespace netstorm::client {
class GameAssets; // 실제 Client가 먼저 읽은 자산의 정체성을 물리 저장소와 공유한다.
class NativeFortSaveVirtual; //저장/보상 가상 조회가 같은 현재 Pool/TYPE/Hash를 빌리는 순수 장치다.
// 실제 분산 SP와 현재 편집 모드의 owner를 보존하며 Session/World를 소유하지 않는다.
struct RawSurfaceRewardDevices {
    std::shared_ptr<o::ScrambledSpStore> sp; // 같은 ClientSpGlobals의 실제 저장소를 aliasing 소유권으로 빌린다.
    std::shared_ptr<const std::uint32_t> editor; // 같은 GameplayGlobals의 실제 editor DWORD 주소를 보존한다.
    std::function<void()> refreshSp; // 실제 현재 예산 serial의 Push/Attach/깊이 복원을 호출한다.
    std::function<void(std::uint32_t,std::int32_t)> creditWallet; // 원본 조기 반환 뒤 같은 Player/brain 지갑에 가산한다.
};
struct RawSurfaceResourceDevices { //같은Client게임난수와선택된큰자산장치만장기로빌린다.
    std::function<o::GameRandom&()> random; //새seed/snapshot없이실제현재GameRandom한개를조회한다.
    std::function<void(o::Sid)> carrierCheck; //실제가상cc의현재Carrier검사를선택위치에서호출한다.
    o::RegularHandler damageableRegular; //실제피해사건의현재큰세계종단이며자연기본사건은장기본문이직접판정한다.
    std::function<void(o::Sid)> startConstruction; //실제abstract/현재Form조회뒤건설Start만선택해호출한다.
    std::function<void(o::Sid,std::uint32_t)> destroy; //선택된현재가상삭제종단이며NoWorld미복원을성공으로꾸미지않는다.
    std::function<void(o::Sid,std::uint32_t)> geyserRegistration; //현재Player9/brain순회의실제416720등록부모다.
};
struct RawSurfaceProcessDevices { //실제Client현재선택/선택해제부모만장기로빌린다.
    std::function<std::uint32_t()> selected; //현재전체DWORD선택전역이며Form이라는이유로0/WORD를합성하지않는다.
    std::function<void()> clearSelection; //실제현재Client선택정리부모를선택된효과위치에서호출한다.
};
struct RawSurfaceTransmissionDevices { //실제Client의현재Registry/Player장치만Storage수명으로보관한다.
    std::function<void(std::span<std::uint8_t>)> broadcast; //현재Player/NULL/로컬선택뒤대상별같은버퍼를실제가상+4에넘긴다.
    std::function<bool(std::int32_t)> connected; //writer뒤현재Player+50bit1만읽는48f4b0이다.
    std::function<void(std::int32_t,std::span<std::uint8_t>)> send; //현재4e2dc0관문과선택된실제Registry송신이다.
};
struct RawSurfacePopBindings { //현재실제Pop writer만짧게빌리며해제뒤죽은World를호출하지않는다.
    std::function<void(o::Sid,std::uint32_t)> transmit; //원문Activate뒤의같은Squid전송/방송부모다.
    o::BattlePacketSend broadcast; //기존독립fixture가관측하는writer반환크기의const계약이다.
    std::function<bool(std::int32_t)> connected; //기존독립World가명시연결한현재Player관문이다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; //기존독립대상전송의const관측계약이다.
    std::function<std::int32_t(o::Sid,std::span<std::uint8_t>,std::uint32_t)> processPacket; //외부rawKernel을쓰는현재World는자기실제Host를사용한다.
    std::function<std::shared_ptr<void>()> pin; //실제현재전체Runtime의선택호출수명이며binding자체의shared소유권과구별한다.
};
struct RawSurfaceDisplayBindings { //같은장기표시의현재카메라/Renderer장치또는짧은World대여다.
    std::function<void()> changed; //현재World의기존호스트표시갱신관찰이며World없는경로에는이짧은관찰자가없다.
    std::function<o::SquidDisplayView()> view; //현재카메라snapshot/실제Root지도영역이며새기본viewport를만들지않는다.
    std::function<bool()> suppressed; //현재Renderer의원문전체갱신억제전역이다.
    std::function<void(o::SquidDisplayRect,std::uint32_t)> invalidate; //계산된실제그림/그림자범위를같은dirty표에기록한다.
    std::function<void(std::span<const std::uint8_t>)> metaRefresh; //선택된메타VT만실제파생88장치를요구한다.
    std::function<std::shared_ptr<void>()> pin; //현재World의실제전체owner를dirty/카메라callback반환까지보유한다.장기Client장치는비워둔다.
};
// Session보다 오래 사는 실제 Pool/공간/자산 배열이다. Runtime 모듈의 수명과 구별한다.
class RawSurfaceStorage : private o::SquidDisplaySink {
private:
    std::shared_ptr<o::RawAuthorityState> authority_; // Pool의 현재 서버 포인터와 World 실행기보다 먼저 만들고 마지막에 반납한다.
    std::shared_ptr<o::FortSerializationGlobals> serializationGlobals_; //실제Client의장기판본/깊이owner이며World/Sessioncallback을소유하지않는다.
    std::uint32_t localLoadingDepth_{1}; //독립SurfaceWorld의기존초기깊이1 fallback만보존한다. 실제Client는공유owner를필수연결한다.
    std::shared_ptr<o::RawGameAuxState> gameAux_; // 실제 장부/게임 TYPE 참조보다 먼저 구성한다.
    o::RawCheckingFlag checking_; // 대여/World반납뒤에도같은현재Client진단DWORD를읽는다.독립저장소기본값은0이다.
    o::SquidPostPopDevices postPopDevices_; //현재실제Client AI/로컬owner/선택의장기장치다.
    std::weak_ptr<const RawSurfacePopBindings> popRuntime_; //현재typed장치/외부Kernel만짧게빌리고반납뒤장기기본전송을사용한다.
    RawSurfaceRewardDevices rewardDevices_; // 같은 실제 SP/모드 owner를 처리기보다 오래 보관한다.
    RawSurfaceResourceDevices resourceDevices_; //실제Client장치만보관하며짧은Session주소를저장하지않는다.
    RawSurfaceProcessDevices processDevices_; //실제Native선택/정리장치만보관하며World/Session을캡처하지않는다.
    RawSurfaceTransmissionDevices transmissionDevices_; //실제Client만명시연결하며Source포인터로Native정책을추정하지않는다.
    std::weak_ptr<o::RawSquidContainedUnpop> currentContainedUnpop_; //현재Session의파생종단만약하게빌린다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> contentCommand_; //같은Client입력큐의실제452fe0장치다.
    RawSurfaceDisplayBindings displayDevices_; //실제Client장치만장기보관하며Frame/UI객체를소유하지않는다.
    std::weak_ptr<const RawSurfaceDisplayBindings> displayRuntime_; //현재World의표시변화관찰/기존view는짧은대여만빌린다.
    std::optional<std::string>* displayAssertionDetail_{}; // 같은 Client의005e4798 소유자를 빌리며 Storage보다 오래 살아야 한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> displayAssertion_; // 현재 표시 진단 장치이며 World/Session callback을 보관하지 않는다.
    std::function<double()> graphClock_; // Client 수명 동안 현재 게임 시각을 읽는 장치다. World 대여/반납은 교체하지 않는다.
    std::function<void(std::int32_t,std::string_view)> graphTrace_; // 실제 Client의 현재 로그 장치를 빌리며 World보다 오래 살아 있다.
    bool patchDestroyDiagnostics_{}; // 실제 Client만 명시 선택하며 독립 typed World와 CD는 기존 삭제 계약을 유지한다.
    std::function<void()> allocationDump_; //CD 소진에서만 선택하는 현재 Client 객체 덤프 장치다.
    std::function<void(std::string_view)> allocationTell_; //CD 회수 뒤 같은 Client 공통 Tell 부모를 호출한다.
public:
    // 대여 중에는 두 번째 월드가 같은 풀을 초기화하지 못하게 한다.
    class Use {
    public:
        ~Use(); // 월드의 모든 모듈을 반납한 뒤 대여만 해제한다. Kernel을 정리하지 않는다.
        Use(const Use&)=delete;Use& operator=(const Use&)=delete; // 동일 대여를 복사해 조기 해제하지 않는다.
    private:
        friend class RawSurfaceStorage;
        explicit Use(RawSurfaceStorage& storage); // 이미 검증한 단일 소유자의 반납 경계다.
        RawSurfaceStorage& storage_; // 실제 물리 소유자는 대여보다 오래 살아야 한다.
    };
    ~RawSurfaceStorage(); // 최종 ProcessHost/Kernel을 실제 Pool/TYPE보다 먼저 반납한다.
    bool UsesNativePostPopDevices() const { return static_cast<bool>(postPopDevices_.aiNotice); } //실제Client장치의결합수명만조회하며게임상태/ACK를생성하지않는다.
    void BindPostPopDevices(o::SquidPostPopDevices devices); //현재Client큰하위와원문자연관문을연결만한다.
    o::SquidPop& Pop(); //같은공간/장부/표시와현재전송경계를사용하는실제Pop부모다.
    o::SquidUnpop& Unpop(); //같은void/Hash/spot/건물옆면/현재종속종단을실행한다.
    o::SquidPostPop& PostPop(); //공통장부와실제표면접두를장기소유하며파생대여는반납한다.
    o::SquidFrame& Frame(); //같은현재Pop/Unpop/표시로일반프레임변경을실행한다.
    o::RawBridgeConnect& Connections(); //현재연결생성/색/소유자전파의동일실제몸체다.
    o::RawIslandPostPop& Surface(); //현재noIsland의실제프레임/받침/소유자전파접두다.
    o::RawSquidContainedUnpop& ContainedUnpop(); //현재공통종속/부모통지/UI입력의장기종단이다.
    void BindContainedUnpop(const std::shared_ptr<o::RawSquidContainedUnpop>& binding); //현재동일Pool의Session파생종단만약하게결합한다.
    void BindContentCommand(std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> command); //실제Client입력getter를연결만한다.
    void BindSavedGame(bool& savedGame); //실제Client의현재005c85d4를첫World전부터같이읽는다.
    void BindPopRuntime(const std::shared_ptr<const RawSurfacePopBindings>& runtime); //현재실제writer만weak로빌린다.
    bool UsesNativeTransmissionDevices() const { return static_cast<bool>(transmissionDevices_.broadcast); } //실제Native송신장치의명시결합수명만조회하며typed/GameState/성공값을추정하지않는다.
    void BindTransmissionDevices(RawSurfaceTransmissionDevices devices); //첫Factory/Host/World앞에현재Client장치만연결한다.
    std::int32_t SerializeProcessForm(o::SquidProcess& process,std::span<std::uint8_t> buffer,bool parentChanged,std::uint32_t force); //같은실제Kernel객체의파생writer에현재헤더와caller의같은예약을넘긴다.
    std::uint32_t CurrentProcessSelection() const; //Native공통삭제의선택비교위치에서실제Client전체DWORD를현재조회한다.
    void BindResourceDevices(RawSurfaceResourceDevices devices); //첫자원파생앞에실제장기장치만연결하며난수/생성/대여는실행하지않는다.
    void BindRewardDevices(RawSurfaceRewardDevices devices); // 첫 Native 사용 전에 실제 장치만 연결하며 SP/난수/World를 초기화하지 않는다.
    o::SquidRewardState& RewardState(); // 같은 장기 전역 뷰를 Session과 보상 caller가 공유한다.
    o::SquidReward& Reward(); // 같은 Pool/TYPE/장부의 장기 HP·보상을 필요한 순간 한 번 구성한다.
    void BindProcessDevices(RawSurfaceProcessDevices devices); //첫Host/삭제기전현재Client장치만결합하며선택/삭제를실행하지않는다.
    o::RawSquidDestroy& BaseDestroy(); //Native공통삭제깊이/Unpop/Pool의장기실제owner다.
    void BroadcastProcessForm(std::int32_t length); //실제writer의양수반환뒤같은공통2048BYTE를Native대상별가상송신에넘긴다.
    o::RawSquidTransmission& Transmission(); //공통2048BYTE/헤더/압축전역과실제현재FormHost를장기로공유한다.
    void UnpopAsset(o::Sid sid,std::uint32_t flags); //현재TYPE와동일Unpop/표시·종속장치를사용하고정상뒤현재World의호스트changed만알린다.
    void SetAssetOwner(o::Sid sid,std::uint32_t owner); //같은실제가상owner분배를World없이선택하며장부/현재모드를복제하지않는다.
    o::RawPopResult PopAsset(o::Sid sid,float x,float y,std::uint32_t flags); //원문PATCH는좌표/spot뒤현재TYPE/SHP를읽으며CD의기존사전크기계약은유지한다.
    void RepopAsset(o::Sid sid,std::uint32_t flags); //현재같은슬롯의float좌표로실제가상Pop을다시실행한다.
    o::RawGraph& Graph(); //같은현재장부/Graph물리와TYPE/진단/시계를World없이빌린다.
    o::RawSquidNeighbors& Neighbors(); //같은Pool/Hash/spot/현재발자국을읽는장기조회기다.
    o::RawBuildingSpace& Building(); //현재isle/다리TYPE와같은공간을Pop/Unpop이공유한다.
    o::SquidOwner& Owner(); //같은현재모드/장부/PlayerFactory의공통가상owner몸체다.
    o::SquidDisplay& Display(); //같은TYPE/SHP/진단과현재Renderer로실제표시부모를World없이제공한다.
    void BindDisplayDevices(RawSurfaceDisplayBindings devices); //첫실제Client물리준비뒤현재Renderer/카메라getter만결합하며조회/난수/그리기는실행하지않는다.
    void BindDisplayRuntime(const std::shared_ptr<const RawSurfaceDisplayBindings>& runtime); //같은현재World대여만weak로공개하고만료뒤장기Client장치를쓴다.
    o::Sid CreateAsset(std::uint32_t type,std::uint32_t flags=0); //기존 World의 changed prefix 뒤 같은 현재 Factory Create를 실행하고 새 World를 요구하지 않는다.
    NativeFortSaveVirtual& FortSaveVirtual(); //현재 Robust/HasLevel/Count의 동일 장기 조회기를 처음 필요할 때 구성하며 World/파일/등록을 만들지 않는다.
    o::SquidFactory& Factory(); //같은Pool/현재TYPE/HP·깊이/진단으로실제Create를World없이실행하며Acquire/reset/Host대여를추가하지않는다.
    RawProcessHostStorage& Processes(); // 같은 Storage의 장기 Host/상태와 현재 World 대여를 제공한다.
    RawSurfaceStorage(o::OriginalEdition edition,std::span<const o::RiftTypeRecord> inputTypes,std::span<const o::RiftTypeFrames> inputFrames,std::span<const o::SquidDisplayShape> inputShapes,const GameAssets* source=nullptr,std::shared_ptr<o::RawAuthorityState> authority={},std::shared_ptr<o::RawGameAuxState> gameAux={}); // 배열은 최초 한 번만 확보한다.
    RawSurfaceStorage(const RawSurfaceStorage&)=delete;RawSurfaceStorage& operator=(const RawSurfaceStorage&)=delete; // 현재 모든 실행기가 빌리는 실제 물리를 복제하지 않는다.
    RawSurfaceStorage(RawSurfaceStorage&&)=delete;RawSurfaceStorage& operator=(RawSurfaceStorage&&)=delete; // Graph/Pool/공간 참조가 남은 객체의 주소를 이동하지 않는다.
    std::unique_ptr<Use> Acquire(); // 기존 Pool/해시/spot 및 fresh Graph/집계3표의 일반 설치 의미를 같은 물리에서 유지하며 TYPE 수정은 보존한다.
    std::unique_ptr<Use> AcquirePrepared(); // 명시 Native 정리/prefix 뒤 같은 Pool/Hash/spot/Graph/집계3표/모드/종류를 재초기화하지 않고 대여한다.
    bool Borrowed() const; // 현재 실제 월드가 물리를 빌렸는지 읽는다.
    std::uint64_t Generation() const; // 성공한 월드 대여 횟수이며 원본 게임 값으로 쓰지 않는다.
    const std::shared_ptr<o::RawAuthorityState>& Authority() const { return authority_; } // Client 전역/Session과 같은 실제 서버·권한 주소의 정체성이다.
    const std::shared_ptr<o::RawGameAuxState>& GameAux() const { return gameAux_; } // 같은 Gameplay/Host가 pin하는 실제 게임 보조 owner다.
    void BindSerializationGlobals(std::shared_ptr<o::FortSerializationGlobals> globals); //실제같은깊이owner를대여전한번연결하며값/reset/권한은쓰지않는다.
    std::uint32_t& LoadingDepth(); //실제Template/Core와같은현재깊이를읽고쓴다. 독립Surface만기존1fallback을쓴다.
    const std::uint32_t& LoadingDepth() const; //Pop진입마다동일주소를빌리고시점별사본을동기화하지않는다.
    void BindChecking(o::RawCheckingFlag checking); // 비소유현재뷰또는독립자체값을수명안전하게복사하며bool사본을동기화하지않는다.
    o::RawCheckingFlag Checking() const override { return checking_; } // 호출위치에서현재owner를읽을수있는같은뷰다.
    void BindDisplayDiagnostics(std::optional<std::string>& detail,std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion); // 첫 World 이전에 같은 Client 상세/진단 수명을 연결한다.
    void RecordDisplayFrameError(std::uint32_t type,std::int32_t frame,std::uint32_t abstract); //현재 같은 TYPE/shape로43d40의 상세를 저장한다.
    void RecordAssertionDetail(std::string_view text); // 00436d20처럼 전달된 문구를 같은 Client 상세 소유자에 복사하며 원문 포인터를 보관하지 않는다.
    void AssertDisplay(std::string_view expression,std::string_view file,std::uint32_t line) const; // 선택된 원문 진단 장치가 복귀하면 caller도 계속한다.
    void EnablePatchDestroyDiagnostics(); // 첫 실제 PATCH World 전에 같은 Client의 공통 삭제 진단 경로를 선택한다.
    std::optional<o::SquidDestroyDiagnostics> DestroyDiagnostics() const; // 선택된 경우 같은 장기 owner의 현재 진단/로그만 World에 빌려 준다.
    void BindAllocationDiagnostics(std::function<void()> dump,std::function<void(std::string_view)> tell); //CD 소진의 객체 덤프와 Tell을 World보다 긴 Client 수명으로 연결한다.
    void BindGraphTrace(std::function<void(std::int32_t,std::string_view)> trace); // 첫 World 전에 현재 Client의 로그 writer를 연결한다.
    void UpdateAmbientWind(o::GameRandom& random); // 같은 Client 난수를 한 번/선택 시 두 번 소비하며 현재 세 바람 전역을 갱신한다.
    o::AmbientWindState wind; // 실제 PE 최초0/1/0이며 World Acquire/반납에서 재초기화하지 않는 Client 수명이다.
    o::BoardAngleDiagnostics AngleDiagnostics() const; // 같은 현재 Client checking/원문 진단을 각도 부모에 빌린다.
    void BindGraphClock(std::function<double()> clock); // 같은 Client GameClock 장치를 장기 owner가 빌린다.
    double ReadGraphClock() const; // 독립 저장소는0이며 실제 장치는 현재 정지/누적 시간을 반영한다.
    void TraceGraph(std::int32_t level,std::string_view text) const; // 현재 장치가 있으면 원문 수준/텍스트를 그대로 전달한다.
    std::uint32_t& GeyserType() { return gameAux_->GeyserType(); } // 모든 실제 타입 소비자의 현재 DWORD다.
    const std::uint32_t& GeyserType() const { return gameAux_->GeyserType(); } // 대여/반납으로 타입 주소를 교체하지 않는다.
    const GameAssets* Source() const; // Client 자산 정체성이다. 독립 raw 검사에서는 nullptr이다.
    std::vector<o::RiftTypeRecord> types; // 같은 수정 가능한 전역 TYPE이며 Session마다 원본 파일로 덮어쓰지 않는다.
    std::vector<o::RiftTypeFrames> frames;std::vector<o::SquidDisplayShape> shapes; // 자산보다 짧고 모든 Session보다 긴 프레임/셰이프 배열이다.
    std::vector<std::vector<o::FrameCode>> codes; // Graph의 동일 프레임 코드 배열이다.
    o::SidPool pool;o::SquidHash hash;std::vector<std::uint8_t> spots; // Reset(0)은 기존 SID 바이트/공간 배열의 주소를 유지한다.
    std::uint32_t disallowSquidAllocation{}; //005c8458의최초BSS0이며모든World가같은금지DWORD를빌린다.
    o::RawPathSearchStorage pathSearchWork; //59514c/595150의실제배열은World재대여때초기화하지않고진단과같이빌린다.
    o::BoardTargetState boardTargetState; //545370/545374 실패점을 Pool과 같은 장기 수명에 두고 Board/AI가 직접 빌린다.
    o::RawGraphStorage graph; // 기록255행과 flood4096DWORD만 장기 보유한다. 실행기/TYPE snapshot/복구 정책은 World 지역이다.
    o::SquidPostPopList recentGeysers; //00595094/count와595098의고정5DWORD이며정적SID8목록과다른실제Client수명이다.
    std::array<std::uint32_t,9> geyserPlayerDirections{16448,4,0,6,2,7,3,5,1}; //0053181c의현재9DWORD이며World대여/반납에서새표로교체하지않는다.
    o::RawStateSidLists sidLists; //실제 정적 SID8목록/count와 등록20포인터를 World/Session 전후에 보존한다.
    o::RawTotalMadeStorage totalMade; // 현재 로컬/누적 로컬/전역256DWORD만 장기 보유하며 ownerFactories/로딩 정책은 World 지역이다.
    o::RawIslandStorage islands;o::ChunkMap chunks; // 같은3204BYTE 섬과 mode/크기/256×12BYTE 청크를 Session 전후에도 유지한다. 실행기/효과는 포함하지 않는다.
    std::array<std::uint32_t,o::kPlayerCount+1> priestOwnerState{}; //00595428..00595448의9DWORD 최초BSS0이다. 실제 사제 SID목록/count는 sidLists에 있고 지갑은 포함하지 않는다.
    o::SquidPostPopState postPopState{.sharedTotalMade=&totalMade,.sharedTotalCost=&gameAux_->TotalCost(),.sharedProviders=&sidLists.providers,.sharedFactories=&sidLists.factories}; //공통depth/생산dirty/Graph와실제장부의주소를World뒤에도유지한다.
    o::SquidDeletionState deletionState{.sharedLostX=&gameAux_->LostX(),.sharedLostY=&gameAux_->LostY()}; //공통pre/post의현재분할/선택/억제와손실위치를같은장기owner로빌린다.
    o::IslandPostPopState islandPostPopState{1,false}; //독립기존깊이만fallback으로보존하고실제Client는위공유깊이/저장게임주소를읽는다.
    std::uint32_t containedType{6},containedGumpType{3},containedUiEvent{0x02000039}; //원문공통내용물/Gump/UpdateContents의같은현재장기전역이다.
    o::BuildingSpaceState buildingSpaceState; //독립World의기존terrain인자fallback이며실제Client는현재GameAux isle TYPE를직접읽는다.
    o::SquidOwnerMode ownerMode; // 미니맵과 현재 월드가 같은 실제 세 모드 값을 읽는다.
    o::BridgeConnectState links; // 생산 UI와 현재 월드가 같은 실제 표면 종류 전역을 읽는다.
    o::BridgeDecayScanState bridgeDecayScan; //0052f96c/970·005453a8/b8의 같은 현재 스캔 범위·커서·기한을 World 반납 뒤에도 유지한다.
    float bridgeDecayPeriod{10.0F}; //0052f968의 원문 최초float10이며 Native 정리는 이 현재 값을 읽고 덮어쓰지 않는다.
private:
    o::SquidDisplayView CurrentDisplayView() const; //현재짧은대여또는같은Client의실제카메라/viewport만조회한다.
    bool Suppressed() const override; //선택된현재Renderer/World의전체표시억제를읽는다.
    void Invalidate(o::SquidDisplayRect rectangle,std::uint32_t flags) override; //현재실제dirty표또는기존World표시소비자에전달한다.
    void FrameError(std::uint32_t type,std::int32_t frame,std::uint32_t abstract) override; //43d40의실제상세owner를사용한다.
    void Assertion(std::string_view expression,std::string_view file,std::uint32_t line) override; //정상복귀하는실제Client진단이다.
    void MetaRefresh(std::span<const std::uint8_t> bytes) override; //파생메타갱신은선택된실제장치가없으면명시실패한다.
    void RefreshChecking(); //이미존재하는실행기의진단view만갱신하고새모듈/World/Native효과는만들지않는다.
    void NoteChanged(); //현재World의기존호스트changed관찰만알리며원문게임상태를새로쓰지않는다.
    void TransmitPop(o::Sid sid,std::uint32_t flags); //현재World가없어도같은장기writer/실제Registry를사용한다.
    float ResourceDamageableRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload); //44b900의현재사건/abstract/현재Form관문뒤필요한실제큰하위만선택한다.
    o::RawNuggetRegular& ResourceNuggetRegular(); //현재프레임/공간/권한/같은Regular확보메모리의실제장기본문이다.
    void DefaultResourcePostPop(o::Sid sid,std::uint32_t flags); //현재실제VT의Geyser/Nugget/직접Carrier·Damageable본문만분배한다.
    o::RawCarrierPostPop& ResourceCarrierPostPop(); //실제같은로딩깊이/권한/TYPE/지면소유자/공통깊이를장기로연결한다.
    bool AllocateResourceRegular(); //원문new28의동일확보메모리이며실패뒤난수를소비하지않는다.
    void ConstructResourceRegular(o::Sid sid,std::uint32_t event,float payload); //확보한같은객체주소에서현재Host에생성·부착한다.
    o::SquidDestroyHooks DefaultProcessAssetHooks(); //실제선택/전송과미설치자산가상몸체를구별하는장기Formfallback이다.
    o::SquidFormDispatch& ProcessFormDispatch(); //같은공통삭제깊이에실제ProcessForm호스트를등록하며다른Form은필요한전용owner를요구한다.
    float HandleDefaultRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload); //PE의현재VT+5c가복원된원문base인경우만해당본문을실행한다.
    std::int32_t WriteDefaultForm(o::SquidProcess& process,bool parentChanged,std::uint32_t force); //Dais/Construction의실제writer를같은공통버퍼로분배하며다른파생누락은선택위치에서드러낸다.
    o::BridgeConnectHooks SurfaceObjects(); //연결/받침이공유하는같은실제Factory/Owner/Pop/Frame/통지다.
    std::unique_ptr<o::RawSquidContainedUnpop> containedUnpop_; //현재실제공통종단은Pool/현재전역보다짧고World보다길다.
    std::unique_ptr<o::RawGraph> graph_; //Graph배열/스택과현재장부는앞의같은장기물리이며생성은재초기화하지않는다.
    std::unique_ptr<o::RawSquidNeighbors> neighbors_; //현재이웃/finder는같은실제공간과주소고정TYPE를사용한다.
    std::unique_ptr<o::RawBuildingSpace> building_; //건물옆면의실제WORD지도/진단을장기보유한다.
    std::unique_ptr<o::SquidOwner> owner_; //공통소유자변경은같은장부와모드를공유한다.
    std::unique_ptr<o::SquidDisplay> display_; //현재같은Pool/TYPE/SHP의장기표시실행기이며Host/공장보다먼저소멸하지않는다.
    std::unique_ptr<o::SquidPostPop> postPop_; //같은공통depth/장부와현재표면접두를유지한다.
    std::unique_ptr<o::SquidUnpop> unpop_; //현재Hash/spot/표시와종속해제를공유한다.
    std::unique_ptr<o::SquidPop> pop_; //현재공간쓰기를실행하고실제postPop/전송을선택한다.
    std::unique_ptr<o::SquidFrame> frame_; //장기Pop/Unpop/표시의같은인스턴스를빌린다.
    std::unique_ptr<o::RawBridgeConnect> connections_; //현재표면연결과공통owner를실행한다.
    std::unique_ptr<o::RawIslandPostPop> surface_; //현재깊이/저장게임과동일받침생성접두다.
    std::unique_ptr<o::SquidFactory> factory_; //공통장부/Pool보다먼저반납하고장기Host가마지막Regular를반납할때까지실제공장을유지한다.
    o::CarrierPostPopState resourceCarrierState_; //현재원문권한/로딩/간헐천TYPE주소만빌리며지역기본값으로복제하지않는다.
    o::AssetPostPopState resourceAssetState_; //장기Geyser경로가소비하는실제목록/표면TYPE주소이며다른자산용사본은사용하지않는다.
    std::unique_ptr<o::RawCarrierPostPop> resourceCarrier_; //같은공통PostPop을직접호출하는장기실제부모다.
    std::unique_ptr<o::RawAssetPostPop> resourceGeyser_; //Geyser전용본문만공유하며다른자산의가상등록을합성하지않는다.
    std::unique_ptr<o::RawNuggetRegular> resourceNuggetRegular_; //장기기본Host의실제왕복/낙하본문이며현재Sessionoverride를보존한다.
    o::SquidRewardState rewardState_; // Session 이후에도 같은 현재 장기 모드/TYPE/비율을 읽는다.
    std::unique_ptr<o::SquidReward> reward_; // Pool/TYPE/장부보다 먼저 반납하며 지역 Session callback을 저장하지 않는다.
    std::unique_ptr<NativeFortSaveVirtual> fortSaveVirtual_; //같은 Storage/GameAux를 빌리는 실제 조회기이며 해당 소유자보다 먼저 반납한다.
    std::unique_ptr<o::RawNuggetPostPop> resourceNugget_; //같은Host/난수/확보메모리의너겟후처리다.
    std::unique_ptr<o::RegularProcess> resourceRegular_; //할당과생성사이동일new28의소유권만보존한다.
    std::unique_ptr<o::RawSquidDestroy> baseDestroy_; //같은Native공통pre/post깊이를World반납뒤에도보존한다.
    std::unique_ptr<o::SquidFormDispatch> processForms_; //장기Host와같은Pool의실제ProcessForm경로이며UI/내용물가짜처리기는등록하지않는다.
    std::unique_ptr<o::RawSquidTransmission> transmission_; //같은Pool/현재TYPE/GameAux보다먼저반납하는공통실제송신기다.
    std::unique_ptr<RawProcessHostStorage> processes_; // 실제 Pool/TYPE 뒤에 선언하여 그 참조보다 먼저 최종 반납한다.
    const GameAssets* source_;bool borrowed_{};std::uint64_t generation_{}; // 최초 자산 정체성·단일 대여·호스트 관측 번호다.
};
// 실제 보관 중인 자산에서 프레임/SHP를 한 번 구성한다. 게임이나 창은 실행하지 않는다.
std::shared_ptr<RawSurfaceStorage> MakeRawSurfaceStorage(const GameAssets& assets,std::shared_ptr<o::RawAuthorityState> authority={},std::shared_ptr<o::RawGameAuxState> gameAux={});
// 저장/지형 어댑터에서 넘기는 초기 위치다. 등록 뒤 현재 값은 raw 슬롯만 읽는다.
struct SurfaceSeed { /* type: 타입 표에서 객체 종류를 찾는 번호. */ /* owner: 객체나 지역을 소유한 플레이어 번호. */ std::uint32_t type{},owner{};/* frame: 타입에서 사용할 그림 프레임 번호. */ std::int32_t frame{};/* x: X 좌표. */ /* y: Y 좌표. */ float x{},y{}; };
// 표면 객체의 현재 SID와 직렬화할 배치 정보이다.
struct SurfaceSnapshot { /* sid: 원본 풀에서 객체를 찾는 SID 번호. */ o::Sid sid{};/* object: 해당 SID나 출력 항목에 대응하는 객체 자료. */ SurfaceSeed object; };
// 해상도 변경으로 Renderer가 교체되어도 현재 장치에 전달할 수 있는 표시 콜백이다.
struct SurfaceDisplayHooks {
    // suppressed: 현재 표시나 후처리를 억제하는 전역 상태를 조회하는 콜백.
    std::function<bool()> suppressed;
    // invalidate: 변경한 영역의 재그리기를 요청하는 콜백.
    std::function<void(o::SquidDisplayRect,std::uint32_t)> invalidate;
};
// 비표면 runtime이 참조하는 실제 월드 자료다. span은 원본 배열이며 풀/해시/장부의 사본을 만들지 않는다.
// 이 참조는 월드보다 오래 살 수 없고 연결 모듈 해제 전 ClearProcesses를 호출해야 한다.
struct RawWorldCore {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    o::SidPool& pool;
    // hash: 좌표별 객체 후보를 찾는 공간 해시.
    o::SquidHash& hash;
    // spots: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<std::uint8_t> spots;
    // types: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const o::RiftTypeRecord> types;
    // frames: 타입별 프레임 자료.
    std::span<const o::RiftTypeFrames> frames;
    // shapes: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료.
    std::span<const o::SquidDisplayShape> shapes;
    // bookkeeping: 객체 종류·소유자별 등록 목록과 개수를 관리하는 장부.
    o::SquidPostPopState& bookkeeping;
    // ownerMode: 판본·서버·로딩 상태에 따른 소유권 처리 정책.
    o::SquidOwnerMode& ownerMode;
    // factory: 타입에 맞는 객체를 생성하는 처리기.
    o::SquidFactory& factory;
    // owner: 소유 플레이어 변경을 객체 필드와 장부에 반영하는 처리기.
    o::SquidOwner& owner;
    // frame: 프레임 변경을 물리 프레임·통지·갱신 요청에 반영하는 처리기.
    o::SquidFrame& frame;
    // pop: 객체를 공간 지도에 등록하는 Pop 처리기.
    o::SquidPop& pop;
    // unpop: 객체의 공간 등록을 해제하는 Unpop 처리기.
    o::SquidUnpop& unpop;
    // postPop: 객체 Pop 이후 목록·공간 후처리를 수행하는 처리기.
    o::SquidPostPop& postPop;
    // display: 객체·프레임에서 실제 화면 출력 요청을 만드는 처리기.
    o::SquidDisplay& display;
    // building: 일반 건물의 공간 등록·해제를 반영하는 처리기 또는 상태.
    o::RawBuildingSpace& building;
    // graph: 서로 연결된 표면의 Graph 소속을 관리하는 처리기.
    o::RawGraph& graph;
    // neighbors: 발자국에 인접한 표면·객체를 조회하는 검색기.
    o::RawSquidNeighbors& neighbors;
    // destroy: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기.
    o::RawSquidDestroy& destroy;
    // virtualDestroy: 타입별 가상 삭제 경로를 선택하는 분배기.
    o::RawSquidDestroyDispatch& virtualDestroy;
    // surfaces: 표면 객체의 공간·연결·수명 처리 자료.
    o::RawSurfaceLifecycle& surfaces;
    // bridgeEvents: 다리의 연결·붕괴 변화에 따른 이벤트 처리기.
    o::RawBridgeEvents& bridgeEvents;
    // forms: 객체에 부착된 Form의 생성·통지·수명 처리기.
    o::SquidFormDispatch& forms;
    // dependencies: 같은 풀/삭제 깊이/ProcessForm 통지를 사용하는 실제 감시 Form 쌍이다.
    o::DependFormHost& dependencies;
    // processes: 공유 커널에 연결한 객체 프로세스 호스트.
    o::SquidProcessHost& processes;
    // processState: 프로세스 실행·부착·이벤트 분배에 사용하는 상태.
    o::SquidProcessState& processState;
    // kernel: 프로세스 등록과 프레임 실행을 담당하는 커널.
    o::Kernel& kernel;
    // bridgeState: 같은 월드의 실제 표면 소유자 분배·연결이 읽는 현재 mutable 전역이다.
    o::BridgeConnectState& bridgeState;
};
// 표면의 기존 훅을 합성한 미션 runtime 경계다. 새 월드 생성 후 첫 객체 생성/Load 전에 한 번 연결한다.
struct RawWorldRuntimeHooks {
    // assets: 타입·셰이프·팔레트 자산 또는 해당 자산의 처리 경계.
    o::SquidDestroyHooks assets;
    // destroy: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기.
    o::SquidAssetDestroy destroy;
    // regular: 주기 동작을 수행하는 Regular 프로세스 연결.
    o::RegularHandler regular;
    // transmit: Form 데이터 작성과 송신을 연결하는 전송 경계.
    o::ProcessFormTransmission transmit;
    // walkerFall: 지정 이동 유닛의 추락 처리를 실행하는 콜백.
    std::function<bool(o::Sid)> walkerFall;
    // loadingDepth: 현재 로딩 중첩 깊이를 외부 상태에 전달하는 콜백.
    std::function<void(std::uint32_t)> loadingDepth;
    // formTransmit: 실제 DependForm 서버 Pop의 현재 raw Form/flags 전송 요청 경계다.
    o::FormPopTransmit formTransmit;
    o::BattlePacketSend squidPacket; //일반Squid writer가완성한현재패킷을같은실제방송분배기로보낸다.
    std::function<bool(std::int32_t)> squidConnected; // 원문 Player+50 bit1의 대상별 접속 관문이다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> squidSend; // 같은 현재 대상의 연결 객체로만 패킷을 보낸다.
    o::DependFormNotice dependencyNotice; // watcher의 실제 가상+34다. 누락 시 기존 ProcessForm 통지를 사용한다.
    std::function<void(const o::BridgeLifecycleEvent&)> bridgeEffect; // 실제 세션의 다리 파편/위치 소리 소비자이며 runtime 설치 전에 필수로 공급한다.
};
// 같은 SID 풀·공간 지도에서 표면 객체의 생성·등록·수명·표시를 관리하는 월드이다.
class RawSurfaceWorld {
public:
    // 프레임/타입/SHP를 소유하여 GUI 참조와 raw 조회가 같은 판본을 사용하게 한다.
    RawSurfaceWorld(o::OriginalEdition edition,std::span<const o::RiftTypeRecord> types,
        std::span<const o::RiftTypeFrames> frames,std::span<const o::SquidDisplayShape> shapes,
        o::BridgeConnectState links,std::uint32_t terrainType,o::Kernel* processKernel=nullptr,std::shared_ptr<RawSurfaceStorage> storage={},bool adoptPrepared=false,bool* sharedSavedGame=nullptr,std::shared_ptr<o::RawAuthorityState> authority={},std::shared_ptr<o::RawGameAuxState> gameAux={},std::shared_ptr<o::FortSerializationGlobals> serialization={}); // 준비 대여는 실제 물리를 사용하며 저장 게임 관문은 첫 객체 전부터 Client의 같은 bool을 빌린다.
    // 실제 Client는 같은 전역 Kernel을 빌리고 독립 월드만 자체 Kernel을 소유한다. 빌린 Kernel은 월드보다 오래 살아야 한다.
    // 빌린 경우 호출자는 모듈 반납 전에 현재 대여의 최종 정리를 마쳐야 한다. 전체 Kernel 소멸은 장기 Storage가 소유한다.
    ~RawSurfaceWorld();
    RawSurfaceWorld(const RawSurfaceWorld&)=delete;
    RawSurfaceWorld& operator=(const RawSurfaceWorld&)=delete;
    // 본섬→행 우선 noIsland→다리 순으로 일반 Pop을 실행하고 마지막에 전체 Graph를 재구성한다.
    void Load(std::span<const SurfaceSeed> seeds);
    // 같은 풀에 실제 미션 모듈을 구성할 때 사용한다. 반환 참조의 수명은 이 월드와 같다.
    RawWorldCore Core();
    const std::shared_ptr<RawSurfaceStorage>& Storage() const; // 미니맵도 Session이 아닌 동일 물리 소유자를 직접 빌린다.
    void SynchronizePeer(std::uint32_t owner); //004a7350의 실제 같은 월드/전송/공간 재등록을 참가자별로 실행한다.
    void TransmitDeletion(o::Sid sid,std::uint32_t flags); // 공통 삭제의 -1/-2/0/flags를 같은 현재 가상 writer·버퍼·방송 부모로 전달한다.
    //원본옵션의TYPE+c4 쓰기가같은월드의188TYPE를직접사용하게한다. 새사본을만들지않는다.
    std::span<o::RiftTypeRecord> MutableTypes();
    // 최초객체/Runtime등록전에원본모드의실제소유권·연결색·다리붕괴소비자를함께설정한다.
    void ConfigureInstallModes(bool fort,bool battle,bool challenge,bool editor);
    // 미리보기도 같은 실제 연결 생성기와 현재 표면 전역을 사용한다.
    o::RawBridgeConnect& BridgeConnections();
    // 같은 저장소를 사용하는 자산/Regular/Form 전송/낙하/로딩 경계를 설치한다. 미연결 콜백을 생략하지 않는다.
    void BindRuntime(RawWorldRuntimeHooks hooks);
    void BindRuntimePin(std::function<std::shared_ptr<void>()> pin); //실제외곽owner공개뒤현재World대여들에같은pin장치를연결한다.빈함수는최종C++소멸전반납이다.
    void BindContainedUnpop(const std::shared_ptr<o::RawSquidContainedUnpop>& binding); // 같은 풀의 Session 종단을 약한 참조로 빌린다. 반납/소멸 뒤에는 월드 기본 종단을 사용한다.
    // 첫 객체 생성 전에 공통 삭제의 관찰 경계를 실제 보상/현재 선택/선택해제·소리 소비자로 한 번 교체한다.
    void BindDeletion(o::SquidReward& reward,std::function<o::Sid()> selected,o::SquidDeletionHooks hooks);
    // 후설치와 같은 현재 원본 삭제 전역이다. 별도 세션 사본을 만들지 않는다.
    o::SquidDeletionState& DeletionState();
    // 현재 후설치된 실제 공통 삭제의 검사/Pre/Post를 파생 자산과 공유한다.
    o::SquidDestroyLifecycle& CommonDeletion();
    // 원본 Fort 순서대로 Create/필드 저장/Pop하는 로더의 시작과 종료다. 종료 시 Graph를 재구성한다.
    void BeginLoad();
    void FinishLoad();
    // 실제 캠페인 부모가 Graph(false)를 이미 실행한 뒤 로딩 상태만 종료한다. Graph/풀/해시를 재구성하지 않는다.
    void FinishLoadAfterGraph();
    // 로더와 실제 명령이 사용하는 같은 가상 생성·SHP 공간 등록·해제·재등록이다.
    o::Sid Create(std::uint32_t type,std::uint32_t flags=0);
    void Pop(o::Sid sid,float x,float y,std::uint32_t flags);
    void Unpop(o::Sid sid,std::uint32_t flags);
    void Repop(o::Sid sid,std::uint32_t flags);
    void AttachBaseAsset(o::Sid sid,o::Sid parent,std::uint32_t flags); //004ad330의 진단 복귀/좌표/내용물 UI→공통 부착을 같은 World에서 실행한다.
    // 외부 연결 모듈을 해제하기 전에 예약 객체를 먼저 정리한다. 월드 자료는 유지한다.
    void ClearProcesses();
    RawProcessHostStorage::ClearPass ClearProcessesForState(); // 실제 단일 Clear 정상 복귀의 증빙을 발급한다.
    void ReturnProcessBindingsForState(RawProcessHostStorage::ClearPass&& pass); // 같은 증빙으로 현재 처리기 대여만 끝내며 Regular 잔류는 유지한다.
    bool ProcessBindingsReturned() const; // 실제 소비된 Returned 증빙의 존재를 조회하며 공개 성공 setter는 제공하지 않는다.
    void DrainProcesses(); // 명시 전체 Kernel의 최종 소멸 전용이며 원문 Clear 한 번과 구별한다.
    void DrainCurrentProcessBindingsForShutdown(); // 현재 대여의 일반 종료만 반납하며 이전 Native 잔류와 다른 Storage는 건드리지 않는다.
    // 파생 로더의 중첩 깊이를 표면과 비표면 몸체에 동일하게 전달한다.
    void SetLoadingDepth(std::uint32_t depth);
    // 원본 고정 게임 시각을 모든 Regular에 공급한다. 정지 중에는 커널을 실행하지 않는다.
    void RunFrame(o::FrameTime time,bool paused);
    void PrepareFrame(o::FrameTime time); // 같은 시각만 공급한다. 실제 Client는 이후 전역 Kernel을 정확히 한 번 실행한다.
    void BindBridgeDecay(const std::uint32_t& editor,std::function<void(o::Sid)> carriers,std::function<void(o::Sid)> sound,std::function<void(o::FrameTime)> time={}); //현재 Editor/대피/소리/원문 현재 시각을 첫 Frame 전에 연결한다.
    void DecayBridgeCell(o::FrameTime time,o::Sid sid); //같은 현재 시각에서 실제 수명/Graph/가상 삭제/표시를 실행한다.
    void ScanBridgeDecay(o::FrameTime time,bool paused); //독립 World도 같은 현재 스캔 저장소와 정지/Editor/권한 관문을 사용한다.
    // 카메라/viewport와 현재 Renderer의 표시 전달 경로를 갱신한다.
    void SetDisplay(o::SquidDisplayView view,SurfaceDisplayHooks hooks);
    void BindMetaRefresh(std::function<void(std::span<const std::uint8_t>)> refresh); //현재 World의 실제 메타88/8c 장치를 설치하며 일반 view 교체로 반납하지 않는다.
    // 지정 위치의 실제 객체/받침을 조회한다. 없으면 SID 0이다.
    o::Sid Find(float x,float y,std::uint32_t type) const;
    // vtable +0x10의 삭제 재정의를 거쳐 연결 객체·장부·프로세스를 함께 정리한다.
    bool Destroy(o::Sid sid,std::uint32_t flags=0);
    // 저장 건물의 소유자를 받침에 반영하는 기존 초기 월드 어댑터의 경계다.
    void SetSupportOwner(float x,float y,std::uint32_t owner);
    // 사제/신전의 전용 몸체를 상위에서 선택한 뒤 나머지 자산 +74를 현재 표로 호출한다.
    void SetAssetOwner(o::Sid sid,std::uint32_t owner);
    // 표시/검사에 현재 살아 있는 자산만 제공한다. ProcessForm은 제외한다.
    std::vector<SurfaceSnapshot> Objects() const;
    // 변경 요청은 한 번만 소비한다. 원본 표시가 전체 갱신으로 억제되어도 논리 변경은 남긴다.
    bool TakeChanged();
    // 깊이/Graph/프로세스/공간의 실제 상태를 읽기 전용으로 노출한다.
    std::string Report() const;
private:
    struct Impl;
    // impl_: 공개 인터페이스 뒤의 구현 상태와 연결 객체.
    std::unique_ptr<Impl> impl_;
};
}
