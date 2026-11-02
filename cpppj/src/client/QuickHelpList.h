// 원문 CreateHelpList의 같은 전용 Help·지역 BaseFile·TYPE 순회를 조립한다.
#pragma once
#include "client/InputEvent.h"
#include <array>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { // 파일 출력에도 실제 QuickHelp 몸체와 공통 Gump 수명을 사용한다.
constexpr std::uint32_t kCreateHelpListCommand=0x0200009cU; //00542964의 실제 일반 입력 명령이다. 기본 메뉴 항목을 추가하지 않는다.
// 원문의 지역24BYTE 주소를 장치의 같은 파일 소유권에 연결한다. 호스트 포인터/FILE을 바이트에 직렬화하지 않는다.
using QuickHelpListFile=std::span<std::uint8_t,24>;
// 생성/일반열기/쓰기/명시Close/소멸을 별개로 유지한다. 선택한 경계만 필수다.
struct QuickHelpListFileHooks {
    std::function<void(QuickHelpListFile)> construct,close,destruct; //41a5a0/41a680/41b410의 같은 지역 파일이다.
    std::function<void(QuickHelpListFile,std::string_view,std::uint32_t,std::uint32_t)> open; //전체41b4c0(name,1,1)이며 성공을 반환값으로 대신하지 않는다.
    std::function<void(QuickHelpListFile,std::string_view,std::int32_t)> write; //41a910의 바이트 길이와 offset=-1이다. 짧은 fwrite를 재시도하지 않는다.
};
// 실제 현재 TYPE 표와 디스크/공통 Cleanup/Tell을 동기 호출 동안 빌린다.
struct QuickHelpListContext {
    QuickHelpListFileHooks file; //원문 지역파일을 같은 실제 VFS/CRT 장치에 연결한다.
    std::function<std::int32_t()> typeCount; //00541348을 초기 관문과 매 Cleanup 뒤에 다시 읽는다.
    std::function<std::uint32_t(std::uint32_t)> flags2; //첫49a840의+ec만 읽는다. 비제로는 비트 종류와 무관하게 포함한다.
    std::function<std::string(std::uint32_t)> typeName; //포함된 TYPE의 두 번째49a840+4이며 첫NUL 뒤는 서식화하지 않는다.
    std::function<void()> cleanup; //모든 TYPE 뒤4662b0이며 제외된 TYPE에도 호출한다.
    std::function<void(std::string_view)> tell; //성공 시 Kill 뒤의 실제 pieceDump 섹션을 표시한다.
};
// 같은 전용 부모의 +bc와 Show/공통 Kill만 공급한다. 별도 Help/HTML 모형을 만들지 않는다.
struct QuickHelpListWindowHooks {
    std::function<std::uint32_t()> openEmpty; //4c62a0의 비null 빈 주제,mode0,learn0을 기존 빈 주제 계약으로 요청한다.
    std::function<void(std::uint32_t,std::function<void(std::string_view)>)> attachStream; //현재 +bc에 살아 있는 지역파일의 동기 writer를 설치한다.
    std::function<void(std::uint32_t)> detachStream; //정상 종료의+bc=0이며 Close보다 먼저다.
    std::function<void(std::uint32_t)> abandonStream; //호스트 예외에서 callback만 반납하고 원문의 남은+bc 토큰은 지우지 않는다.
    std::function<void(std::uint32_t,std::uint32_t,std::string_view)> show; //4c5970을 직접 호출하며 요청 이력/추가 번역을 만들지 않는다.
    std::function<void(std::uint32_t)> kill; //같은 실제466480의 지연삭제다. 즉시 Cleanup으로 바꾸지 않는다.
};
//004c63e0의 전체 부모다. OpenHelp=0이면 파일/TYPE/출력/Tell을 전혀 요구하지 않는다.
void CreateQuickHelpList(const QuickHelpListContext& context,const QuickHelpListWindowHooks& window);
//현재 일반 입력의03ffffff 마스크와 공통 release 관문이다. 두 인자는 읽지 않는다.
bool DispatchCreateHelpList(InputEvent event,const std::function<void()>& create);
//원문 이름표의 ASCII 대소문자 비교다. 알 수 없는 이름은ffffffff다.
std::uint32_t CreateHelpListScriptCode(std::string_view name);
}
