// 원본 메뉴의 실제 배경/강조/구분선/체크 그림/항목 글자/하위 메뉴 표시를 같은 픽셀 대상에 출력한다.
#pragma once
#include "client/MenuGumpLayout.h"
#include "client/GumpBackgroundDraw.h"
#include "client/GumpDecoration.h"
#include "client/StyleText.h"

namespace netstorm::client {
// 원본 현재 흰색/비활성 회색·구분선 배경 타입/프레임·체크 그림 타입 전역이다. 미초기화 PE 값을 기본값으로 쓰지 않는다.
struct MenuGumpDrawState { /* foreground: 본문 출력의 팔레트 색 인덱스. */ /* disabled: 비활성 출력 색 또는 동작 차단 상태. */ /* backgroundType: 배경을 출력할 타입 번호. */ /* backgroundFrame: 배경을 출력할 프레임 번호. */ /* markerType: 선택·체크 표식을 그릴 타입 번호. */ std::uint32_t foreground{},disabled{},backgroundType{},backgroundFrame{},markerType{}; };
// 같은 기본/메뉴 객체와 체크 표시의 원색 SHP 출력 경계다. 배경·글꼴·StyleText는 기존 처리기를 직접 사용한다.
struct MenuGumpDrawHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<MenuGumpNode*(std::uint32_t)> node;
    // base: 파생 처리에서 공통 동작을 위임하는 기본 객체를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> base;
    // marker: 선택·체크 표식을 지정 위치에 그리는 콜백.
    std::function<void(std::uint32_t,ScreenPoint)> marker;
};
// 00476080/CD 004523b0의 정상 출력과 임시 클립 복구다. 자식 장식/제목은 수집 목록에서 따로 출력한다.
class MenuGumpDraw {
public:
    // 모든 참조와 출력 표면은 Draw 처리기 및 연결한 가상 훅보다 오래 살아야 한다.
    MenuGumpDraw(GumpCreateState& create,GumpDrawState& clip,MenuGumpDrawState& state,
        const GumpCollection& collection,const GumpBackgroundDraw& background,const GumpTextDraw& text,
        const StyleText& style,const MenuGumpLayout& layout,MenuGumpDrawHooks hooks);
    // 자체 경계 클립→기본 배경→Position→항목별 현재 문맥/강조/그림/글자/하위 표시→클립 복구다.
    void Draw(std::uint32_t token) const;
private:
    // 같은 실제 노드만 허용한다. 항목은 itemCount와 별개로 도달한 40칸 범위를 확인한다.
    MenuGumpNode& Node(std::uint32_t token) const;
    MenuGumpItem& Item(std::uint32_t token,std::int32_t index) const;
    // 00464930/CD 004936c0: 명시 float32 로컬 rect를 현재 화면 Position에 더해 signed 절삭한다.
    // 패치의 bottom Y float32 중간 저장과 CD의 정수 유지 차이를 보존한다.
    ScreenRect Rect(std::uint32_t token,const std::array<float,4>& local) const;
    // 행/가로/compact 선택 배경은 현재 메뉴 type/frame으로 0x2100000 배경을 출력한다.
    void Highlight(std::uint32_t token,std::int32_t index) const;
    // 세로 구분선은 두 개의 1픽셀 높이 명암 배경이며 각 pass에서 현재 전역을 다시 읽는다.
    void Separator(std::uint32_t token,std::int32_t index) const;
    // styled이면 저장 조각의 문맥을 쓰고 plain이면 항목의 60바이트/NUL과 임시 문맥을 사용한다.
    void Text(std::uint32_t token,std::int32_t index,ScreenPoint point,const std::array<std::uint32_t,8>& context) const;
    // create_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& create_;
    // clip_: 출력을 제한하는 클리핑 영역 또는 상태.
    GumpDrawState& clip_;
    // state_: 원본 현재 흰색/비활성 회색·구분선 배경 타입/프레임·체크 그림 타입 전역이다. 미초기화 PE 값을 기본값으로 쓰지 않는다 (MenuGumpDrawState).
    MenuGumpDrawState& state_;
    // collection_: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (GumpCollection).
    const GumpCollection& collection_;
    // background_: 배경 출력 자료 또는 배경 Gump 토큰.
    const GumpBackgroundDraw& background_;
    // text_: 표시하거나 해석할 본문 문자열.
    const GumpTextDraw& text_;
    // style_: 글꼴·색·정렬을 선택하는 텍스트 출력 문맥.
    const StyleText& style_;
    // layout_: 좌표 배치 계산 또는 판본별 필드 위치 정보.
    const MenuGumpLayout& layout_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (MenuGumpDrawHooks).
    MenuGumpDrawHooks hooks_;
};
// 복원 메뉴의 실제 Draw만 교체하며 버튼/배경/텍스트 등 기존 다른 파생 처리기를 보존한다.
void BindMenuGumpDraw(GumpDrawHooks& hooks,std::function<const MenuGumpDraw*(std::uint32_t)> resolve);
// 체크 그림은 타입+154의 J 구간 첫 번호+2 물리 프레임이다. 기존 장식 원색 픽셀 캐시를 공유한다.
void BindMenuGumpDrawAssets(MenuGumpDrawHooks& hooks,const GameAssets& assets,GumpDecorationPixelTarget& pixels);
}
