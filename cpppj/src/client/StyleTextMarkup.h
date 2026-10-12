// StyleText ~[...]의 원본 정규식 명령을 그림 조각/태그/색/그림자/소리와 같은 저장소에 연결한다.
#pragma once
#include "client/Markup.h"
#include "o/ConfigInterface.h"

namespace netstorm::client {
// 원본 zone 옵션과 그림 맞춤 전역이다. 허용 값은 bool로 자르지 않고 0/비0로 읽는다.
struct StyleTextMarkupState {
    std::int32_t allowSounds{1},allowBackgroundColor{1},allowForegroundColor{1},allowOffset{1};
    std::int32_t fitWidth{65},fitHeight{60};
};
// 원본 타입 +114/+118/+11c/+12c/+130의 그림 선택 필드다. 글자 표의 빈 칸은 -1이다.
struct StyleTextMarkupType {
    std::int32_t frameCount{},defaultFrame{},gumpFrame{},letterKinds{};
    std::array<std::int32_t,16> first{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
};
// 이름 검색은 -1 실패, 타입 조회는 현재 선택 필드를 반환한다. black/sound는 실제 도달 시에만 요구한다.
struct StyleTextMarkupHooks {
    std::function<std::int32_t(std::string_view)> findType;
    std::function<StyleTextMarkupType(std::int32_t)> type;
    std::function<std::uint32_t()> black;
    std::function<void(std::string_view)> sound;
};
// 원본 StyleText 파서가 사용하는 제한된 정규식 여섯 개를 원본 검색/greedy 순서로 처리한다.
// 전체 Regexp 모듈 복원과 TextGump의 W 명령은 이 클래스의 범위 밖이다.
class StyleTextMarkup {
public:
    // 상태/공통 조각/그림/자산·옵션·소리 경계는 생성한 bracket 훅보다 오래 살아야 한다.
    StyleTextMarkup(StyleTextState& fragments,const Markup& pictures,StyleTextMarkupState& state,
        StyleTextMarkupHooks hooks,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // I→S→T→C→B→E 중 처음 성립한 형식만 처리한다. CD는 I→T이며 허용되지 않은 옵션도 소비한다.
    void Apply(std::string_view command,std::array<std::uint32_t,8>& context,
        std::uint32_t& head,std::uint32_t& metadata) const;
    // 기존 글꼴/팔레트/그림 가상 훅을 유지하면서 대괄호 처리를 이 객체에 연결한다.
    StyleTextHooks Hooks(StyleTextHooks hooks) const;
private:
    // 현재 타입 선택 필드를 조회한다. 호스트 배열 범위 밖 입력은 공급 경계가 거부한다.
    StyleTextMarkupType Type(std::int32_t type) const;
    // 캡처된 타입·프레임 식을 원본 signed 비교/감김/클램프로 해석하고 공통 체인 뒤에 추가한다.
    void Picture(std::string_view type,std::string_view frame,std::uint32_t& head,std::uint32_t metadata) const;
    StyleTextState& fragments_;
    const Markup& pictures_;
    StyleTextMarkupState& state_;
    StyleTextMarkupHooks hooks_;
    o::OriginalEdition edition_;
};
// 실제 타입 이름·논리 프레임·특수 프레임·A~P 첫 인덱스를 같은 GameAssets에서 공급한다.
void BindStyleTextMarkupAssets(StyleTextMarkupHooks& hooks,const GameAssets& assets);
// E 명령의 현재 검정색을 기존 원본 팔레트 기본 별칭에 연결한다.
void BindStyleTextMarkupPalette(StyleTextMarkupHooks& hooks,const PaletteColorTable& palette);
// allowzonesounds/zonebgcolor/zonefgcolor/zoneoffset가 존재하면 현재 옵션을 갱신한다.
void LoadStyleTextMarkupOptions(StyleTextMarkupState& state,o::ConfigInterface& configuration);
// 기존 이름 재생 계층의 선언이다. 대괄호 파서는 장치나 버퍼를 직접 소유하지 않는다.
class SoundPlayer;
// S 명령의 이름 재생을 원본 loop/volume/pan/priority/limit 모두 0인 실제 재생 계층에 연결한다.
void BindStyleTextMarkupSound(StyleTextMarkupHooks& hooks,SoundPlayer& sound);
}
