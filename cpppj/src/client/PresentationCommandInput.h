// 일반 입력의 글꼴/음질 변경을 기존 현재 캐시·Config·장치 수명에 전달한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <optional>
#include <string_view>

namespace netstorm::client { // Guide/Gump/공통 release 뒤이며 Console·정지 앞인 두 명령이다.
class FontStore; // 기존 글꼴 참조를 유지한 채 캐시를 다시 만드는 실제 소유자다.
class Renderer; // 글리프 셰이프 주소로 저장한 압축 해제 이미지의 현재 소비자다.
void RebuildPresentationFonts(FontStore& fonts,Renderer& renderer,std::optional<std::string_view> face); // 이전 글리프 주소가 재사용되기 전에 표시 캐시를 반납하고 같은 FontStore를 재생성한다.
inline constexpr std::uint32_t kChangeFontFaceCommand=0x0200001bU,kChangeSoundQualityCommand=0x020000ddU; //00542764/00542a68의 실제 초기 DWORD다.
std::uint32_t PresentationCommandScriptCode(std::string_view name); //두 원문 이름만 ASCII 대소문자 무시로 조회한다.
bool IsPresentationCommand(std::uint32_t code); //키/다른 명령의 하위BYTE와 혼동하지 않는다.
struct PresentationCommandHooks { //선택한 원문 분기에서만 실제 경계를 요구한다.
    std::function<std::optional<std::string_view>(std::int32_t)> fontFace; //4a2df0의 현재 OS 목록 인덱스이며 범위 밖은 NULL을 뜻한다.
    std::function<void(std::optional<std::string_view>)> rebuildFonts; //NULL은 Arial, 유효 빈 이름은 빈 이름으로 같은 장기 FontStore를 재생성한다.
    std::function<std::int32_t()> width,height; //캐시 재생성 뒤 현재 화면 크기를 원문 순서로 다시 읽는다.
    std::function<void(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::uint32_t)> invalidate; //499370의 전체 화면0/0/W/H/0을 전달한다.
    std::function<std::string_view()> currentFontFace; //전체 dirty 뒤4a2e30의 현재 최대31BYTE 이름을 다시 읽는다.
    std::function<void(std::string_view,std::string_view)> set; //4414d0의 실제 Config Set/동기 알림이다.
    std::function<void()> closeSound; //4a8ef0의 기존 음악 스레드 합류·버퍼/장치 종료 부모다.
    std::function<void(std::string_view)> config; //441580 파서와 같은 현재 Config·오디오 재해석을 동기 수행한다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> sound; //재해석 성공 뒤 priestmove.wav와 다섯0을 기존 SoundPlayer에 넘긴다.
};
bool DispatchPresentationCommandInput(InputEvent event,const PresentationCommandHooks& hooks); //대상 release는 효과 없이 true이며 다른 명령은 false다.
}
