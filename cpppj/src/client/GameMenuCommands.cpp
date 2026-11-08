// 보호PE의초기명령DWORD를이름/GUI변환과구별한다.
#include "client/AiUtilityCommandInput.h" //772의세이름/명령도같은현재허용표를공유한다.
#include "client/MakeAiCommandInput.h" //일반 MakeAi 이름과 같은 HTML/큐 명령이다.
#include "client/GameMenuCommands.h"
#include "client/VortexCommandInput.h" // 신전 세 가상명령과 ViewMeta의 실제 이름표도 같은 일반 큐로 보낸다.
#include "client/CreateAtInput.h" //같은 일반 명령의 원문 이름과 숫자다.
#include "client/DrawWater.h" //같은 일반 명령 이름/현재 DWORD만 허용한다.

#include "client/AiDebugCommandInput.h" //원문두진단명령의실제이름/허용표다.
#include "client/QuickHelpList.h" //같은 원문 Help목록 이름/숫자를 일반입력에 연결한다.
#include "client/PresentationCommandInput.h" //글꼴/음질도 같은 실제 명령 이름과 입력 큐를 사용한다.
#include "client/ConfigurationCommandInput.h" //현재 설정 세 이름/명령도 같은 원문 허용표를 공유한다.
#include "client/UtilityCommandInput.h" //원문유틸리티5이름/숫자도같은일반입력허용표를쓴다.
#include "client/LanguageRequest.h" //언어 요청/알림 이름도 같은 원문 입력 큐로 보낸다.
#include "client/RitualChoiceInput.h" //의식선택명령도같은HTML/일반입력으로요청한다.
#include "client/AddRandomInput.h"
#include "client/DestroyAtInput.h"
#include "client/UrlInput.h" // 스크린샷View버튼과대본도기존Help링크분배기를사용한다.
#include "client/CaptureInput.h" //BeginCapture 버튼도 같은 입력/현재 Client 상태를 사용한다.
#include "client/RecheckCdInput.h" //RecheckCD 이름·메타·큐 명령도 같은 일반 입력의 현재 serial 정리를 사용한다.
#include "client/IntroMenu.h" // PlayIntro도 같은 이름·메타·일반 입력 큐로 State39를 예약한다.
#include "client/ScenarioObjectInput.h"
#include "client/RawKnowledgeMenu.h" //원문ShowTec두명령의숫자/버튼허용을기존표에공유한다.
#include "client/ScenarioEffectInput.h"
#include "o/OriginalText.h"
#include <array>
#include <utility>
namespace netstorm::client {
namespace {
// 00542xxx는원문주소식별자다. 런타임호스트포인터로역참조하지않는다.
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,60> kCommands{{
    {0x005429a8U,kCreateInitialGeysersCommand},{0x005429f8U,kBrainCrankMoneyCommand},{0x005428d8U,kAiParamSetCommand}, //772의원문세일반명령이다.
    {0x005429d0U,kMakeAiCommand}, //005429d0의 실제 초기 일반 명령이다.
    {0x00542760U,kDrawWaterCommand}, //원문 후단 진단 명령이며 기본 메뉴 노출은 바꾸지 않는다.
    {0x542a34U,kAiReportingCommand},{0x542a38U,kDebugSwitchCommand}, //조건부AI메뉴의실제현재명령이다.
    {0x00542964U,kCreateHelpListCommand}, //원문 일반입력의 현재 전용Help 출력부모다.
    {0x00542764U,kChangeFontFaceCommand},{0x00542a68U,kChangeSoundQualityCommand}, //원문 두 현재 옵션 명령이다.
    {0x00542b84U,kShowTecSideMenuCommand},{0x00542b88U,kShowTecMenu2Command}, //원문추가지식선택버튼의현재명령도같은허용표를쓴다.
    {0x00542988U,kSetLanguageCommand},{0x0054298cU,kLanguageChangedCommand}, //원문의 두 언어 명령이다.
    {0x00542984U,0x020000a4U}, //영역 확인창의 DecendToChalLevel도 같은 현재 명령 조회와 공통 메뉴 허용표를 공유한다.
    {0x00542b40U,0x0200011cU},{0x00542b44U,0x0200011eU}, //RequestMenu/RefreshPlayers의실제일반입력과HTML버튼이같은보호PE명령을사용한다.
    {0x00542a54U,kPlayIntroCommand}, // 원문 PlayIntro 이름의 초기 DWORD다.
    {0x00542ae8U,kRecheckCdCommand}, //RecheckCD의 이름·메타·큐 명령이다. 실제 CD 재탐색이 아니라 현재 serial 정리다.
    {0x00542968U,kCaptureCommand}, //연속 화면 저장의 이름과 정수 명령을 같은 큐로 보낸다.
    {0x00542864U,kUrlCommand}, // 원문 URL의 이름·숫자·HTML 버튼을 같은 입력으로 보낸다.
    {0x00542a74U,kCreateAtCommand}, //현재 Runtime 생성 부모만 허용하고 기본 UI는 추가하지 않는다.
    {0x00542a78U,kDestroyAtCommand}, // 원문DestroyAt의 문자열 토큰도 같은 일반 입력/버튼 큐다.
    {0x00542954U,kAddRandomCommand}, // 원본AddRandom 이름도 같은 일반 입력/버튼 큐를 사용한다.
    {0x0054287cU,0x02000061U}, //원문 Cheat의 같은 일반 SP 가산 명령이다.
    {0x00542780U,0x02000022U},
    {0x005427b8U,0x02000030U},
    {0x005428acU,0x0200006dU},
    {0x005428dcU,0x02000079U},
    {0x00542914U,0x02000088U},
    {0x00542924U,0x0200008cU},
    {0x00542928U,0x0200008dU},
    {0x00542938U,0x02000091U},
    {0x00542944U,0x02000094U},
    {0x00542948U,0x02000095U},
    {0x0054295cU,0x0200009aU}, //PlaySound의 문자열 인자와 여섯 음향 인자를 전달한다.
    {0x005427acU,0x0200002dU}, //DigUp 이름도 기존 일반 입력의 같은 보상 경로를 사용한다.
    {0x00542a44U,kUpgradeAltarCommand}, //원문의제단업그레이드요청이며즉시지급과구별한다.
    {0x00542a48U,kGetNewTechCommand}, //원문의기술선택6b요청이다.
    {0x00542ad8U,kRequestStormpowerCommand}, //원문의시간초과SP보상선택요청이다.
    {0x00542970U,0x0200009fU}, //GetTechnology의 이름과 HTML 버튼도 같은 일반 입력으로 전달한다.
    {0x00542974U,0x020000a0U},
    {0x005429acU,0x020000aeU},
    {0x005429d4U,0x020000b8U},
    {0x005429fcU,0x020000c2U},
    {0x00542a08U,0x020000c5U},
    {0x00542a0cU,0x020000c6U},
    {0x00542a20U,0x020000cbU}, //SaveGoMain의 두 저장 선택은 같은 실제 Native State3 예약 명령을 사용한다.
    {0x00542a30U,0x020000cfU},
    {0x00542a28U,0x020000cdU}, //GetFullDeck의 원본 입력 ID다.
    {0x00542a4cU,0x020000d6U},
    {0x00542a60U,0x020000dbU},
    {0x00542a8cU,0x020000e6U},
    {0x00542a90U,0x020000e7U},
    {0x00542aa0U,0x020000ebU}, //Tips의 이전·다음·확인은 같은 TellTip 명령의 두 번째 인자를 쓴다.
    {0x00542ab4U,0x020000f0U},
    {0x00542ab8U,0x020000f1U},
    {0x00542700U,0x02000001U},
    {0x005429ecU,0x020000beU},
    {0x00542b54U,0x02000123U},
}};
}
// 등록한원문전역과동일명령ID의첫쌍만반환한다.
std::optional<std::uint32_t> GameMenuCommandValue(std::uint32_t address) {
    if (address==0x00542a40U) return 0x020000d3U; // 개발자 Ctrl+Shift+F7도 원문의 NextStage 전역과 같은 명령을 큐에 예약한다.
    if (address==0x0054279cU) return kChangePalCommand;if (address==0x0054291cU) return kChangePriorityCommand;if (address==0x00542a10U) return kSetFortTypeCommand; //세 설정명령의 원문 현재 전역도 같은 숫자 조회로 연결한다.
    for (const auto& [source,value]:kCommands) if (source==address) return value; // 주소와명령숫자를뒤바꾸지않는다.
    return {};
}
// 원본name→ID표에서확인한메뉴확인창의필수이름이다.
std::uint32_t GameMenuScriptCode(std::string_view name) {
    const auto makeAi=MakeAiCommandScriptCode(name);if (makeAi!=0xffffffffU) return makeAi;
    const auto ai=AiUtilityCommandScriptCode(name);if (ai!=0xffffffffU) return ai;
    const auto createAt=CreateAtScriptCode(name);if (createAt!=0xffffffffU) return createAt; //원문 등록 이름을 같은 문자열 고리에 보낸다.
    const auto water=DrawWaterScriptCode(name);if (water!=0xffffffffU) return water;

    const auto aiDebug=AiDebugCommandScriptCode(name);if (aiDebug!=0xffffffffU) return aiDebug;
    const auto helpList=CreateHelpListScriptCode(name);if (helpList!=0xffffffffU) return helpList;
    const auto presentation=PresentationCommandScriptCode(name);if (presentation!=0xffffffffU) return presentation;
    const auto utility=UtilityCommandScriptCode(name);if (utility!=0xffffffffU) return utility; //기본메뉴노출없이원문등록이름만연결한다.
    const auto configured=ConfigurationCommandScriptCode(name);if (configured!=0xffffffffU) return configured; //원문 등록 이름만 연결하고 기본 메뉴는 추가하지 않는다.
    const auto effect=ScenarioEffectScriptCode(name);if (effect!=0xffffffffU) return effect; // 일반 표시 명령도 같은 HTML/메타/대본 이름표로 조회한다.
    const auto object=ScenarioObjectScriptCode(name);if (object!=0xffffffffU) return object; // 시나리오/메뉴 버튼도 원문 객체 이름표를 공유한다.
    const auto vortex=VortexCommandScriptCode(name);if (vortex!=0xffffffffU) return vortex; // 보호 이름표의 네 항목만 허용하며 신규 메뉴는 추가하지 않는다.
    const auto key=o::AsciiLower(name); // 이름비교만원문ASCII대소문자를무시한다.
    if (key=="metarefresh") return 0x020000c6U; //PE5431b8의 실제 이름이며 현재 모드 판정 뒤 기존16h/TCP11을 재요청한다.
    if (key=="battledesc") return 0x020000f3U; //PE543320의 실제 HTML 이름이며 양수 고리 토큰을 CS 논리 주소와 구별해 해석한다.
    if (key=="chalsailjoinbattle") return 0x0200006bU; //PE542ee0의 실제 이름이며 현재 진단 경고/좌석 참가 소비자에 연결한다.
    if (key=="chalsailpos") return 0x0200006cU; //PE542ee8의 실제 이름이며 packed 위치를 현재 항해 소비자가 읽는다.
    if (key=="chalforcejoinbattle") return 0x020000e3U; //PE5432a0의 실제 경고 확인은 옵션을 바꾸지 않고 직접 참가한다.
    if (key=="chaljoinbattlenowarning") return 0x020000feU; //PE543378의 실제 확인은 경고 억제 설정 뒤 참가한다.
    if (key=="querydecendtochallevel") return 0x020000e8U; //PE5432c8의 원문 오자를 그대로 지원하며 현재 영역 번호의 확인 Tell을 연다.
    if (key=="decendtochallevel") return 0x020000a4U; //PE5430a8→542984의 실제 이름/명령이며 영역 확인창 Yes가 같은4b8810 부모를 실행한다.
    if (key=="requestmenu") return 0x0200011cU; //PE543420→542b40의서버섹션요청이며로컬Tell/Zsend별칭으로바꾸지않는다.
    if (key=="refreshplayers") return 0x0200011eU; //PE543428→542b44의현재CS이름표/NSInfo갱신이며새서버요청을만들지않는다.
    if (key=="serverforcebattlestart") return 0x020000f5U; //PE00543330 이름표/00542ac8의 실제 일반 명령이다.
    if (key=="delstatefile") return 0x020000acU; //PE005430e8→005429a4의 실제 저장상태 삭제 명령이다.
    if (key=="statereconnect") return 0x020000abU; //PE005430e0→005429a0의 실제 저장 복구 명령이다.
    if (key=="forceserverdeath") return 0x020000a9U; //PE005430d0→00542998의 실제 서버 이전 명령이다.
    if (key=="setlanguage") return kSetLanguageCommand;
    if (key=="languagechanged") return kLanguageChangedCommand;
    if (key=="playintro") return kPlayIntroCommand;
    if (key=="url") return kUrlCommand; // 외부 주소뿐 아니라 내부 Help/cmd 링크도 같은4a00c0이해석한다.
    if (key=="capture") return kCaptureCommand; //543070의 실제 이름이며 새 모드 관문을 추가하지 않는다.
    if (key=="recheckcd") return kRecheckCdCommand; //543370의 실제 이름이다. 종료된 온라인 검증/CD 재탐색으로 바꾸지 않고 현재 serial 정리만 연결한다.
    if (key=="addrandom") return kAddRandomCommand; //00543048의 원문 이름이며 수량1은 실제 일반 입력이 전달한다.
    if (key=="destroyat") return kDestroyAtCommand; //00543290의 실제 이름이며 좌표/필터 문자열을 보존한다.
    if (key=="changemoney") return 0x02000061U; // 원문542e90의 실제 이름이며 기존 signed SP 가산 몸체를 공유한다.
    if (key=="playsound") return 0x0200009aU; // 원문543058의 이름→54295c이며 다른 소리 명령의 별칭이 아니다.
    if (key=="viewpriest") return 0x020000b8U; // 원문543148의 명시 owner를 기존4950d0 경로에 전달한다.
    if (key=="setallbridge") return 0x020000d6U; // 원문543238의 품질 숫자를 기존 전체 다리 변경에 전달한다.
    if (key=="digup") return 0x0200002dU; // 원문542cf0의 이름이며 메뉴 노출 여부를 새 실행 관문으로 넣지 않는다.
    if (key=="quitapp") return 0x02000001U;
    if (key=="requestquitapp") return 0x02000094U;
    if (key=="menujustclosed") return 0x020000beU;
    if (key=="reloadtheams") return 0x02000123U;
    if (key=="pause") return 0x020000cfU;
    if (key=="techhelp") return 0x02000049U; //원문 TechHelp 이름표와54281c의 현재 일반 명령이다.
    if (key=="getnewtech") return kGetNewTechCommand;
    if (key=="requeststormpower") return kRequestStormpowerCommand;
    if (key=="upgradealtar") return kUpgradeAltarCommand; //원문세이름과메뉴행이같은요청입력에연결된다.
    if (key=="getfulldeck") return 0x020000cdU;
    if (key=="toggleminimap") return 0x02000037U; //원본542d40 이름표→5427d4의 독립 미니맵 전환 명령이다.
    if (key=="togglebottomgump") return 0x02000088U; // 원본542fc8 이름표→542914의 하단 채팅 전환이며0/1/2 인자는 기존 도킹 경로가 해석한다.
    if (key=="addisland") return 0x02000022U; // 00542780의 실제 Editor 섬 추가 명령이다.
    if (key=="savegomain") return 0x020000cbU;
    if (key=="telltip") return 0x020000ebU;
    if (key=="newfort") return 0x02000079U; // LOADLOADFORT가 예약하는 새 이름 창의 원본 명령이다. y=0/1은 실제 큐에 그대로 보존한다.
    if (key=="screenmodeabschange") return 0x02000030U; // 원본 DirectDrawManager/Window Mode가 사용하는 절대 모드 명령이다.
    if (key=="screenmodechange") return 0x02000002U; // 실제542704 조정 비트 반전 명령이다.
    if (key=="acceptscreenmodechange") return 0x020000e4U; // 실제542a84 확인 뒤 기억 명령이다.
    return 0xffffffffU;
}
// 실제큐연결허용과선택후미연결명령진단에같은표를쓴다.
bool IsGameMenuCommand(std::uint32_t code) {
    if (IsConfigurationCommand(code)) return true; //실제 같은 분배가 처리하는 설정 명령이다.
    if (IsUtilityCommand(code)) return true; //같은일반분배가처리하는명령만HTML/버튼에서허용한다.
    for (const auto& [address,value]:kCommands) if (value==code) return true; // 같은값의주소항목이여럿이어도허용결과는같다.
    return code==0x02000049U || code==0x02000002U || code==0x020000e4U; // TechnicalHelp와 화면 조정/확인의 실제 원문 기본 전역 명령이다.
}
// 원문은 uVar&03ffffff 전체가1b일 때만 ESC이며 일반 명령ID의 하위BYTE를 키로 쓰지 않는다.
bool IsEscapeKeyPress(const InputEvent& event) { return !(event.code&InputCode::kRelease) && (event.code&0x03ffffffU)==0x1bU; }
// 원본 F9는 Alt를 별도로 거부하지 않으며 Shift만 목적 명령을 바꾸고 두 인자는 항상0이다.
bool QueuePlayerListKey(const InputEvent& event,InputQueue& queue) {
    if ((event.code&(InputCode::kRelease|InputCode::kControl)) || (event.code&0x03ffffffU)!=(0x78U<<InputCode::kVirtualKeyShift)) return false;
    queue.PushCommand(event.code&InputCode::kShift ? 0x020000cfU : 0x020000f0U,0,0);return true;
}
}
