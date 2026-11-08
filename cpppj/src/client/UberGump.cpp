#include <cstdio> //원문좌표로그의실제%f서식을사용한다.
#include "client/RawProductionControls.h"
#include "client/RawCameraPan.h"
#include "client/MoveDialog.h" //HTML 명령의 같은 실제 모달 위치 처리를 사용한다.
#include "client/RawEdgeScroll.h"
#include "client/RawMinimapControls.h"
#include "client/CampaignUserInputServices.h"
#include "client/ClientCampaignTransport.h" // 같은 실제 World/장기 전역과 공유 전송 수명을 로그인 전에 연결한다.
#include "client/RitualChoiceInput.h" //의식선택은실제일반명령에서6b요청만작성한다.
#include "client/NativeMetaSail.h" //의식 패킷6b와 구별되는 일반 입력6b/6c/e3/fe의 원문 항해 소비자다.
#include "client/ClientMetaPointerInput.h" //GameWorld 없는 메타의 현재 마우스/픽셀 선택과 객체 입력 수명이다.
#include "client/ClientMetaBattleDescription.h" //HTML 문자열과 표식된 CS 주소의 서로 다른 현재 수명을 구별한다.
#include "client/NativeMetaConnectedState.h" //메타28~33의 실제 HTML serial/카메라 장치를 분배기에 공급한다.
#include "client/NativeMetaObjectInput.h" //영역 클릭/메뉴의 e8 확인 Tell도 같은 일반 입력에 분배한다.
// 원본 004d0120·004cebf0·004cf270·004ce580의 메뉴/명령 흐름을 표시 기반과 연결한다.
// 전체 Gump/StyleText와 게임 규칙은 후속이다. 저장 미션 월드는 GameWorld에 연결한다.
#include "client/UberGump.h"
#include "client/CurrentTypeHitPoints.h" // Help도 같은 Client의 현재 TYPE/전역/진단을 읽는다.
#include "client/CreateAtInput.h" //현재 문자열 고리와 실제 생성 Session을 공유한다.
#include "client/DataManagerFileRead.h" //명시 Native Template만 현재 FILE 전체 읽기를 사용한다.
#include "client/DrawWater.h" //후단 표시와 일반 명령이 같은 장기 상태를 공유한다.
#include "client/QuickHelpFileRead.h" //최초Help파일만같은Client의필수전체읽기를사용한다.
#include "client/HelpListDisk.h" //현재Client VFS/지역파일의 같은 Help목록writer다.
#include "client/ProductPresentation.h"
#include "client/ExternalHelpClient.h" //원래 외부 Help 정책을 현재 Client/Windows 장치에 연결한다.
#include "client/UrlInput.h"
#include "client/ClientMetaPlayerMenu.h" //World없는NSInfo도현재Client의전체Player메뉴장치를사용한다.
#include "client/ClientMetaSpriteRefresh.h" //실제 메타 World 생성 직후 같은 현재 Renderer/Gump의88/8c를 결합한다.
#include "client/ClientMetaSpriteDraw.h" //선택용 임시 pane도 실제 같은 CS/Image·TYPE·글꼴 출력을 사용한다.
#include "client/NativeBattleInfoMenus.h" //실제옵션선택문자열/정체성과메뉴장치를같은Client에연결한다.
#include "o/RawBattleOptionMutation.h" //메타제외목록도같은현재52f7e0전역옵션을조회한다.
#include "o/RawBattleOptionText.h" //현재선택getter와보호PE원문포인터를같은Xlat정체성에연결한다.
#include "client/CameraHome.h" //World없는LookAtSid도기존433ba0의현재Pool/TYPE와공통즉시카메라를사용한다.
#include "client/AiUtilityCommandInput.h" //772의실제세부모를같은일반입력에연결한다.
#include "o/RawAiCommandState.h" //World없는현재Player의NULL brain도같은raw부모가판정한다.
#include "client/MakeAiCommandInput.h" //현재모드와 실제Session의 일반MakeAi를 공유한다.
#include "client/UtilityCommandInput.h" //기존저장/별칭/Run/강조/SID보기부모를일반명령으로연결한다.
#include "client/CaptureInput.h"
#include "client/ClientCaptureSequence.h"
#include "client/RecheckCdInput.h"
#include "client/IntroMenu.h" // 원문 메인 Intro 표시값과 일반 입력의 실제 State39 요청이다.
#include "client/MainMenuActivity.h" //실제 메인 부모의 현재 모달·45초 자동 데모 장치다.
#include "client/ClientCdGlobals.h"
#include "client/TechnicalHelp.h" //실제 파일 가용성 조회와 별도 Technical Help 명령을 연결한다.
#include "client/ClientDiagnostics.h"
#include "client/ClientBuildReplay.h" // 일반 F7의 기존 UI/정지 관문 뒤 원문 재생 입력을 소비한다.
#include "client/SendMoneyInput.h"
#include "client/ClientBridgeDecay.h"
#include "client/DialogCommandLifetime.h"
#include "client/ThemeTechnologyCommand.h"
#include "client/ProductionShortcuts.h"
#include "client/RawMenuControls.h"
#include "client/RawMovieControls.h"
#include "client/RawHtmlWindowContext.h"
#include "client/HtmlZsend.h" //Zsend는원문세문자열을그대로같은현재11번송신기에보낸다.
#include "client/RawGameMenus.h"
#include "client/ScreenResolutions.h" // 메인/ESC가 같은 원본 확인 흐름과 호환 화면비 선택을 사용한다.
#include "client/GameMenuCommands.h"
#include "client/AiDebugCommandInput.h" //일반진단두명령은현재Player/장기DWORD를사용한다.
#include "client/PresentationCommandInput.h" //일반 글꼴/음질 입력은 현재 캐시와 음향 장치 부모로 전달한다.
#include "client/FontFaceEncoding.h" //변경 후 현재31BYTE ACP 이름만 실제 UTF-8 Config 경계에서 변환한다.
#include "client/ConfigurationCommandInput.h" //원문 모드별 palette와 공유 priority/State 일반 명령을 기존 부모에 연결한다.
#include "client/RendererDiagnostics.h" //명시특수전역뒤원문7/10/13키의현재DWORD만토글한다.
#include "client/LanguageRequest.h" //언어 요청/알림도 원문 설정/큐와 이미 수행한 Gump 전달을 공유한다.
#include "client/LanguageOptions.h" //6차: 옵션 메뉴 Language 하위 메뉴가 한국어 번호(kKoreanLanguageNumber)를 쓴다.
#include "client/GameMenuActions.h"
#include "client/ViewKeyInput.h"
#include "client/PauseKeyInput.h" //Pause 가상키도 같은 일반 Pause 명령 FIFO를 정지 관문 전에 사용한다.
#include "client/RotationKeyInput.h" //C/c는 World 없이도 같은 현재 Client 옵션을 반전하는 일반 입력이다.
#include "client/ConstructionHistoryKey.h" //Tab은 같은 최근 로컬 건설 위치를 현재 즉시 카메라로 순환한다.
#include "client/LostPositionKey.h" //U/u는 같은 공통 삭제 owner의 최근 손실 좌표를 현재 카메라에 전달한다.
#include "client/GameTimeDisplay.h" //같은장기CRT시계와T/t표시DWORD를연결한다.
#include <ctime> //원문CRT clock을조회하며게임시계와구별한다.
#include "client/SavedCameraKey.h" //숫자 호출/Shift 실제 기호 저장의 언어 표와 현재 지도 중심을 공유한다.
#include "client/GifScreenshot.h"
#include "client/RawUserHighlight.h" // Editor F8도 같은 Client 선택 전역의 원본 강조 몸체를 사용한다.
#include "client/TellParameters.h"
#include "client/TellSection.h"
#include "client/TellFileMenus.h"
#include "client/ClientAudio.h"
#include "client/ClientMain.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientHtmlWindowGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/CampaignClientServices.h"
#include "client/FortSubscriberClient.h" // 등록 메시지는 같은 장기 Template의 writer/Changed를 Session 없이도 사용한다.
#include "client/CampaignObjectMenuClient.h"
#include "client/RawObjectMenuOpen.h" //같은 실제 가상ac 반환값으로만 우클릭 열기음향을 판정한다.
#include "client/RawCampaignMissionStartBindings.h"
#include "client/RawCampaignTutorialOneBindings.h"
#include "client/MissionGuideInput.h"
#include "client/RawBattleLeave.h"
#include "client/MissionLifecycle.h"
#include "client/GuideMissionTransitions.h"
#include "client/CampaignGuideRelease.h" // 같은 실제 Tutorial/AI/Config 해제를 World·입력 소유권과 분리한다.
#include "client/MissionTechnology.h"
#include "o/RawMissionBattleOptions.h"
#include "o/RawGemReward.h"
#include "client/MissionFortRequest.h"
#include "client/CampaignTemplateStorage.h"
#include "client/TellMapMenus.h"
#include "client/FortSelection.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientCdGlobals.h"
#include "client/CdDirectory.h"
#include "client/ClientFortGlobals.h"
#include "client/FortCurrentNameDevices.h"
#include "client/EditorIslandWindow.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawKnowledgeMenu.h"
#include "o/RawMissionGuide.h"
#include "client/GifImage.h"
#include "client/GumpVisual.h"
#include "client/GumpBackground.h"
#include "client/GameWorld.h"
#include "client/RawCampaignCursor.h"
#include "client/RawCampaignCursorDisplay.h"
#include "client/ClientColorSortInput.h"
#include "client/OwnerColorTableState.h"
#include "client/CampaignCursorClientEffects.h"
#include "client/UserInput.h"
#include "client/WindowTitle.h"
#include "client/WarningMessages.h"
#include "client/HtmlConditionEffect.h"
#include "o/OriginalText.h"
#include "o/Template.h"
#include "client/RawNsInfoContext.h"
#include "client/RawUberRootControls.h"
#include "client/RawChatWindowContext.h"
#include "client/RawMonitorTime.h"
#include "client/RawBackgroundGifControls.h"
#include "client/RawTextEntryControls.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientChatRuntime.h"
#include "client/RawChatPacket.h"
#include "client/RawChatReceive.h"
#include "client/ScreenMouseKey.h" // 원문 Guide 앞 전체화면 F12의 즉시 장치 전환이다.
#include "client/GumpFormBinding.h" // 월드 부착 Gump도 같은 실제 Pool/현재 카메라를 투영한다.
#include "client/ClientDotCommandGlobals.h"
#include "client/ClientStaticStrings.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientSpGlobals.h"
#include "client/ClientStateWorldCleanup.h"
#include "client/ClientCursorStateCleanup.h"
#include "client/ClientKencloudGlobals.h"
#include "client/EditorFiles.h"
#include "client/EditorMapCatalogFiles.h" //저장 작업본의 현재 읽기와 원본 순서/새 이름 목록을 동일 Catalog에 제공한다.
#include "client/EditorFortFileDevices.h"
#include "client/EditorLocalTransport.h"
#include "client/EditorIslandDialog.h" // 실제 Add Island 명령의 release와 전용 창 생성 경계를 분리한다.
#include "client/ExistingEditorAfterCheckpoint.h"
#include "client/NativeBattleReturnState.h" // 일반 전투 복귀의 실제 결과 지역값/앞뒤 부모를 같은 Fort 설치에 연결한다.
#include "client/MultiplayerOptionsWindow.h" // 원문 일반 옵션 창을 같은 현재 Fort/이름과 입력 큐에 연결한다.
#include "client/LanBattleLobby.h" // 로컬 명단 합의와 실제 일반 State7의 요청 수명을 연결한다.
#include "client/ClientSpGlobals.h" // 대기실 Zone ID에 실제 time32 장치를 사용하며 게임 난수를 소비하지 않는다.
#include "client/NativeMainMenuState.h" //State3의 원문 부모와 호스트 표시 확정은 다른 시점이다.
#include "client/NativeIntroState.h" // 첫 설치의 영상 부재도 원문 39→40→41→3 순서를 사용한다.
#include "client/NativeMetaState.h"
#include "client/MetaConnectionRuntime.h"
#include "client/FortCoreDataLoad.h"
#include "client/ClientChallengeGlobals.h"
#include "client/NativeMetaDisplay.h" //메타 이탈도 같은 CS/Pool의 실제 삭제와 가변 배열 반납을 사용한다.
#include "client/RawCampaignFactoryDisplay.h" //메타의같은가상표/배율그림도기존공통출력기를사용한다.
#include "client/EditorSaveGoMain.h" //Yes/No의 실제 저장과 next3 예약을 일반 입력 뒤에 실행한다.
#include "client/EditorTestBattle.h" //c2의 snapshot/Guide 교체와 편집 복귀 순서를 실제 일반 입력에 연결한다.
#include "client/SaveFortNameDialog.h" //실제 TextEntry/두 버튼/전체 Fort 이름 부모를 사용하는 모달이다.
#include "client/TellTip.h" //같은 현재 팁 번호와 실제 Config/Tell을 일반 입력에서 갱신한다.
#include "client/TextClipboard.h" //최초 TextEntry 생성 전에 같은 CF_TEXT 장치를 준비한다.
#include "o/RawStartupStateCheck.h" //State1과3의 startupStateCheck가 같은 저장 파일/시계를 빌린다.
#include "o/RawDataManagerSelection.h"
#include "o/RawSavedStateRead.h" //startupStateCheck의 실제 저장 파일 부재 관문도 원문 reader에서 판정한다.
#include "o/RawBattleReconnect.h" // 저장 질문/서버 실패의 실제 같은 후보 선택 부모를 사용한다.
#include "o/RawConnectionHealth.h" // 실제 프레임의 연결 활동/지연/자동 서버 이전을 같은 상태에 연결한다.
#include "o/RawBattleResourceFrame.h" // Kernel 뒤 같은 장기 자원 전역의 프레임을 실행한다.
#include "client/ClientConnectionActivityGlobals.h" // 수신과 프레임 감시가 같은22칸/기한/자동 이전 DWORD를 빌린다.
#include "client/SavedStateFileRead.h" // 같은 startup reader를 실제 사용자 FILE/Player/Core에 연결한다.
#include "client/SavedStateFiles.h" //State1/3의 같은 저장파일 이름/State3 정리를 실제 설치 경로에 연결한다.
#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <bit>
#include <iostream>
#include <cstring> // 잘못된 파일의 현재 기본 이름 비교는 원문의 CRT 대소문자 무시 비교를 사용한다.
#include <cstdio> //TellTip의 원문 지역 sprintf 장치이며 전역 임시 문자열 고리를 예약하지 않는다.
#include <utility>
#ifndef NOMINMAX
#define NOMINMAX //Win32 파일 속성 장치의 min/max 매크로가 기존 표준 호출을 바꾸지 않게 한다.
#endif
#include <windows.h> //원문004dd560의 GetFileAttributesA 부재/디렉터리 판정을 실제 Windows 장치에 연결한다.
#include "client/ObjectOwnerCommands.h"
#include "client/DetonateInput.h"
#include "client/ScenarioObjectInput.h"
#include "client/VortexCommandInput.h" // 실제 신전 가상6c 인자와 일반 Descend 입력을 보존한다.
#include "client/ClientNsInfoGlobals.h" // ViewMeta가 현재 시작 서버 행의 개수를 검증한다.
#include "client/ScenarioEffectInput.h"
#include "client/AddRandomInput.h"
#include "client/DestroyAtInput.h"
#include "platform/Utf8Path.h" //Windows10/11의 한글 Edit 작업본은 원본 ANSI 경로와 달리 UTF-8/W 경계로 전달한다.

namespace netstorm::client {
// 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 미션 진행/적 상태/기술은 바꾸지 않는다.
void UberGump::InspectBridgePatterns(std::string_view patterns) { rawButtons_->ConfigureTestBridgePatterns(patterns); }
namespace {
constexpr std::uint32_t kDialogDoNothing=0x02000089U,kStartDemo=0x020000c0U; // 보호00542918/005429f4의 원본 명령이다.
// HTML·메타·Help가 같은 현재 원본 명령 번호를 조회한다. 호스트 전환 이름을 원본 번호로 만들지 않는다.
std::uint32_t DialogScriptCode(std::string_view name) {
    const auto lower=o::AsciiLower(name);const auto mission=MissionControlScriptCode(name); // 이름과 실제 Guide 명령 표를 한 번 조회한다.
    if (lower=="tell") return 0x02000095U;
    if (lower=="donothing") return kDialogDoNothing;
    if (lower=="startdemo") return kStartDemo;
    if (lower=="config") return kScriptConfig;
    if (lower=="warning") return kScriptWarning;
    if (lower=="viewspot") return kViewSpotCommand; // RunForIt [Init] 등이 입장 카메라를 옮기는 대본 명령이다(단위676).
    if (lower=="movedialog") return kMoveDialogCommand; // 커스텀 시나리오/안내창의 원문 위치 명령이다.
    if (lower=="setowner") return kSetObjectOwnerCommand; // 원본 Tell.SetOwner의 일반 입력 명령이다.
    if (lower=="connect") return 0x020000c4U; // 원문 Connect이며 로컬 대기실의 명시 역할 인자는 같은 일반 입력 큐로 보낸다.
    if (lower=="salvage") return 0x0200005dU; // 원본542e70의이름→54286c명령을HTML확인버튼에서도같은회수입력으로연결한다.
    const auto money=SendMoneyScriptCode(name);if (money!=0xffffffffU) return money; // asksend의 Yes가 원본 SendAllBucks를 같은 입력 큐로 보낸다.
    const auto fort=FortSelectionScriptCode(name);if (fort!=0xffffffffU) return fort; // 실제 맵 행도 같은 원문 번호와 문자열 고리를 사용한다.
    if (mission!=0xffffffffU) return mission;
    const auto knowledge=KnowledgeScriptCode(name); // 기술 창/지급도 동일 입력 큐를 사용한다.
    return knowledge!=0xffffffffU ? knowledge : GameMenuScriptCode(name);
}
// 실제 HTML 메뉴가 선택하는 현재 복원 명령도 같은 원본 메뉴 큐를 사용할 수 있다.
bool IsTellMenuCommand(std::uint32_t command) {
    if (command==kAskEditFortCommand || command==kReadFortCommand || command==kScrollMapListCommand || command==0x020000c4U || command==0x020000f5U || command==0x020000acU || command==0x020000abU || command==0x020000a9U) return true; // HTML 맵 행/로컬 Connect/시작/저장 복구·삭제·서버 실패도 같은 일반 입력 큐를 사용한다.
    static constexpr std::array<std::string_view,12> names{"Tell","DoNothing","StartDemo","Config","Warning","MissionBegin","MissionRestart","MissionAbort","LeaveBattle","MissionFork","MissionEnd","SetOwner"}; // 같은 실제 이름 조회와 숫자 코드를 공유한다.
    for (const auto name:names) if (command==DialogScriptCode(name)) return true; // 별도 호스트 Action 번호로 바꾸지 않는다.
    return false;
}
// 팝업 아래에 남는 기존 UI 글자를 같은 원본 글리프 몸체로 먼저 합성한다. 월드 전용 SceneImage는 UI 글자를 생략한다.
void RasterMenuBackdropText(IndexedImage& image,std::span<const RenderText> texts) {
    const auto width=static_cast<int>(image.width),height=static_cast<int>(image.height);const ScreenRect all{0,0,width,height};
    GumpDrawState clip;GumpPixelTarget target(image.indices,width,height,clip);GumpGlyphColorState colors;const auto hooks=MakeGumpGlyphPixelHooks(target,colors);
    // 현재 각 글자의 내부 클립과 원본 출력 문맥을 그대로 적용한다.
    for (const auto& text:texts) {
        SetGumpDrawClip(clip,text.clip ? ClipScreenRect(all,*text.clip) : all);
        const GumpTextContext context{(text.shadow ? 2U : 0U)|(text.outline ? 4U : 0U),0,text.color,0,0,text.shadowColor,text.shadowOffset.x,text.shadowOffset.y};
        DrawBitmapGlyphText(text.bytes,text.font,{text.x,text.y},context,hooks,static_cast<std::int32_t>(text.bytes.size()));
    }
}
}
// 화면 검사도 GUI가 표시하는 같은 raw 월드의 삭제/프로세스 경로를 사용한다.
bool UberGump::InspectDestroySurface(std::string_view type,float x,float y) {
    if (!world_ || (type!="bridge" && type!="island" && type!="noIsland")) throw std::invalid_argument("검사할 raw 월드/표면 종류 오류");
    const auto number=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+client_.Assets().Find(type).block);
    const auto sid=world_->Surfaces().Find(x,y,number);
    if (!sid.value) throw std::out_of_range("검사할 raw 표면 없음");
    return world_->Surfaces().Destroy(sid);
}
// 실제 HTML이 사용하는 같은 조회기의 값을 검사/분석에 빌리며 새 별칭이나 번호표를 만들지 않는다.
std::uint32_t HtmlScriptCommandValue(std::string_view name) { return DialogScriptCode(name); }
// 검사 호출 시점만 커널로 옮기고 실제 섹션 해석·페이지 교체·정지는 일반 Tell에 맡긴다.
void UberGump::InspectTell(std::string target) { Tell(std::move(target)); }
namespace {
// 원본 메인 메뉴의 돌 버튼 폭·높이·피치와 목록 행 높이.
constexpr int kButtonWidth = 75, kButtonHeight = 19, kButtonPitch = 79;
// 화면 조합은 GIF/VFX의 원래 팔레트 번호를 사용한다.
void Paste(IndexedImage& canvas, const IndexedImage& image, int x, int y, ScreenRect clip) {
    DrawIndexedImage(canvas.indices, static_cast<int>(canvas.width), static_cast<int>(canvas.height), image, x, y, clip);
}
// 크기 변경 없이 화면 좌표에 맞춰 질감을 반복한다.
void Tile(IndexedImage& canvas, const IndexedImage& image, ScreenRect rect) {
    // 영역 위에서 아래로 원본 크기의 타일을 반복한다.
    for (int y = rect.top; y < rect.bottom; y += static_cast<int>(image.height)) {
        // 한 줄의 마지막 타일은 판정 영역에 맞춰 자른다.
        for (int x = rect.left; x < rect.right; x += static_cast<int>(image.width)) Paste(canvas, image, x, y, rect);
    }
}
// 설정의 UTF-8 문자열을 원본 글꼴의 Windows-1252 바이트로 되돌린다.
std::string FontBytes(std::string_view text) {
    const auto encoded = o::EncodeDisplayText(text); return std::string(encoded.begin(), encoded.end());
}
// 6차 한국어: 옵션 메뉴 Language 하위 메뉴 항목이다. 영어(원문 번호0/1)와 한국어(7)를 각 언어 자신의 이름으로 보이고 현재 언어에 체크한다.
std::vector<GameMenuLanguage> LanguageMenuChoices(int current) {
    return {{FontBytes("English"),0U,current==0 || current==1},{FontBytes("한국어"),kKoreanLanguageNumber,current==static_cast<int>(kKoreanLanguageNumber)}};
}
// 7차: 후처리 장치가 있으면 옵션 메뉴 훅에 FSR 해상도, 표시 크기 조회, Anti-Aliasing/FSR 토글을 연결한다. 장치가 없으면 원문 구성 그대로 둔다.
}
void UberGump::ConnectPostProcessMenu(GameMenusHooks& hooks) {
    hooks.presentSize=[this] { return std::array<std::uint32_t,2>{static_cast<std::uint32_t>(client_.PresentWidth()),static_cast<std::uint32_t>(client_.PresentHeight())}; };
    if (!client_.PostProcessAvailable()) return;
    hooks.fsrResolutions=kFsrScreenResolutionChoices;
    hooks.postProcessOptions=true;
}
namespace {
// 자동 검사 보고에는 개행/탭을 제거하여 한 항목 한 줄을 유지한다.
std::string Field(std::string text) { std::replace(text.begin(), text.end(), '\t', ' '); std::replace(text.begin(), text.end(), '\n', ' '); std::replace(text.begin(), text.end(), '\r', ' '); return text; }
}

