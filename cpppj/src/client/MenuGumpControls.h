// controlled 메뉴의 문자열/체크/제목/머리/자식/구분선 작성과 CD 자식 문맥 전달을 같은 메뉴 저장소에 연결한다.
#pragma once
#include "client/MenuGumpLayout.h"
#include "client/ScrollingTextGumpMarkup.h"
#include "o/ConfigInterface.h"

namespace netstorm::client {
// 같은 기본/메뉴 객체와 원본 ReadInt(key,&value) 경계다. 반환값은 쓰지 않으며 누락 설정은 전달 초기값 0을 유지한다.
// 설정 조회는 현재 controlled 칸을 바꿀 수 있으므로 실제 Append 직전에 칸을 다시 읽는다.
struct MenuGumpControlHooks {
    std::function<MenuGumpNode*(std::uint32_t)> node;
    std::function<GumpCreateNode*(std::uint32_t)> base;
    std::function<void(std::string_view,std::int32_t&)> readInt;
};
// 원본 controlled 래퍼의 정상 몸체다. 기존 제목 파서/항목 Append를 공유하며 자동 Client 조립은 후속이다.
// 문자열은 원본의 NUL 종료 바이트 입력이다. 번호 인자/임의 printf 서식과 포인터 alias는 별도 공용 서식기 범위다.
class MenuGumpControls {
public:
    // 판본·등록/controlled 표·같은 노드/기존 처리기는 래퍼보다 오래 살아야 하며 동일 상태를 사용해야 한다.
    MenuGumpControls(GumpCreateState& create,const MenuGumpState& state,const MenuGump& menu,
        const MenuGumpLayout& layout,const ScrollingTextGumpMarkup& title,MenuGumpControlHooks hooks);
    // 00477cb0/CD 00451100: include가 0이면 생략, enabled가 0이면 disabled다. 명령 0은 command 필드를 쓰지 않는다.
    // 패치는 Menu 색을 조회하고 enabled 또는 forceColor에서 접두를 붙인다. CD는 색/서식 처리 없이 원문을 전달한다.
    void AddText(std::string_view text,std::uint32_t command=0,std::uint32_t parameter=0,
        bool include=true,bool enabled=true,bool forceColor=false) const;
    // 00477dd0/CD 00451150: 체크 비트/명시 인자/조건/비활성 처리다. 패치는 command 0에도 command 비트를 세운다.
    void AddChecked(bool checked,std::string_view text,std::uint32_t command,std::uint32_t parameter=0,
        bool include=true,bool enabled=true) const;
    // 00477ee0/CD 004511b0: 패치의 제목 색/~3~E와 CD의 ~3을 구분한다.
    // fancy는 현재 title에 실제 다중 줄 Parse만 호출하고, 일반 메뉴는 disabled 항목으로 Append/Reflow한다.
    void AddTitle(std::string_view text) const;
    // 00478020/CD 00451210: 패치의 머리 색/~E와 CD의 무인자 서식 복사를 구분하며 제목과 같은 경로로 추가한다.
    void AddHeader(std::string_view text) const;
    // 00478160/CD 00451270: 자식 포인터/체크/비활성 항목이다. 패치는 항상 styled, CD는 원문 '~' 유무를 사용한다.
    void AddSubmenu(std::string_view text,std::uint32_t child,bool enabled=true,bool checked=false) const;
    // 00478260/CD 004512e0: 선두/연속/40개 한도는 실제 기존 Append가 처리하고 미작성 명령/인자는 보존한다.
    void AddSeparator() const;
    // 00475ef0/CD 00451620: 실제 메뉴 생성/소멸 개수가 0인지다. controlled 표/visible/dead를 검사하지 않는다.
    bool NoMenus() const;
    // CD 00452a00: 0x20 자식들의 현재 등록 ID를 따라 문맥 8DWORD를 복사하고 재귀한다. 자신은 쓰지 않는다.
    // hidden/dead 필터·Redraw/Reflow·저장 StyleText 조각의 문맥 변경은 원본 몸체에도 없다.
    void CopyChildTextContextCd(std::uint32_t token,const std::array<std::uint32_t,8>& context) const;
    // CD 004529b0: 각 방문 항목의 강조를 지우다가 첫 비영 등록/visible 자식에서 true로 즉시 반환한다.
    bool ClearSelectionUntilVisibleChildCd(std::uint32_t token) const;
private:
    // 원본이 실제 조회하는 기본/메뉴/항목/등록 ID만 검사한다. 없는 조회와 원본 배열 밖은 호스트 예외다.
    GumpCreateNode& Base(std::uint32_t token) const;
    MenuGumpNode& Node(std::uint32_t token) const;
    MenuGumpItem& Item(std::uint32_t token,std::int32_t index) const;
    std::uint32_t Registered(std::uint32_t id) const;
    // 현재 조회/판본을 읽고 Patch의 assert 계약과 CD의 null 현재 메뉴 생략을 구분한다.
    bool Patch() const;
    std::uint32_t CurrentRequired() const;
    void RequireCd() const;
    // 현재 설정에서 전경→배경 순서로 읽는다. 앞선 조회가 바꾼 상태를 다음 조회/현재 칸에서 반영한다.
    std::array<std::int32_t,2> Colors(std::string_view foreground,std::string_view background) const;
    // 고정 %d/%s 원본 색 접두의 결과를 생성한다. 입력 '%'는 %s 인자로 들어가므로 재해석하지 않는다.
    std::string Colored(std::string_view text,const std::array<std::int32_t,2>& colors) const;
    // 제목/머리의 판본별 256바이트 포맷과 fancy Parse 또는 현재 일반 항목 추가다.
    void Heading(std::string_view text,bool title) const;
    // 문맥 입력을 참조로 유지하고 현재 next/항목 수를 읽는다. 실제 재귀 경로의 순환만 거부한다.
    void CopyChildren(std::uint32_t token,const std::array<std::uint32_t,8>& context,std::vector<std::uint32_t>& active) const;
    GumpCreateState& create_;
    const MenuGumpState& state_;
    const MenuGump& menu_;
    const MenuGumpLayout& layout_;
    const ScrollingTextGumpMarkup& title_;
    MenuGumpControlHooks hooks_;
};
// 원본 전역 설정 ReadInt 경계를 기존 ConfigInterface에 연결한다. 원본 설정 파일을 쓰지 않는다.
void BindMenuGumpControlConfig(MenuGumpControlHooks& hooks,o::ConfigInterface& config);
}
