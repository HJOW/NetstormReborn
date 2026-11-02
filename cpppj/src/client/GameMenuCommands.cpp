// 보호PE의초기명령DWORD를이름/GUI변환과구별한다.
#include "client/GameMenuCommands.h"
#include "client/LanguageRequest.h" //언어 요청/알림 이름도 같은 원문 입력 큐로 보낸다.
#include "client/RitualChoiceInput.h" //의식선택명령도같은HTML/일반입력으로요청한다.
#include "client/AddRandomInput.h"
#include "client/DestroyAtInput.h"
#include "client/UrlInput.h" // 스크린샷View버튼과대본도기존Help링크분배기를사용한다.
#include "client/CaptureInput.h" //BeginCapture 버튼도 같은 입력/현재 Client 상태를 사용한다.
#include "client/RecheckCdInput.h" //RecheckCD 이름·메타·큐 명령도 같은 일반 입력의 현재 serial 정리를 사용한다.
#include "client/IntroMenu.h" // PlayIntro도 같은 이름·메타·일반 입력 큐로 State39를 예약한다.
#include "client/ScenarioObjectInput.h"
#include "client/ScenarioEffectInput.h"
#include "o/OriginalText.h"
#include <array>
#include <utility>
namespace netstorm::client {
namespace {
// 00542xxx는원문주소식별자다. 런타임호스트포인터로역참조하지않는다.
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,44> kCommands{{
    {0x00542988U,kSetLanguageCommand},{0x0054298cU,kLanguageChangedCommand}, //원문의 두 언어 명령이다.
    {0x00542a54U,kPlayIntroCommand}, // 원문 PlayIntro 이름의 초기 DWORD다.
    {0x00542ae8U,kRecheckCdCommand}, //RecheckCD의 이름·메타·큐 명령이다. 실제 CD 재탐색이 아니라 현재 serial 정리다.
    {0x00542968U,kCaptureCommand}, //연속 화면 저장의 이름과 정수 명령을 같은 큐로 보낸다.
    {0x00542864U,kUrlCommand}, // 원문 URL의 이름·숫자·HTML 버튼을 같은 입력으로 보낸다.
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
    for (const auto& [source,value]:kCommands) if (source==address) return value; // 주소와명령숫자를뒤바꾸지않는다.
    return {};
}
// 원본name→ID표에서확인한메뉴확인창의필수이름이다.
std::uint32_t GameMenuScriptCode(std::string_view name) {
    const auto effect=ScenarioEffectScriptCode(name);if (effect!=0xffffffffU) return effect; // 일반 표시 명령도 같은 HTML/메타/대본 이름표로 조회한다.
    const auto object=ScenarioObjectScriptCode(name);if (object!=0xffffffffU) return object; // 시나리오/메뉴 버튼도 원문 객체 이름표를 공유한다.
    const auto key=o::AsciiLower(name); // 이름비교만원문ASCII대소문자를무시한다.
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
