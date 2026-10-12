// 원본 MenuGump 항목 추가·문자열 설정·측정·전체 레이아웃을 복원한다.
#pragma once
#include "client/MenuGump.h"
#include "client/GumpText.h"
#include <string_view>

namespace netstorm::client {
// 측정 출력은 원본 signed DWORD 두 개다. 각 경계는 실제 도달한 현재 항목/문맥을 읽는다.
// StyleText 소유/파싱/측정과 제목 크기 갱신은 명시 경계이며 콜백은 같은 노드/항목 수를 바꿀 수 있다.
// clearStyle은 기존 소유 내용을 해제하고 items[index].styled를 비운다. parseStyle은 새 결과 토큰을 같은 필드에 저장한다.
// push/attach는 MenuGump의 크기 훅과 같은 가상 분배를 공급한다. plain/styled/marker는 항목 문자열을 측정한다.
struct MenuGumpLayoutHooks {
    std::function<MenuGumpNode*(std::uint32_t)> node;
    std::function<GumpCreateNode*(std::uint32_t)> baseNode;
    std::function<std::array<std::int32_t,2>(std::uint32_t,std::size_t,const std::array<std::uint32_t,8>&)> plain;
    std::function<std::array<std::int32_t,2>(std::uint32_t,std::size_t)> styled;
    std::function<std::array<std::int32_t,2>(std::uint32_t,const std::array<std::uint32_t,8>&)> marker;
    std::function<void(std::uint32_t,std::size_t)> clearStyle;
    std::function<void(std::uint32_t,std::size_t,std::string_view,const std::array<std::uint32_t,8>&)> parseStyle;
    std::function<void(std::uint32_t)> push;
    std::function<void(std::uint32_t,std::uint32_t)> titleFit;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> titleLayout;
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
};
// 같은 메뉴 프레임/기본 저장소를 사용하며 문자열 처리 뒤 실제 전체 크기를 다시 계산한다.
class MenuGumpLayout {
public:
    // 생성 상태/메뉴 크기 처리기와 안정된 저장소는 레이아웃 처리기보다 오래 살아야 하며 같은 상태를 공유해야 한다.
    MenuGumpLayout(GumpCreateState& create,const MenuGump& menu,MenuGumpLayoutHooks hooks);
    // 00477450/CD 00451730: 40개 한도·선두/연속 구분선 생략·자식 숨김/부착·조건부 명령/인자를 보존한다.
    // 자식 부착/StyleText 콜백 뒤 바뀐 현재 항목 수를 다시 읽는다. 이미 쓴 필드는 예외 시에도 유지한다.
    void Append(std::uint32_t token,const char* text,std::uint32_t flags,std::uint32_t command=0,
        std::uint32_t parameter=0,std::uint32_t child=0) const;
    // 00475dd0/CD 00451500: plain은 strncpy(40), styled는 문맥 font=0·Clear→현재 disabled 접두→Parse다.
    // plain 40바이트 복사는 끝에 NUL을 추가하지 않는다. StyleText의 256바이트 원본 버퍼를 넘는 입력은 거부한다.
    void SetText(std::uint32_t token,std::size_t index,const char* text,const std::array<std::uint32_t,8>& context) const;
    // 00475d70/CD 00451430: styled이면 항목 +34 측정으로, 그 외에는 전달 문맥의 기본 글자 측정으로 위임한다.
    std::array<std::int32_t,2> MeasureItem(std::uint32_t token,std::size_t index,const std::array<std::uint32_t,8>& context) const;
    // 00476f60/CD 00451f00: 현재 항목 수·문맥·플래그를 매 반복/콜백 뒤 다시 읽어 두 방향 크기/offset을 쓴다.
    void Reflow(std::uint32_t token) const;
private:
    // 실제 도달한 파생/기본 노드와 항목 offset만 검사한다.
    MenuGumpNode& Node(std::uint32_t token) const;
    GumpCreateNode& BaseNode(std::uint32_t token) const;
    MenuGumpItem& Item(std::uint32_t token,std::size_t index) const;
    GumpCreateState& create_;
    const MenuGump& menu_;
    MenuGumpLayoutHooks hooks_;
};
// 원본 항목의 60바이트를 직렬화한다. NUL이 없는 40바이트 라벨도 뒤 DWORD의 바이트까지 측정할 수 있다.
std::array<char,60> MenuGumpItemBytes(const MenuGumpItem& item);
// 일반 항목/자식 표시 " >"를 기존 FontStore에 연결한다. StyleText 경계는 호출자가 별도로 공급한다.
void BindMenuLayoutText(MenuGumpLayoutHooks& hooks,const GumpText& text);
}