#include "client/UberHud.inc" // 같은 실제 소유자에 전체 HUD 루트/지도 전역과 네 자식 공장을 연결한다.
#include "client/UberEditor.inc" //실제 Edit 선택의 Native State1/파일/후반 UI와 다음 Poll4를 연결한다.
#include "client/UberMeta.inc" //실제 메타19도 World 정리 뒤 같은 장기 Client 장치를 사용한다.
#include "client/UberLan.inc" // 같은 실제 Fort/대기실과 일반 State7/World 교체를 연결한다.
#include "client/UberReconnect.inc" // 같은 실제 저장 reader/현재 Player와 서버 선택을 State15/7에 연결한다.
//446b50의 현재 커서 갱신 뒤 Alt/가운데 폴링을 같은497e00 목표 쓰기로 전달한다.
bool UberGump::PollCameraPan(const InputEvent& pointer) {
    client_.Configuration().ReadInt("scrollWhileMouseOut",scrollWhileMouseOut_);CameraPanHooks hooks;
    hooks.poll=[this](auto code) { return client_.Poll(code); };
    hooks.globals=[this] {
        const auto& screen=client_.GetScreen();const auto dialog=rawButtons_->CurrentDialog();
        // Windows10/11 호환 전체화면은 원본의 해상도 전환 대신 DIB를 확대한다. 이미 논리화된 포인터/지도와 같은 크기를 pan에 주어 물리 WM_SIZE에 따른 속도 변화를 막는다.
        // 창 모드는 원본1:1 입력이므로 현재 WM_SIZE 값을 유지한다. 원문 FTOL/8 계산과 목표 요청 순서는 바꾸지 않는다.
        const auto size=(screen.Flags()&ScreenMode::kFullScreen) ? ScreenPoint{screen.Width(),screen.Height()} : client_.ClientPixelSize();
        return CameraPanGlobals{scrollWhileMouseOut_,static_cast<std::int32_t>(dialog ? dialog : !popup_.empty()),client_.Active() ? 1 : 0,size.x,size.y,
            hud_ ? hud_->left : 0,hud_ ? hud_->top : 0,hud_ ? hud_->mapWidth : 0,hud_ ? hud_->mapHeight : 0};
    }; //호스트 popup도 원래 모달과 같은 관문이며 현재 지도 네 필드는 동일 HUD owner에서 조회한다.
    hooks.closePopups=[this] { rawButtons_->RequestClosePopups(); };
    hooks.request=[this](auto x,auto y) { auto& camera=client_.GameplayGlobals()->Camera();RequestCameraDelta(camera.RendererView(),camera.Target(),client_.GetScreen().Height(),x,y,client_.LocalStateGlobals()->MetaRebuild()); };
    return ProcessCameraPan(pointer,hooks);
}
// 원본 GIF는 색 표의 RGB를 쓰지 않고 파일의 번호를 화면 팔레트에 바로 쓴다(00419ec0→004dae60).
UberGump::UberGump(Client& client) : client_(client),initLoginSubscriber_(client.SubscriberGlobals()->MySubscriber()),initialSubscriber_(client.SubscriberGlobals()->LoginSubscriber()),mapFileSpec_(client.FortGlobals()->FileSpec()),fortSelectionMode_(client.FortGlobals()->SelectionMode()),htmlGlobals_(client.HtmlWindowGlobals()->text),hideBuildings_(client.GameplayGlobals()->HideBuildings()),worldRedraw_(client.KencloudGlobals()->WorldRedraw()),menuConfigRevision_(client.FortGlobals()->Catalog().dirty),commandGump_(client.ChatWindowGlobals()->window.current) {
    scripts_.emplace(std::string(kProductCreditsObject),MakeProductCreditsConfig(client_.Configuration().Registry())); // 원본 파일 대신 UI 수명의 등록 Config로 추가 Credits를 제공한다.
    textClipboard_=std::make_shared<TextClipboard>(Win32TextClipboardDevices()); //API를 연결만 하며 이 생성은 OS 클립보드를 열거나 변경하지 않는다.
    serializationGlobals_=std::make_shared<o::FortSerializationGlobals>(); // 원본BSS와같은한DWORD를모든manager/객체소비자보다오래소유한다.
    templateStorage_=std::make_shared<CampaignTemplateStorage>(serializationGlobals_); // 시작 초기화와 미션별 명시 정리를 별개 수명으로 유지한다.
    templateStorage_->BindDiagnosticLog([client=&client_](auto level,auto text) { client->WriteDiagnosticLog(level,text); }); // 지역 Template 덤프도 원본 level29 formatter/FILE/flush/trace→Chat을 같은 Client 수명에서 사용한다.
    auto& config = client_.Configuration();
    const auto editorRoots=ResolveEditorRoots(client_.GameDirectory(),client_.UserData()); //목록의 작업 namespace는 Native 전송/World가 반납된 뒤에도 같은 자산 경로로 계산한다. 5차: 사용자 홈 정책이면 CustomMaps/Editor다.
    const auto catalogFiles=std::make_shared<EditorFiles>(editorRoots.work,editorRoots.protectedRoots,client_.Files().FileNameOwner()); // 생성은보호경로/같은Client경로소유자만연결한다. 목록의Template읽기와실제Editor가같은장치를공유한다.
    EditorCatalogSourceDevices catalogSource;catalogSource.root=client_.GameDirectory();catalogSource.metadataRoot=editorRoots.metadata;catalogSource.archiveOnly=[this](auto path) { return client_.Files().TryReadArchiveOnly(path); }; //실제 기존 등록순서의 TAFF만 읽어 완료키 삭제의 disk 우회를 막는다.
    editorCatalog_=std::make_shared<EditorCatalogWorkspace>(catalogFiles,std::move(catalogSource)); //한 장기 이관 owner를 목록과 실제 Editor 경로 모두에 공유한다.
    SoloMapCatalogEffects mapEffects; // 현재 실제 장치만 제공하며 목록 분류/행 처리는 기존 몸체를 사용한다.
    mapEffects.sectionNames=templateStorage_->SectionNamesOwner(); // 주Template/분류/메타가같은Client이름표와변환상태를빌린다.
    mapEffects.serializationGlobals=serializationGlobals_; // 목록의두임시MapProbe도실제manager생성자의같은전역0쓰기를공유한다.
    mapEffects.listState=&client_.FortGlobals()->Catalog(); // 이름 변경/설정 증가와 Raw 목록 정리가 같은 dirty DWORD를 쓴다.
    mapEffects.typeCount=[this] { return static_cast<std::int32_t>(client_.Assets().TypeTable().Types().size()); };
    mapEffects.allocationBytes=[](auto) { throw std::logic_error("맵 목록의 빈 State 생성에 필요한 현재 미정 할당 바이트 미연결"); }; // 정상 보호 파일은 이 경계를 선택하지 않는다. 미정 바이트를 임의0으로 만들지 않는다.
    mapEffects.userName=[this] { return world_ ? std::string(world_->Session().CurrentHandle()) : mapUserName_; };
    mapEffects.translate=[this](auto text) { return FontBytes(client_.Translate(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };
    mapEffects.fileSpecKey=[this] { return mapFileSpec_.Current(); };
    mapEffects.enumeratePaths=[this](auto pattern) { return editorCatalog_->Enumerate(pattern); }; //최초 loose 이관 뒤 같은 writable 폴더의 실제 OS 순서만 원문 목록에 전달한다.
    mapEffects.readFile=[this](auto path) { return editorCatalog_->Read(path); }; //분류/메타의 현재 읽기가 같은 durable 삭제 상태를 지키며 원본으로 부활시키지 않는다.
    mapCatalog_=std::make_unique<SoloMapCatalog>(client_.Files(),config,std::move(mapEffects));
    if (client_.UserData()) mapCatalog_->Window().limit=std::numeric_limits<std::int32_t>::max(); // 5차: 사용자 홈 정책의 제품은 Edit 목록 개수 제한(원문 50)을 없앤다. 긴 목록은 50행 창으로 스크롤한다.
    title_ = DecodeGif(client_.Files().Read(config.PathSpec("BackgroundGifSpec", "titleMenu")));
    clouds_ = DecodeGif(client_.Files().Read(config.PathSpec("CloudFileSpec")));
    const auto& type = client_.Assets().Find("fortGump");
    // 원본 타입의 클러스터 이름으로 질감·모서리·체크 표시를 고른다.
    for (const char* name : {"A00", "A01", "I00", "I01", "I02", "I03", "J02"}) {
        const auto frame = client_.Assets().FrameIndex(type, name, 0);
        decorations_.emplace(name, client_.Assets().Shapes().Decode(type.block, frame));
        // 원본 버튼 생성자004254b0은 전역00542664의 프레임1(A01 Sandy marble)을 배경 자식에 전달한다.
        if (std::string_view(name)=="A01") {
            buttonTextureType_=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+type.block);
            buttonTextureFrame_=static_cast<std::uint32_t>(frame);
            buttonTextureMetrics_=client_.Assets().Shapes().SquidMetrics(type.block,frame);
            const auto rect=client_.Assets().Shapes().Blocks()[type.block].frames[frame].rect;
            buttonTextureOffset_={rect.left,rect.top};
        }
    }
    RawButtonHooks buttonHooks{
        [this] { return client_.WallSeconds(); },[this](auto code) { return client_.Poll(code); },
        [this] { if (client_.Audio()) client_.Audio()->Sounds().PlayByName("button.wav",0,0,0,0,0); },
        [this](std::uint32_t form,std::uint32_t flags) {
            if (form>65535U) throw std::logic_error("현재Client UI Form삭제의SID오류");
            client_.GameplayGlobals()->RequireSession().GumpForms().Destroy(o::Sid{static_cast<std::uint16_t>(form)},flags);
        }};
    if (client_.Assets().Edition()==o::OriginalEdition::Patch1078) {
        buttonHooks.invalidate=[this](auto rectangle,auto flags) { client_.GetRenderer().Invalidate(rectangle,flags); }; //장치 교체 뒤에도 같은 현재 Renderer의 원문 dirty 저장소에 전달한다.
        buttonHooks.invalidateAll=[this] { client_.GetRenderer().InvalidateAll(); }; //영역 정보가 없는 기존 적응 계층 요청은 각 소비자의 원문 영역 복원 전까지 이전 그림을 지운다.
        buttonHooks.project=[this](auto object) { return ProjectGumpObject(client_.GameplayGlobals()->RequireSession().Core().pool,client_.GameplayGlobals()->Camera().Snapshot(),object); }; // 미션/Editor/메타 교체와 카메라 이동 뒤에도 같은 현재 Pool/497220을 조회한다.
    }
    rawButtons_=std::make_unique<RawButtonControls>(client_.Assets().Edition(),std::move(buttonHooks),&client_.GameplayGlobals()->MapRevision()); //전체 HUD와 같은 현재 원본 DWORD를 첫 미니맵 생성 전부터 빌린다.
    rawButtons_->BindChecking(o::RawCheckingFlag{client_.GameplayGlobals()->AiDebug().Checking()},&client_.GameplayGlobals()->AiDebug().AssertionDetail()); //기본 UI 생성/메뉴보다 먼저 같은 Client 진단 DWORD와 부착 오류 상세를 연결한다.
    rawButtons_->BindMainMenu({[this] { return client_.LocalStateGlobals()->AutoDemo(); },[this] { return std::bit_cast<std::int32_t>(client_.HtmlWindowGlobals()->window.instances); },[this](auto command,auto first,auto second) { const auto x=scriptStrings_.Store(first),y=first==second ? x : scriptStrings_.Store(second);client_.Input().PushCommand(command,std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y)); },[this] { client_.ClearFullScreenState(); }}); //같은 원문 정적 문자열의 두 인자는 같은80칸 고리 별칭으로 일반 큐에 보낸다. 시작 표식은 실제 부모 callback에서 지운다.
    rawButtons_->ConfigureTestBridgePatterns(client_.TestBridgePatterns()); // 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 첫 조각 생성 전에 명시 CLI만 반영한다.
    if (!client_.TestBridgePatterns().empty()) std::clog<<"테스트 전용 다리 조각 순서: "<<client_.TestBridgePatterns()<<" (원본 분석 결과 아님)\n";
    RawMenuHooks menuHooks;
    menuHooks.command=[this](auto command,auto id,auto parameter) {
        if (command==0x01ffffffU) {
            if (!parameter || parameter>actions_.size()) throw std::logic_error("Client 팝업 DialogAction 연결 번호 오류");
            state_.Post(actions_[parameter-1]); // 기존 DialogAction의 임시 호스트 번호이며 원본 게임 명령 토큰과 구별한다.
        } else {
            // 실제 메뉴 항목이 선택한 미연결 명령은 여기서 거부한다. 돌 버튼의 별도 기본 클릭 통지와 구별한다.
            const auto& nsInfo=*client_.NsInfoGlobals(); // NSInfo 메뉴 작성기와 입력 소비자가 공유하는 현재 명령 owner를 선택 시점에 읽는다.
            const bool nsInfoCommand=command==nsInfo.kickoffCommand || command==nsInfo.sortOptionsCommand || command==nsInfo.setOptionCommand || command==nsInfo.excludeTypeCommand || command==nsInfo.restoreTypeCommand; // 추방과 정렬/설정/제외 메뉴의 실제 다섯 명령도 같은 공통 큐로 전달한다.
            if (command!=0x02000060U && command!=0x03000080U && command!=0x03000032U && command!=0x03000063U && command!=0x0200005dU && command!=0x02000017U && command!=0x020000f3U && command!=0x020000e8U && !nsInfoCommand && !IsNativeMetaSailCommand(command) && !IsScenarioObjectCommand(command) && !IsScenarioEffectCommand(command) && command!=0x0300011aU && command!=0x03000076U && !IsGameMenuCommand(command) && !IsTellMenuCommand(command) && !IsObjectMenuVariantCommand(command) && !IsSendMoneyCommand(command)) throw std::logic_error("현재 Client 원본 메뉴 항목 명령 미연결: "+std::to_string(command)); //실제 메타 설명/항해/영역 확인 소비자까지 연결된 명령만 같은 메뉴→큐 경계에서 허용한다.
            client_.Input().PushCommand(command,std::bit_cast<std::int32_t>(id),std::bit_cast<std::int32_t>(parameter));
        }
    };
    menuHooks.readInt=[this](std::string_view key,std::int32_t& value) { value=client_.Configuration().GetInt(key); };
    menuHooks.sound=[this](std::string_view name) { client_.WorldAudio().Sounds().PlayByName(name,0,0,0,0,0); };
    menuHooks.poll=[this](auto code) { return client_.Poll(code); };menuHooks.cursor=[this](auto number) { client_.GetCursor().Set(static_cast<int>(number)); };
    menuHooks.objectForms=[this]() -> o::GumpFormHost& { return client_.GameplayGlobals()->RequireSession().GumpForms(); }; //게임 GameWorld 또는 명시 메타 Session의 같은 실제 호스트만 조회한다.
    menuHooks.gameSeconds=[this] { return client_.Time().game; }; // 비용 표시도 같은 프레임의 게임 시각을 읽는다.
    menuHooks.wallSeconds=[this] { return client_.WallSeconds(); };menuHooks.translate=[this](auto text) { return FontBytes(client_.Translate(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); }; //공통 안내는 실제 벽시계와 같은 한 번의 번역/표시 경계를 사용한다.
    rawButtons_->EnableMenus(client_.Assets(),client_.Fonts(),std::move(menuHooks)).SetPalette(client_.GetScreen().Colors());
    rawButtons_->Menus()->BindColorMaps([owner=client_.ColorSortInput()]() -> const std::array<ColorMap,9>& { return owner->Maps(); }); // 메뉴 그림/도움말도 현재9표를 사용하며 파일 hit 뒤 재계산하지 않는다.
    RawHelpHooks help;help.globals=&htmlGlobals_; // Tell과 같은 HTML 전역/Config/입력 고리를 빌린다.
    help.translate=[this](auto text) { return FontBytes(client_.Translate(text)); };help.config=[this](auto key) { return FontBytes(client_.Configuration().PathSpec(key)); };
    help.substitute=[this](auto text) { const auto decoded=o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()});return FontBytes(client_.Configuration().SubstituteLocal(decoded)); };
    help.load=[this](auto name) { return ReadClientQuickHelpFile(client_,name); }; //원문check0/extra2이며LF/NUL/성공cache는기존전용창이소유한다.
    help.text=[this](auto token) { // 메뉴의정적도움말주제는같은소유저장소에서읽고Script고리/별칭과구별한다.
        const auto literal=objectMenus_ ? objectMenus_->StaticText(token) : std::nullopt;
        if (literal) return std::string(*literal);
        return std::string(scriptStrings_.Resolve((token&0xf0000000U)==0x70000000U ? token&0x0fffffffU : token));
    };
    help.storeText=[this](auto text) { const auto token=scriptStrings_.Store(text);if (token>0x0fffffffU) throw std::overflow_error("Help 문자열 별칭 토큰 소진");return 0x70000000U|token; }; // 원본 주소와 달리101부터인 고리 토큰이 TYPE<256 관문에 걸리지 않도록 같은 수명에 별칭을 준다.
    help.typeName=[this](auto type) { const auto types=client_.Assets().TypeTable().Types();if (type>=types.size()) throw std::out_of_range("Help 현재 TYPE 이름 범위");return types[type].name; };help.lookup=[this](auto name) { return client_.Assets().TypeTable().Find(name); };
    help.type=[this](auto type) { return world_ ? world_->Session().HelpTypeInfo(type) : ReadAssetQuickHelpType(client_.Assets(),type); };
    help.description=[this](auto type) { const auto types=client_.Assets().TypeTable().Types();if (type>=types.size()) throw std::out_of_range("Help 현재 TYPE 설명 범위");return FontBytes(client_.Translate(types[type].description)); };
    const auto hpStorage=client_.SurfaceStorage();const auto hpGlobals=client_.GameplayGlobals(); // Client 초기화가 메뉴 전에 만든 같은 장기 TYPE/게임 전역을 pin한다.
    help.hitPoints=MakeCurrentTypeHitPoints(client_.Assets().Edition(),[hpStorage,hpGlobals]()->std::span<const o::RiftTypeRecord> { if (!hpStorage || hpStorage->GameAux()!=hpGlobals->GameAux()) throw std::logic_error("Help 현재 TYPE/게임 전역 저장소 불일치");return hpStorage->types; },{o::RawCheckingFlag{hpGlobals->AiDebug().Checking()},o::RawCheckingFlag{hpGlobals->GameAux()->PriestQuarterHp()},hpGlobals->GameAux()->PriestType(),hpGlobals->GameAux()->DaisType()},[hpStorage](auto expression,auto file,auto line) { if (!hpStorage) throw std::logic_error("Help HP의 실제 진단 장치 미설치");hpStorage->AssertDisplay(expression,file,line); }); // World가 없어도 현재 약화/특별 번호와 정상 진단 복귀를 생략하지 않는다.
    help.helpFrame=[this](auto type) { const auto types=client_.Assets().Types();if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=types.size()) return -1;return types[type-o::kFirstAssetTypeNumber].definition.specialFrames.helpFrame; };help.nearestColor=[this](auto r,auto g,auto b) { return client_.GetScreen().FindColor(r,g,b); };
    help.queue=[this](auto command,auto first,auto second) { const auto x=scriptStrings_.Store(first),y=scriptStrings_.Store(second);client_.Input().PushCommand(command,std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y)); };
    help.script=[this](auto text) { QueueScriptCommand(text,client_.Input(),scriptStrings_,{DialogScriptCode,[](auto name) { std::clog<<"도움말 스크립트 명령 미연결: "<<name<<'\n'; }}); };
    help.external=[this](auto text) { OpenClientExternalHelp(client_,text); }; //같은 설정의 autoOpen/프로토콜 명령과 실제 Windows 실행을 사용한다.
    help.learn=[this](auto type) { if (!world_) throw std::logic_error("도움말 기술 획득의 현재 Session 없음");world_->Session().LearnNewTechnology(type); }; //실제 저장·설정·NewTech 안내를 의식의 로컬 획득과 공유한다.
    help.scripted=[this] { return state_.phase==ClientPhase::Dialog || state_.phase==ClientPhase::Briefing; };help.scriptName=[this] { return std::string(client_.HtmlWindowGlobals()->tell.current.data()); }; // 창 생략/실패도 기록하는 원문 현재 Tell 이름을 읽는다.
    help.battle=[this] { return world_ && world_->Session().Core().ownerMode.battle; };help.challenge=[this] { return world_ && world_->Session().Core().ownerMode.challenge; };
    help.html.command=[this](auto&,auto text) { QueueScriptCommand(text,client_.Input(),scriptStrings_,{KnowledgeScriptCode,[](auto name) { std::clog<<"도움말 HTML 명령 미연결: "<<name<<'\n'; }}); };
    BindHtmlConditionConfig(help.html,client_.Configuration(),client_.Assets().Edition());BindHtmlPictureText(help.html);
    BindHtmlBackground(help.html,htmlGlobals_); // Help와 Tell의 실제565e70을 같은 Client 수명에 보관한다.
    if (client_.Assets().Edition()==o::OriginalEdition::Patch1078) rawButtons_->Menus()->EnableHelp(std::move(help)); // 10.72에10.78 전용 이미지/창을 임의로 설치하지 않는다.
    objectMenus_=std::make_unique<CampaignObjectMenuClient>(client_,*rawButtons_,[this](std::string_view key) { state_.Post({"Tell",std::string(key)}); });
    MessageGumpHooks pauseContext; // 같은 현재 Session의 모드/이름과 Client 번역을 Pause 생성 시 조회한다.
    pauseContext.challenge=[this] { return client_.SurfaceStorage()->ownerMode.challenge ? 1U : 0U; }; //World 없는 준비된 MetaMap도 같은 현재00594fc8을 읽는다.
    pauseContext.battle=[this] { return client_.SurfaceStorage()->ownerMode.battle; }; //kind3 일반 가지에서만 현재00594fbc를 요구한다.
    pauseContext.playerName=[this](std::int32_t owner) {
        if (owner<=0 || owner>=9) return; // 원본48f480은 범위 밖에서 빈 문자열 포인터만 반환한다.
        if (!world_) throw std::logic_error("Pause 이름 조회의 현재 Session 없음");
        static_cast<void>(world_->Session().PlayerRecords().subspan(static_cast<std::size_t>(owner)*180U+12U)); // 반환 이름은 종류11에서 읽지 않는다.
    };
    pauseContext.challengePlayerName=[this](auto owner) { static_cast<void>(client_.ChallengeGlobals()->ParticipantNameBytes(owner,[this](auto expression,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\Ch.cpp",line); })); }; //42bbb0의 현재범위 진단/active/같은 이름 주소 조회다.
    pauseContext.playerText=[this](std::int32_t owner)->std::span<const char> {
        static constexpr std::array<char,1> empty{}; // 원문 범위 밖48f480이 반환하는 실제 빈 NUL 문자열이다.
        if (owner<=0 || owner>=9) return empty;if (!world_) throw std::logic_error("동맹 이름의 현재 Session 없음");const auto name=world_->Session().PlayerRecords().subspan(static_cast<std::size_t>(owner)*180U+12U,32);return {reinterpret_cast<const char*>(name.data()),name.size()};
    }; // 이름 내용을 미리 복제하지 않고 원문 서식 시점의 현재 NUL까지 읽는다.
    pauseContext.challengePlayerText=[this](auto owner)->std::span<const char> { const auto bytes=client_.ChallengeGlobals()->ParticipantNameBytes(owner,[this](auto expression,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\Ch.cpp",line); });return {reinterpret_cast<const char*>(bytes.data()),bytes.size()}; }; //실제 기록40BYTE 또는 같은 빈 NUL 주소를 번역 뒤까지 빌린다.
    pauseContext.gameTimeSeconds=[this] { auto& game=*client_.GameplayGlobals();return ReadGameTimeSeconds(game.BattleClockStamp(),game.BattleClockOrigin(),[] { return static_cast<std::uint32_t>(std::clock()); }); }; //기존로그인과같은CRT경계/현재두DWORD를사용한다.
    pauseContext.translate=[this](auto value) { return FontBytes(client_.Translate(value)); };
    warnings_=&rawButtons_->EnableWarnings([this] { return client_.WallSeconds(); },[this] {
        client_.WorldAudio().Sounds().PlayByName("newWarnMessage.wav",0,0,0,0,0);
    },std::move(pauseContext),[this] { return client_.GameplayGlobals()->world.missionPost; }); //원문의현재00594fa4를같은장기owner에서매Show진입에읽는다.
    fileMenus_=std::make_shared<TellFileMenus>(client_.Configuration(),[this](auto pattern) { return client_.Files().MatchFinder(pattern); }); // 실제 FileFinder의 디스크/TAFF/보조 순서와 장기 Config 수명을 공유한다.
    o::MissionGuideHooks guideHooks; // 같은 실제 현재 미션/전환 큐/창과 복원한 Guide 반복 몸체다.
    guideHooks.current=[this]() -> o::MissionStartState* { return missionRuntime_ && missionRuntime_->guided ? &missionRuntime_->state : nullptr; };
    guideHooks.pending=[this] { return state_.Pending() || client_.LocalStateGlobals()->State().next[0] || (world_ && world_->Connecting() && missionRuntime_ && worldMissionToken_==missionRuntime_->saveToken) ? 1U : 0U; }; // 현재 Native 다음 상태와 같은 미션의 설치 대여도 Guide보다 먼저 소비한다.
    guideHooks.dialogsClear=[this] { return rawButtons_->CurrentDialog()==0; }; // 원문4c8e90의 같은565dd4이며 Kill 뒤 인스턴스 수와 구별한다.
    guideHooks.stage=[this](o::MissionStartState& state,std::uint32_t sequence) {
        TutorialMissionStageHooks hooks; // 현재 단계가 실제로 도달한 소비자만 호출한다.
        hooks.initialize=[this] { InitializeMission(); };hooks.start=[this] { StartMissionWorld(); };
        hooks.tellGlobal=[this](std::string_view name,std::uint32_t mode) {
            if (mode!=1) throw std::logic_error("Guide init Tell의 원본 인자 위반");static_cast<void>(TryTell(std::string(name),false,nullptr,mode));
        };
        hooks.frame=[this] { if (!missionRuntime_ || !missionRuntime_->tutorial) throw std::logic_error("Guide 프레임의 실제 미션 연결 누락");missionRuntime_->tutorial->Frame(); };
        hooks.showStage=[this] { if (!missionRuntime_ || !missionRuntime_->tutorial) throw std::logic_error("Guide First의 현재 안내 연결 누락");missionRuntime_->tutorial->TellStage(); };
        hooks.tellMission=[this](std::string_view section) { bool created=false;return mission_ && mission_->Section(section) && TryTell(std::string(section),false,&created) && created ? 1U : 0U; };
        hooks.closeTransient=[this] { rawButtons_->RequestClosePopups(); };
        hooks.pause=[this] { if (!missionRuntime_ || !missionRuntime_->tutorial) throw std::logic_error("Guide 안내의 실제 자기 정지 연결 누락");missionRuntime_->tutorial->Gate().PauseForDialog(); };
        RunTutorialMissionStage(state,sequence,hooks);
    };
    guideHooks.postpone=[this](o::MissionStartState&) {
        if (!missionRuntime_) throw std::logic_error("Guide 재지연의 현재 미션 없음");
        if (missionRuntime_->tutorial) missionRuntime_->tutorial->Gate().Postpone();
        else if (missionRuntime_->gate.nextStageSeconds!=0.0) missionRuntime_->gate.nextStageSeconds=static_cast<double>(missionRuntime_->gate.delaySeconds)+client_.Time().game; // 설치 전 기본0은 그대로 유지한다.
    };
    guideHooks.release=[this] { ReleaseCurrentGuideMission(); }; // 원문 Guide의 현재 소멸은 World·메뉴·음악 전환을 예약하지 않는다.
    guideHooks.invalidStage=[] { throw std::logic_error("Guide 현재 미션 순번0..6 위반"); };
    if (client_.Assets().Edition()==o::OriginalEdition::Patch1078) guide_=std::make_unique<o::RawMissionGuide>(client_.Assets().Edition(),std::move(guideHooks)); // Guide 몸체의 복원 판본은10.78이며 CD 메인 메뉴 생성은 기존대로 둔다.
    if (client_.Assets().Edition()==o::OriginalEdition::Patch1078) {
        if (client_.LocalStateGlobals()->State().next[0]!=42) client_.LocalStateGlobals()->State().next[0]=3;pageName_="main"; //첫 Tick에서도 실제 State3를 소비하여 최초 안내/현재 Core/모드가 원문과 같은 부모를 지난다.
    } else MainMenu();
    Compose(); client_.ShowScene();
}
// 입력 참조를 먼저 없애고 메뉴가 소유한 월드를 이 구현 파일에서 제거한다.
UberGump::~UberGump() { ClearWorld();ReleaseCurrentGuideMission(); }
// 먼저 Tutorial→Start 순서로 같은 Session의 참조를 닫고 미션의 실제 데이터와 Script는 Guide 해제까지 남긴다.
void UberGump::DetachMissionWorldBindings() {
    if (missionRuntime_) { missionRuntime_->tutorial.reset();missionRuntime_->start.reset(); } //Tutorial이 빌리는 Start를 먼저 파괴하지 않는다. gate/state/guided는 이 경계에서 쓰지 않는다.
    userInput_.reset(); //현재 World를 빌리는 호스트 입력도 Session보다 먼저 반납한다.
    worldMissionToken_=0; // 반납/미션 교체 뒤 이전 접속 완료가 새 Runtime을 설치하지 못하게 한다.
}
// 같은 공용 소멸이 Tutorial 전역·실제 AI·기본 Config만 반납한다. 원문에는 World·입력·Resume·메뉴 음악 전환이 없다.
void UberGump::ReleaseCurrentGuideMission() {
    if (world_ && world_->Connecting() && missionRuntime_ && worldMissionToken_==missionRuntime_->saveToken) rawButtons_->Menus()->CloseMissionLoading(); // 현재 미션의 진행 안내만 반납하며 World는 원래 State 정리 위치까지 유지한다.
    ReleaseCampaignGuideMission(client_,state_,mission_,missionRuntime_,missionFortRequest_,*missionRestrictions_,missionNumber_,worldMissionToken_,client_.SurfaceStorage()->links.mission);
}
// 입력 참조를 먼저 없애고 월드의 실제 소유권을 해제한다. 위치 효과음은 카메라 0의 전체 메뉴 영역으로 되돌린다.
void UberGump::ClearWorld() {
    CloseLanLobby();lan_.reset();state_.Cancel("LoadLanBattleWorld"); // Player 주소를 다시 만들기 전에 제어 연결과 이전 일반 시작 요청을 반납한다.
    if (world_ && world_->Session().ConnectionCallbackActive()) throw std::logic_error("패킷 콜백 실행 중 전체 World 정리 재진입"); // UI/공유 저장소를 일부 반납하기 전에 전체 정리를 거부한다.
    if (rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->CloseSaveFortNameDialogs(); // 숫자 창도 현재 Session의 확정 장치를 반납한 뒤 월드를 해제한다.
    ClearPage(); // 현재 부모의 OnExit를 월드/미션 수명보다 먼저 같은 큐에 반납한다.
    if (rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->CloseEditorIslandWindows(); // 창/루트 드래그의 실제 Session 대여를 월드보다 먼저 반납한다.
    if (world_) client_.LocalStateGlobals()->CaptureEnd()=0.0; // 현재 월드 정리 어댑터에서004b2df0의005c8618 +0쓰기를 Session/UI/Kernel 반납 전에 실행한다. 전체 State7 cleanup 연결은 후속이다.
    if (world_) mapUserName_=world_->Session().CurrentHandle(); // 원본 Client 전역 이름은 월드를 해제한 뒤에도 다음 목록 조회에 남는다.
    ReleaseMissionFortLoad(state_,missionFortRequest_); //전체 종료도 같은 요청 owner 반납을 사용한다.
    if (rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->CloseMissionLoading(); // 로딩 중 이탈한 부모를 다음 장면에 남기지 않는다.
    if (rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->CloseHelp(); // 동적 정보/그림의 현재 Session보다 먼저 실제 창을 반납한다.
    if ((world_ || (editor_ && editor_->metaWorld)) && rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->Close(); //메타참가자팝업도같은Form을반납한뒤에만실제Session/월드포인터를해제한다.
    gameMenuState_.reset(); // 같은Session의검산참조를월드보다먼저반납한다.
    if (rawButtons_) rawButtons_->DisableMinimap(); // 같은Session/공통Gump보다먼저공간통지와미니맵을반납한다.
    if (rawButtons_) rawButtons_->DisableProduction(); // 실제 Session보다 먼저 UI Form을 반납한다.
    if (rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->CloseNsInfo(); // 현재 모드/문자/공통 부모가 살아 있을 때 NSInfo 전체 자식을 먼저 반납한다.
    if (rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->CloseChatWindows(); // 실제 Chat 자식도 현재 월드 장치가 살아 있을 때 반납한다.
    if (rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->CloseMetaDisplayGumps(); //메타 Form이 빌린 Session보다 먼저 같은 실제 글자/버퍼를 반납한다.
    client_.ClearThunderFlashes();
    if (world_) client_.GetRenderer().SetExtraCollection({}); // 월드 반납 전에 빌린 실제 커서 수집/Draw callback을 해제한다.
    client_.GetRenderer().SetOverlayProcedure({}); // 실제 메뉴 픽셀 소비자가 반납한 월드를 다시 읽지 않도록 먼저 제거한다.
    if (objectMenus_) objectMenus_->Clear();
    DetachMissionWorldBindings(); //같은 Session을 빌리는 실행기만 닫는다. 실제 Guide 데이터/자기정지/소멸 전역은 선택된 ReleaseCurrentGuideMission까지 보존한다.
    if (world_) world_->Session().DetachConnectionTransport(); // 실제 하위 수신기를 소멸하기 전에 자기 등록 대여만 반납한다.
    auto retiringWorld=std::move(world_); // 실제 소멸 callback의 현재 subscriber 조회는 이미 NULL인 같은 슬롯을 본다.
    if (retiringWorld) {
        retiringWorld.reset(); // Session이 같은 전역 Kernel을 먼저 비운 뒤 나머지 월드 소유권을 반납한다.
        client_.LocalStateGlobals()->LoginEnd()=0.0;client_.LocalStateGlobals()->ConnectEnd()=0.0; //004b2df0 말미의 두 별도 기한0은 실제 월드 반납 뒤에 쓴다. ActivityEnd와 State 블록은 보존한다.
    }
    if (editor_) { editor_->Close();editor_.reset(); } //현재 Session 수신기 뒤에 같은 작업 파일/양끝/State 분배기의 호스트 수명을 반납한다.
    client_.WorldAudio().SetView({0,0,0,0,client_.GetScreen().Width(),client_.GetScreen().Height()});
}
// 현재 어댑터의 월드 수명으로 원본 번개 부모 조회 경계를 제공한다.
bool UberGump::HasWorld() const { return world_!=nullptr; }
ScreenRect UberGump::CurrentMapView() const { return rawButtons_->MapViewRect(); } // 원문4c6b30의 현재 지도 전역이며 World의 과거 clip을 사용하지 않는다.
RawCampaignSession* UberGump::InputSession() const { return world_ ? &world_->Session() : editor_ ? editor_->metaWorld.get() : nullptr; } //호출마다 현재 owner를 읽고 Session/loaded/Native State를 새로 만들지 않는다.
// Native 부모가 실제 AI/링크/Kernel을 정리하고 Core0을 선택한 뒤 호출하는 호스트 소비자 반납이다.
void UberGump::RetireWorldForState() {
    auto* session=world_ ? &world_->Session() : editor_ ? editor_->metaWorld.get() : nullptr;
    if (!session) throw std::logic_error("State Runtime 반납의 현재 게임/메타 Session 소유자 누락");
    if (session->ConnectionCallbackActive()) throw std::logic_error("패킷 콜백 실행 중 State Runtime 반납 재진입"); // 공유 Pool/Player를 사용하는 실행자가 남아 있으면 전체 부모가 시작되면 안 된다.
    CloseLanLobby(); // Native 자식/Player 반납 전에 제어 소켓을 닫되 같은 State7 요청의 장기 owner는 유지한다.
    if (rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->CloseSaveFortNameDialogs(); // State 전환도 같은 숫자/이름 모달의 장치를 먼저 닫는다.
    if (world_) mapUserName_=session->CurrentHandle(); //메타 수명은 호스트 사용자 이름을 빈 게임 handle로 덮지 않는다. State7의 새 Guide 요청/Zone도 보존한다.
    gameMenuState_.reset();if (objectMenus_) objectMenus_->Clear(); //현재 Runtime에 대한 메뉴/Form 비소유 참조만 반납한다.
    client_.GetRenderer().SetExtraCollection({});client_.GetRenderer().SetOverlayProcedure({}); //새 HUD를 지우지 않고 이전 월드 Draw callback만 닫는다.
    DetachMissionWorldBindings();briefingSections_.clear();briefingIndex_=0;fortObjects_=0; //Runtime의 Session 참조만 반납한다. gate/state/Script와 Native mission은 State1 이름 비교 뒤 실제 closeGuide 또는 메인 메뉴 해제까지 유지한다.
    session->DetachConnectionTransport(); // 같은 실제 Session의 등록을 하위 처리기 소멸보다 먼저 닫는다.
    if (world_) { auto retiring=std::move(world_);retiring.reset(); }else { editor_->metaInput.reset();editor_->metaDisplay.reset();editor_->metaWorld.reset(); } //선택/입력과 표시기의 비소유 참조를 Session보다 먼저 반납한다.
}
// 같은34Effect/직접 전역/Runtime 조정기를 실제 공통 UI와 원문 부모 위치에 연결한다.
void UberGump::CleanupNativeStateWorld(std::uint32_t reason,std::shared_ptr<MetaConnectionRuntime> handoff) {
    if (handoff && (!editor_ || editor_->metaTransport!=handoff)) { const auto current=world_ ? world_->Session().SharedConnectionTransport():nullptr;if (!current || !current->SharesNetwork(handoff->SharedNetwork())) throw std::logic_error("Native State7의 현재 World/메타 인계 owner 불일치"); } //인계후에도같은실제장치인Source만허용하며다른World 콜백보호는아래유지한다.
    if ((world_ && world_->Session().ConnectionCallbackActive()) || (editor_ && editor_->metaTransport && editor_->metaTransport->CallbackActive() && editor_->metaTransport!=handoff)) throw std::logic_error("패킷 콜백 실행 중 Native/LAN 정리 금지");CloseLanLobby(); //제어 연결이 빌린 Player 주소를 재할당하기 전에 현재 callback 대여부터 확인한다.
    ClientStateWorldDevices devices;devices.cancelInitialCursor=[this] { CancelClientCursorForState(client_,*client_.SurfaceStorage()); };
    devices.killGumps=[this] { KillGumpsForState(); };devices.cleanupGumps=[this] { CleanupGumpsForState(); };devices.replaceUber=[this] { ReplaceHudForState(); };devices.refreshSp=[this] { RefreshStormPower(); };
    devices.retireRuntime=[this] { RetireWorldForState(); };devices.assertion=[](auto line) { throw std::logic_error("원문 State 중복 모드 진단:"+std::to_string(line)); };
    devices.leaveMetamap=[this] {
        NativeMetaDisplayHooks hooks;hooks.destroy=[this](auto sid,auto flags) {
            auto& session=client_.GameplayGlobals()->RequireSession();if (sid>65535U || sid>=session.Core().pool.Capacity()) throw std::out_of_range("메타 이탈의 실제 DWORD SID 범위");
            static_cast<void>(session.World().Destroy(o::Sid{static_cast<std::uint16_t>(sid)},flags));
        };LeaveMetaDisplay(*client_.ChallengeGlobals(),client_.LocalStateGlobals()->MetaZones().records,hooks);
    }; //004795a0의현재CS/존을지우고필요한가상삭제만같은실제World에위임한다.
    CleanupClientStateWorld(reason,{client_,*client_.SurfaceStorage(),*templateStorage_,*rawButtons_->Menus(),client_.WorldAudio()},client_.SurfaceStorage(),devices);
    if (editor_) editor_->metaTransport.reset(); //Native가10/11을 끊고 Player를 재초기화한 뒤 옛 span을 더 읽기 전에 host TCP owner를 반납한다.
}
// 별도 로그인 값 없이 현재 월드의 같은 Session 전역을 직접 조회한다.
std::uint32_t UberGump::CurrentSubscriber() const { return client_.CurrentSubscriber(); }
// 실제 공통 메뉴와 Client 전역을 사용하며 표시용 별도 로그/스타일을 만들지 않는다.
void UberGump::ForwardDiagnosticChat(std::uint32_t enabled,std::string_view format) { rawButtons_->Menus()->ForwardDiagnosticChat(enabled,format); }
void UberGump::ShowCaptureNotice(std::string_view body,std::uint32_t width,std::uint32_t height) {
    const auto screenWidth=client_.GetScreen().Width(),screenHeight=client_.GetScreen().Height();const auto serial=rawButtons_->Menus()->ShowHtmlWindow(LoadingHtmlContext({mission_ ? 85:0,0,screenWidth,screenHeight}),CaptureNoticeFontBytes(body),0,width,height);
    if (!serial) return;state_.phase=ClientPhase::Dialog;pageName_="capturewritten";popup_.clear();
    if (missionRuntime_ && missionRuntime_->tutorial) missionRuntime_->tutorial->Gate().PauseForDialog();else if (mission_) client_.Pause(true);RebuildControls();
} //같은 전체HTML의기존모달/닫기수명을사용하며별도Tell섹션으로치환하지않는다.
// 원문은 주소 formatter의 지역 결과와 별개로 빈 %s를 넘긴다. 번역 뒤의 실제 두 치환만 수행한다.
void UberGump::ShowLoginProgress(o::LocalMissionProgress kind,std::int32_t remaining) {
    std::string_view source;
    switch (kind) {
    case o::LocalMissionProgress::MissionStarting:source="Initializing.";break;
    case o::LocalMissionProgress::ReconnectStarting:source="Game Server disconnected.  Reconnecting to new Game Server.";break;
    case o::LocalMissionProgress::ConnectingCountdown:source="Connecting to Game Server %s - Countdown %d.";break;
    case o::LocalMissionProgress::ReconnectCountdown:source="Reconnecting to Game Server %s - Countdown %d.";break;
    case o::LocalMissionProgress::ServerLost:source="Connection to Game Server lost.";break;
    case o::LocalMissionProgress::ConnectingStarting:source="Connecting to Game Server. %s";break;
    case o::LocalMissionProgress::Connecting:source="Connecting to Game Server %s";break;
    case o::LocalMissionProgress::Reconnecting:source="Reconnecting to Game Server %s";break;
    case o::LocalMissionProgress::Transferring:source="Connected to server %s - Transferring.";break;
    case o::LocalMissionProgress::ReconnectTransferring:source="Reconnecting to new Game Server. Transferring.";break;
    case o::LocalMissionProgress::MissionTransferring:source="Starting Mission...";break;
    default:throw std::out_of_range("현재 로그인 안내 종류 범위");
    }
    auto text=client_.Translate(source);const auto address=text.find("%s");if (address!=std::string::npos) text.erase(address,2);const auto countdown=text.find("%d");if (countdown!=std::string::npos) text.replace(countdown,2,std::to_string(remaining));
    rawButtons_->Menus()->ShowMissionLoading(LoadingHtmlContext(CurrentMapView()),FontBytes(text),[this] { return FontBytes(client_.Translate("Cancel")); });rebuild_=true; // 현재 loading serial의 갱신만 예약하며 Compose/프레임 대기/State 확정을 중첩하지 않는다.
}
// 원문의 cf570 반환 serial을 호출자가 별도005c85b0에 저장하며 문자열은 표시 바이트다.
std::uint32_t UberGump::ShowLoginWaiting(std::string_view body) {
    const auto serial=rawButtons_->Menus()->ShowMissionLoading(LoadingHtmlContext(CurrentMapView()),body,[this] { return FontBytes(client_.Translate("Cancel")); });rebuild_=true;return serial;
}
// 테스트 전용: 현재 모달 Gump의 실제 serial을 읽는다. 값을 바꾸지 않으며 열린 모달이 없으면 0이다(단위696).
std::uint32_t UberGump::CurrentDialog() const { return rawButtons_->CurrentDialog(); } // 날씨도 소리 요청 뒤의 현재 모달을 읽는다.
std::uint32_t UberGump::CurrentDialogSerial() const { const auto token=rawButtons_->CurrentDialog();const auto* node=token ? rawButtons_->Menus()->Node(token):nullptr;return node ? node->serial:0U; }
// 실제 현재 Console만 전환하며 root 열기/닫기와 옵션 소비는 각 원문 부모가 담당한다.
void UberGump::ToggleChatInput() { rawButtons_->Menus()->ToggleChatInput(); }
// 기본 메뉴를 초기화한 다음 프레임에 미션을 시작한다.
void UberGump::StartMission(std::string name) { state_.Post({"MissionBegin",std::move(name)}); }
// 캠페인 이름의 설정 객체를 보존하여 officalN.title 같은 참조를 복원한다.
o::Config& UberGump::Script(std::string_view name) {
    const auto key = o::AsciiLower(name);
    if (const auto found = scripts_.find(key); found != scripts_.end()) return *found->second;
    auto script = std::make_unique<o::Config>(client_.Configuration().Registry(), key);
    const auto path = client_.Configuration().PathSpec("missionSpec", key);
    script->LoadBytes(client_.Files().Read(path)); script->Set("fileName", key);
    const auto [it, inserted] = scripts_.emplace(key, std::move(script)); (void)inserted;
    return *it->second;
}
// 현재 Client의 같은 전체 HTML 장치를 Tell과 미션 안내가 공유한다.
RawHtmlWindowContext UberGump::LoadingHtmlContext(ScreenRect mapView) { // 미션 안내의 실제 전체 HTML 경계를 같은 장기 소유자에 연결한다.
    htmlMapView_=mapView;RawHtmlWindowContext context;context.globals=client_.HtmlWindowGlobals();context.wallSeconds=[this] { return client_.WallSeconds(); };
    context.queue=[this](auto command,auto first,auto second) { client_.Input().Push(command,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };context.link=[this](auto token) { rawButtons_->Menus()->ActivateHtmlWindowLink(token); };
    context.metadata.commands={DialogScriptCode,[](auto name) { std::clog<<"HTML 메타 명령 미연결: "<<name<<'\n'; }};context.metadata.strings=&scriptStrings_;context.metadata.maps=mapCatalog_.get();context.metadata.fileSpec=&mapFileSpec_;context.metadata.fileMenus=fileMenus_;
    context.metadata.currentMap=[this]() -> std::string_view { htmlCurrentFortName_=mission_ ? missionRuntime_ && missionRuntime_->guided ? std::string(client_.FortGlobals()->Current()) : mission_->FortName() : std::string{};return htmlCurrentFortName_; };context.metadata.fallbackMap=[this]() -> std::string_view { return client_.FortGlobals()->Pending(); };
    context.metadata.send=[this](auto first,auto second,auto third) { SendHtmlZsend(first,second,third,[this](auto target,auto packet) { static_cast<void>(client_.ConnectionGlobals()->SendMutable(target,packet,client_.PlayerRecords()->Records())); }); }; //NULL연결은원문처럼아무송신도하지않으며월드/임의새접속/직접수신우회를추가하지않는다.
    context.metadata.input=[this](auto mode,auto title,auto continuation) { SaveFortNameDialogContext dialog;dialog.editor.now=[this] { return client_.WallSeconds(); };BindTextClipboard(dialog.editor,textClipboard_);dialog.translate=[this](auto text) { return FontBytes(client_.Translate(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };dialog.tell=[this](auto section) { Tell(std::string(section)); };rawButtons_->Menus()->ShowHtmlInputDialog(std::move(dialog),std::bit_cast<std::uint32_t>(mode),title,continuation); }; //원문004d3ed0은mode/제목/다음Tell섹션이며입력제한/설정키가아니다.
    context.metadata.tech=[this](auto target) { if (!world_) throw std::logic_error("HTML 전체 기술 목록의 현재 Session 없음");rawButtons_->Menus()->AppendTechnology(world_->Session(),target,[this](auto text) { return FontBytes(client_.Translate(text)); }); }; //같은 HTML controlled 열과 실제 대상 Form을 사용한다.
    context.metadata.themeTech=[this](auto target,auto theme) { if (!world_) throw std::logic_error("HTML 원소 기술 목록의 현재 Session 없음");rawButtons_->Menus()->AppendThemeTechnology(world_->Session(),target,theme,[this](auto text) { return FontBytes(client_.Translate(text)); }); }; //같은 실제 HTML의 현재 열·가상 레벨/기술 부모를 사용한다.
    context.factory.globals=&htmlGlobals_;context.factory.html.command=[this](auto&,auto text) { QueueHtmlScriptCommand(text); };
    BindHtmlConditionConfig(context.factory.html,client_.Configuration(),client_.Assets().Edition());BindHtmlPictureText(context.factory.html);
    BindHtmlBackground(context.factory.html,htmlGlobals_); // 미션 안내의 전체 HTML도 같은 현재 배경을 쓴다.
    context.factory.substitute=[this](auto text,auto) { return FontBytes(client_.Configuration().SubstituteLocal(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };context.factory.fallback=[this] { return FontBytes(client_.Configuration().PathSpec("OkButton")); };
    context.read=[this](auto address)->std::uint32_t { const auto map=mission_ && rawButtons_->UberRoot() && rawButtons_->UberRoot()->Current() ? rawButtons_->MapViewRect() : htmlMapView_;switch (address) {
        case 0x531860:case 0x5c78f0:return static_cast<std::uint32_t>(client_.GetScreen().Width());case 0x531864:case 0x5c78f4:return static_cast<std::uint32_t>(client_.GetScreen().Height());
        case 0x5ca9b4:return static_cast<std::uint32_t>(map.right-map.left);case 0x5ca9cc:return std::bit_cast<std::uint32_t>(map.left);case 0x5ca9d0:return std::bit_cast<std::uint32_t>(map.top);case 0x5ca9d4:return std::bit_cast<std::uint32_t>(map.right);case 0x5ca9d8:return std::bit_cast<std::uint32_t>(map.bottom);case 0x563e38:return rawButtons_->RootToken();
        case 0x5cab30:case 0x55a480:return client_.ScriptMenuGlobal(address); // Checked의 실제 Client/공급/Player 전역이다.
        default:throw std::logic_error("전체 HTML 현재 Client 전역 미연결: "+std::to_string(address));
    } };return context;
}
// 같은 HTML 입력은 자신이 소유한 원문 문자열 고리와 일반40칸 큐를 그대로 사용한다.
void UberGump::QueueHtmlScriptCommand(std::string_view text) {
    QueueScriptCommand(text,client_.Input(),scriptStrings_,{DialogScriptCode,[](auto name) { std::clog<<"HTML 스크립트 명령 미연결: "<<name<<'\n'; }});
}
// 실제 전체 부모는 Kill만 예약하고 공통 Cleanup에서 OnExit/인스턴스를 반납한다.
void UberGump::ClearPage() {
    rawButtons_->Menus()->RequestCloseHtmlWindows();page_={};
}
// 기존 메인/팝업 메뉴만 호스트 전환 큐를 사용한다. HTML 선택은 실제 부모가 원시 입력 큐에 쓴다.
void UberGump::ActivatePageAction(DialogAction action) {
    if (QueueFortSelectionAction(action.command,action.argument,client_.Input(),scriptStrings_)) return; // 메인Edit의원문명령은버튼콜백에서같은일반입력고리에직접예약한다.
    if (GameMenuScriptCode(action.command)!=0xffffffffU) { QueueScriptCommand(action.command+','+action.argument,client_.Input(),scriptStrings_,{GameMenuScriptCode,{}});return; } // 메인 Quit도 원문 버튼처럼 현재 일반 입력 큐에 예약하여 확인/종료음/반환을 같은 처리기에 맡긴다.
    state_.Post(std::move(action));
}
// Mission Init의 원문 이름 선택 위치에서 실제 두 버퍼/고정Template/Config/dirty를 사용한다.
void UberGump::SelectCurrentFortName(std::string_view name) {
    auto& names=*client_.FortGlobals();auto& gameplay=*client_.GameplayGlobals();o::FortCurrentNameHooks hooks; // 현재 게임 하위가 필요할 때만 같은 실제 Session에 연결한다.
    hooks.assertion=[](auto line) { std::clog<<"Fort 현재 이름 진단 "<<line<<'\n'; };hooks.refreshTitle=[this] { RefreshWindowTitle(); };
    if (world_) hooks=world_->Session().BindFortCurrentName(std::move(hooks)); // 전체 Battle snapshot이나 미설치 저장은 해당 원문 선택 위치에서 거부한다.
    o::FortCurrentNameGlobals globals{[&gameplay] { return gameplay.player.templateOwner; },[this] { return client_.SurfaceStorage()->ownerMode.fort ? 1U : 0U; },
        [&gameplay] { return gameplay.world.editor; },[this] { return mission_ ? 1U : 0U; },names.Catalog().dirty}; // 현재 owner/모드는 원문 각 callback 위치에서 재조회한다.
    const EditorFiles* work=editor_ && gameplay.world.editor ? &editor_->files : nullptr; //실제 Editor의 rename/snapshot/old 삭제는 같은 보호 작업본 장치만 사용한다.
    FortCurrentNameDevices devices(names,*templateStorage_,client_.Configuration(),std::move(globals),std::move(hooks),work,[this](auto configured) { return editorCatalog_->Resolve(configured); }); //절대 source FileSpec도 old/new snapshot/삭제의 같은 작업 namespace로 매핑한다.
    const std::string source(name.substr(0,name.find('\0')));devices.Set({reinterpret_cast<const std::uint8_t*>(source.c_str()),source.size()+1}); // 현재 요청의 안정된 NUL 입력을 동기 부모가 끝날 때까지 유지한다.
}
// 다음 상태를 먼저 쓰고 실제 Battle14의 파일/State/섬/Notify 하위를 같은 부모로 실행한다.
void UberGump::RequestFortEditState() {
    auto native=client_.LocalStateGlobals()->State();auto& player=client_.GameplayGlobals()->player;const std::uint32_t battle=world_ ? world_->Session().BattleActive() : 0U;
    RequestFortEdit({fortSelectionMode_,native.next[0],battle,native.phase[0],player.templateOwner,player.battleOwner},FortEditEffects());
}
// 일반 복구State7도 실제 pending/current 버퍼와 기존 전체 이름 부모를 사용한다.
void UberGump::SelectReconnectFortForState() {
    const auto native=client_.LocalStateGlobals()->State();if (native.current[0]!=7 || !native.current[33]) throw std::logic_error("복구 Fort 이름 선택의 실제 State7/재접속 표식 누락");SelectCurrentFortName(client_.FortGlobals()->Pending());
}
//같은 현재 이름/파일 장치와 실제 Session의 State/섬 검색/Notify를 원문 선택 위치에 제공한다.
FortSelectionHooks UberGump::FortEditEffects() {
    FortSelectionHooks hooks;hooks.resolveString=[this](auto token) { return scriptStrings_.Resolve(token); };
    hooks.lastEditPath=[this] { return client_.Configuration().PathSpec("lastEditFort"); };
    hooks.pending=[this](auto name) { const std::string source(name);SetPendingFortName(client_.FortGlobals()->PendingBytes(),{reinterpret_cast<const std::uint8_t*>(source.c_str()),source.size()+1},[] { std::clog<<"Fort pending 이름의 점 진단\n"; }); };
    hooks.refresh=[this](auto mode) { static_cast<void>(mapCatalog_->Refresh(static_cast<std::int32_t>(mode))); };
    hooks.tell=[this](auto name) { Tell(std::string(name)); };hooks.gameSeconds=[this] { return client_.Time().game; };
    hooks.currentFortPath=[this] { return std::string(EnsureEditorFortFiles().PrepareCurrent()); }; // 캠페인의 첫 편집 요청도 현재 Fort의 같은 작업본을 준비한 뒤 원문 reader에 넘긴다. 보호 원본에는 쓰지 않는다.
    hooks.loadFort=[this](auto slot,auto path) {
        if (!editor_ || !editor_->paths) throw std::logic_error("편집 복귀의 실제 파일 reader 없음");TemplateFileLoadHooks load;
        load.wrongName=[] { std::clog<<"편집 복귀의 Template 파일 이름 불일치\n"; };load.tooManyTypes=[] { std::clog<<"편집 복귀의 TYPE 개수 진단\n"; };
        load.rejectFile=[this](auto name) { editor_->files.Delete(name); };load.typeCount=[this] { return static_cast<std::int32_t>(client_.Assets().TypeTable().Types().size()); };
        load.currentSlot=[this] { return client_.GameplayGlobals()->player.templateOwner; };load.refreshTitle=[this] { RefreshWindowTitle(); };
        templateStorage_->LoadFile(slot,{},path,editor_->paths->BindLoad(std::move(load))); //Request 부모는 raw State만 사용한다. 뒤 State1은 작업 파일을 다시 해석해 새 World를 설치한다.
    };
    hooks.core=[this](auto slot) { if (!world_) throw std::logic_error("편집 복귀의 실제 State Session 없음");return world_->Session().MutableFortState(slot); };
    hooks.ownedIslands=[this](auto owner) { if (!world_) throw std::logic_error("편집 복귀의 실제 섬 목록 없음");return static_cast<std::int32_t>(world_->Session().OwnedIslandCount(owner)); };
    hooks.notify=[this](auto slot) { if (!world_) throw std::logic_error("편집 복귀의 실제 Notify Session 없음");world_->Session().BindFortCurrentName({}).notify(slot); };
    return hooks;
}
// 5차: 이전/다음 행의 인자(이동할 행 수)만큼 표시 창을 옮기고 같은 Tell 대화창을 목록 재열거 없이 다시 연다.
// 범위 보정은 다음 메뉴 작성(AppendTellMapMenus)이 한다. 같은 대화창이 없으면 아무것도 열지 않는다.
bool UberGump::ScrollMapList(const InputEvent& event) {
    if (event.code&InputCode::kRelease) return false;
    std::int32_t delta=0; // 이동할 행 수이며 음수는 이전 창이다.
    try { delta=std::stoi(std::string(scriptStrings_.Resolve(std::bit_cast<std::uint32_t>(event.y)))); }
    catch (const std::exception&) { return true; } // 잘못된 인자는 목록을 바꾸지 않는다.
    auto& window=mapCatalog_->Window();
    window.windowFirst=std::max<std::int32_t>(0,window.windowFirst+delta);window.keepCache=true;
    if (lastTellTarget_.empty()) return true;
    const auto target=lastTellTarget_; // Tell이 lastTellTarget_을 다시 쓰므로 먼저 복사한다.
    ClearPage();Tell(target);return true;
}
//현재Guide/Gump가 소비하지 않은 원문 사건만 받고 숫자State 예약을 선착순UI큐와 분리한다.
bool UberGump::FortSelectionEvent(const InputEvent& event) {
    const auto command=event.code&0x03ffffffU;
    if (command==kScrollMapListCommand) return ScrollMapList(event); // 5차: 원본에 없는 목록 스크롤 행이며 ReadFort/AskEditFort 분배와 독립이다.
    if (command!=kAskEditFortCommand && command!=kReadFortCommand) return false;
    auto native=client_.LocalStateGlobals()->State();auto& player=client_.GameplayGlobals()->player;
    const std::uint32_t battle=world_ ? world_->Session().BattleActive() : 0U; //원문 진입의 첫 관문만 캡처하고 owner/State는 같은 현재 저장소를 빌린다.
    const auto result=DispatchFortSelectionCommand(event,{fortSelectionMode_,native.next[0],battle,native.phase[0],player.templateOwner,player.battleOwner},FortEditEffects());
    if (result) Refresh();return result;
}
// 실제 전체 창의 Kill 직후 표시 phase만 복귀시키며 소멸/OnExit를 앞당기지 않는다.
void UberGump::SyncTellPresentation() {
    if (!popup_.empty() || (state_.phase!=ClientPhase::Dialog && state_.phase!=ClientPhase::Briefing) || rawButtons_->Menus()->HtmlWindowsOpen()) return;
    const bool inWorld=world_ || mission_; //Editor는 미션 Script가 없어도 같은 설치 World로 돌아간다.
    state_.phase=inWorld ? ClientPhase::Mission : ClientPhase::MainMenu;pageName_=inWorld ? (world_ && client_.GameplayGlobals()->world.editor ? "editor" : "mission") : "main";page_={};
    if (!(missionRuntime_ && missionRuntime_->tutorial)) client_.Pause(false);RebuildControls();
}
// 명시 사용자 명령에서 잘못된 섹션은 기존 진단을 유지한다. 선택적 미션 사건은 TryTell을 사용한다.
void UberGump::Tell(std::string target, bool briefing) {
    bool created=false; // 섹션 존재와 실제 창 생성(serial 비제로)을 구별한다.
    if (!TryTell(target,briefing,&created)) { std::clog<<"TELL cannot find \""<<target<<"\".\n";return; } // 원문cf730의 누락 경고/0 반환이며 새 창을 만들었다고 보고하지 않는다.
    if (created && missionRuntime_ && missionRuntime_->tutorial)
        missionRuntime_->tutorial->Gate().PauseForDialog(); // 명시 Tell도 같은 자기 깊이만 보유한다.
}
// 원본004cf730은 phase와 무관하게 현재 Mission의 설정에서 전체 target을 조회한다.
bool UberGump::TryTell(std::string target,bool briefing,bool* windowCreated,std::uint32_t suppress) {
    if (windowCreated) *windowCreated=false;
    if (o::AsciiLower(target)=="blank") {
        popup_.clear();ClearPage(); //원문 Blank는 같은 HTML을 닫으며 Native 장치/연결/현재 State 분배기를 반납하지 않는다.
        if (client_.LocalStateGlobals()->State().current[0]==3) { state_.phase=ClientPhase::MainMenu;pageName_="main";RebuildControls();Compose();return true; } //원문 Blank의 실제 메인 부모는 뒤 Tips가 열리기 전에 구성한다. Native 공통 State 확정은 이 callback 밖에서 수행한다.
        if (world_ || mission_) { state_.phase=ClientPhase::Mission;pageName_=world_ && client_.GameplayGlobals()->world.editor ? "editor" : "mission"; }
        else { state_.phase=ClientPhase::MainMenu;pageName_="main";page_={};input_.Cancel();RefreshWindowTitle(); }
        client_.Pause(false);RebuildControls();return true;
    }
    const auto result=ShowTellInConfig(client_.HtmlWindowGlobals()->tell,FontBytes(target),suppress,client_.Configuration(),
        [this](auto section)->std::optional<std::string> { return mission_ ? mission_->Section(section) : std::nullopt; },
        [this,&target](auto body,auto mode,auto width,auto height) { // 원문의 UTF-8 본문을 실제 좁은 HTML 공장에 한 번만 전달한다.
            const auto screenWidth=client_.GetScreen().Width(),screenHeight=client_.GetScreen().Height();
            return rawButtons_->Menus()->ShowHtmlWindow(LoadingHtmlContext({mission_ ? 85 : 0,0,screenWidth,screenHeight}),FontBytes(ProductTellBody(target,body)),mode,width,height); // 사용자 요청의 세 번째 항목도 같은 원문 HTML 창/메뉴/입력 장치를 사용한다.
        });
    if (!result.found) return false;rebuild_=true;
    if (!result.serial) return true; // 빈 부모/창 생략은 기존 표시 부모나 phase를 교체하지 않는다.
    lastTellTarget_=target; // 5차: 실제 창을 만든 섹션만 기억한다(맵 목록 스크롤의 재표시 대상).
    state_.phase=briefing ? ClientPhase::Briefing : ClientPhase::Dialog;pageName_=o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(client_.HtmlWindowGlobals()->tell.current.data()),std::char_traits<char>::length(client_.HtmlWindowGlobals()->tell.current.data())});popup_.clear();
    if (mission_ && !(missionRuntime_ && missionRuntime_->tutorial)) client_.Pause(true);
    if (windowCreated) *windowCreated=true;RebuildControls();return true;
}
// 메인 메뉴로 돌아올 때 미션 설정 객체도 등록 목록에서 제거한다.
void UberGump::MainMenu() {
    client_.GetRenderer().SetScene({}); client_.GetRenderer().SetBackground(nullptr);
    client_.GetRenderer().SetOverlay(nullptr);
    if (world_ && client_.Assets().Edition()==o::OriginalEdition::Patch1078) CleanupNativeStateWorld(0); //현재 Runtime 이탈은 원문 State3의 같은 전체 cleanup을 먼저 수행한다. Editor 모드0을 호스트에서 합성하지 않는다.
    ClearWorld(); ReleaseCurrentGuideMission(); fortObjects_ = 0; briefingSections_.clear(); briefingIndex_ = 0;
    client_.SurfaceStorage()->ownerMode.fort=true;client_.GameplayGlobals()->player.fortMode=true; // 원문 State3 공통 prefix의00594fb8=1을 같은 물리와 기존 Player 별칭에 전달한다. 전체 State3 부모의 나머지 연결은 별도다.
    state_.phase = ClientPhase::MainMenu; pageName_ = "main"; popup_.clear(); ClearPage(); input_.Cancel();
    client_.Pause(false); RefreshWindowTitle(); RebuildControls();
}
// 실제 State3 공통 확정 뒤 호스트의 표시만 선택하고 Native 장치/상태/초기 안내는 그대로 유지한다.
void UberGump::PublishNativeMainMenuState() {
    const auto native=client_.LocalStateGlobals()->State();
    if (world_ || native.phase[0]!=3 || native.current[0] || native.immediate[0] || native.next[0]!=4) throw std::logic_error("실제 State3 공통 확정 뒤 메인 표시 계약");
    client_.GetRenderer().SetScene({});client_.GetRenderer().SetBackground(nullptr);client_.GetRenderer().SetOverlay(nullptr);popup_.clear();
    if (!rawButtons_->Menus()->HtmlWindowsOpen()) { state_.phase=ClientPhase::MainMenu;pageName_="main";page_={}; } //Welcome/검증 안내/Tips가 열려 있으면 그 실제 Dialog를 유지한다.
    input_.Cancel();RefreshWindowTitle();RebuildControls();
}
// 등록 설정의 현재 ID만 Fort에 동기화한다. 외부 서버의 은퇴한 유효성 검사를 호출하지 않는다.
void UberGump::SynchronizeRegisteredSubscriber(std::uint32_t id) {
    SynchronizeClientRegisteredSubscriber(client_,*templateStorage_,id,std::span<const std::uint8_t>(*missionOptions_).subspan(0x3e,9),[this] {
        static_cast<void>(EnsureEditorFortFiles());return editor_->files.Writer(); // 이미지 생성/현재 저장·숨김 관문을 통과한 실제 파일 효과에서만 같은 작업 namespace를 요구한다.
    });
}
// 현재 미션의 원본 제목 설정을 적용한다. 데모 모드는 같은 미션 객체의 직접 기록을 읽는다.
void UberGump::RefreshWindowTitle() {
    WindowTitleState state;state.fortName=std::string(client_.FortGlobals()->Current());state.editor=client_.GameplayGlobals()->world.editor!=0;state.mission=client_.GameplayGlobals()->world.missionPost; // 현재 Fort는 World 생성 전에도 존재하며 State3의 첫 BYTE 삭제 뒤에는 빈 제목을 선택한다.
    if (mission_ && state.mission) { state.fortName=missionRuntime_ && missionRuntime_->guided ? std::string(client_.FortGlobals()->Current()) : mission_->FortName(); } // 원문 Init이 연결된 미션은 같은 실제 현재 Fort를 읽는다. 다른 기존 로더의 이름 계약은 유지한다.
    if (!state.editor && !state.mission && !state.fortName->empty()) {
        const auto owner=client_.GameplayGlobals()->player.templateOwner;const auto headers=templateStorage_->Headers(); // 원문 현재 owner의 같은 20BYTE 헤더를 조회한다.
        if (owner>=9 || headers.size()<static_cast<std::size_t>(owner+1U)*20U) throw std::out_of_range("일반 Fort 제목의 현재 Template 헤더 범위");
        const auto header=headers.subspan(static_cast<std::size_t>(owner)*20U,20);
        state.templatePresent=header[0] || header[1] || header[2] || header[3];state.templateReadOnly=header[8] || header[9] || header[10] || header[11]; // 실제 +0/+8 DWORD가 모두 비영일 때 readonly 접두를 붙인다.
    }
    if (missionRuntime_) { state.demo=missionRuntime_->gate.demo!=0;state.testBattle=missionRuntime_->mode64!=0; } // 현재 미션의 +68/+64를 함께 읽어 데모 우선과 Test Battle 제목 선택을 보존한다.
    netstorm::client::RefreshWindowTitle(state,{
        [this](std::string_view key,std::optional<std::string_view> argument) { return client_.Configuration().PathSpec(key,argument); },
        [this] { return mission_ ? mission_->Get("title").value_or(std::string()) : std::string(); },
        [this](std::string_view title) { client_.Title(title); }});
}
// 원본 Begin의 현재 finish/release 뒤 새 빈 미션을 만든다. StartDemo의 직접 지정값도 설치 전에 공급한다.
void UberGump::BeginMission(std::string name,std::uint32_t demo,std::uint32_t mode) {
    if (const auto parent=rawButtons_->MainMenuToken()) rawButtons_->KillGump(parent); //원본에 없는 --mission 예약은 첫 Native State3보다 앞일 수 있다. 실제 Begin 시 일반 메인 버튼 선택과 같은 부모 Kill을 예약해 Guide의 dialogsClear를 막지 않게 한다.
    auto source=mission_ ? mission_->FileSource() : o::ConfigInterface::FileReader{};auto effects=MissionTransitionEffects();effects.create=[this,demo,mode,source=std::move(source)](std::string_view target) { CreateMission(std::string(target),demo,mode,source); }; //기존 Guide를 해제하기 전에 명시 namespace를 소유 복사하며 다음 공장이 같은 source를 사용한다.
    BeginGuideMission(name,effects);
}
// Guide 공장의 현재 Client 소비자다. 이전 미션 해제는 호출자가 원본 순서로 마친 상태여야 한다.
void UberGump::CreateMission(std::string name,std::uint32_t demo,std::uint32_t mode,o::ConfigInterface::FileReader source) {
    if (mission_ || missionRuntime_) throw std::logic_error("Guide 새 미션 공장 전에 기존 현재가 남아 있음"); //기존 World는 새 Init 뒤 State7의 실제 cleanup 위치까지 살아 있을 수 있다.
    mission_ = CreateGuideMissionScript(client_.Configuration(),name,client_.SurfaceStorage()->links.mission,std::move(source));
    if (!mission_->Loaded()) throw std::runtime_error("Missing mission: " + name);
    const auto type = o::AsciiLower(mission_->MissionType());
    if (type != "tutorial" && type != "battle") throw std::runtime_error("Mission type pending: " + mission_->MissionType());
    if (nextMissionSaveToken_==0xffffffffU) throw std::overflow_error("Mission 저장 수명 토큰 소진");
    missionRuntime_=std::make_unique<CampaignGuideRuntime>(*mission_,demo,mode,++nextMissionSaveToken_);
    missionRuntime_->guided=missionRuntime_->guided && client_.Assets().Edition()==o::OriginalEdition::Patch1078; // 복원하지 않은 판본의 순번을 새 Guide에 분배하지 않는다.
    briefingSections_.clear();briefingIndex_=0;state_.phase=ClientPhase::LoadingMission;pageName_="loading";popup_.clear();ClearPage();client_.Pause(false);RebuildControls(); // 제목은 실제 단계1의 Header 적용/설치에서 갱신한다.
    if (!missionRuntime_->guided) { Compose();client_.PresentScene();state_.Post({"LoadMissionWorld",{}}); } // Guide.Init이 시작되기 전 로딩 모달로 자체 관문을 막지 않는다. 실제 안내는 로드 호출에서 표시한다.
}
// Guide의 각 교체 함수가 같은 Runtime/Script/월드와 소유 정지 깊이를 직접 사용하게 한다.
GuideMissionTransitionHooks UberGump::MissionTransitionEffects() {
    GuideMissionTransitionHooks hooks;hooks.current=[this] { return missionRuntime_!=nullptr; };
    hooks.finish=[this] {
        if (!missionRuntime_) throw std::logic_error("현재 미션 없는 Guide 가상 종료");
        if (missionRuntime_->guided) RunTutorialMissionStage(missionRuntime_->state,6,{}); // 실제 Tutorial+1c의 빈 종료다. 다른 형식은 기존 teardown 어댑터를 유지한다.
    };
    hooks.release=[this] { ReleaseCurrentGuideMission(); }; //원문 Guide 해제는 Mission만 닫는다. World/Player/Kernel/HUD 반납은 뒤 State1/7의 전체 부모가 수행한다.
    hooks.create=[this,source=mission_ ? mission_->FileSource() : o::ConfigInterface::FileReader{}](std::string_view name) { CreateMission(std::string(name),0,0,source); }; //Restart/Fork도 해제 뒤 기본 자산 경계로 돌아가지 않도록 현재 명시 source의 수명을 보유한다.
    hooks.get=[this](std::string_view key) { if (!mission_) throw std::logic_error("현재 설정 없는 Guide Fork");return mission_->Get(key).value_or(""); };
    hooks.mode=[this](std::uint32_t value) { if (!missionRuntime_) throw std::logic_error("재시작 공장 뒤 모드 수신 누락");missionRuntime_->mode64=value; };
    hooks.demo=[this](std::uint32_t value) { if (!missionRuntime_) throw std::logic_error("재시작 공장 뒤 데모 수신 누락");missionRuntime_->gate.demo=value; };
    return hooks;
}
// 원본 Init은 현재 전역/Config를 쓰고 로드 요청 뒤 Header/flags를 수행한다. 실제 월드는 다음 루프에 설치한다.
void UberGump::InitializeMission() {
    if (!mission_ || !missionRuntime_ || !missionRuntime_->guided) throw std::logic_error("Tutorial Init의 현재 Guide 계약 위반");
    TutorialInitHooks hooks;hooks.clearTemplates=[this] { templateStorage_->ClearAll(); }; //원문004c3290 첫 호출은 살아 있는 Editor/Session과 별개로 같은9개 manager를 반납한다. Session은 현재 헤더를 다시 조회하며 Template 참조를 고정 보유하지 않는다.
    hooks.resetOptions=[this] { o::ResetMissionBattleOptions(o::OriginalEdition::Patch1078,*missionOptions_); };
    hooks.get=[this](std::string_view key,std::optional<std::string_view> fallback) { auto value=FontBytes(mission_->Get(key).value_or(""));if (const auto end=value.find('\0');end!=std::string::npos) value.resize(end);if (value.empty() && fallback) value=*fallback;return value; };
    hooks.set=[this](auto key,auto value) { const auto text=o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});static_cast<void>(mission_->Set(key,text)); };
    hooks.requestFort=[this](auto name) { RequestMissionFort(name);state_.Post({"LoadMissionFort",missionFortRequest_->fort}); };
    hooks.header=[this] { ApplyMissionTechnology(*mission_,client_.Assets().TypeTable(),std::span(*missionRestrictions_)); };
    InitializeTutorialMission(missionRuntime_->init,missionRuntime_->state.flags,missionRuntime_->gate.demo,{missionRuntime_->gate.tutorialNumber,missionNumber_,tutorialSeen_,initLoginSubscriber_,initialSubscriber_},hooks);
    rebuild_=true;
}
// 실제 Fort 선택/Zone 복제를 두 미션 경로가 공유한다. 다음 상태는 동일 Client 소유144BYTE다.
void UberGump::RequestMissionFort(std::string_view name) {
        if (state_.Pending() || missionFortRequest_) throw std::logic_error("Init 요새 로드 요청 전 다른 전환/이미지가 남아 있음");
        auto request=std::make_shared<MissionFortLoadRequest>();MissionFortRequestHooks effects; // 원본 부모가 쓴 요청과 실제 복제 이미지를 같은 수명으로 소유한다.
        effects.selectFort=[this,request](auto value) { SelectCurrentFortName(value);const auto current=client_.FortGlobals()->Current();request->fort=o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(current.data()),current.size()}); }; // 원문00460980 전체 복귀 뒤 실제 현재 이름을 State 설치 요청에 보유한다.
        effects.duplicate=[request](auto& packet) { return CaptureMissionFortZone(*request,packet); };
        const std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};const auto& port=client_.GameplayGlobals()->GameServerPort(); //원문00513870의9BYTE와 같은 현재005318e8을 Fort 선택/설정 복귀 뒤에도 참조한다.
        auto& next=client_.LocalStateGlobals()->State().next; // 실제005c8878의144BYTE에 쓰며 캡처를 Native 전역 대신 사용하지 않는다.
        WriteMissionFortRequest(name,{next,client_.LocalStateGlobals()->ZonePending(),address,port},effects);request->next=next;request->sequence=client_.LocalStateGlobals()->ZonePending(); // 요청은 같은 State/005c85c0의 작성 결과만 관측 캡처한다.
        missionFortRequest_=std::move(request);
}
// State1에서 이미 고른 파일은 재선택하지 않고 현재 Guide의 실제 Subscriber 요청을 인계한다.
void UberGump::RequestCurrentGuideFort(RawCampaignSession& session) {
    if (!missionRuntime_ || state_.Pending() || (missionFortRequest_ && !missionFortRequest_->consumed)) throw std::logic_error("현재 Guide Fort 요청의 수명/지연 큐 중복");
    // 이미 소비된 이전 이미지는 기존 Session의 공유 수명에 맡기고 새 요청 성공 뒤 교체한다. 미소비 이미지는 덮어쓰지 않는다.
    auto request=std::make_shared<MissionFortLoadRequest>();GuideFortRequestHooks effects;const auto& port=client_.GameplayGlobals()->GameServerPort();
    effects.subscriber=[&session](auto id,auto name) { session.ChangeFortSubscriber(id,std::span(reinterpret_cast<const std::uint8_t*>(name.data()),name.size()+1)); }; //고정 Champion의 NUL을 전달하며 복귀 후 writer가 같은 장기 포트의 현재값을 읽는다.
    effects.handle=[&session] { return session.CurrentHandle(); };effects.duplicate=[request](auto& packet) { return CaptureMissionFortZone(*request,packet); };
    const std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};auto& local=*client_.LocalStateGlobals(); // 원문 로컬9BYTE 주소와 기존 일반 Init의 같은 장치 경계다.
    WriteCurrentGuideFortRequest({local.State().next,local.ZonePending(),address,port},effects);request->next=local.State().next;request->sequence=local.ZonePending();
    const auto name=client_.FortGlobals()->Current();request->fort=o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()});missionFortRequest_=std::move(request);
}
// Guide-only 조건 없이 실제 현재 Runtime을 읽으며 선택한 미션 수명이 바뀌면 물리 경계로 거부한다.
o::FortMissionSaveHooks UberGump::FortMissionSaveEffects() {
    o::FortMissionSaveHooks hooks; // write는 각 Snapshot이 소유한 실제 stream에 동기적으로 연결한다.
    hooks.current=[this] { return missionRuntime_ ? missionRuntime_->saveToken : 0U; };
    hooks.name=[this]()->std::optional<std::span<const std::uint8_t>> {
        if (!missionRuntime_) throw std::logic_error("현재 Mission 없는 이름 물리 접근");
        const auto& name=missionRuntime_->init.name; // 같은 Runtime의 NUL 포함 버퍼이며 저장 중 복제하지 않는다.
        return std::span(reinterpret_cast<const std::uint8_t*>(name.c_str()),name.size()+1);
    };
    hooks.field=[this](auto token,auto offset) {
        if (!missionRuntime_ || token!=missionRuntime_->saveToken) throw std::logic_error("선택 Mission의 저장 수명 만료");
        if (offset==0x58) return missionRuntime_->state.sequence;
        if (offset==0x54) return missionRuntime_->state.flags;
        throw std::out_of_range("Mission 저장의 실제 필드 범위");
    };
    hooks.assertion=[](auto) { throw std::logic_error("Mission 저장 원문 이름 진단"); };
    return hooks;
}
// 캡처한 요새 요청을 다음 State 자리에서 실제 설치한다. Start는 원래 Guide 단계3에 남긴다.
void UberGump::LoadMissionWorld(std::optional<std::string> requestedFort) {
    if (!mission_ || !missionRuntime_) throw std::logic_error("미션 부모의 현재 Script/Runtime 계약 위반");
    if (client_.Edition()!=o::OriginalEdition::Patch1078) { LoadMissionWorldBody(std::move(requestedFort));return; } // 기존 CD 어댑터는 새10.78 부모로 덮어쓰지 않는다.
    if (!missionFortRequest_) RequestMissionFort(requestedFort ? *requestedFort : mission_->FortName());
    try { RunCampaignMissionStarting(client_,[this,&requestedFort] { LoadMissionWorldBody(requestedFort); },[this](auto address,auto port) { if (!world_) throw std::logic_error("State7 설치 뒤 현재 World 없는 상태8 접속");world_->Session().ConnectLocalConnectionForState(address,port); }); }
    catch (...) { if (rawButtons_->Menus()->MissionLoadingOpen()) rawButtons_->Menus()->CloseMissionLoading();throw; } // 후반 복귀 뒤8 접속 실패의 잔여 안내도 같은 부모 수명에서 정리한다.
}
// 실제 State7 몸체는 공통 Run 안에서 기존 World를 반납하고 같은 장기 물리에 새 World를 설치한다.
void UberGump::LoadMissionWorldBody(std::optional<std::string> requestedFort) {
    if (!mission_ || !missionRuntime_) throw std::logic_error("Guide 초기화의 현재 Script/Runtime 계약 위반");
    state_.phase=ClientPhase::LoadingMission;pageName_="loading"; // 접속 준비 중의 미설치 World를 활성 게임 화면으로 그리지 않는다.
    const bool nativePlayers=client_.Assets().Edition()==o::OriginalEdition::Patch1078; //실제 State7의 앞선 Player 초기화 완료를 뒤 Session에 인계하는 호스트 대여 선택이다.
    if (nativePlayers && !missionFortRequest_) RequestMissionFort(requestedFort ? *requestedFort : mission_->FortName()); // 일반 Battle도 실제 요청을 작성하며 기본 상태0을7로 간주하지 않는다.
    struct LoadingRetirement { // 준비 실패는 즉시 정리하고 정상 안내는 실제 비동기 완료에 인계한다.
        RawMenuControls& menus; // 로드/실패 반납보다 오래 사는 같은 실제 UI 소유자다.
        bool continued{}; // 정상 비동기 접속에만 안내 수명을 다음 완료/이탈 경계로 넘긴다.
        ~LoadingRetirement() { if (!continued) menus.CloseMissionLoading(); } // 준비 실패의 현재 안내는 반납하고 정상 대기는 일반 프레임까지 유지한다.
    } loading{*rawButtons_->Menus()};
    if (!nativePlayers) { rawButtons_->Menus()->ShowMissionLoading(LoadingHtmlContext({85,0,client_.GetScreen().Width(),client_.GetScreen().Height()}),FontBytes(client_.Translate("Starting Mission...")),[this] { return FontBytes(client_.Translate("Cancel")); });rebuild_=true;Compose();client_.PresentScene(); } //10.72의 기존 어댑터만 파일 전 표시를 유지한다.10.78은 원문 State7 안내 위치에서 생성한다.
    if (nativePlayers) { CaptureCampaignMissionStarting(client_,*missionFortRequest_);CleanupNativeStateWorld(1);client_.LocalStateGlobals()->CdPresent()=client_.CdGlobals()->Check(MakeWindowsCdPresenceHooks(&client_.StaticStrings()->FileNames())); } // 최초 복사/로그는 같은 부모가 이미 수행했다. 기본 분모/정리 전 지역 값 뒤 cleanup/CD 순서를 유지한다.
    else if (world_) ClearWorld(); //10.72의 기존 동기 World 어댑터는 해당 판본 전체 State 복원 전까지 유지한다.
    if (world_) throw std::logic_error("미션 State cleanup 뒤 이전 World 대여가 남아 있음"); //원문 정리의 실제 반납을 확인한 뒤에만 새 Session을 만든다.
    if (nativePlayers) {
        if (commandGump_) static_cast<void>(rawButtons_->BubbleGump(commandGump_,0x02000088U,0,0)); // 원문 State7의 CD 뒤 명령 창 숨김이다.
        auto& local=*client_.LocalStateGlobals();o::RestoreLocalMissionStarting(local.State().current,*missionFortRequest_->starting,local.GameTicks(),local.SourceType()); // 명령 창 효과 뒤 바뀐 현재+84도 다시 읽고 기계어의 시각→출처 순서로 복원한다.
        client_.SurfaceStorage()->ownerMode.battle=true;if (client_.Audio()) client_.Audio()->StartScene(true); //469fc0의 실제 전투 음악 갱신은 Zone/월드 설치보다 앞이다.
        if (missionRuntime_->guided) PauseMissionWorldEntry([this] { return missionRuntime_!=nullptr; },[this] { o::PauseMissionFrameForDialog(missionRuntime_->gate,[this] { return client_.Paused(); },[this] { client_.PushGamePause(); }); }); // 같은 자기 정지 상태를 커서 생성 전에도 원문 위치에서 적용한다.
    }
    if (!missionRuntime_->guided) { missionRuntime_->gate.tutorialNumber=std::bit_cast<std::uint32_t>(o::ConfigParseLong(mission_->Get("tutorialNumber").value_or("")));ApplyMissionDemo(mission_->Get("demo").value_or(""),missionRuntime_->gate.demo);o::ResetMissionBattleOptions(o::OriginalEdition::Patch1078,*missionOptions_);o::SetTotalMadeAllowAll(*missionRestrictions_,1);initLoginSubscriber_=1; }
    const auto selectedFort=missionFortRequest_ ? missionFortRequest_->fort : (requestedFort ? *requestedFort : mission_->FortName()); // 실제 선택/요청에서 캡처한 이름을 이후 설정 재조회로 바꾸지 않는다.
    auto fortFileSelect=mission_->FortFileSelector([workspace=editorCatalog_,working=static_cast<bool>(mission_->FileSource())](std::string_view configured) {
        return working ? platform::PathToUtf8(workspace->Resolve(configured)) : std::string(configured); // 작업본만 실제 writable 경로를 사용하고 기본 자산은 원문 fortSpec 이름으로 출처/권한을 조회한다.
    });
    o::FortTemplate fort; // 실제10.78은 State9의 지역 읽기와 서버 수신의 전역 설치 전까지 편의 파싱 자료를 만들지 않는다.
    o::MissionPlayers players; // 실제 Player/SP/기술은 raw Start가 읽는다. 이 구형 표시 사본은10.72 어댑터에서만 만든다.
    MissionFortSourceImage fortFile; //10.72의 기존 동기 설치만 최초 파일 이미지를 보관한다.
    std::shared_ptr<const std::vector<std::uint8_t>> fortImage; //10.78은 선행 I/O가 없으므로 비어 있다.
    fortObjects_=0;
    if (!nativePlayers) {
        auto selected=fortFileSelect(selectedFort);fortFile={selected.read(),std::move(selected.fileName)};fortImage=std::make_shared<const std::vector<std::uint8_t>>(std::move(fortFile.image));fort=o::FortTemplate::Parse(*fortImage,client_.Assets().TypeTable());
        players=o::MissionPlayers::Load([this](std::string_view key) { return mission_->Get(key); },fort,client_.Assets().TypeTable(),6500); //10.72 전체 State 복원 전까지 기존 표시 입력을 유지한다.
    }
    // 1-1/1-2도 원본 missionType은 Tutorial이다. 지원 두 종류 모두 같은 실제 raw 월드를 생성한다.
    auto services=MakeCampaignClientServices(client_,*rawButtons_,{
        [this] { RefreshWindowTitle(); },[this](std::string_view key) { Tell(std::string(key)); },
        [this](std::int32_t kind,std::int32_t player,std::int32_t parameter) { warnings_->Remove(kind,player,parameter);rebuild_=true; },[this](auto packet) { ReceiveChatMessage(packet); }}); // 현재 클라이언트의 설정·음향·입력·UI다.
    services.adoptInitializedPlayers=nativePlayers; //앞선 실제 cleanup의 같은9기록을 대여하며 생성자에서 두 번째로 재할당하지 않는다. 일반 World/로그인 어댑터의 나머지 기존 경계는 유지한다.
    if (nativePlayers) services.localLogin->startingProgress=[this] { rawButtons_->Menus()->ShowMissionLoading(LoadingHtmlContext({85,0,client_.GetScreen().Width(),client_.GetScreen().Height()}),FontBytes(client_.Translate("Starting Mission...")),[this] { return FontBytes(client_.Translate("Cancel")); });rebuild_=true; }; // 실제 인자20/즉시8 예약 뒤 만들며 중첩 Compose/Present/프레임 대기를 추가하지 않는다.
    if (nativePlayers) services.localLogin->creatingIslands=[this] { rawButtons_->Menus()->ShowMissionLoading(LoadingHtmlContext({85,0,client_.GetScreen().Width(),client_.GetScreen().Height()}),FontBytes(client_.Translate("Creating Islands.")),[this] { return FontBytes(client_.Translate("Cancel")); });rebuild_=true; }; // 원문cf570의 같은 안내 serial 재사용/본문 갱신을 사용한다.
    services.factoryHelp=[this](std::string_view section,std::uint32_t mode) { const auto token=scriptStrings_.Store(section);if (token>0x0fffffffU) throw std::overflow_error("Factory 도움말 문자열 별칭 토큰 소진");rawButtons_->Menus()->OpenHelp(0x70000000U|token,static_cast<std::int32_t>(mode),0); }; //기존Help와같은문자열고리/별칭과실제전용창을사용한다.
    services.missionOutcome.demo=[this] { return missionRuntime_ ? missionRuntime_->gate.demo : 0U; };
    services.missionOptions=missionOptions_;services.missionRestrictions=missionRestrictions_;services.missionFortRequest=missionFortRequest_;services.templateStorage=templateStorage_; // 같은 Init/요청/Template 소유자만 전달하며10.78의 파일 읽기를 State7에 추가하지 않는다.
    if (nativePlayers) {
        services.missionFortFileSelect=[this,select=std::move(fortFileSelect)](auto) {
            const auto current=client_.FortGlobals()->Current();if (current.empty()) throw std::logic_error("현재 Fort 파일 선택의 원문 빈 이름 진단");
            return select(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(current.data()),current.size()})); //00460680은 각 지역/서버 읽기 시점의 같은 curFort를 재조회하며 최초 요청 이름으로 고정하지 않는다.
        };services.missionFortLoad.readOnly=[this](auto path) { return client_.Files().ReadOnly(path); };
        if (!mission_->FileSource()) services.missionFortLoad=BindDataManagerFileRead(std::move(services.missionFortLoad),client_.Files()); //일반10.78만 연결하며 명시Editor 작업본/삭제 장부 namespace는 유지한다.
        services.missionFortLoad.tooManyTypes=[] { throw std::logic_error("미션 Template 파일의 numTypes<256 진단"); };
        services.missionFortLoad.rejectFile=[](auto path) { throw std::runtime_error("미션 Template 잘못된 서명의 원본 삭제 요청: 보호 파일은 삭제하지 않습니다: "+std::string(path)); }; // 원본 삭제 요청을 보호 자산에 자동 적용하지 않는다.
        services.remoteFortFiles=std::shared_ptr<EditorFiles>(editorCatalog_,&editorCatalog_->Files()); // 같은 장기 작업 namespace를 빌리며 수신 자료를 설치 자산에서 복사하지 않는다.
        services.remoteFortPath=[this](auto owner,auto localOwner) {
            const auto configured=SelectRemoteFortPath(*client_.FortGlobals(),client_.Configuration(),owner,localOwner,[](auto expression) { std::clog<<"Fortspec 원본 진단: "<<expression<<'\n'; });
            return platform::PathToUtf8(editorCatalog_->Resolve(configured)); // 원문 경로 조회 뒤에만 보호 검사와 사용자 홈 매핑을 수행한다.
        };
    } else { services.missionFortImage=fortImage;services.missionFortFileName=std::move(fortFile.fileName);services.missionFortRead=mission_->FortReader(); } //10.72의 기존 부분 어댑터만 동기 입력을 유지한다.
    services.fortBadges=client_.FortBadges();services.fortMission=FortMissionSaveEffects(); // 실제 Client 단일 장부와 현재 Runtime getter를 같은 Session에 빌린다.
    services.fortTypeConversion=client_.FortTypeConversion(); // 같은 Client의 현재 타입 변환표 소유자를 전달한다.
    services.missionOutcome.tellBlocked=[this] { return state_.phase==ClientPhase::Dialog || state_.phase==ClientPhase::Briefing ? 1U : 0U; };
    services.missionOutcome.section=[this](std::string_view section) { bool created=false;return mission_ && mission_->Section(section) && TryTell(std::string(section),false,&created) && created; };
    services.missionOutcome.closeTransient=[this] { rawButtons_->RequestClosePopups(); };
    services.missionOutcome.onSection=[this] {
        if (!missionRuntime_ || !missionRuntime_->tutorial) throw std::logic_error("현재 미션의 프레임 정지 연결 누락");
        missionRuntime_->tutorial->Gate().PauseForDialog();
    };
    tutorialSeen_=0; // 원본 State7의 세계 재설정4b2df0도 Init의5ca8f0을0으로 되돌린다. 사제 체력을 앞선1쓰기만으로 바꾸지 않는다.
    auto world=std::make_shared<GameWorld>(client_.Assets(),client_.Files(),std::move(fort),std::move(players),
        selectedFort,std::move(services),ScreenPoint{client_.GetScreen().Width(),client_.GetScreen().Height()},true);
    world->elapsed=[this]() { return client_.Time().delta; }; world->paused=[this]() { return client_.Paused(); };
    world->frameTime=[this]() { return client_.Time(); };
    world->BindHideBuildings([this] { return hideBuildings_; }); // 메뉴 체크·현재 Draw·선택 후보가 같은 원본 DWORD를 조회한다.
    // 카메라/해상도 변경 직후 위치 효과음의 기준을 갱신한다. 월드와 소리 묶음의 수신은 모두 주 스레드에서 실행한다.
    world->soundViewChanged=[this](SoundView view) { client_.WorldAudio().SetView(view); };
    // 장치 재생성 뒤에도 현재 Renderer로 raw 변경 영역을 전달한다.
    world->surfaceDisplay={
        [this]() { return client_.GetRenderer().FullRedrawPending(); },
        [this](o::SquidDisplayRect rect,std::uint32_t flags) { client_.GetRenderer().Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }};
    world_=std::move(world); // 호스트 어댑터는 원문 프로세스 슬롯을 차지하지 않는다.
    if (nativePlayers) world_->Session().BindConnectionTransport(MakeClientCampaignTransport(client_,CompositeTransportRole::InternalMission,world_),world_); // 실제 World 전체를 콜백 동안 유지하며 생성/바인딩으로 접속을 앞당기지 않는다.
    worldMissionToken_=missionRuntime_->saveToken; // 준비한 World를 현재 미션 수명에만 귀속한다.
    if (missionRuntime_->mode64) { if (!editor_) throw std::logic_error("편집 Test Battle의 실제 작업 파일 owner 없음");world_->Session().BindFortFileSave(client_.ReadOnlyForts(),editor_->files); } //테스트 복귀의 선택된 Notify도 실제 작업본 경로에만 저장한다.
    missionRuntime_->start=std::make_unique<RawCampaignMissionStartBindings>(world_->Session(),*mission_,missionRuntime_->gate,missionRuntime_->state,
        world_->Session().MissionStartSettings(),world_->Session().MissionAiSettings(),MakeCampaignMissionStartEffects(client_,*world_,[this] {
            if (commandGump_!=0) static_cast<void>(rawButtons_->BubbleGump(commandGump_,0x02000088U,0,0)); //0042e440은현재Chat의부모에실제숨김명령을보내며0이면생략한다.
        }));
    if (nativePlayers) { world_->PrepareLocalConnectionForState();loading.continued=true;return; } // 같은 부모로 복귀한 뒤7 확정/즉시8을 실행하고 다음9는 자연 Tick에 남긴다.
    world_->BeginLocalConnection();loading.continued=true;FinishMissionWorldLoad(); //10.72의 기존 동기 어댑터와 새10.78 상태 경로를 구별한다.
}
// 실제 연결/로더가 완료된 같은 미션만 입력·Tutorial·Start의 소비자를 얻는다.
void UberGump::FinishMissionWorldLoad() {
    if (!mission_ || !missionRuntime_ || !world_ || worldMissionToken_!=missionRuntime_->saveToken || !world_->Session().LocalConnectionReady()) throw std::logic_error("미션 완료 설치의 실제 접속/현재 수명 계약 오류");
    struct LoadingRetirement { // State13이 닫은 안내와 완료 설치 실패의 잔여 안내를 구별한다.
        RawMenuControls& menus; // World 반납보다 오래 사는 같은 실제 메뉴 소유자다.
        ~LoadingRetirement() { if (menus.MissionLoadingOpen()) menus.CloseMissionLoading(); } // 이미 닫힌 안내를 다시 반납하지 않고 실패의 잔여 노드만 정리한다.
    };
    LoadingRetirement loading{*rawButtons_->Menus()};const auto type=o::AsciiLower(mission_->MissionType()); // 완료 사이 다른 Script의 형식을 사용하지 않는다.
    fortObjects_=world_->Session().World().Objects().size(); // 진단 mission 행은 선행 저장 레코드 수 대신 실제 설치 완료 시점의 월드 객체 수를 관측한다.
    if (type=="tutorial" && !missionRuntime_->guided) ApplyMissionTechnology(*mission_,client_.Assets().TypeTable(),world_->Session().TotalMade()); // guided Header는 이미 설치 전 같은 전역 표를 썼다.
    userInput_=std::make_unique<UserInput>(client_,*world_);
    // 브리핑으로 게임 시계가 멈추기 전에도 첫 카메라/표시 영역이 음악·효과음에 전달되게 한다.
    world_->Resize(client_.GetScreen().Width(),client_.GetScreen().Height());
    // 직전 미션의 날씨 팔레트가 유지된 재진입도 현재 화면 색을 사용한다.
    world_->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical(),&client_.ColorSortInput()->Maps());
    // 같은 단일 커서에 현재 장치/트리·비용 표시·선택 해제·실제 로그인 전송을 공급한다.
    auto cursorEffects=MakeCampaignCursorClientEffects(client_,*rawButtons_);
    cursorEffects.send=[session=&world_->Session()](std::uint32_t target,std::span<const std::uint8_t> packet) { session->SendLocalPacket(std::bit_cast<std::int32_t>(target),packet); };
    BindCampaignUserInputServices(client_,*world_,cursorEffects.lifecycle.selectCursor,[this](auto section) { Tell(std::string(section)); });
    cursorEffects.lifecycle.highlight=[this](std::uint32_t mode) { world_->Highlight(mode); };
    cursorEffects.drop.refresh=[this](std::uint32_t sid,std::uint32_t force,std::uint32_t move) {
        // 원본Drop의004d5c90(0,0,1)을 같은 실제 선택으로 전달한다. Select 파사드는 둘째0/셋째1을 보존한다.
        if (force || move!=1) throw std::logic_error("현재 커서 선택 변경의 실제 인자 계약 오류");world_->Select(sid);
    };
    world_->Session().Cursor().SetEffects(std::move(cursorEffects));
    world_->Session().Cursor().Display().SetExternal([this](std::uint32_t token) { return rawButtons_->GumpDepth(token); },{});
    briefingSections_.clear(); briefingIndex_ = 0;
    if (type=="tutorial") {
        CampaignTutorialOneEffects frameEffects; // 원본 화면 프레임/입력이 함께 사용하는 현재 UI/시계다.
        frameEffects.frame.gameSeconds=[this] { return client_.Time().game; };
        frameEffects.frame.paused=[this] { return client_.Paused(); };
        frameEffects.frame.dialogsClear=[this] { return rawButtons_->CurrentDialog()==0; };
        frameEffects.frame.knowledgeOpen=[this] { return world_->Session().CursorDropSettings().lockDepth==0; };
        frameEffects.frame.guideOpen=[this] { return client_.HtmlWindowGlobals()->window.instances!=0; }; // 원본005cab2c는 Kill 뒤에도 실제 소멸까지 창 개수를 유지한다.
        frameEffects.frame.blockingGumpOpen=[this] { return rawButtons_->BlockingGumpOpen(); };
        frameEffects.frame.pause=[this] { client_.PushGamePause(); };
        frameEffects.frame.resume=[this] { client_.PopGamePause(); };
        frameEffects.frame.endDemo=[this] { client_.Input().Push(kMissionReloadCommand,0,0); }; // 원본452f90의 일반 큐 예약이다. 실행 중인 프레임 콜백 안에서 월드를 해제하지 않는다.
        frameEffects.tell=[this](std::string_view section) { bool created=false;return mission_->Section(section) && TryTell(std::string(section),true,&created) && created; };
        frameEffects.closeTransient=[this] { rawButtons_->RequestClosePopups(); };
        // 현재 월드의 선택 getter와 서로 다른 두 원본 선택 정리 몸체를 빌린다.
        frameEffects.selected=[this] { return world_->Selected(); };frameEffects.resetSelection=[this] { world_->ResetSelection(); };frameEffects.clearSelection=[this] { world_->ClearSelection(); };
        missionRuntime_->tutorial=std::make_unique<RawCampaignTutorialOneBindings>(world_->Session(),*mission_,*missionRuntime_->start,
            missionRuntime_->gate,missionRuntime_->state,std::move(frameEffects));
    }
    // 원본 초기 브리핑은 A.이며 A1. 등은 본문 Tell 명령이 넘긴다.
    if (missionRuntime_->gate.demo!=0) briefingSections_.push_back("Demo");
    else if (mission_->Section("A.")) briefingSections_.push_back("A.");
    state_.phase = ClientPhase::LoadingMission; pageName_ = "loading"; popup_.clear(); ClearPage(); RebuildControls();
    RefreshWindowTitle();
    if (missionRuntime_->tutorial) {
        state_.phase=ClientPhase::Mission;pageName_="mission";briefingSections_.clear();client_.Pause(false); // A/Demo 안내는 공통10호출 뒤 실제 TellStage가 연다.
    } else client_.Pause(true);
    // 원본 004b6dd0의 미션 시작 절차(전투 여부를 켜고 FUN_00469fc0): 난수로 고른 원소 곡으로 전투 음악을 시작한다.
    if (client_.Assets().Edition()!=o::OriginalEdition::Patch1078 && client_.Audio()) client_.Audio()->StartScene(true); //10.78은 실제 State7 앞단에서 이미 실행했고 기존 CD 어댑터만 이전 위치를 유지한다.
    // 원본004b6dd0의 음악 시작 뒤00463a40→가상+28은 Start가 아닌 자기 정지다.
    if (missionRuntime_->guided && client_.Assets().Edition()!=o::OriginalEdition::Patch1078) {
        PauseMissionWorldEntry([this] { return missionRuntime_!=nullptr; },[this] { missionRuntime_->tutorial->Gate().PauseForDialog(); });
        // 다음 단계 flags는 원래 Init의 로드 요청/Header 뒤 이미 썼으며 여기서 다시 쓰지 않는다.
    }
    if (!missionRuntime_->guided) StartMissionWorld(); // 기존 Battle의 시작도 같은 접속 완료 뒤 한 번만 실행한다.
}
// 현재 설치된 같은 Session/Script와 장기 순번으로 원본 단계3 Start를 한 번 실행한다.
void UberGump::StartMissionWorld() {
    if (!missionRuntime_ || !missionRuntime_->start || !world_) throw std::logic_error("Guide 단계3의 실제 설치/Start 연결 누락");
    missionRuntime_->start->Start();
}
// 첫 브리핑을 표시하거나 브리핑 종료 뒤 실제 월드 입력과 표시를 활성화한다.
void UberGump::AdvanceBriefing() {
    if (briefingIndex_ < briefingSections_.size()) { Tell(briefingSections_[briefingIndex_++], true); return; }
    state_.phase = ClientPhase::Mission; pageName_ = "mission"; ClearPage(); popup_.clear(); client_.Pause(false);
    world_->Resize(client_.GetScreen().Width(),client_.GetScreen().Height());RebuildControls(); //임시 키 보관 상태를 제거했으므로 새 크기의 실제 폴링만 다음 입력에서 읽는다.
}
// 저장한serial은루트등록칸이나단순인스턴스수로대체하지않는다.
bool UberGump::GameMenuOpen() const { return gameMenuState_ && rawButtons_->SerialGump(gameMenuState_->menuSerial)!=0; }
// 004d958c의serial조회와Kill/0쓰기/66710순서를유지한다.
void UberGump::ToggleGameMenu() {
    if (!InputSession()) return; //메타도 실제 준비된 Session과 같은 메뉴 serial을 사용한다.
    const auto token=gameMenuState_ ? rawButtons_->SerialGump(gameMenuState_->menuSerial) : 0U; // 현재비dead인동일serial만선택한다.
    if (!token) OpenGameMenu();
    else { rawButtons_->KillGump(token);gameMenuState_->menuSerial=0;rawButtons_->RequestClosePopups();rebuild_=true; }
}
// 같은 원문 Options 작성기의 현재 설정/장치/모드만 공급한다. ESC의 Session getter 결합은 기존 작성 위치에 남긴다.
GameMenusHooks UberGump::CurrentGameMenuHooks() {
    GameMenusHooks hooks; // 메인과 ESC가 같은 Client 전역을 각 원문 조회 시점에 읽는다.
    hooks.global=[this](std::uint32_t address)->std::uint32_t {
        if (const auto value=GameMenuCommandValue(address)) return *value;
        switch (address) {
        case 0x563e38:return rawButtons_->RootToken();
        case 0x531860:case 0x5c78f0:return static_cast<std::uint32_t>(client_.GetScreen().Width());
        case 0x531864:case 0x5c78f4:return static_cast<std::uint32_t>(client_.GetScreen().Height());
        case 0x5c78d4:return client_.GetScreen().Flags();
        case 0x5c7910:return 1; // 원본 DirectDraw 설치 관문 대신 Windows10/11 호환 전체화면 기능을 메뉴에 공개한다.
        case 0x5c7b14:return client_.WorldAudio().DeviceReady() ? 1U : 0U;
        case 0x531900:return std::bit_cast<std::uint32_t>(client_.Configuration().GetInt("soundVolume"));
        case 0x531904:return std::bit_cast<std::uint32_t>(client_.Configuration().GetInt("musicVolume"));
        case 0x55b4b0:return client_.Paused() ? 1U : 0U; // 이소비자는원본깊이의양수여부만검사한다.
        case 0x59a8ac:return hideBuildings_;
        case 0x5c85a4:return client_.GameplayGlobals()->world.editor;
        case 0x594fa4:return client_.GameplayGlobals()->world.missionPost;
        case 0x594fb8:return client_.SurfaceStorage()->ownerMode.fort;
        case 0x594fbc:return client_.SurfaceStorage()->ownerMode.battle;
        case 0x594fc8:return client_.SurfaceStorage()->ownerMode.challenge;
        default:throw std::logic_error("게임메뉴현재전역미연결: "+std::to_string(address));
        }
    };
    hooks.config=[this](auto key) { return client_.Configuration().GetInt(key); };
    hooks.readInt=[this](auto key,std::int32_t& value) { client_.Configuration().ReadInt(key,value); };
    hooks.translate=[this](auto text) { return FontBytes(client_.Translate(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };
    hooks.editingMission=[this] { return missionRuntime_ ? missionRuntime_->mode64 : 0U; };hooks.demo=[this] { return missionRuntime_ ? missionRuntime_->gate.demo : 0U; }; //원문 Guide getter는 현재 미션이 없으면0을 돌려준다.
    hooks.compatibilityResolutions=kCompatibilityScreenResolutions; // 원본과 다른 Windows10/11 호환 화면비 추가이며 기본 원문 세 해상도는 유지한다.
    hooks.compatibilityLanguages=LanguageMenuChoices(client_.LanguageNumber()); // 6차: 옵션 메뉴 Language 하위 메뉴(영어/한국어)다.
    ConnectPostProcessMenu(hooks); // 7차: 후처리 장치가 있으면 FSR 해상도와 Anti-Aliasing/FSR 토글을 추가한다.
    return hooks;
}
// 원문004cebf0의 메인 Help/Options를 실제 클릭 버튼과 공통 메뉴/모달/명령 큐에 연결한다.
void UberGump::OpenMainMenuPopup(bool options) {
    std::uint32_t anchor{}; // 원문 버튼 배열 대신 현재 화면 번호와 실제 노드 사이의 대응만 찾는다.
    for (std::size_t index=0;index<actions_.size();++index) { // 현재 메인 버튼의 실제 명령·인자를 조회하고 생성 순서나 토큰 산술은 쓰지 않는다.
        const auto& action=actions_[index];if (o::AsciiLower(action.command)=="menu" && action.argument==(options ? "options" : "help")) { anchor=rawButtons_->ControlGump(static_cast<int>(index));break; }
    }
    if (!anchor) throw std::logic_error("메인 Help/Options의 현재 버튼 없음");
    auto& menus=*rawButtons_->Menus();menus.SetPalette(client_.GetScreen().Colors());
    menus.OpenMainMenuPopup(anchor,options,[&](const auto& widgets,const auto& controls,const auto& menu) {
        auto hooks=CurrentGameMenuHooks();BindGameMenusControls(hooks,widgets,controls,menu,[this](auto text) { return scriptStrings_.Store(text); });
        if (options) return RawGameMenus(std::move(hooks)).Options();
        widgets.BeginVertical(0);
        const auto topic=scriptStrings_.Store("F1Help");if (topic>0x0fffffffU) throw std::overflow_error("메인 Help 주제 별칭 소진"); // 원본 문자열 포인터를 TYPE<256과 혼동하지 않는 기존 Help 별칭 계약이다.
        controls.AddText(hooks.translate("General Help - ~iF1~."),0x02000017U,0x70000000U|topic,true,true,false);
        const auto technical=hooks.translate("Technical Help");controls.AddText(technical,0x02000049U,0,true,ClientTechnicalHelpAvailable(client_),false);
        controls.AddText(hooks.translate("Version"),0x02000095U,scriptStrings_.Store("About"),true,true,false);
        return menu.TakeCurrent();
    });
    rebuild_=true; // 실제 팝업 수명은 공통 Gump가 맡는다. 메인 버튼을 다시 생성하거나 별도 중앙 목록을 설치하지 않는다.
}
// 실제현재Session의조회/검산과기존공통controlled메뉴를한수명안에서작성한다.
void UberGump::OpenGameMenu(bool playerList) {
    auto* current=InputSession();if (!current || state_.phase!=ClientPhase::Mission) throw std::logic_error("게임메뉴의 현재 실제 Session 없음"); //Editor/메타도 같은 장기 저장소와 원본 메뉴를 사용한다.
    auto& session=*current; //원본cc210은 앞서 열린 다른 팝업을 임의로 닫지 않는다.
    if (!gameMenuState_) gameMenuState_=std::make_unique<GameMenuBarState>(session.GameMenuTrackingOffset());
    rawButtons_->Menus()->SetPalette(client_.GetScreen().Colors());
    rawButtons_->Menus()->OpenAttached([&](const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& menu) {
        auto hooks=CurrentGameMenuHooks();
        hooks.playerMenu=[&](auto subscriber) { if (!world_) throw std::logic_error("비메타 Player 하위의 현재 GameWorld 없음");return objectMenus_->BuildPlayerMenu(*world_,subscriber,widgets,controls,menu); }; //메타 Players는 원문처럼 작성 전에0을 반환한다.
        BindGameMenusControls(hooks,widgets,controls,menu,[this](auto text) { return scriptStrings_.Store(text); });
        hooks.node=[this](auto token) { return rawButtons_->Menus()->Node(token); };
        hooks=session.BindGameMenus(std::move(hooks));const auto globals=hooks.global;const auto translate=hooks.translate; // 바인더가교체한같은현재소유자의getter를빌린다.
        hooks.compatibilityResolutions=kCompatibilityScreenResolutions; // 원본과 다른Windows10/11 DIB 호환 화면비만 명시 추가하며 기본 원문 검사는 빈 목록을 유지한다.
        hooks.compatibilityLanguages=LanguageMenuChoices(client_.LanguageNumber()); // 6차: 게임 중 옵션 메뉴에도 같은 Language 하위 메뉴를 둔다.
        ConnectPostProcessMenu(hooks); // 7차: 게임 중 옵션 메뉴에도 같은 후처리 항목을 둔다.
        RawGameMenus children(std::move(hooks));
        if (playerList) return children.Players(true); // 원본004d8a56은 명령 인자와 무관하게 fancy=1을 넘기며 현재 루트 중앙에 직접 부착한다.
        GameMenuBarHooks bar; // 가로막대만 현재 serial과 실제 미니맵 왼쪽 경계를 사용한다.
        bar.prepare=[&session] { session.PrepareGameMenus(); };bar.translate=translate;
        bar.read=[this,globals](GameMenuBarValue value) {
            switch (value) {
            case GameMenuBarValue::Editor:return globals(0x5c85a4);
            case GameMenuBarValue::Battle:return globals(0x594fbc);
            case GameMenuBarValue::Mission:return globals(0x594fa4);
            case GameMenuBarValue::Cheat:return globals(0x5cab30);
            case GameMenuBarValue::Width:return static_cast<std::uint32_t>(client_.GetScreen().Width());
            case GameMenuBarValue::Left:return std::bit_cast<std::uint32_t>(CurrentMapView().left); //005ca99c는 같은 공통 HUD가 도킹한 현재 지도 원점이다.
            case GameMenuBarValue::Root:return rawButtons_->RootToken();
            }
            throw std::logic_error("게임메뉴조회종류오류");
        };
        bar.about=[this] { const auto value=client_.Configuration().Configuration().Get("helpWord");if (!value) throw std::logic_error("게임메뉴현재helpWord포인터없음");return FontBytes(*value); };
        bar.technicalHelp=[this] { return ClientTechnicalHelpAvailable(client_) ? 1U : 0U; };
        bar.child=[&](GameMenuBarChild child) {
            switch (child) {
            case GameMenuBarChild::Game:return children.Game();case GameMenuBarChild::Edit:return children.Edit();
            case GameMenuBarChild::View:return children.View();case GameMenuBarChild::Options:return children.Options();
            case GameMenuBarChild::Players:return children.Players();
            case GameMenuBarChild::Cheat:return rawButtons_->Menus()->CreateCheatMenu(translate,[globals] { return globals(0x54287c); });
            }
            throw std::logic_error("게임메뉴하위종류오류");
        };
        BindGameMenuBarControls(bar,widgets,controls,menu,[this](auto token) { return rawButtons_->Menus()->Node(token); },[this](auto text) { return scriptStrings_.Store(text); });
        return RawGameMenuBar(*gameMenuState_,std::move(bar)).Open();
    },false);
    rebuild_=true; // 실제메뉴열기는게임시계를정지하거나별도중앙팝업상태를만들지않는다.
}
// 현재 복원 범위에서 의미가 있는 원본 옵션을 구성한다. 소리 장치와 전체화면 항목은 후속이다.
// 공통 입력/Guide가 소비하지 않은 현재 메뉴 명령을 같은 Session·음향·설정·시계에 전달한다.
bool UberGump::GameMenuCommand(const InputEvent& event) {
    const auto code=event.code&0x03ffffffU;
    if (IsConfigurationCommand(code)) {
        if (event.code&InputCode::kRelease) return true; //원문 공통 뗌은 현재 장기 owner/State/Config 장치를 조회하지 않는다.
        auto& game=*client_.GameplayGlobals();auto& modes=client_.SurfaceStorage()->ownerMode;ConfigurationCommandHooks configured;
        configured.text=[this](auto token) { return scriptStrings_.Resolve(token); };
        configured.set=[this](auto key,auto value) { static_cast<void>(client_.Configuration().Set(key,o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}))); };
        configured.get=[this](auto key) { return FontBytes(client_.Configuration().PathSpec(key)); }; //현재 Config 치환값을 다시 원문 palette 이름 바이트로 넘긴다.
        configured.palette=[this](auto name) { const std::string owned(name);client_.ChangeFortPalette({reinterpret_cast<const std::uint8_t*>(owned.c_str()),owned.size()+1}); }; //공유 설정 고리의 재진입과 별개인 NUL 수명이다.
        configured.thread=[this](auto priority) { client_.SetNativeThreadPriority(priority); };
        configured.config=[this](auto assignment) { ApplyScriptConfig(assignment,client_.Configuration()); };
        configured.state=[this](auto slot) { if (!world_) throw std::logic_error("SetFortType의실제State Session 없음");return world_->Session().MutableFortState(slot); }; //같은 Client TemplateStorage의 새 현재 manager/State를 getter가 조회한다.
        configured.notify=[this](auto slot) { if (!world_) throw std::logic_error("SetFortType의실제Notify Session 없음");world_->Session().BindFortCurrentName({}).notify(slot); }; //콜백마다 현재 Session을 읽고 같은 기존 Notify/파일 장치를 쓴다.
        configured.assertion=[](auto line) { std::clog<<"원문 SetFortType 입력 진단 "<<line<<'\n'; }; //원문 진단의 정상 복귀를 일반 모드 거부로 바꾸지 않는다.
        static_cast<void>(DispatchConfigurationCommandInput(event,{[&modes] { return modes.battle ? 1U : 0U; },[&modes] { return modes.challenge ? 1U : 0U; },[&game] { return game.world.editor; },[&game] { return game.player.templateOwner; },client_.ThreadPriorityValue()},configured));Refresh();return true;
    }
    if (DispatchSetLanguage(event,[this](auto language) { client_.SetLanguage(language); })) return true; //현재 Gump/Guide 뒤 원문 일반 입력 위치다.
    if (code==0x020000f0U) { if (!(event.code&InputCode::kRelease)) OpenGameMenu(true);return true; } // 원본 일반 분배의 뗌 거부 뒤 같은 Player List 전체 부모를 연다.
    if (code==0x02000030U) {
        if (!(event.code&InputCode::kRelease)) {
            client_.ChangeScreenMode(std::bit_cast<std::uint32_t>(event.y));
            if (!popup_.empty()) { popup_.clear();ClearPage();if (world_ && state_.phase==ClientPhase::Mission) client_.Pause(false);RebuildControls(); } // 호환 모드 전환 때 닫힌 호스트 Options의 입력 목록도 같이 반납한다.
            ++menuConfigRevision_;Refresh();
        }
        return true;
    } // 원본 절대 모드 값은 같은 입력 큐 뒤 실제 호환 Screen 장치로 전달한다.
    if (code==0x02000002U) { if (!(event.code&InputCode::kRelease)) { const auto previous=client_.GetScreen().Flags();client_.ChangeScreenMode(previous^std::bit_cast<std::uint32_t>(event.y));Tell("DirectDrawVerify("+std::to_string(previous)+")"); }return true; } // 원본 조정도 같은435ba0 부모를 거치고15초 Tell/Cancel 절대 복구 경로를 사용한다.
    if (code==0x020000e4U) { if (!(event.code&InputCode::kRelease)) { client_.AcceptScreenMode();Tell("AdjustDirectDraw"); }return true; } // 원본 확인 뒤에만 현재 전체화면 모드를 기억하고 같은 조정 창으로 돌아간다.
    if (code==0x020000beU) {
        if (state_.phase==ClientPhase::Dialog) Tell("Blank"); // 원본 대화 부모의 MenuJustClosed→Kill에 대응하는 페이지 수명 정리다.
        rawButtons_->RequestClosePopups();rebuild_=true;return true;
    }
    if (code==0x0200006dU) {
        if (event.code&InputCode::kRelease) return true;
        if (event.y) {
            ApplyScriptConfig(scriptStrings_.Resolve(std::bit_cast<std::uint32_t>(event.y)),[this](auto key,auto value) {
                const auto decode=[](std::string_view bytes) { return o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()}); }; // 실제 설정의 내부 인코딩 경계만 변환한다.
                return client_.Configuration().Set(decode(key),decode(value));
            });
            client_.ApplyAudioOptions(); // 현재 복원된 옵션 장치에 실제 효과음·음악 값을 반영한다.
            client_.ApplyPresentationOptions(); // 7차: 안티앨리어싱/FSR 토글도 바로 반영한다.
        }
        ++menuConfigRevision_;worldRedraw_=1;rawButtons_->RouteInput(event);Refresh();return true; // 원본 이 분기의 두 번째466200 호출을 보존한다.
    }
    if (IsPresentationCommand(code)) {
        PresentationCommandHooks effects;effects.fontFace=[this](auto index) { return client_.Fonts().FaceAt(index); };effects.rebuildFonts=[this](auto face) { RebuildPresentationFonts(client_.Fonts(),client_.GetRenderer(),face); }; // 글리프 주소 재사용 전에 실제 Renderer 캐시를 반납한다.
        effects.width=[this] { return client_.GetScreen().Width(); };effects.height=[this] { return client_.GetScreen().Height(); };
        effects.invalidate=[this](auto left,auto top,auto width,auto height,auto flags) { client_.GetRenderer().Invalidate({left,top,width,height},flags);rebuild_=true; }; //새 글꼴을 쓰기 전에 같은 호스트 장면을 재수집한다.
        effects.currentFontFace=[this] { return client_.Fonts().CurrentFace(); };effects.set=[this](auto key,auto value) { client_.Configuration().Set(std::string(key),FontFaceToUtf8(value));client_.ApplyAudioOptions(); }; //4414d0의 현재 Config와 기존 음향 재해석을 동기 사용한다.
        effects.closeSound=[this] { client_.WorldAudio().Shutdown(); };effects.config=[this](auto assignment) { ApplyScriptConfig(assignment,client_.Configuration());client_.ApplyAudioOptions();client_.ApplyPresentationOptions(); };
        effects.sound=[this](auto name,auto a,auto b,auto c,auto d,auto e) { client_.WorldAudio().Sounds().PlayByName(name,a,b,c,d,e); };
        return DispatchPresentationCommandInput(event,effects); //release/정지/모달/실패의 관문은 원문의 해당 부모만 실행한다.
    }
    if (!IsGameMenuCommand(code)) return false;
    if (DispatchTechnicalHelpInput(event.code,[this] { static_cast<void>(LaunchClientTechnicalHelp(client_)); })) { Refresh();return true; } //원문 일반 명령은 현재 설정을 다시 해석하고435f10을 실행한다.
    auto* inputSession=InputSession();auto binding=inputSession ? inputSession->BindGameMenus({}) : GameMenusHooks{}; //메타도 같은 실제 Player/SP/모드 getter를 빌린다.
    if (!inputSession) binding.global=[this](std::uint32_t address)->std::uint32_t {
        const auto& gameplay=*client_.GameplayGlobals();const auto& surface=*client_.SurfaceStorage(); // 월드 반납 뒤에도 같은 Client 소유자의 현재 모드/Player를 읽으며 초기0을 합성하지 않는다.
        switch (address) {
        case 0x5c85a4:return gameplay.world.editor;case 0x594fb8:return surface.ownerMode.fort;case 0x594fc8:return surface.ownerMode.challenge;
        case 0x594fa4:return gameplay.world.missionPost;case 0x595344:return gameplay.player.playerKnown;case 0x540c70:return gameplay.player.battleOwner;
        default:throw std::logic_error("월드 밖 메뉴의 다른 현재 전역 미연결");
        }
    };
    const auto session=[this]()->RawCampaignSession& { auto* current=InputSession();if (!current) throw std::logic_error("현재 메뉴 명령의 Session 없음");return *current; }; //필요한 분기에서만 현재 GameWorld/MetaMap owner를 다시 조회한다.
    GameMenuActionHooks hooks;
    hooks.command=[](auto command) { const auto value=GameMenuCommandValue(static_cast<std::uint32_t>(command));if (!value) throw std::logic_error("메뉴 명령의 현재 원문 ID 없음");return *value; };
    hooks.queue=[this](auto command,auto first,auto second) { client_.Input().PushCommand(command,first,second); };
    hooks.tell=[this](auto name) { Tell(std::string(name)); };
    hooks.config=[this](auto assignment) { ApplyScriptConfig(assignment,client_.Configuration());client_.ApplyAudioOptions();client_.ApplyPresentationOptions(); };
    hooks.sound=[this](auto name,auto a,auto b,auto c,auto d,auto e) { client_.WorldAudio().Sounds().PlayByName(name,a,b,c,d,e); };
    hooks.text=[this](auto token) { return scriptStrings_.Resolve(token); }; // 실제 HTML 명령이 저장한 같은 고리 문자열을 PlaySound 소비 순간에 읽는다.
    hooks.paused=[this] { return client_.Paused(); };
    hooks.status=[this](auto kind,auto owner,auto enabled,auto parameter,auto text) {
        if (kind!=11 || owner || parameter || !text.empty()) throw std::logic_error("다른 메뉴 상태 안내 분기 미연결");
        warnings_->SetView(client_.GetScreen().Width(),client_.GetScreen().Height(),client_.GetScreen().Colors());warnings_->PauseStatus(enabled!=0);
    };
    hooks.cancelCursor=[session] { session().Cursor().Cancel(); };
    hooks.pauseClock=[this] { client_.PushGamePause(); };hooks.resumeClock=[this] { client_.PopGamePause(); };
    hooks.muteSound=[this] { client_.WorldAudio().PushMute(); };hooks.resumeSound=[this] { client_.WorldAudio().PopMute(); };
    hooks.player=binding.player;hooks.centerObject=[this,session](auto sid) { static_cast<void>(session());if (world_) world_->CenterOnObject(sid);else if (editor_ && editor_->metaInput) editor_->metaInput->CenterOn(sid);else throw std::logic_error("메뉴 카메라의 현재 입력 owner 없음"); };
    hooks.viewPriest=[this](auto owner) { if (world_) world_->ViewPriest(owner);else if (editor_ && editor_->metaInput) editor_->metaInput->ViewPriest(owner); }; //일반 F5와 명시 메뉴 owner를 구별하며 메타도 실제 참여 조회를 사용한다.
    hooks.reloadCount=[this,session](auto list) { return world_ ? session().GameMenuReloadCount(list==GameMenuReloadList::Second) : 0; };
    hooks.reloadSid=[session](auto list,auto index) { return session().GameMenuReloadSid(list==GameMenuReloadList::Second,index); };
    hooks.form=[session](auto sid)->std::span<const std::uint8_t> { if (sid>65535) throw std::out_of_range("메뉴 Form SID 폭");return session().Core().pool.Slot(o::Sid{static_cast<std::uint16_t>(sid)}); };
    hooks.reloadAt=[session](auto x,auto y,auto mode) { session().ReloadTheamAt(x,y,mode); };
    hooks.getFullDeck=[session] { session().GetFullDeck(); }; //선택된원문모드에서현재Session을다시조회한다.
    hooks.addLocalSp=[session](float amount) { session().MoneyInputHooks().addLocal(amount); }; //송금 입력과 같은 실제 로컬 가산 몸체를 빌리며 송금 패킷은 보내지 않는다.
    hooks.assertion=[](auto line) { throw std::logic_error("원본 메뉴 입력 진단: "+std::to_string(line)); };
    const auto result=GameMenuActions({binding.global,hideBuildings_,worldRedraw_},std::move(hooks)).Handle(event.code,std::bit_cast<std::uint32_t>(event.x),std::bit_cast<std::uint32_t>(event.y));
    if (!result) return false;if (*result) quit_=true;Refresh();return true;
}
void UberGump::OpenMenu(std::string name) {
    if (client_.Edition()==o::OriginalEdition::Patch1078 && !world_ && state_.phase==ClientPhase::MainMenu) {
        const auto key=o::AsciiLower(name);if (key=="help" || key=="options") { OpenMainMenuPopup(key=="options");return; }
    } // 10.78 메인 버튼은 원문 전용 팝업을 즉시 작성한다. CD/기존 호스트 보조 메뉴의 계약은 별도로 유지한다.
    if (o::AsciiLower(name)=="game" && InputSession()) { OpenGameMenu();return; } //미션/메타의 실제 수평 부모를 같은 현재 Session에 연결한다.
    popup_ = o::AsciiLower(name); ClearPage(); auto& config = client_.Configuration();
    // 목록 행에는 돌 버튼과 달리 누름 즉시 실행 규칙을 적용한다.
    const auto add = [this](std::string label, std::string command, std::string argument, bool checked = false, bool enabled = true) {
        page_.items.push_back({std::move(label), {std::move(command), std::move(argument)}, true, checked, enabled});
    };
    if (popup_ == "options") {
        if (client_.GetScreen().Flags()&ScreenMode::kFullScreen) { add("Adjust Direct Draw","Tell","AdjustDirectDraw");add("Window Mode","ScreenModeAbsChange","10"); }
        else add("Direct Draw / Full Screen","Tell","DirectDrawManager"); // 원본 Tell/설정의 전체화면 선택 값을 그대로 사용한다.
        add("Resolution >", "Menu", "resolution");
        add("Sound On", "Toggle", "sound", config.GetInt("sound") != 0);
        add("Play Music", "Toggle", "music", config.GetInt("music") != 0);
        add("Wind Noise", "Toggle", "playWind", config.GetInt("playWind") != 0);
        add("Speaker Swap L/R", "Toggle", "swapLeftRightSpeakers", config.GetInt("swapLeftRightSpeakers") != 0);
        add("Sound Effect Volume >", "Menu", "soundvolume"); add("Music Volume >", "Menu", "musicvolume");
        add("Edge Scroll in Fullscreen", "Toggle", "edgeScroll", config.GetInt("edgeScroll") != 0);
        add("Auto-Demo", "Toggle", "autoDemo", config.GetInt("autoDemo") != 0);
        add("Tell Tips at Startup", "Toggle", "tellTips", config.GetInt("tellTips") != 0);
        if (client_.PostProcessAvailable()) { add("Anti-Aliasing", "Toggle", "antiAliasing", config.GetInt("antiAliasing") != 0);add("FSR 1.0 Upscaling", "Toggle", "fsr1", config.GetInt("fsr1") != 0); } // 7차: 후처리 장치가 있을 때만 보인다.
        add("Pass Server Diagnostic", "", "", false, false);
    } else if (popup_ == "resolution") {
        for (const auto& size:kScreenResolutionChoices) { // 두1280폭도 높이까지 대조하고 ESC와 같은 원본 경고→Yes/No→저장/종료 흐름을 사용한다.
            const auto width=std::to_string(size[0]),height=std::to_string(size[1]);
            add(width+" by "+height,"Tell","SETSIZE("+width+','+height+')',client_.PresentWidth()==static_cast<int>(size[0]) && client_.PresentHeight()==static_cast<int>(size[1]));
        }
        if (client_.PostProcessAvailable()) for (const auto& size:kFsrScreenResolutionChoices) { // 7차: FSR 해상도는 후처리 장치가 있을 때만 보이며 같은 SETSIZE 흐름이다.
            const auto width=std::to_string(size[0]),height=std::to_string(size[1]);
            add(width+" by "+height,"Tell","SETSIZE("+width+','+height+')',client_.PresentWidth()==static_cast<int>(size[0]) && client_.PresentHeight()==static_cast<int>(size[1]));
        }
    } else if (popup_ == "soundvolume" || popup_ == "musicvolume") {
        const std::string key = popup_ == "soundvolume" ? "soundVolume" : "musicVolume";
        // 원본 음량 단계는 1~5다. 장치 연결 전에 설정 저장을 검사할 수 있다.
        for (int i = 1; i <= 5; ++i) add("Volume " + std::to_string(i), "Volume", key + "=" + std::to_string(i), config.GetInt(key) == i);
    } else if (popup_ == "help") {
        add("About NetStorm", "Tell", "About");
    } else if (popup_ == "game") {
        add("Leave Battle", "Tell", "LeaveNormal");
    } else throw std::runtime_error("Unknown popup: " + name);
    if (world_) client_.Pause(true); //별도 카메라 키 보관 없이 다음 원문 폴링이 현재 모달을 판정한다.
    opened_ = client_.Time().wall; RebuildControls();
}
// 원본 파생 열기/선택/작성/위치/실제 Form 부착을 현재 사건으로 실행한다.
void UberGump::OpenObjectMenu(o::SquidId id,const InputEvent& event) {
    if (!world_ || !id || !world_->Object(id)) return;
    static_cast<void>(OpenPointerObjectMenu([this,id,&event] { return objectMenus_->Open(*world_,o::Sid{static_cast<std::uint16_t>(id)},event); },[this](auto name) { if (auto* audio=client_.Audio()) audio->Sounds().PlayByName(name,0,0,0,0,0); }));rebuild_=true; //실제가상ac가비영일때만원문열기음향을재생하며noAudio는기존검사장치정책을따른다.
}
// 같은 실제 큐의 원본 정수 명령과 물리 사건을 분배한다.
void UberGump::Event(InputEvent event) {
    if (DispatchScreenMouseKey(event,[this] { return client_.GetScreen().Flags(); },[this](auto mode) { client_.ChangeScreenMode(mode); })) rebuild_=true; // 원본 DirectDraw 호출은 Windows10/11에서 borderless/DIB 호환 장치로 실행한다. 같은 사건은 Guide/모달에 계속 전달한다.
    if (const auto* movie=rawButtons_->Menus()->Movies();movie && movie->Active()) { const auto routed=rawButtons_->RouteInput(event);if (routed.consumed) { rebuild_=true;return; } } // 실제 MovieGump 모달을 일반 ESC/키 처리보다 먼저 같은 Gump Route에 전달한다.
    const auto transitionCode=event.code&0x03ffffffU; // 원본 Guide 제어 사건은 일반 입력/프레임보다 먼저 소비한다.
    if (DispatchPlayIntroInput(event,client_.LocalStateGlobals()->State())) return; // 현재 두 인자는 쓰지 않고 Native State39/40/41의 기존 영상/복귀 수명에 맡긴다.
    if (transitionCode==kMissionBeginCommand || transitionCode==kMissionReloadCommand || transitionCode==kMissionForkCommand || transitionCode==kMissionEndCommand) {
        GuideMissionCommandHooks hooks;
        hooks.begin=[this](std::uint32_t token) { const std::string name(scriptStrings_.Resolve(token));BeginMission(name); }; // 고리가 해제되기 전에 실제 수명을 확인하고 이름을 소유 복사한다.
        hooks.restart=[this] {
            if (!mission_ || !missionRuntime_) throw std::logic_error("현재 미션 없는 Restart의 원본 선행 조건 위반");
            const auto request=CaptureMissionReload(mission_->Name(),missionRuntime_->mode64,missionRuntime_->gate.demo);RestartGuideMission(request,MissionTransitionEffects());
        };
        hooks.fork=[this](std::uint32_t parameter) { ForkGuideMission(parameter,MissionTransitionEffects()); };
        hooks.end=[this] { EndGuideMission(MissionTransitionEffects()); }; // 00463e40은 가상 종료→Guide 반납뿐이며 World/입력은 같은 실제 State cleanup까지 유지한다.
        hooks.invalidBegin=[](std::uint32_t) { throw std::logic_error("원본 Guide Begin의 signed param2>100 문자열 주소/토큰 관문 위반"); };
        if (DispatchGuideMissionCommand(event,hooks)) return;
    }
    if (DispatchMissionAbort(event,client_.Input(),[this](std::string_view section) { Tell(std::string(section)); })) return;
    if (!missionRuntime_ && DispatchMissionFirst(event,{})) return; // 원본 Guide는 현재 미션이 없어도 First를 소비한다. 실제 Runtime이 있는 비Tutorial 미션을 빈 Guide로 판정하지 않는다.
    if (missionRuntime_ && missionRuntime_->tutorial) {
        if (missionRuntime_->tutorial->Event(event)) return; //F4/H의 실제 ViewHome 큐가 다음 사건으로 한 번 도착한다. 원시 키를 별도 Home 명령으로 합성하지 않는다.
    }
    bool worldUiRouted=false; // 실제 HTML 모달과 월드 사건은 같은 공통 Gump에서 먼저 한 번만 소비를 판정한다.
    if (popup_.empty() && ((world_ && state_.phase==ClientPhase::Mission) || (editor_ && editor_->metaInput) || rawButtons_->Menus()->HtmlWindowsOpen() || (state_.phase==ClientPhase::MainMenu && rawButtons_->Menus()->HasMenus()))) {
        const auto old=rawButtons_->Pressed();const auto routed=rawButtons_->RouteInput(event);worldUiRouted=true; // nativeGuide뒤466200의현재성공토큰이다.
        if (old!=rawButtons_->Pressed()) rebuild_=true;
        if (routed.activated) {
            if (*routed.activated<0 || static_cast<std::size_t>(*routed.activated)>=actions_.size()) throw std::logic_error("현재UI부모click명령범위오류");
            ActivatePageAction(actions_[static_cast<std::size_t>(*routed.activated)]);rebuild_=true;return;
        }
        if (routed.consumed) { rebuild_=true;return; }
        if (!(event.code&InputCode::kRelease) && (transitionCode==InputCode::kLeftButton || transitionCode==InputCode::kRightButton)) rawButtons_->RequestClosePopups(); // UI밖누름은팝업Kill뒤같은사건으로월드에진행한다.
    }
    if (DispatchScreenshotKey(event,[this] { client_.SaveGifScreenshot(); },[this](auto name) { Tell(std::string(name)); })) rebuild_=true; // Guide/Gump 뒤,console·정지와 일반 F7 배너 처리보다 앞이다. Tell이 연 정지를 아래에서 다시 읽는다.
    static_cast<void>(DispatchDiagnosticKey(event,[this] { client_.DumpDiagnostics(); })); // 같은 원문 위치에서 객체→프로세스 순서로 출력하며 정지/console은 아래에서 다시 읽는다.
    bool recentCommandHandled=false; //최근12명령의실제효과뒤press는현재공통꼬리로진행하며늦은dispatcher만건너뛴다.
    if (IsPresentationCommand(event.code & 0x03ffffffU)) { static_cast<void>(GameMenuCommand(event));recentCommandHandled=true;if (event.code&InputCode::kRelease) return; } //원문 두 설정명령은 Guide/Gump 뒤 공통 뗌 관문을 거쳐 HelpOpen/Console/정지보다 먼저 실행한다.
    if (IsConfigurationCommand(event.code)) { static_cast<void>(GameMenuCommand(event));recentCommandHandled=true;if (event.code&InputCode::kRelease) return; } //원문 세 설정명령은 HelpOpen/모달/Console/정지보다 앞에서 현재 부모를 실행한다.
    const bool aiDebugHandled=IsAiDebugCommand(event.code); //413d50뒤에도같은사건의모달/Console/정지/세계입력꼬리를유지한다.
    if (aiDebugHandled) {
        if (world_) static_cast<void>(world_->Session().DispatchAiDebugCommand(event));
        else {
            AiDebugInputState state;state.metamap=[this] { return client_.SurfaceStorage()->ownerMode.challenge ? 1U : 0U; };
            state.player=[this](auto owner) { return std::span<const std::uint8_t>(client_.PlayerRecords()->Records()).subspan(owner*180,180); };state.body=[](auto)->std::span<std::uint8_t> { throw std::logic_error("World 없는 비영AI본문 미연결"); };
            state.readDebug=[this](auto address) { return client_.GameplayGlobals()->ReadDebugWord(address); };state.writeDebug=[this](auto address,auto value) { client_.GameplayGlobals()->WriteDebugWord(address,value); };static_cast<void>(DispatchAiDebugCommandInput(event,state));
        }
        if (event.code&InputCode::kRelease) return; //원문공통뗌은하위관문을읽지않고끝난다.press는별도return/dirty없이다음꼬리로진행한다.
    }
    bool aiUtilityHandled=false; //772 press는뒤의현재Help/Console/정지/월드꼬리로진행한다.
    if (IsAiUtilityCommand(event.code & 0x03ffffffU)) { //Guide/Gump/뗌 뒤·HelpOpen/Console/공통 정지 앞의 원문 순서다.
        o::AiCommandStateHooks worldlessAi; //World가 없더라도 현재 Player의 NULL brain은 자연 반환하며 비영 Runtime을 합성하지 않는다.
        worldlessAi.players=[this] { return std::span<const std::uint8_t>(client_.PlayerRecords()->Records()); };
        worldlessAi.body=[](std::uint32_t)->std::span<std::uint8_t> { throw std::logic_error("World없는 비영 AI 본체의 실제 Runtime 미연결"); };
        AiUtilityCommandHooks aiCommands;
        aiCommands.initialGeysers=[this] { if (!world_) throw std::logic_error("세계 간헐천의 실제 World 미설치");world_->Session().GenerateWorldGeysers(); };
        aiCommands.crankMoney=[this,worldlessAi] { if (world_) world_->Session().CrankAiMoney();else o::CrankAiMoney(worldlessAi); };
        aiCommands.parameter=[this,worldlessAi](auto packed) { if (world_) world_->Session().SetAiParameter(packed);else o::SetAiParameter(packed,worldlessAi); };
        aiUtilityHandled=DispatchAiUtilityCommandInput(event,aiCommands);if (event.code&InputCode::kRelease) return; //공통뗌만즉시반환하고선택효과뒤새dirty/정지관문은추가하지않는다.
    }
    bool makeAiHandled=false; //누름 효과 뒤 원문 공통 입력 꼬리를 진행한다.
    if (IsMakeAiCommand(event.code&0x03ffffffU)) {
        MakeAiCommandHooks h;h.editor=[this] { return client_.GameplayGlobals()->world.editor; };h.missionPost=[this] { return client_.GameplayGlobals()->world.missionPost; };
        h.makeAi=[this](auto owner) { if (!world_) { if (client_.SurfaceStorage()->ownerMode.battle) throw std::logic_error("battle MakeAi의 실제 현재 World 없음");return; }world_->Session().MakeAi(owner); }; //world밖 battle0은원문417fa0의정상반환이며battle1의실제부모누락은거부한다.
        makeAiHandled=DispatchMakeAiCommandInput(event,h);if (event.code&InputCode::kRelease) return; //뗌만 즉시 반환하며 누름은 현재 Help/Console/정지/세계 입력으로 간다.
    }
    UtilityCommandHooks utility; //원문5명령은Guide/Gump/뗌뒤이며Console/공통정지보다앞에있다.
    utility.text=[this](auto token) { return scriptStrings_.Resolve(token); };utility.screenshot=[this] { client_.SaveGifScreenshot(); };utility.tell=[this](auto section) { Tell(std::string(section)); };
    utility.readString=[this](auto key,auto& value) { return client_.Configuration().ReadString(key,value); };utility.url=[this](auto value) { rawButtons_->Menus()->ActivateHelpLinkText(value); };
    utility.run=[this](auto section,auto mode) { static_cast<void>(TryTell(std::string(section),false,nullptr,mode)); }; //기존cf730의suppress1을그대로사용하며일반Tell로바꾸지않는다.
    utility.highlight=[this](auto mode) {
        if (world_) { world_->Highlight(mode);return; } //살아있는월드는기존공유Carrier/선택/커서몸체를그대로사용한다.
        UserHighlightHooks effects;effects.selectCursor=[this](auto cursor) { client_.GetCursor().Set(cursor); }; //이 명령은 고정6만 전달하므로 역투영/이동의 mode5에 도달하지 않는다.
        RawUserHighlight(client_.GameplayGlobals()->selection.HighlightState(),std::move(effects)).Highlight(mode);
    };
    utility.center=[this](auto sid) {
        if (!sid) return;if (world_) { world_->CenterOnObject(sid);return; }
        const auto& storage=*client_.SurfaceStorage();if (sid>65535U || sid>=storage.pool.Capacity()) throw std::out_of_range("LookAtSid의현재물리SID범위");
        CameraHomeState current;current.width=client_.GetScreen().Width();current.height=client_.GetScreen().Height(); //CenterOn은이현재화면과같은Pool/TYPE만읽으며Reset의모드/로딩기본값을사용하지않는다.
        CameraHomeHooks effects;effects.contentPreview=[](o::Sid) { throw std::logic_error("미설치World의LookAtSid내용물preview Runtime미연결"); }; //Carrier일때만카메라적용뒤도달한다.실제loadingDepth를0으로합성하거나preview성공을만들지않는다.
        CameraHome(storage.pool,storage.types,current,[this](float x,float y) { CenterViewPoint(x,y); },std::move(effects)).CenterOn(o::Sid{static_cast<std::uint16_t>(sid)}); //예약SID1도원문처럼현재좌표/foot을읽고같은즉시카메라전체부모를실행한다.
    };
    if (DispatchUtilityCommandInput(event,utility)) { rebuild_=true;recentCommandHandled=true;if (event.code&InputCode::kRelease) return; }
    if (DispatchUrlInput(event,[this](auto token) { return scriptStrings_.Resolve(token); },
        [this](auto text) { rawButtons_->Menus()->ActivateHelpLinkText(text); })) { rebuild_=true;return; } //모드/현재World와무관하게같은내부Help·cmd·외부정책을사용한다.
    if (DispatchCaptureInput(event,client_.ScreenCapture().State().active,{[this] { client_.ScreenCapture().Start(); },[this] { client_.ScreenCapture().Stop(); },[this](auto section) { Tell(std::string(section)); }})) { rebuild_=true;return; } //현재active우선/param2=1시작이다. 개발자키55a484조건은켜지않는다.
    if (DispatchRecheckCdInput(event,client_.CdGlobals()->RecheckSerial(),{[this](auto serial) { return rawButtons_->SerialGump(serial); },[this](auto token) { rawButtons_->KillGump(token); }})) { rebuild_=true;return; } //4d8b8d: 현재 serial 조회→공통 Kill→0쓰기다. 실제 CD 재탐색(440c60)이나 온라인 검증은 호출하지 않는다(단위696).
    if (DispatchDrawWater(event,client_.GameplayGlobals()->DrawWater(),worldRedraw_)) { rebuild_=true;recentCommandHandled=true;if (event.code&InputCode::kRelease) return; } //Guide/Gump/뗌 뒤 HelpOpen/Console/정지 앞이며 모드 관문을 추가하지 않는다.
    if (DispatchCreateHelpList(event,[this] {
        std::optional<o::BaseFileSystem> userHelpFiles; // 5차: 사용자 홈 정책이면 목록 HTML을 사용자 Cache에 만든다. 동기 호출 동안만 살며 같은 이름 고리를 공유해 원문의 경로 칸 소비를 유지한다.
        if (client_.UserData()) { platform::UserDataRoot::EnsureDirectory(client_.UserData()->Cache());userHelpFiles.emplace(client_.UserData()->Cache(),std::filesystem::path{},client_.Files().FileNameOwner()); }
        QuickHelpListContext context;context.file=MakeHelpListDiskHooks(userHelpFiles ? client_.UserData()->Cache():client_.GameDirectory(),userHelpFiles ? &*userHelpFiles:&client_.Files()); //원문 전체41b4c0와 같은 현재 파일 소유자다. 기존파일의 r+b tail을 유지한다.
        context.typeCount=[this] { const auto size=client_.Assets().TypeTable().Types().size();if (size>0x7fffffffU) throw std::out_of_range("Help 목록의 원문 signed TYPE 개수");return static_cast<std::int32_t>(size); }; //월드/지식/임무 상태로 TYPE 목록을 거르지 않는다.
        context.flags2=[this](auto type) { const auto types=client_.Assets().TypeTable().Types();if (type>=types.size()) throw std::out_of_range("Help 목록 TYPE flags2 범위");return types[type].flags2; };
        context.typeName=[this](auto type) { const auto types=client_.Assets().TypeTable().Types();if (type>=types.size()) throw std::out_of_range("Help 목록 TYPE 이름 범위");return types[type].name; };
        context.cleanup=[this] { rawButtons_->CleanupGumps(); };context.tell=[this](auto section) { Tell(std::string(section)); };
        rawButtons_->Menus()->CreateHelpList(std::move(context));
    })) { rebuild_=true;recentCommandHandled=true;if (event.code&InputCode::kRelease) return; } //Guide/Gump/뗌 뒤이며 Console/정지/월드·편집 모드 조건을 추가하지 않는다.
    if (transitionCode==0x02000037U) {
        if (!(event.code&InputCode::kRelease)) { EnsureHudFactories();rawButtons_->ToggleMinimap(hud_->createMinimap); }
        rebuild_=true;return; //원본 일반 입력의5427d4→480810을 동일 공장/지연Kill에 연결하며 뗌은 전환하지 않는다.
    }
    if (transitionCode==0x02000017U || transitionCode==0x02000057U) {
        if (!(event.code&InputCode::kRelease)) rawButtons_->Menus()->OpenHelp(std::bit_cast<std::uint32_t>(event.y ? event.y : event.x),transitionCode==0x02000057U ? 0 : 1,0);
        rebuild_=true;return; // Tutorial 소비 뒤 원본 Help/요청의 현재 주제/TYPE 인자를 보존한다.
    }
    if (transitionCode==0x02000095U) { if (!(event.code&InputCode::kRelease)) {
        const auto token=std::bit_cast<std::uint32_t>(event.y);const auto literal=objectMenus_ ? objectMenus_->StaticText(token) : std::nullopt; // 정적 원문 포인터와 Script 고리 토큰을 서로의 저장소로 잘못 해석하지 않는다.
        const auto name=std::string(literal ? *literal : scriptStrings_.Resolve(token));static_cast<void>(TryTell(name));
    }return; } // Help Back과 객체 메뉴 Tell도 같은 실제 HTML 경로로 돌아간다.
    if (IsSendMoneyCommand(transitionCode)) {
        if (event.code&InputCode::kRelease) return; // 원문 공통 뗌 관문은 현재 월드/하위 장치 조회보다 먼저다.
        if (!world_) throw std::logic_error("송금 일반 입력의 실제 월드 없음");auto hooks=world_->Session().MoneyInputHooks();
        hooks.numberDialog=[this](auto target) { OpenSendMoneyDialog(target); };hooks.askAll=[this](auto target) { Tell("asksend("+std::to_string(target)+")"); };
        static_cast<void>(DispatchSendMoneyInput(event,hooks));rebuild_=true;return;
    }
    if (transitionCode==kSetObjectOwnerCommand) {
        const auto gameplay=client_.GameplayGlobals(); // release/미허용 모드/선택0은 원문처럼 Runtime을 요구하지 않는다.
        ObjectOwnerCommandHooks hooks;hooks.editor=[gameplay] { return gameplay->world.editor; };hooks.missionPost=[gameplay] { return gameplay->world.missionPost; };hooks.selected=[gameplay] { return gameplay->selection.selected; };
        hooks.owner=[this](auto sid,auto owner) { if (!world_) throw std::logic_error("선택된 실제 소유자 효과의 월드 미설치");if (sid>65535U) throw std::out_of_range("소유자 명령의 물리 SID WORD 범위");world_->Session().SetObjectOwner(o::Sid{static_cast<std::uint16_t>(sid)},owner); };
        hooks.follow=[this](auto sid) { world_->Session().FollowObjectOwner(sid); }; //가상 owner 뒤 현재 Editor가 선택한 같은 Session 부모다.
        static_cast<void>(DispatchObjectOwner(event,worldRedraw_,hooks));rebuild_=true;return; //원문4d81c5도 미니맵이 아닌0059a8b0의 현재 DWORD를 증가시킨다.
    }
    if (transitionCode==kDialogDoNothing) return; // 원본 명령 자체는 효과가 없으며 HTML 부모는 발화 시 이미 Kill되었다.
    if (transitionCode==kEditorTestBattleCommand) {
        EditorTestBattleHooks hooks;hooks.snapshot=[this] { if (!world_) throw std::logic_error("Test Battle의 실제 Editor Session 없음");world_->Session().SaveCurrentMap(); };
        hooks.currentFortName=[this] { const auto bytes=client_.FortGlobals()->CurrentBytes();return std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size()); }; //늦은 이름 복사는 같은 실제256BYTE의 NUL까지 읽는다.
        hooks.guide=MissionTransitionEffects();hooks.guide.create=[this,workspace=editorCatalog_](auto name) { CreateMission(std::string(name),0,0,[workspace](auto path) { return workspace->Read(path); }); }; //실제 편집 작업본의 동일 namespace를 Script/별칭 Fort/재로드까지 소유한다.
        hooks.pending=FortEditEffects().pending;hooks.requestEdit=[this] { RequestFortEditState(); };
        static_cast<void>(DispatchEditorTestBattle(event,fortSelectionMode_,hooks));rebuild_=true;return;
    }
    if (FortSelectionEvent(event)) return; // Guide와 실제 Gump 입력 뒤 원문 일반 부모의 같은 두 명령을 처리한다.
    if (MultiplayerOptionsEvent(event)) return; // 일반 옵션의 이름/연결 예약도 같은 Guide/Gump 뒤 일반 입력 큐에서 소비한다.
    NativeMetaSailInputHooks metaSail;metaSail.passedDiags=[this] { return client_.NsInfoGlobals()->passedDiags; };metaSail.noShowWarn=[this] { return client_.Configuration().GetInt("noShowWarnFailedDiags"); };metaSail.serverSelection=[this] { return client_.NsInfoGlobals()->serverSelection; };
    metaSail.freeSlots=[this](auto battle) { return client_.ChallengeGlobals()->FreeBattleSlots(battle); };metaSail.tell=[this](auto section) { Tell(std::string(section)); };metaSail.suppress=[this] { client_.Configuration().SetInt("noShowWarnFailedDiags",1); };
    metaSail.join=[this](auto parameter) { if (!editor_ || !editor_->metaTransport) throw std::logic_error("전투 참가 입력의 현재 메타 전송 owner 미연결");editor_->metaTransport->JoinBattle(parameter); };metaSail.sail=[this](auto position) { if (!editor_ || !editor_->metaTransport) throw std::logic_error("항해 위치 입력의 현재 메타 전송 owner 미연결");editor_->metaTransport->SailPosition(position); };
    if (DispatchNativeMetaSailInput(event,metaSail)) { recentCommandHandled=true;if (event.code&InputCode::kRelease) return; } //누름의 효과 뒤 원문 일반 입력 꼬리를 유지하고 뗌은 장치를 읽지 않는다.
    if (DispatchNativeMetaZoneRequest(event,[this](auto section) { Tell(std::string(section)); })) { recentCommandHandled=true;if (event.code&InputCode::kRelease) return; } //e8은 일반 입력의 현재 signed 번호로 확인창만 열고 누름 꼬리를 유지한다.
    if (transitionCode==0x020000c6U) {
        if (event.code&InputCode::kRelease) return;if (client_.SurfaceStorage()->ownerMode.challenge) { if (!editor_ || !editor_->metaTransport) throw std::logic_error("MetaRefresh의 현재 메타 전송 owner 없음");editor_->metaTransport->RefreshZones(); }else client_.WriteDiagnosticLog(9,"The commandMetaRefresh only works in metamap mode.\n");recentCommandHandled=true;
    } //004d80ec은 메타에서만4b40c0(11)을 재호출하고 비메타 진단/누름 뒤에도 같은 일반 입력 꼬리를 유지한다.
    if (transitionCode==0x020000f3U) { if (!(event.code&InputCode::kRelease)) OpenMetaBattleDescription(std::bit_cast<std::uint32_t>(event.y));return; } //004d8ab5는 Guide/Gump/뗌 뒤·Console/정지 앞에서 현재param2를 설명 모달에 전달한다.
    if (transitionCode==0x020000f5U) { if (!(event.code&InputCode::kRelease)) client_.LocalStateGlobals()->Begin()=1;return; } // ServerForceBattleStart는 원문처럼 Begin만1을 쓰며 다음 타이머가 실제6e를 송신한다.
    if (transitionCode==0x020000acU) { if (!(event.code&InputCode::kRelease)) static_cast<void>(DeleteSavedStateFile(client_.GameDirectory(),client_.StaticStrings()->FileNames(),client_.UserSavedStateFile()));return; } // DelStateFile은4b5880처럼 현재 경로를 작성해 unlink하고 반환을 무시한다. 새 상태 요청/월드 반납을 추가하지 않는다.
    if (transitionCode==0x020000abU || transitionCode==0x020000a9U) { if (!(event.code&InputCode::kRelease)) RequestBattleReconnect(transitionCode==0x020000abU);return; } // StateReconnect는1, ForceServerDeath는0을 같은004b8cb0 부모에 전달한다.
    if (transitionCode==0x02000122U) { if (!(event.code&InputCode::kRelease)) { if (!world_) throw std::logic_error("강제 동맹 일반 입력의 현재 World 없음");world_->Session().RequestForcedAlliance(std::bit_cast<std::uint32_t>(event.y)); }return; } // 원문 팀 시작/동맹 요청도 같은 일반 입력 위치에서78h만 송신한다.
    if (DispatchTellTip(event,client_.LocalStateGlobals()->TipNumber(),{
        [](std::int32_t value) { char text[32]{};std::snprintf(text,sizeof(text),"%d",value);return std::string(text); }, //원문 호출자의 지역 버퍼와 signed 정수 서식만 사용한다.
        [this](auto key,auto value) { client_.Configuration().Set(std::string(key),std::string(value)); }, //동기 옵션 재해석도 같은 현재 TipNumber를 갱신한다.
        [this](auto section) { Tell(std::string(section)); }
    })) { rebuild_=true;return; }
    if (DispatchEditorIslandDialog(event,[this] {
        if (!world_) throw std::logic_error("섬 추가의 실제 현재 Editor Session 없음");EditorIslandWindowContext context;context.session=&world_->Session();context.translate=[this](auto text) { return FontBytes(client_.Translate(text)); };
        context.tell=[this](auto name) { return rawButtons_->Menus()->ShowEmbeddedTell(name,client_.Configuration(),[this](auto section)->std::optional<std::string> { return mission_ ? mission_->Section(section) : std::nullopt; }); };
        context.random=[this](auto limit) { return client_.RandomGlobals()->Game().Next(limit); };context.camera=[this](float x,float y) { world_->CenterCamera(x,y); };context.revision=[this] { ++client_.GameplayGlobals()->MapRevision(); };context.pulseRegistered=[this](auto id) { static_cast<void>(rawButtons_->PulseRegisteredIfIdle(id)); };
        rawButtons_->Menus()->ShowEditorIslandWindow(std::move(context));rebuild_=true;
    })) return; // release는 소비만 하고 press는 실제 공장/부착/모달 등록을 수행한다.
    if (DispatchEditorSaveGoMain(event,client_.LocalStateGlobals()->State().next[0],[this] { if (!world_) throw std::logic_error("SaveGoMain의 현재 Session 없음");world_->Session().SaveCurrentMap(); })) return;
    if (transitionCode==0x02000079U) {
        if (!(event.code&InputCode::kRelease)) OpenSaveFortNameDialog(std::bit_cast<std::uint32_t>(event.y));
        rebuild_=true;return; //실제 일반 입력의 fortMode 숫자와 고정 canCancel1을 같은 모달 부모에 전달한다.
    }
    if (transitionCode==0x020000d6U) {
        if (!(event.code&InputCode::kRelease) && world_) world_->Session().SetAllBridgeQuality(std::bit_cast<std::uint32_t>(event.y));
        rebuild_=true;return; //원문 SetAllBridge는 일반 입력의 뗌 거부 뒤 숫자 인자를 그대로 현재 월드에 전달한다.
    }
    if (transitionCode==kViewSpotCommand) {
        if (!(event.code&InputCode::kRelease)) CenterViewSpot(scriptStrings_.Resolve(std::bit_cast<std::uint32_t>(event.y)));
        return; //원문4d84cc는 같은 사슬에서 모드 관문 없이 인자 문자열을433ad0에 넘긴다(단위676).
    }
    if (transitionCode==kMoveDialogCommand) {
        if (!(event.code&InputCode::kRelease)) rawButtons_->MoveCurrentDialog(scriptStrings_.Resolve(std::bit_cast<std::uint32_t>(event.y)));
        return; //원문 일반 입력에서 실제 현재 모달만 이동하고 호스트 재생성으로 위치를 초기화하지 않는다.
    }
    if (transitionCode>=0x02000125U && transitionCode<=0x02000127U) {
        if (!(event.code&InputCode::kRelease) && world_) world_->Session().SetBridgeQuality(std::bit_cast<std::uint32_t>(event.y),transitionCode-0x02000125U);
        rebuild_=true;return; // 원본00542b5c/60/64의 y=SID와 고정 품질0/1/2다. 현재 팝업 수명과 무관한 일반 입력에서 소비한다.
    }
    if (transitionCode==0x0200002dU) {
        if (!(event.code&InputCode::kRelease) && world_) {
            const auto sid=std::bit_cast<std::uint32_t>(event.y);if (sid>=world_->Session().Core().pool.Capacity()) throw std::out_of_range("Dig Up의 실제 SID 범위");
            static_cast<void>(world_->Session().GemReward().Reveal(o::Sid{static_cast<std::uint16_t>(sid)}));
        }
        rebuild_=true;return; // 원본005427ac의 일반 입력은 메뉴의 special 노출 조건을 실행 관문으로 다시 추가하지 않는다.
    }
    if (DispatchAddRandomInput(event,client_.GameplayGlobals()->world.editor!=0,client_.GameplayGlobals()->world.missionPost!=0,
        [this](auto token) { return client_.Assets().TypeTable().Find(scriptStrings_.Resolve(token)); },
        [this](auto type,auto quantity) { if (!world_) throw std::logic_error("AddRandom의 실제 현재 Session 없음");world_->Session().AddRandomObjects(type,quantity); })) { rebuild_=true;return; } // 원문 타입 문자열/숫자와현재모드를 같은 기존 배치 부모에 전달한다.
    const bool createAtHandled=DispatchCreateAtInput(event,client_.GameplayGlobals()->world.editor!=0,client_.GameplayGlobals()->world.missionPost,[this](auto token) {
        if (!world_) throw std::logic_error("CreateAt의 실제 현재 World/Runtime 없음");world_->Session().CreateAt(scriptStrings_.Resolve(token)); //현재 토큰을 실제 parser에 전달하며 별도 성공 객체를 만들지 않는다.
    }); //원문 생성 뒤에도 현재 Help/모달/세계 입력 꼬리를 진행한다.
    if (createAtHandled) { rebuild_=true;if (event.code&InputCode::kRelease) return; } //공통 뗌은 소비만 하며 누름은 조기 반환하지 않는다.
    if (DispatchDestroyAtInput(event,client_.GameplayGlobals()->world.editor!=0,client_.GameplayGlobals()->world.missionPost!=0,
        [this](auto token) { if (!world_) throw std::logic_error("DestroyAt의 실제 현재 Session 없음");world_->Session().DestroyAt(scriptStrings_.Resolve(token)); })) { rebuild_=true;return; } // 원문문자열을현재권한/점검색/파생삭제에전달한다.
    if (IsScenarioEffectCommand(transitionCode)) {
        ScenarioEffectInputHooks hooks; // 같은 현재 화면/기술 부모와 실제 표시 수명을 빌린다.
        hooks.width=[this] { return client_.GetScreen().Width(); };
        hooks.technology=[this] { if (!world_) throw std::logic_error("Lightning의 현재 Session 없음");return world_->Session().CurrentTechnologyParent(); };
        hooks.smoke=[this](auto parent,auto x,auto y) { if (!world_) throw std::logic_error("StartSmoke의 현재 Session 없음");o::SmokeRequest request;request.parent=o::Sid{static_cast<std::uint16_t>(parent)};request.hotspotX=x;request.hotspotY=y;static_cast<void>(world_->Session().Particles().AddSmoke(request)); };
        hooks.lightning=[this](auto parent,auto from,auto to,auto life) { if (!world_) throw std::logic_error("Lightning의 현재 Session 없음");static_cast<void>(world_->Session().Particles().AddScreenBolt(o::Sid{static_cast<std::uint16_t>(parent)},from,to,life)); };
        static_cast<void>(DispatchScenarioEffectInput(event,hooks));rebuild_=true;return;
    }
    if (DispatchVortexCommandInput(event,[this](o::Sid target,std::uint32_t code,std::uint16_t parameter) {
        if (!world_) throw std::logic_error("신전 명령의 실제 현재 Session 없음");static_cast<void>(world_->Session().CommandObject(target,code,parameter));
    })) { rebuild_=true;return; } // 일반 객체 입력과 같은 뗌/대상0 처리이며 확인WORD를 버리지 않는다.
    if (DispatchViewMetaInput(event,client_.LocalStateGlobals()->ViewMetaCommand(),{
        [this] { return static_cast<std::int32_t>(client_.NsInfoGlobals()->servers.size()); },
        [this] { client_.WriteDiagnosticLog(19,"An invalid server is selected. Forcing number 0.\n");client_.Configuration().Set("curServer","0"); },
        [this](auto territory) { if (!world_) throw std::logic_error("Descend 저장의 실제 현재 Session 없음");world_->Session().RequestFortDescend(territory); }})) return;
    if (DispatchScenarioObjectInput(event,[this](o::Sid target,std::uint32_t code) {
        if (!world_) throw std::logic_error("시나리오 객체 명령의 현재 Session 없음");return world_->Session().CommandObject(target,code);
    })) { rebuild_=true;return; } // 원문처럼 메뉴를 열지 않은 대상과 객체의0반환도 같은 현재 사건에서 처리한다.
    if (DispatchDetonateInput(event,[this](o::Sid target,std::uint32_t code) {
        if (!world_) throw std::logic_error("Detonate의 현재 Session 없음");static_cast<void>(world_->Session().CommandObject(target,code));
    })) { rebuild_=true;return; } // 원본은 실제6c의0반환 뒤에도 다음 사건으로 진행한다. 미연결 명령 예외를 만들지 않는다.
    if (transitionCode==kStartDemo) {
        if (!(event.code&InputCode::kRelease)) BeginMission(std::string(scriptStrings_.Resolve(std::bit_cast<std::uint32_t>(event.y))),1);
        return; // 원본005429f4의 실제 세 번째 인자를 동일 공장으로 전달한다.
    }
    if (IsRitualChoiceCommand(transitionCode)) {
        if (event.code&InputCode::kRelease) return;if (!world_) throw std::logic_error("의식 선택 요청의 실제 현재 Session 없음");static_cast<void>(DispatchRitualChoiceInput(event,world_->Session().RitualChoiceInputs()));return;
    } //원문일반부모의같은비동기요청이며0200009f기술즉시지급으로우회하지않는다.
    if (DispatchGetTechnologyInput(event,[this](auto type) { if (!world_) throw std::logic_error("GetTechnology의 실제 현재 Session 없음");world_->Session().LearnNewTechnology(type); })) return; // 원문492a30의 기존 기술 추가/snapshot/이름·그림/NewTech 안내를 같은 Session에서 실행한다.
    if (transitionCode==kCheatAllTechnologyCommand) {
        if (!(event.code&InputCode::kRelease)) { if (!world_) throw std::logic_error("현재 월드 없는 원소 기술 지급");ApplyThemeTechnology(world_->Session(),event.x);rebuild_=true; }
        return; // 일반 입력의 key-up 거부 뒤 원문 첫 인자를 사용하며 창 수명 효과를 넣지 않는다.
    }
    if (!(event.code&InputCode::kRelease) && transitionCode==(0x70U<<InputCode::kVirtualKeyShift)) { client_.Input().PushCommand(0x02000017U,0,0x61000003);return; } // 원본4d8c47의 F1Help 문자열이며 Control/Shift를 별도 거부하지 않는다.
    if (rawButtons_->Menus()->HelpOpen()) { if (!worldUiRouted) static_cast<void>(rawButtons_->Event(event));rebuild_=true;return; } // 현재 공통 모달/스크롤/태그/돌 버튼을 실제 GumpInput에 전달한다.
    if (!rawButtons_->CurrentDialog() && rawButtons_->Menus()->OpenChatInput(event)) { rebuild_=true;worldUiRouted=true; } //원문4d8c60의 모달 뒤/정지 앞 Enter다. 같은 사건을 새로 표시한 Console에 다시 보내지 않는다.
    if (!rawButtons_->CurrentDialog()) { auto& globals=*client_.GameplayGlobals();if (DispatchRendererDiagnosticKey(event,globals.player.special,globals.specialAuxiliary,globals.DrawFire(),globals.DrawSpot(),globals.DrawBoard(),worldRedraw_,[this](InputEvent input) { const auto point=RawCursorPosition::ScreenToWorld(input.x,input.y,client_.GameplayGlobals()->Camera().Snapshot().worldBits);std::array<char,128> text{};const auto count=std::snprintf(text.data(),text.size(),"cursorPos: %f, %f\n",static_cast<double>(point[0]),static_cast<double>(point[1]));if (count<0 || static_cast<std::size_t>(count)>=text.size()) throw std::runtime_error("원문좌표로그서식실패");client_.WriteDiagnosticLog(0x1d,std::string_view(text.data(),static_cast<std::size_t>(count))); })) { rebuild_=true;return; } } //Guide/Gump/Console뒤정지앞의원문특수관문이다.
    if (transitionCode==0x130000U) {
        if (rawButtons_->CurrentDialog()) return; //원문565dd4의 현재 Console/모달 관문 뒤이며 정지를 별도 조회하지 않는다.
        PauseKeyInputHooks hooks;hooks.command=[] { return GameMenuCommandValue(0x00542a30U).value(); }; //원문004d8dde의 같은 현재 Pause 명령이다.
        hooks.queue=[this](auto command,auto first,auto second) { client_.Input().PushCommand(command,first,second); };
        static_cast<void>(DispatchPauseKeyInput(event,hooks));return; //정지/재개와 음향은 다음 일반 Pause 명령이 소비한다.
    }
    if (transitionCode==0x760000U && !rawButtons_->CurrentDialog() && DispatchBuildReplayKey(client_,event)) return; // 기존 Guide/Gump/Console 뒤이며 재생 입력이 실제 선택된 경우 일반 배너 토글보다 먼저 끝난다.
    if (transitionCode==0x710000U || transitionCode==0x720000U || transitionCode==0x770000U || (transitionCode==0x760000U && (event.code&(InputCode::kShift|InputCode::kControl))!=(InputCode::kShift|InputCode::kControl))) {
        if (rawButtons_->CurrentDialog()) return; // 원문565dd4 모달 관문은 일반 F2/F3/F8보다 앞이다. Guide 뒤 조기 F8 예약을 하지 않는다.
        ViewKeyInputHooks hooks; // 같은 Client 표시 전역과 현재 설정/Tell/큐를 사용한다.
        hooks.paused=[this] { return client_.Paused(); };
        hooks.readInt=[this](auto key,auto& value) { return client_.Configuration().ReadInt(key,value); };
        hooks.tell=[this](auto section) { Tell(std::string(section)); };
        hooks.queue=[this](auto command,auto first,auto second) { client_.Input().PushCommand(command,first,second); };
        hooks.useBanners=[this] { return client_.ColorSortInput()->OwnerTables()->Enabled(); };
        hooks.writeBanners=[this](auto value) { client_.Configuration().SetInt("useBanners",static_cast<std::int32_t>(value)); }; // 같은 원본 Config 쓰기 경계에서 현재 모든 색표를 동기 변경한다.
        hooks.fort=[this] { return client_.SurfaceStorage()->links.mission ? 1U : 0U; };
        hooks.metamap=[this] { return client_.SurfaceStorage()->ownerMode.challenge ? 1U : 0U; };
        hooks.editor=[this] { return client_.GameplayGlobals()->world.editor; };
        hooks.highlight=[this](auto mode) {
            UserHighlightHooks effects;effects.selectCursor=[this](auto cursor) { client_.GetCursor().Set(cursor); }; // F8의1분기는 역투영/이동 장치를 요구하지 않는다.
            RawUserHighlight(client_.GameplayGlobals()->selection.HighlightState(),std::move(effects)).Highlight(mode);
        };
        if (DispatchViewKeyInput(event,{hideBuildings_,worldRedraw_},hooks)) rebuild_=true;return;
    }
    if (transitionCode==0x750000U) { if (rawButtons_->CurrentDialog()) return;if (DispatchBuildRecordKey(client_,event)) return; } //원문 기록 키만 소비하며 일반 F6는 공통 정지 뒤 실제 지식 명령 예약으로 진행한다.
    if (transitionCode=='C' || transitionCode=='c') {
        if (rawButtons_->CurrentDialog()) return; //원문의 현재 Console/모달 관문 뒤에서만 C의 정지를 조회한다.
        static_cast<void>(DispatchRotationKeyInput(event,client_.GameplayGlobals()->CursorGlobals().RotationMode(),{[this] { return client_.Paused(); },[this](auto assignment) { ApplyScriptConfig(assignment,client_.Configuration()); }}));
        rebuild_=true;return; //원문 C는 Editor/미션/메인 모드 조건이 없고 같은 설정 재해석이 우클릭 회전에 이어진다.
    }
    if (transitionCode==9) {
        if (rawButtons_->CurrentDialog()) return; //원문의 Console/모달 뒤 정지 관문과 같은 위치다.
        auto& gameplay=*client_.GameplayGlobals();static_cast<void>(DispatchConstructionHistoryKey(event,gameplay.Camera().ConstructionHistory(),gameplay.WorldCells(),{[this] { return client_.Paused(); },[this](auto x,auto y) { CenterViewPoint(x,y); }}));return;
    }
    if (transitionCode=='T' || transitionCode=='t') {
        if (rawButtons_->CurrentDialog()) return; //원문Console/모달뒤의공통정지관문이다.
        static_cast<void>(DispatchGameTimeKey(event,client_.GameplayGlobals()->ShowGameTime(),[this] { return client_.Paused(); }));return;
    }
    if (transitionCode=='U' || transitionCode=='u') {
        if (rawButtons_->CurrentDialog()) return; //원문 Console/모달 뒤의 공통 정지 관문을 유지한다.
        auto& gameplay=*client_.GameplayGlobals();const auto& lost=*gameplay.GameAux(); //실제 삭제/Native cleanup과 같은005c848c/90 owner이며 Session 사본을 만들지 않는다.
        static_cast<void>(DispatchLostPositionKey(event,lost.LostX(),lost.LostY(),gameplay.WorldCells(),{[this] { return client_.Paused(); },[this](auto x,auto y) { CenterViewPoint(x,y); }}));return;
    }
    SavedCameraKeyHooks savedCamera; //현재 숫자 키만 이 장치를 소비하며 다른 사건은 초기 게이트에서 반환한다.
    savedCamera.paused=[this] { return client_.Paused(); };savedCamera.language=[this] { return client_.LanguageNumber(); };
    savedCamera.sound=[this] { client_.WorldAudio().Sounds().PlayByName("select.wav",0,0,0,0,0); };
    savedCamera.capture=[this] { const auto map=CurrentMapView();return CaptureSavedCameraCenter(client_.GameplayGlobals()->Camera().RendererView(),{map.left,map.top,map.right,map.bottom}); }; //선택음 이후의 같은 실제 지도LTRB/Renderer를 읽는다.
    savedCamera.center=[this](auto x,auto y) { CenterViewPoint(x,y); };
    if (!rawButtons_->CurrentDialog() && DispatchSavedCameraKey(event,client_.GameplayGlobals()->Camera().SavedPositions(),savedCamera)) return; //Guide/Gump/Chat/모달 뒤의 원문 정지 관문이며 World 유무를 별도 제한하지 않는다.
    if (transitionCode==kProductionBridgeCommand) {
        if (!worldUiRouted) static_cast<void>(rawButtons_->Event(event));rebuild_=true;return; // Guide/현재 모달 뒤 같은 Gump 트리로 전달한다. 정지/활성 커서 관문을 추가하지 않는다.
    }
    if (DispatchLanguageChanged(event,worldUiRouted,[this](const auto& notification) { static_cast<void>(rawButtons_->RouteInput(notification)); })) { rebuild_=true;return; } //Chat의기존Push/Attach를같은실제트리에서한번만실행한다.
    if (transitionCode==0x02000088U) {
        if (!worldUiRouted) static_cast<void>(rawButtons_->Event(event));rebuild_=true;return; // 원본 하단 전환은 HUD 효과 뒤 기본 반환0이다. 이미 실행한 Route를 반복하거나 미구현 메뉴 오류로 바꾸지 않는다.
    }
    if (transitionCode==kShowTechnologyCommand || transitionCode==kShowTecSideMenuCommand || transitionCode==kShowTecMenu2Command) {
        if (!(event.code&InputCode::kRelease) && world_) rawButtons_->Menus()->OpenKnowledge(world_->Session(),[this](std::string_view key) { return FontBytes(client_.Translate(key)); },static_cast<std::uint32_t>(client_.LanguageNumber()),transitionCode==kShowTecSideMenuCommand ? KnowledgeMenuKind::Side : transitionCode==kShowTecMenu2Command ? KnowledgeMenuKind::World : KnowledgeMenuKind::Storm);
        rebuild_=true;return; //원문추가두명령도같은뗌/PlayerKnown/메타맵관문을사용하며기본F6에는새UI를노출하지않는다.
    }
    // F6는 아래 실제 UserInput의 공통 정지 관문 뒤에서 처리한다. 직접 ShowTechnology 메뉴 명령과 F9의 정지 앞 예약은 별개다(원문4d96b6/4d9a00).
    if (InputSession() && state_.phase==ClientPhase::Mission && QueuePlayerListKey(event,client_.Input())) return; //메타도 F9/Shift+F9의 같은 Player List/정지 명령을 예약하고 각 소비자의 모드 관문에 맡긴다.
    if (!worldUiRouted && rawButtons_->Menus()->KnowledgeOpen()) {
        if (IsEscapeKeyPress(event)) { rawButtons_->Menus()->RequestClosePopups();rebuild_=true;return; }
        if (transitionCode==InputCode::kLeftButton || transitionCode==InputCode::kRightButton || transitionCode==InputCode::kMiddleButton) { static_cast<void>(rawButtons_->Event(event));rebuild_=true;return; }
    }
    if (client_.Assets().Edition()==o::OriginalEdition::Patch1078 && DispatchBattleRetreatConfirmation(event,[this](auto section) { Tell(std::string(section)); })) return; //원문 이유0 이하의 확인 요청은 같은 Session/Native 상태를 보존한 채 실제 안내창만 연다.
    if ((event.code&0x03ffffffU)==kLeaveBattleCommand) {
        if (event.code&InputCode::kRelease) return; // Guide가 소비한 Abort와 달리 실제 Leave는 원본 일반 입력의 key-up 거부 뒤에 실행한다.
        if (event.y>=1 && client_.Assets().Edition()==o::OriginalEdition::Patch1078) {
            auto& local=*client_.LocalStateGlobals();auto& game=*client_.GameplayGlobals();auto& surface=*client_.SurfaceStorage();
            PrepareBattleLeaveState(event.y,{{local.State(),local.AbortPending(),client_.WorldAudio().ResultState(),game.world.missionPost,game.world.editor,surface.ownerMode.challenge,surface.ownerMode.battle},local.ScriptBreak(),local.SourceType(),game.player.playerKnown},[this] { static_cast<void>(DeleteSavedStateFile(client_.GameDirectory(),client_.StaticStrings()->FileNames(),client_.UserSavedStateFile())); }); // 실제 양수 부모의 이유2/3/9·두 삭제/부분 효과를 공유하며 다음 Tick에 정리한다.
            return; //실제 다음 Poll의 State3에 종료 통지·정리·메뉴를 맡기며 사건 수신 중 World를 반납하지 않는다.
        }
        MainMenu();if (client_.Audio()) client_.Audio()->StartScene(false);return; //CD판은 기존 호스트 경로다.10.78의 확인 요청/양수 이탈은 위의 별도 원문 경로가 처리한다.
    }
    if (objectMenus_ && objectMenus_->Command(event)) { rebuild_=true;return; }
    const auto commandCode=event.code&0x03ffffffU; // 원본명령분배와같이최상위입력플래그만제거한다.
    if (commandCode==kScriptWarning) {
        warnings_->SetView(client_.GetScreen().Width(),client_.GetScreen().Height(),client_.GetScreen().Colors());
        if (event.y) warnings_->Add(scriptStrings_.Resolve(std::bit_cast<std::uint32_t>(event.y)));
        else warnings_->AddNull(); //원문004d88cd는 y=0도 그대로4d3090에 전달하며 유효한 빈 문자열과 구별한다.
        rebuild_=true;return;
    }
    if (!recentCommandHandled && !createAtHandled && !aiUtilityHandled && !makeAiHandled && !aiDebugHandled && GameMenuCommand(event)) return; //앞서 실행한 생성 명령을 늦은 메뉴 dispatcher에 다시 보내지 않는다.
    if (!recentCommandHandled && !createAtHandled && !aiUtilityHandled && !makeAiHandled && !aiDebugHandled && IsGameMenuCommand(commandCode)) throw std::logic_error("현재 메뉴 선택 명령의 실제 장치 미연결: "+std::to_string(commandCode));
    if (IsObjectMenuVariantCommand(commandCode)) throw std::logic_error("현재 파생 객체 메뉴 선택의 실행 미연결: "+std::to_string(commandCode)); // 미복원 개발자/편집 실행을 입력에서 조용히 버리지 않는다.
    // 원본 ESC 글자 사건. 초기 브리핑은 확인 버튼으로만 닫는다.
    if (IsEscapeKeyPress(event)) {
        if (state_.phase == ClientPhase::Briefing || state_.phase == ClientPhase::LoadingMission) return;
        if (!world_ && editor_ && editor_->metaInput && popup_.empty()) { //원문 ESC는 공통 정지 뒤가 아니며 같은 선택 정리를 먼저 실행한다.
            editor_->metaInput->ResetSelection();if (!editor_->metaWorld->CursorDropSettings().lockDepth) { editor_->metaWorld->CancelCursorForState();rawButtons_->RequestClosePopups();rebuild_=true; }else ToggleGameMenu();return;
        }
        if (world_ && userInput_ && state_.phase==ClientPhase::Mission && popup_.empty()) {
            world_->ResetSelection(); // 원문4d9494의정리와Select(0,0,0)은154입력/두정밀도의전체효과가같다.정지중에도선택/강조를먼저정리한다.
            if (!world_->Session().CursorDropSettings().lockDepth) { world_->Session().CancelCursorForState();rawButtons_->RequestClosePopups();rebuild_=true; } // 원문446e70→66710만호출하며일반마우스/설정읽기와팝업닫기를중복하지않는다.
            else ToggleGameMenu(); // 현재커서가없으면같은serial조회로막대를열거나닫는다.
            return;
        }
        if (!popup_.empty()) state_.Post({"Menu", "close"});
        else if (mission_ && state_.phase == ClientPhase::Mission) state_.Post({"Menu", "game"});
        else if (state_.phase == ClientPhase::Dialog) state_.Post({"Tell", "Blank"});
        return;
    }
    if (!popup_.empty() && (event.code & 0x03ffffffu) == InputCode::kLeftButton &&
        (event.code & InputCode::kRelease) == 0 && !rawButtons_->Menus()->Contains({event.x,event.y})) {
        state_.Post({"Menu", "close"}); return;
    }
    if (!worldUiRouted && objectMenus_ && objectMenus_->Active() && rawButtons_->Menus()->Contains({event.x,event.y})) {
        static_cast<void>(rawButtons_->Event(event));rebuild_=true;return;
    }
    if (!world_ && editor_ && editor_->metaInput && popup_.empty()) { //메타는 같은 공통 GUI/모달 뒤에 실제 현재 Pool 선택기로 분배한다.
        if (rawButtons_->CurrentDialog()) return;const auto code=event.code&0x03ffffffU;const bool mouse=code==InputCode::kLeftButton || code==InputCode::kRightButton || code==InputCode::kMiddleButton;
        const bool ui=mouse && (rawButtons_->PickGump({event.x,event.y})!=0 || std::any_of(input_.Controls().begin(),input_.Controls().end(),[&](const GumpControl& control) { return GumpControlInput::Contains(control.rect,{event.x,event.y}); }));if (!ui && !rawButtons_->HasCapture() && editor_->metaInput->Event(event)) { rebuild_=true;return; }
    }
    if (world_ && userInput_ && state_.phase==ClientPhase::Mission && popup_.empty()) {
        if (rawButtons_->CurrentDialog()) return; //4d8c60의 현재 모달은 하위Gump가0을 반환해도 세계 입력 꼬리를 차단한다. 선행 일반명령/정지 중ESC는 위에서 처리한다.
        // 가상키도 상위16비트에 있으므로 버튼과 정확히 일치할 때만 좌표로 UI를 가린다.
        const auto mouse=event.code&0x03ffffffu;
        const bool mouseButton=mouse==InputCode::kLeftButton || mouse==InputCode::kRightButton || mouse==InputCode::kMiddleButton;
        const bool ui=mouseButton && (rawButtons_->PickGump({event.x,event.y})!=0 || std::any_of(input_.Controls().begin(),input_.Controls().end(),[&](const GumpControl& control) { return GumpControlInput::Contains(control.rect,{event.x,event.y}); }));
        if (!ui && !rawButtons_->HasCapture()) { if (const auto id=userInput_->Event(event)) { if (*id) OpenObjectMenu(*id,event); else world_->Select(0); } return; }
    }
    if (popup_.empty() && (state_.phase==ClientPhase::Dialog || state_.phase==ClientPhase::Briefing)) {
        const auto mouse=event.code&0x03ffffffU; // 모달 안내의 현재 실제 버튼 영역만 마우스 입력을 받는다.
        const bool mouseButton=mouse==InputCode::kLeftButton || mouse==InputCode::kRightButton || mouse==InputCode::kMiddleButton; // 뗌도 같은 대상에 전달한다.
        if (mouseButton && !rawButtons_->HasCapture() && std::none_of(input_.Controls().begin(),input_.Controls().end(),[&](const GumpControl& control) {
            return GumpControlInput::Contains(control.rect,{event.x,event.y});
        })) return; // 보존한 생산 패널을 안내창 뒤에서 조작하지 않는다.
    }
    const auto old=rawButtons_->Pressed();auto activated=worldUiRouted ? std::optional<int>{} : rawButtons_->Event(event);
    // 메뉴 행은 현재 어댑터에서 누름 즉시 실행한다. 돌 버튼 명령은 실제 부모 click 통지만 사용한다.
    if (popup_.empty() && !activated && std::any_of(input_.Controls().begin(),input_.Controls().end(),[&](const GumpControl& control) {
        return control.menu && GumpControlInput::Contains(control.rect,{event.x,event.y}); })) activated=input_.Event(event);
    if (old!=rawButtons_->Pressed()) rebuild_=true;
    if (activated && *activated >= 0 && static_cast<std::size_t>(*activated) < actions_.size()) {
        // 원본 004cebf0은 메인 메뉴 버튼 사건을 받자마자 시작 표시를 지운다.
        if (state_.phase == ClientPhase::MainMenu && popup_.empty() && client_.Edition()!=o::OriginalEdition::Patch1078) client_.ClearFullScreenState(); //10.78은 실제 메인 부모가 serial 검색 전에 이미 실행한다.
        ActivatePageAction(actions_[static_cast<std::size_t>(*activated)]);
    }
}
// 네이티브 입력 큐의 버튼 사건과 폴링 좌표를 같은 Gump 판정기에 전달한다.
bool UberGump::Input() {
    // 사건 순서를 유지한다. 첫 상태 전환만 다음 프레임에 예약된다.
    const auto mouse=PrepareGumpPointerBoundary(pointerBoundary_,static_cast<std::uint32_t>(client_.LanguageNumber()),
        [this](auto code) { return client_.Poll(code); },client_.Input()); // Alt/언어3 Ctrl·재진입 release를 같은 원본40칸 큐에 연결한다.
    if (missionRuntime_ && missionRuntime_->tutorial) static_cast<void>(missionRuntime_->tutorial->Event(mouse)); //4d65a7의Guide 폴링 통지는 반환을 검사하지 않는다. 일반 큐 Event의 소비 관문은 별개다.
    if (editor_ && editor_->metaInput) client_.GameplayGlobals()->selection.mouse={mouse.x,mouse.y}; //현재 메타 NSInfo/viewport 관문도 앞선 폴링의 같은 두 DWORD를 읽는다.
    PollEdgeScroll(mouse); // 같은 pointerBoundary flags와 장기 since를 사용하는004d65ac의 실제 호출 위치다.
    const auto mode=world_ ? world_->InputMode() : editor_ && editor_->metaInput ? client_.GameplayGlobals()->selection.mode : 0U; //메타도 같은005cae90을 읽고 페이지 phase로 대체하지 않는다.
    const bool cursorActive=world_ && world_->Session().CursorDropSettings().lockDepth==0; //445330은5325c8==0이며 Session의 같은 깊이를 읽는다.
    const bool logicalModal=!popup_.empty(); // 실제 HTML 모달은 공통 트리에 맡기고 남은 호스트 팝업의 overlay만 막는다.
    const auto pointer=rawButtons_->NotifyPointer(mouse,mode,cursorActive,!logicalModal); // 원본4d69f4는 일반 큐 Pop보다 앞이다.
    if (pointer.overlay && *pointer.overlay==0) client_.GetCursor().Set(1); // 실제466150 반환0의4d68cc→4d69d3 fallback이며 미호출과 구별한다.
    if (pointer.changed) rebuild_=true;
    if (pointer.activated) {
        if (*pointer.activated<0 || static_cast<std::size_t>(*pointer.activated)>=actions_.size()) throw std::logic_error("현재 포인터 부모 click 명령 범위 오류");
        if (state_.phase==ClientPhase::MainMenu && popup_.empty() && client_.Edition()!=o::OriginalEdition::Patch1078) client_.ClearFullScreenState(); //10.78의 실제 부모 통지와 중복하지 않는다.
        ActivatePageAction(actions_[static_cast<std::size_t>(*pointer.activated)]);rebuild_=true;
    }
    if (userInput_ && state_.phase==ClientPhase::Mission && popup_.empty()) userInput_->UpdateCursor(mouse); //4d6b46의446b50은 포인터/Overlay 처리 뒤이며 별도 임시 Scroll을 실행하지 않는다.
    if (!world_ && editor_ && editor_->metaInput && popup_.empty()) editor_->metaInput->Poll(mouse); //같은 원문 Overlay 뒤 위치에서 메타의 현재 이동 강조만 갱신한다.
    const bool cameraPan=PollCameraPan(mouse); //4d6b54의 현재 가운데 폴링은 커서 갱신 뒤·큐 Pop 앞이다.
    while (!client_.Input().Empty()) { //원문 pan 경로는 일반 명령을 재분배하지 않고 같은 큐를 소비한다.
        const auto event=client_.Input().Pop(false);if (cameraPan) continue; //Windows10/11 호환 Alt+Enter는 기존 WM_SYSKEYDOWN이 직접 처리하므로 일반 Enter/건설로 중복 전달하지 않는다.
        Event(event);if (quit_) return true;SyncTellPresentation(); //QuitApp의 즉시1반환 뒤 남은 큐/표시 합성을 실행하지 않는다.
    }
    if (input_.Move({mouse.x, mouse.y}, (mouse.code & InputCode::kOutside) == 0)) rebuild_ = true;
    if (world_ && world_->TakeChanged()) rebuild_=true;
    if (rebuild_) Compose();
    return quit_;
}
// 예약 명령은 메시지/입력보다 앞서 실행한다(원본 004b88c0 자리).
void UberGump::Tick() {
    if ((world_ && world_->Session().ConnectionCallbackActive()) || (editor_ && editor_->metaTransport && editor_->metaTransport->CallbackActive())) return; //재진입 Tick의 State/후속 명령 전체를 현재 수신 뒤 정상 Tick까지 유지한다.
    const auto action=state_.TakeAfterPoll([this] { if (client_.LocalStateGlobals()->State().next[0]==7 && !pendingMetaBattleSource_.expired()) PollEditorState();else if (!(world_ && world_->Session().PollLocalConnectionState())) PollEditorState(); }); //1ah의같은next7은현재World가있어도외부전체State7로처리하며Poll을두번소비하지않는다. 나머지기존미션/Guide/timeout순서는유지한다.
    const bool loading = state_.phase == ClientPhase::LoadingMission;
    if (action) Execute(*action);
    else if (!state_.Pending() && loading && world_ && !world_->Connecting() && !(missionRuntime_ && missionRuntime_->guided)) AdvanceBriefing();
    SyncTellPresentation();
    if (rebuild_) Compose();
}
// 실제 로컬 큐는 일반 원문 슬롯4에서 진행하며 완료가 Guide/입력보다 먼저 현재 UI를 설치한다.
void UberGump::PollMissionPackets() {
    PollLanLobby(); // 대기실 callback이 반환한 뒤에만 다음 Tick의 World 교체를 예약한다.
    const auto previousWorld=world_;const auto meta=editor_ ? editor_->metaTransport:nullptr; //콜백이 UI의 메타 필드를 반납해도 실제 전체 TCP/UDP owner는 외곽 복귀까지 살아 있다.
    if (meta) { meta->Pump();if (world_!=previousWorld) return; } //World없는메타도같은slot4에서TCP→UDP수신을진행하며66h발견은같은Run을동기실행한다.
    const auto world=world_; // 이 폴링과 완료 후처리가 끝날 때까지 실제 표시/Session 소비자를 함께 보유한다.
    if (!world || world->PreparingFort()) return;world->Session().PumpLocalConnection();
    if (world_!=world) return; // callback이 현재 호스트를 바꿨으면 이전 결과를 새 월드에 공개하지 않는다.
    if (world->Connecting() && !missionRuntime_ && ((lan_ && lan_->start && lan_->start->consumed) || (nativeBattleStart_ && nativeBattleStart_->consumed)) && world->CompleteLocalConnection()) { FinishLanBattleWorld();nativeBattleStart_.reset(); }
    else if (world->Connecting() && missionRuntime_ && worldMissionToken_==missionRuntime_->saveToken && world->CompleteLocalConnection()) FinishMissionWorldLoad(); // 이전 미션이면 호스트 World의 공개도 건너뛰고 원래 cleanup까지 닫아 둔다.
}
// 원본 루프8번에서 현재 Guide의 상태/창 관문과0..6 단계를 한 번 실행한다.
void UberGump::MissionFrame() {
    if (guide_) guide_->Run();
}
// 호스트 어댑터가 자기 자신을 소유한 Kernel을 중첩 호출하지 않도록 준비를 분리한다.
void UberGump::PrepareKernelFrame() { if (world_) world_->PrepareKernelFrame(); }
// 현재 단일 Kernel의 이동/생성/삭제가 끝난 같은 프레임에서만 표시 사본을 갱신한다.
void UberGump::FinishKernelFrame() { if (world_) world_->FinishKernelFrame(); }
// 실제 World가 없는 메뉴에서는 원문 비boss 자연 반환까지 같은 전역을 갱신한다.
void UberGump::PollBattleResources() {
    if (world_) { if (!world_->PreparingFort()) world_->Session().RunBattleResources();return; }auto& game=*client_.GameplayGlobals();auto& local=*client_.LocalStateGlobals();
    o::RunBattleResourceFrame({*game.GameAux(),local.LoginEnd(),client_.Time().game,game.Authority()->boss,client_.SurfaceStorage()->ownerMode.battle,game.world.missionPost,game.world.editor,game.player.battleOwner,*missionOptions_,[this] { return std::span<const std::uint8_t>(client_.PlayerRecords()->Records()); }},{}); // 월드 없는 현재 비boss에서는 배치/시계/송신 장치를 합성하지 않는다.
}
// 원본의미니맵프레임은게임정지와별개로커널보다먼저실행한다.
void UberGump::RefreshMinimap() { if (rawButtons_->Minimap() && rawButtons_->Minimap()->Refresh()) rebuild_=true; }
// 실제 공통 삭제 큐의 후반 반납 뒤 표시 상태를 현재 부모에서 다시 조회한다.
void UberGump::CleanupGumps() {
    if (rawButtons_->CleanupGumps()) rebuild_=true;SyncTellPresentation();
}
// 원본 일반/영상잠금 양쪽 가지에서 같은 idle을 한 번 실행하고 표시 변화는 뒤쪽 Frame 합성으로 넘긴다.
void UberGump::IdleGumps() {
    if (rawButtons_->IdleGumps()) rebuild_=true;SyncTellPresentation();
}
// 커널의 이동·날씨와 비동기 Tell을 같은 프레임에 반영한다. 페이지가 같으면 입력 영역/누름 상태를 유지한다.
void UberGump::Frame() {
    if (world_ && world_->TakeChanged()) rebuild_=true;
    if (rebuild_) Compose();
}
// UI 합성을 예약하고 이미 제출된 정적 픽셀도 다시 그리게 한다.
void UberGump::Refresh() { rebuild_=true;client_.GetRenderer().InvalidateAll(); }
// 새 RGB를 월드에 공급한 뒤 UI의 장식·글자·선택 색도 다시 합성한다.
void UberGump::PaletteChanged() {
    if (world_) world_->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical(),&client_.ColorSortInput()->Maps());
    if (auto* display=MetaSpriteDisplay()) display->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical(),&client_.ColorSortInput()->Maps()); //메타도새파일팔레트와같은9색표를사용한다.
    rawButtons_->SetPalette(ButtonColors(client_.GetScreen().Colors()));
    if (rawButtons_->Production()) rawButtons_->Production()->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical(),&client_.ColorSortInput()->Maps());
    if (rawButtons_->Minimap()) rawButtons_->Minimap()->SetPalette(client_.GetScreen().Colors());
    Refresh();
}
// 선택한 원본 명령의 작은 부분집합. 지원하지 않는 항목은 활성화하지 않는다.
void UberGump::Execute(const DialogAction& action) {
    if (o::AsciiLower(action.command)=="loadlanbattleworld") { LoadLanBattleWorld();return; } // 실제 명단/State7 예약 뒤 다음 호스트 Tick에서만 World를 교체한다.
    if (o::AsciiLower(action.command)=="multiplayer") { if (client_.Edition()!=o::OriginalEdition::Patch1078) { Tell("NotImplemented");return; }client_.Input().PushCommand(0x020000c4U,0,0);return; } //10.78 메인 버튼은 원문 일반 Connect의 마지막 Fort 선택 경로를 사용하며 CD의 기존 보류 경계를 유지한다.
    if (o::AsciiLower(action.command)=="loadmissionworld") { LoadMissionWorld();return; } // 비 Tutorial의 Start도 실제 접속 완료에 맡긴다.
    if (o::AsciiLower(action.command)=="loadmissionfort") { if (!missionRuntime_ || !missionRuntime_->guided) throw std::logic_error("현재 Tutorial 없는 지연 요새 로드");LoadMissionWorld(action.argument);return; } // 같은 전환 큐의 다음 루프에서만 설치한다.
    if (QueueMissionControlAction(action.command,action.argument,client_.Input(),scriptStrings_)) return;
    if (QueueFortSelectionAction(action.command,action.argument,client_.Input(),scriptStrings_)) return;
    if (GameMenuScriptCode(action.command)!=0xffffffffU) { QueueScriptCommand(action.command+','+action.argument,client_.Input(),scriptStrings_,{GameMenuScriptCode,{}});return; } // 원본확인창/메뉴닫기효과도같은정수명령큐를사용한다.
    if (KnowledgeScriptCode(action.command)!=0xffffffffU) { QueueScriptCommand(action.command+','+action.argument,client_.Input(),scriptStrings_,{KnowledgeScriptCode,{}});return; }
    const auto command = o::AsciiLower(action.command);
    if (command == "quit") { QueueScriptCommand("RequestQuitApp,0",client_.Input(),scriptStrings_,{GameMenuScriptCode,{}});return; } // 이전 호스트 명령도 원문 요청으로 변환하며 QuitApp의99/효과음/종료 반환을 생략하지 않는다.
    if (command == "tell") { Tell(action.argument, state_.phase == ClientPhase::Briefing); return; }
    if (command == "startdemo") { BeginMission(action.argument,1);return; }
    if (command == "donothing" || command == "resume") {
        if (state_.phase == ClientPhase::Briefing) AdvanceBriefing(); else Tell("Blank"); return;
    }
    if (command == "menu") {
        if (action.argument == "close") { popup_.clear(); ClearPage(); if (world_ && state_.phase==ClientPhase::Mission) client_.Pause(false); RebuildControls(); }
        else OpenMenu(action.argument);
        return;
    }
    if (command == "toggle") {
        auto& config = client_.Configuration(); config.SetInt(action.argument, config.GetInt(action.argument) == 0 ? 1 : 0);
        // 소리·음악·좌우 바꿈은 원본 Interpret Options처럼 장치와 현재 곡에 바로 반영한다.
        if (action.argument == "sound" || action.argument == "music" || action.argument == "swapLeftRightSpeakers") client_.ApplyAudioOptions();
        popup_.clear(); ClearPage(); if (world_ && state_.phase==ClientPhase::Mission) client_.Pause(false); RebuildControls(); return;
    }
    if (command == "volume") {
        const auto equal = action.argument.find('='); client_.Configuration().SetInt(action.argument.substr(0, equal), o::ConfigParseLong(action.argument.substr(equal + 1)));
        client_.ApplyAudioOptions(); // 효과음·음악 음량 단계를 장치에 바로 적용한다.
        popup_.clear(); ClearPage(); if (world_ && state_.phase==ClientPhase::Mission) client_.Pause(false); RebuildControls(); return;
    }
    if (command == "resolution") {
        const int width = o::ConfigParseLong(action.argument); // 폭만 받는 옛 명령이므로 목록에서 같은 폭의 첫 행 높이를 쓴다(원본 세 폭은 4:3 높이).
        int height = 768;
        for (const auto& size : kScreenResolutionChoices) if (static_cast<int>(size[0]) == width) { height = static_cast<int>(size[1]); break; }
        client_.ChangeResolution(width, height); popup_.clear(); ClearPage(); if (world_ && state_.phase==ClientPhase::Mission) client_.Pause(false); RebuildControls(); return;
    }
    throw std::runtime_error("Command pending: " + action.command);
}
// Tell의 호출 단계와 무관하게 다음 합성에서 이전 입력 트리를 폐기하도록 예약한다.
void UberGump::RebuildControls() { controlsChanged_=true;rebuild_=true; }
// 배경 합성과 글자 목록 제출은 입력 판정과 분리한다. 페이지 교체 때만 이전 캡처/등록을 정리한다.
void UberGump::Compose() {
    const bool controls=controlsChanged_; // Tick/Input/Kernel 뒤 Frame 중 어느 합성이든 같은 교체 예약을 소비한다.
    const int width = client_.GetScreen().Width(), height = client_.GetScreen().Height();
    const ScreenRect all{0, 0, width, height};
    const bool nativeFrame=client_.Assets().Edition()==o::OriginalEdition::Patch1078; //10.78은 실제 Frame/Gump 표시를 사용하고10.72의 기존 합성은 별도로 유지한다.
    auto canvas = std::make_shared<IndexedImage>(); canvas->width = width; canvas->height = height;
    canvas->indices.resize(static_cast<std::size_t>(width) * height); canvas->opacity.assign(canvas->indices.size(), 255);
    if (!nativeFrame) Tile(*canvas,clouds_,all); //10.78 구름은 실제 cache/카메라/dirty로 그리며 정적 GIF를 배경으로 대신하지 않는다.
    std::vector<RenderText> text; std::vector<GumpControl> regions;
    // 마지막 파일 로딩의 실제 버튼 색/문맥을 사용한다. 일시 번개 팔레트에서 색 번호를 다시 찾지 않는다.
    const auto buttonColors=ButtonColors(client_.GetScreen().Colors());
    warnings_->SetView(width,height,client_.GetScreen().Colors());
    std::vector<RenderSprite> worldSprites,dialogSprites; // 월드와본문그림은같은실제자산을읽으며UI본문목록은해당패널에만제출한다.
    bool commonDrawSubmitted=false; // 같은 raw 수집/정렬을 배경 또는 Overlay에 이미 제출했는지 기록한다.
    if (!nativeFrame) client_.GetRenderer().SetOverlay(nullptr); //10.78은 단일 목록으로 선택 표시를 그리며 매 합성의 전체 무효화를 만들지 않는다.
    client_.GetRenderer().SetOverlayProcedure({});
    if (world_ && !world_->Connecting() && state_.phase!=ClientPhase::LoadingMission) {
        world_->Resize(width,height);if (!nativeFrame) worldSprites=world_->Sprites(); //10.78은 실제 공통 수집/Draw에서만 가상 표시를 호출하여 부수효과를 중복 실행하지 않는다.
        auto& cursorDisplay=world_->Session().Cursor().Display();cursorDisplay.Settings().gridOption=client_.GetScreen().Flags();
        cursorDisplay.SetText(client_.Fonts(),[this](std::string_view key) { const auto bytes=o::EncodeDisplayText(client_.Translate(key));return std::string(bytes.begin(),bytes.end()); });
        if (!nativeFrame && !client_.GetRenderer().HasExtraCollection()) client_.GetRenderer().SetExtraCollection([this](ScreenRect clip,std::uint32_t flags) {
            if (!world_ || state_.phase!=ClientPhase::Mission || !popup_.empty()) return std::vector<RenderSprite>{};
            return world_->Session().Cursor().Display().Sprites(clip,flags);
        });
        // 정지한 대화상자만 월드를 배경에 합성하고 실제 조작 화면은 살아 있는 스프라이트를 제출한다.
        if (!nativeFrame && (state_.phase!=ClientPhase::Mission || !popup_.empty())) {
            client_.GetRenderer().SetBackground(canvas); client_.GetRenderer().SetScene(worldSprites);
            canvas = std::make_shared<IndexedImage>(client_.GetRenderer().SceneImage());
        }
    }
    if (controls) { labels_.clear(); actions_.clear(); input_.Cancel();rawButtons_->Cancel(); }
    // 같은 순서의 영역/라벨/명령을 제출한다. 그림만 갱신할 때도 번호가 유지된다.
    const auto control = [&](const DialogItem& item, ScreenRect r) {
        const int id = static_cast<int>(regions.size()); regions.push_back({id, r, item.menu, item.enabled, item.instant});
        if (controls) { labels_.push_back(item.label); actions_.push_back(item.action); }
        return id;
    };
    // 메인 버튼의 실제 노드를 준비한다. 표면/선/글자는 GIF와 같은 원본 Gump 정렬에서 한 번만 출력한다.
    const auto button = [&](const DialogItem& item, int x, int y, int w) {
        const int id = control(item, {x, y, x + w, y + kButtonHeight + 1});
        if (client_.Assets().Edition()==o::OriginalEdition::Patch1078) return; //복원 대상10.78은 공통 깊이 정렬에서 한 번 출력한다.
        const auto& font=client_.Fonts().Get(0);const auto contexts=ButtonTextContexts(buttonColors); //CD의 기존 호스트 표시 계약은10.78 공통 부모 접합과 구별하여 보존한다.
        const bool drawRaw=!controls && (popup_.empty() || state_.phase==ClientPhase::MainMenu);const auto label=drawRaw ? rawButtons_->Label(id) : FontBytes(item.label);const ScreenRect rect{x,y,x+w,y+kButtonHeight}; //기존 CD 버튼의 라벨/화면 경계다.
        const auto background=drawRaw ? rawButtons_->BackgroundPlan(id,all) : PlanGumpBackground({rect.left+1,rect.top+1,rect.right-1,rect.bottom-1},all,buttonTextureMetrics_.width,buttonTextureMetrics_.height,GumpBackgroundFlags::kButton); //기존 CD 배경 출력의 타일 계획이다.
        DrawGumpBackground(*canvas,decorations_.at("A01"),buttonTextureOffset_,background,client_.GetScreen().ShadeMaps());
        const auto plan=drawRaw ? rawButtons_->DrawPlan(id,font.Measure(label),font.Height()) : PlanButtonDraw({rect,font.Measure(label),font.Height(),0,0,item.enabled ? 0U : 1U},contexts,buttonColors.edge); //기존 CD 호스트의 글자/테두리 계획이다.
        DrawGumpLines(*canvas,plan.lines);text.push_back({&font,label,x+plan.textOffset.x,y+plan.textOffset.y,static_cast<std::uint8_t>(plan.text.color),static_cast<std::uint8_t>(plan.text.shadowColor),{plan.text.shadowX,plan.text.shadowY},(plan.text.flags&2)!=0,(plan.text.flags&4)!=0}); //CD에10.78의 미복원 전체 메뉴 부모를 대신 적용하지 않는다.
    };
    const bool nativeHud=rawButtons_->UberRoot() && rawButtons_->UberRoot()->Current(); //원문 전체 루트가 현재이면 World 생성 전에도 폐기된 호스트 MainMenu 버튼을 그리지 않는다.
    if ((state_.phase==ClientPhase::MainMenu && (!world_ || !nativeHud)) || (!mission_ && !popup_.empty())) {
        Paste(*canvas, title_, (width - 640) / 2, (height - 480) / 2, all);
        const int x = width / 2 - 156, y = height / 2 - 73;
        const std::string campaign = client_.Assets().Edition()==o::OriginalEdition::Patch1078 ? "UCampaign" : Script("tell").Section("UCampaign") ? "UCampaign" : "Campaign"; // 원문10.78 MainMenu의 고정 target이며 영구 tell Config를 새로 등록하지 않는다.
        // 원본 10.78의 기본8개·감지 성공 시 Intro를 포함한9개 버튼이다. 원문 순서대로 두 행에 배치한다.
        std::vector<DialogItem> buttons{
            {"Campaign", {"Tell", campaign}}, {"Multiplayer", {"Multiplayer", ""}}, {"Demo", {"Tell", "DEMOVILLE"}}, {"Help", {"Menu", "help"}},
            {"Edit", {"AskEditFort", "0"}}};
        if (client_.Edition()==o::OriginalEdition::Patch1078 && client_.LocalStateGlobals()->IntroAvailable()) buttons.push_back({"Intro",{"PlayIntro","0"}}); // 원문0054db98의 최초 감지값이며 매 Compose마다 파일을 재검사하지 않는다.
        buttons.insert(buttons.end(),{{"Credits", {"Tell", "Credits"}}, {"Options", {"Menu", "options"}}, {"Quit", {"RequestQuitApp", "0"}}});
        // 펼침 메뉴 동안 밑에 보이는 버튼은 입력 영역에 넣지 않는다.
        for (std::size_t i = 0; i < buttons.size(); ++i) button(buttons[i], x + static_cast<int>(i % 4) * kButtonPitch, y + static_cast<int>(i / 4) * 23, kButtonWidth);
    }
    if (state_.phase == ClientPhase::Mission && popup_.empty()) {
        // 원본 HUD와 월드를 합성한다. 가로 메뉴는 기존 ESC 입력으로 연다.
        if (!nativeFrame) client_.GetRenderer().SetBackground(canvas);if (nativeFrame) text.clear();client_.GetRenderer().SetScene(std::move(worldSprites),std::move(text)); //실제 Native 배경은 dirty별 Frame 장치가 그린다.
        if (world_ && !nativeFrame) client_.GetRenderer().SetOverlay(world_->SelectionImage()); //10.78 선택 표시는 공통 SpriteDraw의 같은 실제 SID 경로다.
        if (!nativeFrame && rawButtons_->Menus()->HasOverlay()) {
            commonDrawSubmitted=true;
            client_.GetRenderer().SetOverlayProcedure(RenderProcedure{all,[this](std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
                rawButtons_->Menus()->Draw(pixels,pitch,height,client_.GetScreen().Colors(),client_.GetScreen().ShadeMaps(),clip);
            }});
        }
    } else {
        if (!popup_.empty()) {
            if (controls) {
                regions.clear();labels_.clear();actions_.clear();
                // 첫 합성에서는 호스트 DialogAction 번호만 준비한다. 실제 경계는 아래 원본 메뉴 생성 뒤 읽는다.
                for (const auto& item:page_.items) control(item,{0,0,1,1});
            } else {
                regions=rawButtons_->Menus()->Controls();
                // 먼저 기존 바탕 글자를 실제 픽셀로 합성하고 그 위에 같은 원본 메뉴 Draw를 실행한다.
                RasterMenuBackdropText(*canvas,text);text.clear();
                if (!nativeFrame && !nativeHud) { rawButtons_->Menus()->Draw(canvas->indices,width,height,client_.GetScreen().Colors(),client_.GetScreen().ShadeMaps());commonDrawSubmitted=true; } //10.78은 미니맵 Refresh 뒤 같은 단일 목록에서 그리며 CD의 기존 합성만 유지한다.
            }
        } else if (state_.phase == ClientPhase::Dialog || state_.phase == ClientPhase::Briefing) {
            const auto background=std::string(HtmlBackgroundName(htmlGlobals_));
            if (!background.empty()) {
                auto it = decorations_.find(background);
                if (it == decorations_.end()) {
                    // 원본 00419ec0도 파일이 없으면 배경을 만들지 않는다(nsstart 등은 배포 자료에 없을 수 있다).
                    if (const auto bytes = client_.Files().TryRead(client_.Configuration().PathSpec("BackgroundGifSpec", background)))
                        it = decorations_.emplace(background, DecodeGif(*bytes)).first;
                }
                if (it != decorations_.end()) Paste(*canvas, it->second, (width - static_cast<int>(it->second.width)) / 2, (height - static_cast<int>(it->second.height)) / 2, all);
            }
            if (!nativeFrame && !controls && !nativeHud) { rawButtons_->Menus()->Draw(canvas->indices,width,height,client_.GetScreen().Colors(),client_.GetScreen().ShadeMaps());commonDrawSubmitted=true; } //10.78 브리핑도 같은 단일 목록에서 현재 Refresh 결과를 출력한다.
        } else if (!nativeFrame && state_.phase == ClientPhase::LoadingMission && !controls && !nativeHud) {
            rawButtons_->Menus()->Draw(canvas->indices,width,height,client_.GetScreen().Colors(),client_.GetScreen().ShadeMaps());
            commonDrawSubmitted=true;
        }
        if (!nativeFrame) client_.GetRenderer().SetBackground(canvas); //10.78은 같은 nullptr 배경을 다시 설치해 dirty를 전체 갱신으로 바꾸지 않는다.
        if (nativeFrame) { text.clear();client_.GetRenderer().SetScene({}); } //브리핑/ESC 뒤의 월드와 선택 표시도 같은 실제 목록으로 한 번 출력한다.
        else client_.GetRenderer().SetScene(std::move(dialogSprites),std::move(text));
    }
    if (controls) {
        if (state_.phase!=ClientPhase::LoadingMission) rawButtons_->Menus()->CloseMissionLoading();
        std::vector<std::string> originalLabels;originalLabels.reserve(labels_.size());
        // 원본 돌 버튼의 63바이트 라벨은 UTF-8가 아닌 기존 게임 글꼴 바이트를 사용한다.
        for (const auto& label:labels_) originalLabels.push_back(FontBytes(label));
        if (popup_.empty()) rawButtons_->SetControls(regions,originalLabels,width,height,{buttonTextureType_,buttonTextureFrame_,
            static_cast<std::int16_t>(buttonTextureMetrics_.width),static_cast<std::int16_t>(buttonTextureMetrics_.height),buttonColors},world_!=nullptr || (rawButtons_->UberRoot() && rawButtons_->UberRoot()->Current()) || client_.HtmlWindowGlobals()->window.instances!=0,client_.HtmlWindowGlobals()->window.instances!=0,
            state_.phase==ClientPhase::MainMenu && client_.Assets().Edition()==o::OriginalEdition::Patch1078 ? std::optional<std::int32_t>{-83} : std::nullopt,
            state_.phase==ClientPhase::MainMenu && client_.Edition()==o::OriginalEdition::Patch1078); //원문004d0530의 실제 container-80/stone-83/모달/idle을 생성하거나 안내창 뒤 같은 부모를 보존한다.
        if (popup_.empty() && state_.phase==ClientPhase::MainMenu && client_.Edition()==o::OriginalEdition::Patch1078) rawButtons_->LayoutMainMenuControls(regions); // 실제 측정·원문 SCREENH/3 배치의 경계를 같은 검사/사용자 입력에도 공급한다.
        if (!nativeFrame && state_.phase==ClientPhase::LoadingMission && !(missionRuntime_ && missionRuntime_->guided)) {
            rawButtons_->Menus()->SetPalette(client_.GetScreen().Colors());
            // 원본 기본 HUD의 수직 구분선 오른쪽 지도 사각형이다. 전체 dock 드래그 연결 전의 초기값85를 사용한다.
            rawButtons_->Menus()->ShowMissionLoading(LoadingHtmlContext({85,0,width,height}),FontBytes(client_.Translate("Starting Mission...")),[this] { return FontBytes(client_.Translate("Cancel")); });
        }
        if (popup_.empty() && world_ && !world_->Connecting()) {
            if (client_.Assets().Edition()==o::OriginalEdition::Patch1078) InstallHud();
            else { // 미복원 CD 전체 Uber에10.78 몸체를 적용하지 않고 기존 설치 계약을 유지한다.
            if (!rawButtons_->Production()) rawButtons_->EnableProduction(world_->Session(),client_.Assets(),RawProductionHooks{
                [this] { return client_.WallSeconds(); },[this](std::string_view name) { client_.WorldAudio().Sounds().PlayByName(name,0,0,0,0,0); },
                [this](std::string_view key) { state_.Post({"Tell",std::string(key)}); },
                [this](std::string_view key,std::int32_t fallback) { client_.Configuration().ReadInt(key,fallback);return fallback; },
                [this](std::string_view key) { return FontBytes(client_.Translate(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(key.data()),key.size()}))); },
                [this](const InventoryItemNode& item,ScreenPoint point,const std::function<bool(o::Sid)>& affordable) { if (!world_ || !objectMenus_) throw std::logic_error("생산 정보의 현재 월드 없음");return objectMenus_->OpenProduction(*world_,item,point,affordable); }});
            auto& production=*rawButtons_->Production(); // 같은 미션의 안내/복귀는 예산/등록ID를 재생성하지 않는다.
            production.Resize(height);production.SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical(),&client_.ColorSortInput()->Maps());
            if (!rawButtons_->Minimap()) rawButtons_->EnableMinimap(world_->Session(),RawMinimapHooks{
                [this] { return world_->MinimapView(); },[this](auto x,auto y) { world_->CenterCamera(x,y); },
                [this](auto x,auto y) { world_->MoveFromMinimap(x,y); },[this] { return world_->InputMode(); },[this] { return client_.Poll(0xffff); }});
            rawButtons_->Minimap()->Resize(height);rawButtons_->Minimap()->SetPalette(client_.GetScreen().Colors());
            if (!rawButtons_->Menus()->NsInfoRoot()) {
                RawNsInfoContext context;context.globals=client_.NsInfoGlobals();context.challenge=client_.ChallengeGlobals();context.gameplay=client_.GameplayGlobals();context.storage=client_.SurfaceStorage(); // 같은 실제 Client의 고정 소유자다.
                context.read=[this](auto address) { if (address==0x5411c8U) return static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+client_.Assets().Find("fortGump").block);if (address==0x5bec3cU) return client_.GetScreen().Colors().basic[8];if (address==0x5bec2cU) return client_.GetScreen().Colors().basic[7];if (address==0x5b5e58U) return client_.GetScreen().Colors().basic[1];if (address>=0x5accfcU && address<=0x5acd18U && !(address&3U)) return client_.GetScreen().Colors().named.at(42U+(address-0x5accfcU)/4U).index;if (address>=0x5c78b0U && address<=0x5c78ccU && !(address&3U)) return client_.GetScreen().Colors().named.at(50U+(address-0x5c78b0U)/4U).index;if (address==0x59afd8U) return client_.GetScreen().Colors().basic[5];if (address==0x5c78d0U) return client_.GetScreen().Colors().named.at(36).index;if (address==0x5c85c8U) return client_.LocalStateGlobals()->MetadataMode();throw std::out_of_range("NSInfo 현재 원본 전역 미연결"); };
                context.translate=[this](auto value) { return FontBytes(client_.Translate(value)); };context.substitute=[this](std::string_view text) { return FontBytes(client_.Configuration().SubstituteLocal(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };context.substitute=[this](std::string_view text) { return FontBytes(client_.Configuration().SubstituteLocal(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };context.storeText=[this](auto value) { const auto token=scriptStrings_.Store(value);if (token>0x0fffffffU) throw std::overflow_error("NSInfo 공유 문자열 고리 별칭 소진");return 0x70000000U|token; };context.text=[this](auto token) { return scriptStrings_.Resolve(token&0x0fffffffU); };
                BindClientMetaPlayerMenu(context,client_,*rawButtons_->Menus());BindMetaNsInfoInput(context);const auto token=rawButtons_->ConstructNsInfo(std::move(context));auto& node=*rawButtons_->Menus()->Node(token);node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(static_cast<double>(width)*node.scale);node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(static_cast<double>(height)*node.scale);node.flags|=0x20000U; // 기존 월드 UI 설치 경계에서 원문 Uber의 크기/플래그/부착만 적용한다. 전체 Uber 교체는 후속이다.
                rawButtons_->Menus()->Attach(token,0,0,rawButtons_->RootToken(),0);
            }
            }
        }
        if (!popup_.empty()) {
            auto& menus=*rawButtons_->Menus();menus.SetPalette(client_.GetScreen().Colors());
            menus.Open({width/2+2,height/2-27},[&](const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump&) {
                widgets.BeginVertical();
                // 기존 팝업 동작의 호스트 인덱스만 연결하며 실제 게임 명령의 원본 토큰은 이 어댑터와 구분한다.
                for (std::size_t index=0;index<page_.items.size();++index) {
                    const auto& item=page_.items[index];controls.AddChecked(item.checked,FontBytes(item.label),0x01ffffffU,static_cast<std::uint32_t>(index+1),true,item.enabled);
                }
            });
            regions=menus.Controls();
        }
        input_.SetControls(std::move(regions));
        if (world_ && !world_->Connecting()) world_->Session().Cursor().GumpsChanged(); // 접속 준비 중에는 미생성 커서의 입력 관문을 열지 않는다.
        // 새 페이지는 같은 raw 노드로 한 번 다시 합성하며 중간 어댑터 그림은 화면에 제출하지 않는다.
        controlsChanged_=false;Compose();return;
    }
    if (!nativeFrame && !commonDrawSubmitted && (state_.phase==ClientPhase::MainMenu || nativeHud || rawButtons_->Menus()->KnowledgeOpen() || rawButtons_->Menus()->HelpOpen() || warnings_->HasMessages())) client_.GetRenderer().SetOverlayProcedure(RenderProcedure{{0,0,width,height},[this](std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
        rawButtons_->Menus()->Draw(pixels,pitch,height,client_.GetScreen().Colors(),client_.GetScreen().ShadeMaps(),clip);
    }}); //CD의 기존 후합성만 유지한다.10.78 UI는 실제 공통 수집/정렬에서 월드와 함께 그린다.
    if (world_) world_->TakeChanged();
    rebuild_ = false;
}
// 자동 검사는 라벨 좌표를 읽고 실제 버튼 입력을 보낸다. 명령을 직접 호출하지 않는다.
std::optional<ScreenPoint> UberGump::ControlPoint(std::string_view label) const {
    if (rawButtons_->UberRoot() && label.starts_with("hud:")) return rawButtons_->UberRoot()->Point(label); // 실제 구분선의 현재 경계로 정상 입력을 보낸다.
    if (rawButtons_->Minimap() && label.starts_with("minimap:")) return rawButtons_->Minimap()->Point(label);
    if (rawButtons_->Production() && label.starts_with("production:")) return rawButtons_->Production()->Point(static_cast<std::uint32_t>(std::stoul(std::string(label.substr(11)))));
    if (world_ && !world_->Connecting() && (label.starts_with("world:") || label.starts_with("cell:") || label.starts_with("placement:") || label.starts_with("placement-near:") || label.starts_with("placement-reach:") || label.starts_with("placement-fire:"))) return world_->Point(label);
    if (rawButtons_->Menus()->HasMenus() || rawButtons_->Menus()->HtmlWindowsOpen() || rawButtons_->CurrentDialog() || state_.phase==ClientPhase::LoadingMission) if (const auto point=rawButtons_->Menus()->Point(FontBytes(label))) return point;
    // 동일 라벨의 첫 영역을 선택한다.
    for (const auto& control : input_.Controls()) if (labels_.at(static_cast<std::size_t>(control.id)) == label)
        return ScreenPoint{(control.rect.left + control.rect.right) / 2, (control.rect.top + control.rect.bottom) / 2};
    return {};
}
// 실제 메뉴 상태·활성 조건·원본 미션 로드 결과를 한 파일에 기록한다.
std::string UberGump::Report(bool cameraOnly) const {
    std::ostringstream out;
    out<<client_.ScreenDiagnostics(); // 실제 OS 창과 논리/확대 출력의 관찰만 추가한다.
    out<<client_.RendererFrameDiagnostics(); //같은 실제 Frame/구름 요청·캐시·마지막 표시 인자만 관측한다.
    const auto& capture=client_.ScreenCapture();const auto& captureState=capture.State();out<<"screen_capture\t"<<captureState.count<<'\t'<<captureState.active<<'\t'<<captureState.first<<'\t'<<capture.AllocatedFrames()<<'\n'; //검사 보고만 같은 현재 개수/active/first/실제 할당 수를 읽으며 상태나 파일을 바꾸지 않는다.
    out<<"recheck_cd\t"<<client_.CdGlobals()->RecheckSerial()<<'\n'; //검사 보고만 005c85ec의 현재 serial을 읽으며 값을 바꾸지 않는다(단위696).
    out<<"intro_available\t"<<client_.LocalStateGlobals()->IntroAvailable()<<'\n'; // 실제 시작 감지 DWORD의 읽기 전용 관측이다.
    out<<rawButtons_->MainMenuReport()<<"auto_demo\t"<<client_.LocalStateGlobals()->AutoDemo()<<'\n'; //현재 실제 부모/기한/옵션만 읽고 시간이나 실행 여부를 바꾸지 않는다.
    out<<ReportClientBridgeDecay(client_); //현재 메뉴/World가 공유하는 실제 스캔의 읽기 전용 CLI 관측이다.
    const auto& camera=client_.GameplayGlobals()->Camera();out<<"raw_camera_frame\t"<<camera.RendererView().pixels[0]<<'\t'<<camera.RendererView().pixels[1]<<'\t'<<camera.Target().pixels[0]<<'\t'<<camera.Target().pixels[1]<<'\t'<<client_.GameplayGlobals()->PatchDiagnostic()<<'\t'<<client_.LocalStateGlobals()->MetaRebuild()<<'\t'<<pointerBoundary_.initializationFlags<<'\t'<<edgeScrollSince_<<'\n'; //실제 적용/요청·공유 전역만 관측한다.
    out << "phase\t" << static_cast<int>(state_.phase) << "\npage\t" << Field(pageName_) << "\npopup\t" << Field(popup_) << "\n";
    out << "size\t" << client_.GetScreen().Width() << '\t' << client_.GetScreen().Height() << "\n";
    std::int32_t themeMode{};client_.Configuration().ReadInt("theammode",themeMode);out<<"raw_view_keys\t"<<hideBuildings_<<'\t'<<worldRedraw_<<'\t'<<themeMode<<'\n'; // 실제 표시 DWORD 둘과 현재 설정만 읽고 입력 효과를 만들지 않는다.
    const auto& ownerTables=client_.ColorSortInput()->OwnerTables();out<<"raw_owner_tables\t"<<ownerTables->Enabled()<<'\t'<<(ownerTables->Ready() ? 1 : 0);
    if (ownerTables->Ready()) for (const auto type:std::array<std::uint32_t,8>{85,71,128,97,94,111,141,137}) out<<'\t'<<ownerTables->Tokens()[type]; // 같은 현재 포인터 표의 읽기 전용 관측이다.
    out<<'\n';
    out<<"sound_first_free\t"<<client_.WorldAudio().Names().FirstFree()<<'\n'; // 현재 이름표 끝만 읽는 진단이며 재생/적재/장치 상태를 변경하지 않는다.
    out << "paused\t" << client_.Paused() << "\nsound\t" << client_.Configuration().GetInt("sound") << "\n";
    out<<"mouse_cursor\t"<<client_.GetCursor().Index()<<'\t'<<client_.Active()<<'\n'; // 검사 보고서는 실제 표시 장치의 커서 번호와 창 활성을 읽기만 한다. 선택/대상 판정은 바꾸지 않는다.
    if (cameraOnly) { out<<"current_dialog\t"<<rawButtons_->CurrentDialog()<<"\ngame_menu_bar\t"<<(gameMenuState_ ? gameMenuState_->menuSerial : 0U)<<'\t'<<GameMenuOpen()<<"\ncamera_runtime\t"<<(world_!=nullptr)<<'\t'<<client_.Time().game<<'\t'<<client_.Time().number<<'\t'<<client_.GetKernel().Size()<<'\n';return out.str(); } //실제 같은 프레임/장치 값만 읽어 Win32 입력 관측의 큰 파일 재쓰기를 줄인다.
    out<<"map_menu\t"<<rawButtons_->Menus()->HtmlMenuColumns()<<'\t'<<mapCatalog_->Cached().count<<'\t'<<mapCatalog_->Cached().dirty<<'\t'<<mapFileSpec_.Current()<<'\n'; // 실제 부착된 열과 캐시 상태만 관찰한다. 선택/로딩 성공을 합성하지 않는다.
    out<<"html_window_state\t"<<client_.HtmlWindowGlobals()->window.instances<<'\t'<<rawButtons_->DialogCount()<<'\t'<<rawButtons_->Menus()->HtmlWindowsOpen()<<'\n'; // Kill 뒤 인스턴스와 즉시 모달 반납을 구별하는 현재값이다.
    out<<"main_menu_globals\t"<<client_.LocalStateGlobals()->FirstMenuPending()<<'\t'<<client_.LocalStateGlobals()->UserUpdated()<<'\t'<<client_.LocalStateGlobals()->TellTips()<<'\t'<<client_.LocalStateGlobals()->TipNumber()<<'\t'<<client_.WorldAudio().ResultState()<<'\n'; //동일 Client의 현재 DWORD만 읽으며 설정 해석·번호 갱신·시작 안내를 실행하지 않는다.
    out<<"rotation_mode\t"<<client_.GameplayGlobals()->CursorGlobals().RotationMode()<<'\t'<<client_.Configuration().GetInt("rotmode")<<'\n'; //C/메뉴/우클릭의 공유 DWORD와 실제 설정을 쓰기 없이 따로 관측한다(717).
    const auto& constructionHistory=client_.GameplayGlobals()->Camera().ConstructionHistory();out<<"construction_history\t"<<constructionHistory.index;
    for (const auto& point:constructionHistory.points) out<<'\t'<<std::bit_cast<std::uint32_t>(point.x)<<'\t'<<std::bit_cast<std::uint32_t>(point.y); //최근5좌표의 실제 비트를 읽으며 빈 값/NaN을 다른 좌표로 치환하지 않는다(718).
    out<<'\n';
    out<<"saved_camera";for (const auto& point:client_.GameplayGlobals()->Camera().SavedPositions().points) out<<'\t'<<std::bit_cast<std::uint32_t>(point.x)<<'\t'<<std::bit_cast<std::uint32_t>(point.y); //별개10칸의 원시 비트만 읽고 현재 카메라/이력을 쓰지 않는다(719).
    out<<'\n';
    const auto& local=client_.LocalStateGlobals()->Blocks();out<<"fort_selection\t"<<fortSelectionMode_<<'\t'<<local[1][0]<<'\t'<<local[3][0]<<'\t'<<local[4][0]<<'\t'<<Field(std::string(client_.FortGlobals()->Pending()))<<'\n'; // 원문mode/확정phase/현재요청/next/pending을 설치완료와 구별해 읽는다.
    out<<"fort_names\t"<<Field(std::string(client_.FortGlobals()->Current()))<<'\t'<<Field(std::string(client_.FortGlobals()->Pending()))<<'\t'<<menuConfigRevision_<<'\t'<<client_.FortGlobals()->SelectedFileSpec()<<'\n'; // 같은 실제 두 버퍼/공유 dirty/선택 키를 읽기만 한다.
    if (editor_) out<<"editor_runtime\t"<<editor_->committed<<'\t'<<(editor_->transport ? editor_->transport->ClientAlive() : false)<<'\t'<<(editor_->transport ? editor_->transport->PendingAlive() : false)<<'\t'<<client_.GameplayGlobals()->FortKeepGenus()<<'\t'<<client_.GameplayGlobals()->StateAuxFC4()<<'\n'; //실제 공통 확정 복귀/양끝/두 현재 전역만 조회한다.
    std::string rankPrefix;const auto chatManager=client_.ChatManagerGlobals()->Current(); // 실제 생성자가 쓴 NUL 문자열만 보고하며 미정 할당 꼬리를 읽지 않는다.
    if (chatManager) { const auto bytes=client_.ChatManagerGlobals()->Bytes(chatManager).subspan(o::kChatManagerRankPrefixOffset);const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("Chat 보고 접두 NUL 범위");rankPrefix.assign(reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())); }
    std::uint32_t dotCount{};for (std::size_t byte=0;byte<4;++byte) dotCount|=std::uint32_t(client_.DotCommandGlobals()->Bytes()[0xa04+byte])<<(8*byte); // 같은 실제 명령표 count DWORD의 읽기 전용 관측이다.
    out<<"chat_manager\t"<<chatManager<<'\t'<<client_.CoreDataGlobals()->Active()<<'\t'<<Field(rankPrefix)<<'\t'<<dotCount<<'\n'; // 초기화/전송/상태 쓰기를 보고서에서 실행하지 않는다.
    const auto temporary=client_.StaticStrings()->Globals();out<<"static_strings\t"<<temporary.size<<'\t'<<temporary.base<<'\t'<<temporary.current<<'\t'<<temporary.last<<'\n'; // 같은 실제 공통 시작/예약 수명을 읽기만 하며 새 할당하지 않는다.
    const auto playerCore=client_.CoreDataGlobals();const auto players=client_.PlayerRecords()->Records();const auto current=reinterpret_cast<std::uintptr_t>(playerCore->Current().data()),base=reinterpret_cast<std::uintptr_t>(players.data()); // 다른 할당의 포인터 뺄셈을 하지 않고 실제 소유 범위를 읽는다.
    out<<"player_core\t"<<client_.PlayerRecords()->Base()<<'\t'<<client_.PlayerRecords()->Initialized()<<'\t'<<playerCore->Players().size()<<'\t'<<playerCore->Current().size()<<'\t'<<(current && current>=base && current-base<players.size()?std::to_string(current-base):"none")<<'\t'<<(world_ && world_->Session().PlayerCoreGlobals()==playerCore)<<'\n'; // 관측이 선택/캐시/초기화를 실행하지 않는다.
    const auto& sp=client_.SpGlobals()->Store();out<<"sp_globals\t"<<sp.Initialized()<<'\t'<<(sp.FirstPoolCurrent()!=nullptr)<<'\t'<<sp.Pools().size()<<'\t'<<sp.Offset()<<'\t'<<sp.TrackingOffsetCurrent()<<'\t'<<sp.Permutation().front()<<'\t'<<client_.RandomGlobals()->Game().State()<<'\t'<<(world_ && &world_->Session().SpStore()==&sp)<<'\n'; // 실제 현재 저장소/추적/난수와 같은 Session 여부를 읽기만 하며 SP.Get의 lazy 관문을 호출하지 않는다.
    const auto cloud=client_.KencloudGlobals();const auto cg=cloud->Globals();out<<"kencloud\t"<<cg.cache<<'\t'<<cg.end<<'\t'<<cg.phase<<'\t'<<cg.spacer<<'\t'<<cg.speed<<'\t'<<cg.fury<<'\t'<<cg.seedCount<<'\t'<<cloud->Allocations()<<'\t'<<cloud->RendererToken()<<'\t'<<cloud->WorldRedraw()<<'\n'; // 현재 실제 저장소와 장치만 읽고 생성/step/난수 호출은 하지 않는다.
    out<<"fort_catalog\t"<<mapCatalog_->Cached().count<<'\t'<<mapCatalog_->Cached().dirty<<'\t'<<client_.Configuration().GetInt("mayEditMissions")<<'\t'<<mapFileSpec_.Current()<<'\n'; // 실제50칸캐시와현재원문설정/키를읽고목록을다시열거하지않는다.
    out<<"current_dialog\t"<<rawButtons_->CurrentDialog()<<'\n'; // Guide4c8e90과 같은 실제565dd4다.
    out << "thunder_processes\t" << client_.ThunderFlashCount() << '\n';
    const auto& trace=client_.DiagnosticGlobals();out<<"raw_trace\t"<<trace.log.level<<'\t'<<trace.depth<<'\t'<<trace.suppress<<'\t'<<trace.log.indent<<'\t'<<(trace.log.stream!=0)<<'\n'; // 같은 Client 전역만 관측하며 로그를 출력하거나 파일을 열지 않는다.
    out<<"game_menu_bar\t"<<(gameMenuState_ ? gameMenuState_->menuSerial : 0U)<<'\t'<<GameMenuOpen()<<'\t'<<(gameMenuState_ ? gameMenuState_->checksum : 0)<<'\n'; // 같은serial조회와기존SP검산을관찰한다.
    out<<"movie_update_depth\t"<<client_.MovieUpdatePauseDepth()<<'\n'; // 게임 시계 정지와 별개인 실제 현재 갱신 깊이다.
    out<<"state_hud\t"<<(world_!=nullptr)<<'\t'<<(rawButtons_->UberRoot() && rawButtons_->UberRoot()->Current())<<'\t'<<(hud_?hud_->boundMinimap:0)<<'\t'<<(rawButtons_->Minimap()?rawButtons_->Minimap()->RootToken():0)<<'\t'<<client_.GameplayGlobals()->Borrowed()<<'\n'; //생성/현재 World 통지 대여를 분리 관측하며 State 설치 완료를 합성하지 않는다.
    const auto connections=client_.ConnectionGlobals()->Cleanup();std::size_t mainCount{},pendingCount{};for (const auto token:connections.transceivers) if (token) ++mainCount;for (const auto token:connections.pending) if (token) ++pendingCount; //같은 실제 고정표를 관측만 하며 포인터/개수/노드를 새로 만들지 않는다.
    out<<"connection_table\t"<<mainCount<<'\t'<<pendingCount<<'\t'<<connections.pendingCount<<'\t'<<connections.transceivers[9]<<'\t'<<connections.synchronization<<'\n'; //Client 장기의 실제 토큰이 정상 미션 로그인에서도 사용되는지 확인한다.
    out<<rawButtons_->Report()<<'\n';
    out<<rawButtons_->Menus()->Report();
    out<<warnings_->Report();
    out.precision(12); out<<"time\t"<<client_.Time().game<<'\t'<<client_.Time().wall<<"\n";
    out << "fullscreenMarker\t" << client_.Files().TryRead("fullscreenStateFile.dat").has_value() << "\n";
    out << "musicVolume\t" << client_.Configuration().GetInt("musicVolume") << "\nsoundVolume\t" << client_.Configuration().GetInt("soundVolume") << "\n";
    if (mission_) {
        out << "mission\t" << Field(mission_->Name()) << '\t' << Field(mission_->MissionType()) << '\t' << fortObjects_ << "\n";
        out<<"mission_progress\t"<<client_.Configuration().GetInt("Done"+mission_->Name())<<'\n'; // 실제안내Config명령과같은저장값을검사보고에서읽는다.
        if (missionRuntime_) {
            const auto& gate=missionRuntime_->gate;const auto& state=missionRuntime_->state; // 실제 Start의 최종 표식과 중간 쓰기를 읽는다.
            out<<"mission_init\t"<<missionRuntime_->init.namePrefix<<'\t'<<gate.tutorialNumber<<'\t'<<missionNumber_<<'\t'<<tutorialSeen_<<'\t'<<initLoginSubscriber_<<'\t'<<state_.Pending()<<'\n'; // 로드 전/후의 실제 전역과 요청을 읽는다.
            if (missionFortRequest_) out<<"mission_fort_request\t"<<missionFortRequest_->next[0]<<'\t'<<missionFortRequest_->sequence<<'\t'<<missionFortRequest_->next[31]<<'\t'<<missionFortRequest_->zone.size()<<'\t'<<missionFortRequest_->consumed<<'\n'; // 작성한 요청 캡처와 실제 복제 이미지의 소비를 구별한다.
            out<<"mission_start\t"<<gate.started<<'\t'<<state.sequence<<'\t'<<gate.demo<<'\t'<<gate.startSeconds<<'\t'<<gate.demoEndSeconds<<'\n';
            out<<"mission_gate\t"<<gate.tutorialNumber<<'\t'<<gate.stage<<'\t'<<gate.lockFrames<<'\t'<<gate.scriptPaused<<'\t'<<gate.clearFrames<<'\t'
                <<(missionRuntime_->tutorial ? 1 : 0)<<'\t'<<gate.nextStageSeconds<<'\t'<<gate.tellAtSeconds<<'\n'; // 동일 현재 상태의 관측만 제공한다.
            if (world_) {
                out<<"mission_bridges\t"<<world_->Session().TotalMade().BuiltForGenus(4)<<'\n'; // 단계 판정과 같은 누적 장부를 관찰하며 현재 살아 있는 다리 수와 구별한다.
                out<<"mission_options\t"<<unsigned(world_->Session().BattleOption(2))<<'\t'<<unsigned(world_->Session().BattleOption(4))<<'\t'<<world_->Session().EnergySettings().radiusSquared<<'\n'; // 같은 현재 옵션/공급 범위를 읽기만 한다.
                const auto& settings=world_->Session().MissionStartSettings(); // 원본 게임 명령이 공유할 같은 제한 전역이다.
                out<<"mission_rules\t"<<settings.DenySalvage()<<'\t'<<settings.DenyAscend()<<'\t'<<settings.allowAnyCapture<<'\t'<<settings.ordinaryGeysers<<'\n';
            }
        }
    }
    if (world_) {
        const auto core=world_->Session().Core(); // 관찰 전용이며 같은 실제 Kernel 주소와 개수만 읽는다.
        out<<"shared_kernel\t"<<(&core.kernel==&client_.GetKernel())<<'\t'<<core.kernel.Size()<<'\t'<<client_.GetKernel().Size()<<'\n';
        const auto& storage=client_.SurfaceStorage(); // 같은 실제 물리의 정체성과 대여 횟수를 읽기만 한다.
        out<<"shared_surface_storage\t"<<(&core.pool==&storage->pool)<<'\t'<<(&core.hash==&storage->hash)<<'\t'<<(core.types.data()==storage->types.data())<<'\t'<<(core.spots.data()==storage->spots.data())<<'\t'<<storage->Generation()<<'\t'<<storage->Borrowed()<<'\n';
        const auto& gameplay=client_.GameplayGlobals();const auto context=world_->Session().ProductionContext(); // 실제 UI/Session의 같은 전역 주소와 대여만 읽는다.
        out<<"shared_gameplay_globals\t"<<(context.gameplay==gameplay)<<'\t'<<(&world_->Session().CursorDropSettings()==&gameplay->cursor)<<'\t'<<(&world_->Session().ProductionFinder()==&gameplay->finder)<<'\t'<<(world_->Session().Editor()==gameplay->world.editor)<<'\t'<<gameplay->Borrowed()<<'\n';
        out<<"raw_full_deck\t"<<gameplay->FullDeck()<<'\t'<<gameplay->player.templateOwner<<'\t'<<core.bookkeeping.productionDirty<<'\n'; //같은실제세필드의현재값만읽으며Deck확보나갱신을실행하지않는다.
        out<<world_->Report();
    }
    // 화면/월드와 같은 프레임에 위치 효과음이 실제로 읽는 여섯 값을 기록한다. 소리 없는 실행에는 항목을 만들지 않는다.
    if (client_.Audio()) {
        const auto& view=client_.Audio()->View();
        out<<"audio_view\t"<<view.cameraX<<'\t'<<view.cameraY<<'\t'<<view.left<<'\t'<<view.top<<'\t'<<view.right<<'\t'<<view.bottom<<'\n';
        const auto& scene=client_.Audio()->Scene();
        out<<"weather\t"<<scene.index<<'\t'<<scene.tint<<'\t'<<scene.paletteDirty;
        // 현재 팔레트에서 계산한 네 원소 색도 같은 관찰에 기록한다.
        for (const auto tint:scene.tints) out<<'\t'<<tint;
        out<<'\n';
    }
    // 그리기와 같은 판정 표를 기록한다.
    for (const auto& control : input_.Controls()) {
        const auto& label = labels_.at(static_cast<std::size_t>(control.id));
        const auto item = std::find_if(page_.items.begin(),page_.items.end(),[&label](const DialogItem& value) { return value.label == label; });
        const bool checked = item != page_.items.end() && item->checked;
        out << "control\t" << Field(label) << '\t' << control.enabled << '\t' << control.menu << '\t'
            << control.rect.left << '\t' << control.rect.top << '\t' << control.rect.right << '\t' << control.rect.bottom << '\t' << checked << "\n";
    }
    return out.str();
}
//부모가현재battle/showTime을확인한뒤같은board를호출한다.
void UberGump::ShowGameTimeMessage() { warnings_->GameTimeStatus(); }
// CD의 SID 회수 완료 알림도 현재 미션/일반 Tell 검색과 같은 모달 수명을 사용한다.
void UberGump::ShowSidRecoveryNotice() { Tell("BridgesKilled"); }
}
