// 검사 관찰 콜백을 사용하지 않고 현재 클라이언트의 실제 서비스를 빌린다.
#include "client/CampaignClientServices.h"
#include "client/CurrentAllocationBytes.h"
#include "client/SoloMapCatalog.h"
#include "client/RawCampaignMissionStartBindings.h"
#include "client/GameWorld.h"
#include "client/UberGump.h"
#include "client/ClientAudio.h"
#include "client/ClientMain.h"
#include "client/ClientChatRuntime.h"
#include "client/ClientStaticStrings.h"
#include "client/ClientSpGlobals.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawChatChannels.h"
#include "client/ScriptCommand.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <array>
#include <bit>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <type_traits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX //Win32의max매크로가표준numeric_limits와충돌하지않게한다.
#endif
#include <windows.h>
#endif

namespace netstorm::client {
namespace {
// 초기화하지않은현재caller8BYTE를자기프로세스읽기로복사하고C++미정값연산을피한다.
void CurrentBridgeScratchBytes(std::span<std::uint8_t> bytes) {
    if (bytes.size()!=8) throw std::invalid_argument("AI 검색 caller 입력8BYTE 크기 오류");
#ifdef _WIN32
    std::array<std::uint8_t,8> current;SIZE_T read{};
    if (!ReadProcessMemory(GetCurrentProcess(),current.data(),bytes.data(),bytes.size(),&read) || read!=bytes.size()) throw std::runtime_error("AI 검색 현재 caller8BYTE 조회 실패");
#else
    throw std::logic_error("AI 검색 현재 caller8BYTE의 Windows 경계 미설치");
#endif
}
}
// 같은 Client의 실제 타입 수/언어/미정 할당을 파일 목록에 제공한다.
SoloMapCatalogEffects MakeSoloMapCatalogClientEffects(Client& client,std::function<std::string()> currentName) {
    if (!currentName) throw std::invalid_argument("맵 목록의 실제 현재 사용자 이름 미연결");
    SoloMapCatalogEffects effects;effects.typeCount=[&client] { return static_cast<std::int32_t>(client.Assets().TypeTable().Types().size()); };
    effects.allocationBytes=CurrentAllocationBytes;effects.userName=std::move(currentName);
    effects.translate=[&client](std::string_view source) { const auto bytes=o::EncodeOriginalText(client.Translate(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(source.data()),source.size()})));return std::string(bytes.begin(),bytes.end()); };return effects;
}
// 실제 힙 입력과 게임 시계/화면/카메라를 연결한다. 선택된 미복원 세계 생성은 명시 거부한다.
CampaignMissionStartEffects MakeCampaignMissionStartEffects(Client& client,GameWorld& world,std::function<void()> refreshCommandGump) {
    if (!refreshCommandGump) throw std::invalid_argument("미션 Start의 현재 명령 Gump 조회 미연결");
    CampaignMissionStartEffects effects;
    effects.settingsBytes=[](std::uint32_t,std::span<std::uint8_t> bytes) { CurrentAllocationBytes(bytes); };
    effects.gameSeconds=[&client] { return client.Time().game; };
    effects.refreshSp=[&client] { client.GetRenderer().InvalidateAll();if (client.Menu()) client.Menu()->Refresh(); };
    effects.refreshCommandGump=std::move(refreshCommandGump);
    effects.randomGeysers=[] { throw std::logic_error("미션 Start의00486440 세계 랜덤 간헐천 생성 미연결"); };
    effects.resetCamera=[&world] { world.ResetCampaignCamera(); };
    effects.unknownAbilityAssertion=[](std::string_view name) { std::clog<<"미션 AI의 원본 능력 이름 진단: "<<name<<'\n'; };
    effects.unknownAbilityLog=[](std::string_view name) { std::clog<<"미등록 미션 AI 능력: "<<name<<'\n'; };
    return effects;
}
// ID의같은DWORD를실제ReadInt에넘기며시간/등록설정은도달할때만읽는다.화면/음향을요구하지않는다.
CampaignSubscriberServices MakeCampaignSubscriberClientServices(Client& client) {
    static_assert(sizeof(std::uint32_t)==4 && sizeof(std::int32_t)==4 && alignof(std::uint32_t)==alignof(std::int32_t)); //원문DWORD폭/대응형의정렬을확인한다.
    static_assert(std::is_same_v<std::make_signed_t<std::uint32_t>,std::int32_t> && std::is_same_v<std::make_unsigned_t<std::int32_t>,std::uint32_t>); //서로대응하는signed/unsigned형에만허용되는같은객체alias다.
    CampaignSubscriberServices result; //이서비스가빌린Client는전체Session보다오래살아야한다.
    result.readRegistered=[&client](auto key,std::uint32_t& value) { return client.Configuration().ReadInt(key,reinterpret_cast<std::int32_t&>(value)); }; //사본없이기존ReadInt가Config.Get복귀뒤현재ID를다시비교한다.
    result.configure=[&client](auto expression) { static_cast<void>(ApplyScriptConfig(expression,client.Configuration())); }; //00441580의기존토큰/Config.Set/변경callback을그대로재사용한다.
    result.clock=[] { return static_cast<std::uint32_t>(std::clock()); }; //GameClock/벽시각이아닌실제CRT clock경계다.
    result.assertion=[](auto line) { std::clog<<"Subscriber 원본 진단 줄: "<<static_cast<std::uint32_t>(line)<<'\n'; };return result;
}
// Player SetAlliance의 실제 채널 수정과004410f0(defaultAiName)의 현재 원본 바이트를 같은 Client에 연결한다.
void BindCampaignAiClientServices(Client& client,CampaignAiServices& ai) {
    ai.names.language=[&client] { return static_cast<std::uint32_t>(client.LanguageNumber()); }; //원본현재언어0/1의이름번역생략관문을공유한다.
    ai.names.translate=[&client](std::string_view source) { const auto bytes=o::EncodeOriginalText(client.Translate(source));return std::string(bytes.begin(),bytes.end()); };
    ai.names.substitute=[&client](std::string_view source) { const auto text=client.Configuration().SubstituteLocal(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(source.data()),source.size()}));const auto bytes=o::EncodeOriginalText(text);return std::string(bytes.begin(),bytes.end()); }; //같은현재설정치환과원본좁은문자열을사용한다.
    // 원본 Clear는 채널 BYTE만 지우고 Add는 현재 자기 subscriber의 자연 -1 반환까지 수행한다.
    ai.participation.clearChat=[&client](std::uint32_t channel) { client.ChatChannels().Clear(channel); };
    ai.participation.addChat=[&client](std::uint32_t subscriber,std::string_view name,std::uint32_t channel) {
        static_cast<void>(client.ChatChannels().AddToChannel(subscriber,name,channel));
    };
    // Xlat 키로 바꾸지 않는다. 현재 언어 Config/치환 결과를 매 요청마다 원본 Windows-1252로 인코딩한다.
    ai.defaultName=[&client] {
        const auto bytes=o::EncodeOriginalText(client.Configuration().PathSpec("defaultAiName")); // 현재 언어 교체/설정 변경을 반영한 실제 좁은 바이트다.
        return std::string(bytes.begin(),bytes.end());
    };
}
// 같은 번역 원문은 다음 번역 조회까지 유지하며 서식 결과는 Client 공유 고리에 둔다.
o::EditorPlayersHooks MakeEditorPlayerClientHooks(Client& client) {
    o::EditorPlayersHooks hooks;auto translated=std::make_shared<std::vector<std::uint8_t>>(); // 첫 Human 복사가 끝난 뒤에만 다음 번역이 같은 버퍼를 재사용한다.
    hooks.translate=[&client,translated](auto key)->std::span<const std::uint8_t> { *translated=o::EncodeOriginalText(client.Translate(key));translated->push_back(0);return *translated; };
    hooks.format=[owner=client.StaticStrings()](auto reservation,auto pattern,auto value) { return owner->FormatInteger(reservation,pattern,value); }; // Source와 output의 별도 Native 수명을 유지한다.
    return hooks;
}
// 원본 time32/번역/설정/음향/입력을 동일 클라이언트의 저장소와 장치에 연결한다.
RawCampaignSessionServices MakeCampaignClientServices(Client& client,RawButtonControls& controls,CampaignClientUi ui) {
    if (!ui.refreshWindowTitle || !ui.tell || !ui.removeMessage)
        throw std::invalid_argument("실제 캠페인 클라이언트 UI 경계 누락");
    auto& audio=client.WorldAudio(); // noAudio도 같은 실제 이름표/비활성 재생 계층을 사용한다.
    auto& player=audio.Sounds(); // 반환한 콜백이 빌리는 장기 실제 SoundPlayer다.
    RawCampaignSessionServices services; // 게임 규칙 경계는 실제 세션의 명시 누락 판정을 유지한다.
    services.subscriberGlobals=client.SubscriberGlobals(); // Init/Chat/모든 Session이 같은 두 DWORD를 수명째 빌린다.
    services.playerRecords=client.PlayerRecords(); // 실제 Player/factory를 Client 수명에 소유하고 Session 대여 중 재할당하지 않는다.
    services.spGlobals=client.SpGlobals(); // 공통004391ed에서 구성한 같은 SP/추적 전역을 재초기화 없이 빌린다.
    services.coreDataGlobals=client.CoreDataGlobals(); // 원문 Core 선택/캐시·Login 패킷·Chat이 같은 장기 슬롯을 직접 사용한다.
    services.gameplayGlobals=client.GameplayGlobals(); // 초기 HUD와 같은 실제 현재 Player/커서/검색/미션 전역이다.
    services.kencloudGlobals=client.KencloudGlobals(); // 섬/지역 도장이 같은 실제0059a8b0에 월드 다시 그리기를 요청한다.
    services.processKernel=&client.GetKernel(); // 모든 실제 Form/경로/Regular와 번개가 같은 원문 전역 Kernel을 사용한다.
    services.surfaceStorage=client.SurfaceStorage(); // 현재 Core의 Pool/해시/TYPE/spot을 같은 실제 Client 물리에 연결한다.
    services.surfaceStorage->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner()); // 이미 pin한 같은 실제 장치만 확인하며 현재 시각을 복사하지 않는다.
    services.colorSortInput=client.ColorSortInput(); // 실제 Client의 같은 고정 초기입력 소유자를 반드시 전달한다.
    services.screenPalette=[&client] { return client.GetScreen().Palette(); }; // Screen 재생성 뒤에도 그때의 현재 팔레트를 조회한다.
    services.randomGlobals=client.RandomGlobals(); // Session교체/Subscriber/파편/Seed/지형이같은Client의실제CRT를사용한다.
    services.localStateGlobals=client.LocalStateGlobals(); // 메뉴/Editor에서 사용한 같은 State와00544d58 기한을 로그인도 직접 빌린다.
    services.connectionActivityGlobals=client.ConnectionActivityGlobals(); // Login/Editor가 같은 실제22프레임/22벽시각과 Zone 간극을 빌린다.
    services.connectionFrameNumber=[&client] { return static_cast<std::uint32_t>(client.Time().number); }; // 프레임을 생성 시 캡처하지 않고 실제Receive의 원문 시점에 하위DWORD를 읽는다.
    services.removeConnectionStatus=[remove=ui.removeMessage](std::uint32_t kind,std::uint32_t player) { remove(static_cast<std::int32_t>(kind),static_cast<std::int32_t>(player),0); }; // 아래 섬 warningEvent의 move 전에 실제 WarningMessages.Remove 장치를 소유 복사한다.
    services.fortTechnologyUi=[&client] { RefreshClientChatRankPrefix(client,client.Assets(),*client.SurfaceStorage()); }; // 같은 실제 Chat/Core/아이콘을 기존 Fort Technology 부모의0042f540 위치에서 갱신한다.
    services.editorPlayerUi=MakeEditorPlayerClientHooks(client); // 실제 기존 Editor 설치가 요구할 번역/공유 고리 장치다.
    services.templateStateBytes=CurrentAllocationBytes; // 원본이 쓰지 않는 State 바이트를 임의0 대신 현재 호스트 할당 입력에서 받는다.
    // 실제 커서 비용과 같은 Gump 트리/게임 시계에 Cast/Drop 금액 표시를 붙인다.
    services.carrierDropPopup=[&controls](std::int32_t cost,std::int32_t x,std::int32_t y,std::uint32_t reference) { auto* menus=controls.Menus();if (!menus) throw std::logic_error("Cast/Drop 비용표시 실제Gump미설치");menus->ShowSp(cost,x,y,reference); };
    services.ai.allocationBytes=CurrentAllocationBytes; // 실제 AI 생성자가 보존하는 바이트의 현재 힙 입력이다.
    services.ai.bridgeScratchBytes=CurrentBridgeScratchBytes; //검색의실제caller초기8BYTE는힙할당입력과별도로공급한다.
    services.ai.blockingGumpOpen=[&controls] { return controls.BlockingGumpOpen(); }; // 데모 로컬 준비가 같은 실제 Gump 직계 목록을 읽는다.
    services.ai.assertion=[](std::string_view text) { std::clog<<"AI 원본 진단: "<<text<<'\n'; }; // 원본 진단 후 계속하는 경계를 기존 장치 로그로 전달한다.
    services.production.allocationBytes=CurrentAllocationBytes; // Deck 생성자도 초기화하는 5DWORD 이외의 입력을 보존한다.
    services.production.effects.configInt=[&client](std::string_view key) {
        std::int32_t value{};client.Configuration().ReadInt(key,value);return value;
    };
    services.production.effects.modeAssertion=[] { std::clog<<"생산 재계산 원본 진단: inFortMode || inBattleMode\n"; };
    // 원본 _time32의 지원 범위만 허용한다. 날짜를 DWORD로 감아 다른 난수 시드로 바꾸지 않는다.
    services.unixSeconds=ClientCurrentUnixSeconds; // 공통 시작과 같은 실제 time32 장치를 매 원문 선택 위치에서 호출한다.
    services.subscriber=MakeCampaignSubscriberClientServices(client); //실제Subscriber의같은등록Config/clock/진단을빌린다.정상ID는선택하지않는다.
    auto you=std::make_shared<std::vector<std::uint8_t>>(); // 원본 부모가 NUL까지 복사하는 동안 유지할 번역 바이트다.
    // 번역은 요청 시 다시 조회한다. 설치 사이에 바뀐 언어를 이전 문자열로 고정하지 않는다.
    services.install.translateYou=[&client,you]() -> std::span<const std::uint8_t> {
        *you=o::EncodeOriginalText(client.Translate("You"));you->push_back(0);return *you;
    };
    // 원본 strcpy 입력의 첫 NUL까지만 UTF-8 설정 저장소에 넣고 뒤의 물리 바이트는 무시한다.
    services.install.setConfig=[&client](std::string_view key,std::span<const std::uint8_t> bytes) {
        const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{}); // 좁은 문자열의 실제 종료 위치다.
        if (end==bytes.end()) throw std::invalid_argument("캠페인 설정 문자열 NUL 누락");
        client.Configuration().Set(key,o::DecodeOriginalText(bytes.first(static_cast<std::size_t>(end-bytes.begin()))));
    };
    services.install.refreshWindowTitle=std::move(ui.refreshWindowTitle); // 월드 설치 중 현재 미션 제목도 갱신한다.
    // 원본 0047d890의 사건을 같은 실제 Gump 트리로 분배한다. 생산 노드 자체의 구현은 별도다.
    services.ownership.refreshProduction=[&controls] { controls.RefreshProduction(); };
    services.assets.postPop.refreshProduction=services.ownership.refreshProduction; // 신전 PostPop도 동일한 사건을 보낸다.
    // Player 부모 통지는 현재 등록표와 대상 가상+28을 직접 사용한다.
    services.playerContent.gump=[&controls](std::uint32_t id) { return controls.RegisteredGump(id); };
    services.gumpNotice=controls.ObjectNotice(); // 같은등록표와기본/메뉴의가상+4가부모삭제를지연Kill로받는다.
    services.playerContent.handle=[&controls](std::uint32_t token,std::uint32_t event,std::uint32_t form,std::uint32_t reason) {
        static_cast<void>(controls.HandleGump(token,event,form,reason));
    };
    // 원본의 철자가 theammode이며 각 요청마다 기본값 1에서 현재 설정을 읽는다.
    services.ownership.themeMode=[&client] {
        std::int32_t value=1; // 설정 누락 시 원본이 유지하는 기본값이다.
        client.Configuration().ReadInt("theammode",value);return std::bit_cast<std::uint32_t>(value);
    };
    services.requestMusic=[&audio](std::string_view name) { audio.RequestMusic(name); }; // 희생 wrapper도 같은 실제 장면 음악 상태를 사용한다.
    services.soundNames=&audio.Names(); // TYPE 소리와 보호막 프로세스가 공유하는 유일한 이름표다.
    // 정지와 무관한 실제 벽시계 및 실제 장치의 재생·정지·재생 여부를 사용한다.
    services.sound=MakeSoundProcessHooks(player,[&client] { return client.WallSeconds(); });
    // 원본 공통 preDestroy의 전역 재생 인자(반복0/음량0/좌우0/우선1/한도0)를 유지한다.
    services.unitLost=[&player](o::Sid) { player.PlayByName("unitLost.wav",0,0,0,1,0); };
    // 내용물 너겟 삭제는 같은 실제 입력 큐와 위치 효과음을 사용한다.
    services.contentLifetime.localNotice=[&client](std::uint32_t command) { client.Input().PushCommand(command,0,0); };
    services.contentLifetime.sound=[&player](float x,float y) { player.PlayNameAt(x,y,"guy.wav",0); };
    services.priest.shield=MakePriestShieldNoticeHooks(player,{}); // 원본 안내 소리의 우선 인자 1을 유지한다.
    // 00493d30은 004a89c0으로 인자 없는 Tell 키를 포맷한다. Xlat으로 섹션 이름을 바꾸지 않는다.
    services.priest.shield.translate=[](std::string_view key) { return std::string(key); };
    services.factoryTell=ui.tell; //Factory 자금/진행 중 안내도같은실제Tell설정/창/게임시계를사용한다.
    services.priest.shield.tell=std::move(ui.tell); // 실제 Tell이 안내 창과 게임 시계 정지를 처리한다.
    services.refreshSp=[&client] { client.GetRenderer().InvalidateAll();if (client.Menu()) client.Menu()->Refresh(); }; // 원본43dad0의 같은 실제 화면/SP 표시 갱신이며 별도 SP 사본을 만들지 않는다.
    services.hit.general.warningEvent=[remove=std::move(ui.removeMessage)] { remove(12,0,0); }; // 원본 섬 경고는 종류12 삭제 뒤 음향·Tell 순서다.
    services.priest.fallSound=MakePriestFallSound(player); // 사제 낙하의 현재 위치/파일명/반복 없음이다.
    // Vortex PostPop의 TYPE+74 전역 재생은 반복·음량·좌우·우선·한도가 모두 0이다.
    services.assets.postPop.playImpactSound=[&player](std::uint32_t sound) { player.Play(sound,0,0,0,0,0); };
    // 실제 총 명령을 원본 마스크/순서가 복원된 단일 입력 큐에 추가한다.
    services.assets.queueCommand=[&client](std::uint32_t command,std::int32_t first,std::int32_t second) {
        client.Input().PushCommand(command,first,second);
    };
    // Missile ctor/충돌과 Path Step은 실제 위치 한 번 재생에 반복 0·우선 0을 넘긴다.
    services.archer.missile.firingSound=services.archer.missile.impactSound=
        [&player](float x,float y,std::uint32_t sound) { player.PlayOnceAt(x,y,sound,0,0); };
    services.motion.path.stepSound=[&player](o::Sid,float x,float y,std::uint32_t sound) {
        player.PlayOnceAt(x,y,sound,0,0);
    };
    // 원본 Player 진단 그룹과 문자열을 실제 프로세스 로그 스트림에 순서대로 출력한다.
    services.player.log=[](std::uint32_t group,std::string_view text) { std::clog<<"["<<group<<"] "<<text; };
    // 원본 predictableGeysers의 기본값을 유지하고 호출 때마다 현재 Config를 다시 읽는다.
    services.construction.configInt=[&client](std::string_view key,std::int32_t fallback) {
        auto value=fallback;client.Configuration().ReadInt(key,value);return value;
    };
    services.construction.wallSeconds=[&client] { return client.WallSeconds(); }; // 비결정적 TimeSeed만 실제 벽시각을 읽는다.
    services.construction.refresh=[&client] { client.GetRenderer().InvalidateAll(); }; // 장치 재생성 뒤에도 현재 Renderer를 갱신한다.
    // 원본 진단 뒤 계속하는 계약을 보존하며 실제 로그 스트림에 진단을 남긴다.
    services.construction.negativeQuantity=[] { std::clog<<"로컬 간헐천 원본 진단: reps >= 0\n"; };
    services.construction.boundsAssertion=[](o::PlayerGeyserBoundsAssertion assertion) {
        constexpr std::array<std::string_view,5> descriptions{"참여하지 않은 Player","metamap PlayerId","전투 밖 배치","Player1..8 범위","Canon 프레임 부재"}; // 원본 진단 분류와 같은 순서의 실제 로그 설명이다.
        std::clog<<"Player 간헐천 범위 원본 진단: "<<descriptions.at(static_cast<std::size_t>(assertion))<<'\n';
    };
    BindCampaignAiClientServices(client,services.ai); // 장치 없는 검사와 실제 GUI 월드가 동일한 장기 Client 경계를 사용한다.
    CampaignLoginServices login; //실제Client의현재설정/시계/표시를원본로컬로그인에연결한다.
    login.configInt=[&client](std::string_view key,std::int32_t fallback) { client.Configuration().ReadInt(key,fallback);return fallback; };
    login.wallSeconds=[&client] { return client.WallSeconds(); };login.clock=[] { return static_cast<std::uint32_t>(std::clock()); };
    login.refresh=[&client] { client.GetRenderer().InvalidateAll(); };
    login.playerUiBusy=[&controls] { return controls.Menus() && controls.Menus()->HasMenus(); };
    login.playerNotice=[&client](std::uint32_t kind,std::uint32_t owner,std::uint32_t value) { std::clog<<"Player UI "<<kind<<' '<<owner<<' '<<value<<'\n';client.GetRenderer().InvalidateAll(); };
    login.log=[](std::string_view text) { std::clog<<text; };services.localLogin=std::move(login);
    return services;
}
}
