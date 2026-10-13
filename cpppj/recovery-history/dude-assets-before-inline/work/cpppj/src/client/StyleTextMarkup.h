// StyleText ~[...]의 원본 정규식 명령을 그림 조각/태그/색/그림자/소리와 같은 저장소에 연결한다.
#pragma once
#include "client/Markup.h"
#include "o/ConfigInterface.h"
#include <optional>

namespace netstorm::client {
// 원본 zone 옵션과 그림 맞춤 전역이다. 허용 값은 bool로 자르지 않고 0/비0로 읽는다.
struct StyleTextMarkupState {
    // allowSounds: 허용 소리.
    // allowBackgroundColor: 허용 배경 색.
    // allowForegroundColor: 허용 전경 색.
    // allowOffset: 허용 오프셋.
    std::int32_t allowSounds{1},allowBackgroundColor{1},allowForegroundColor{1},allowOffset{1};
    // fitWidth: 맞춤 너비.
    // fitHeight: 맞춤 높이.
    std::int32_t fitWidth{65},fitHeight{60};
};
// 원본 타입 +114/+118/+11c/+12c/+130의 그림 선택 필드다. 글자 표의 빈 칸은 -1이다.
struct StyleTextMarkupType {
    // frameCount: 사용 가능한 프레임 개수.
    // defaultFrame: 기본 프레임.
    // gumpFrame: Gump 프레임.
    // letterKinds: 방향글자 종류.
    std::int32_t frameCount{},defaultFrame{},gumpFrame{},letterKinds{};
    // first: 구간 또는 연결 목록의 첫 항목.
    std::array<std::int32_t,16> first{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
};
// 이름 검색은 -1 실패, 타입 조회는 현재 선택 필드를 반환한다. black/sound는 실제 도달 시에만 요구한다.
struct StyleTextMarkupHooks {
    // findType: 그림 이름에서 현재 타입 번호를 찾는 콜백.
    std::function<std::int32_t(std::string_view)> findType;
    // type: 타입 표에서 객체 종류를 찾는 번호를 조회하는 콜백.
    std::function<StyleTextMarkupType(std::int32_t)> type;
    // black: 현재 팔레트의 검정 색 인덱스를 조회하는 콜백.
    std::function<std::uint32_t()> black;
    // sound: 해당 동작의 소리를 재생하는 콜백.
    std::function<void(std::string_view)> sound;
    // 다중 줄 숫자 타입의 현재 상한과 제한 모드에서 읽는 SHP 헤더 +1a signed 높이 WORD다. 무자산은 nullopt다.
    std::function<std::int32_t()> typeCount;
    // frameHeight: 타입·프레임의 실제 출력 높이를 조회하는 콜백.
    std::function<std::optional<std::int16_t>(std::int32_t,std::int32_t)> frameHeight;
};
// 원본 StyleText 파서가 사용하는 제한된 정규식 여섯 개를 원본 검색/greedy 순서로 처리한다.
// 다중 줄은 CD의 W 소리 형식과 별도 그림 선택/제한 규칙도 같은 정규식 조회로 처리한다.
class StyleTextMarkup {
public:
    // 상태/공통 조각/그림/자산·옵션·소리 경계는 생성한 bracket 훅보다 오래 살아야 한다.
    StyleTextMarkup(StyleTextState& fragments,const Markup& pictures,StyleTextMarkupState& state,
        StyleTextMarkupHooks hooks,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // I→S→T→C→B→E 중 처음 성립한 형식만 처리한다. CD는 I→T이며 허용되지 않은 옵션도 소비한다.
    void Apply(std::string_view command,std::array<std::uint32_t,8>& context,
        std::uint32_t& head,std::uint32_t& metadata) const;
    // 004ba8f0/CD 004b6c80의 대괄호 처리다. CD는 I→W→T이며 W의 두 번째 캡처는 사용하지 않는다.
    // restricted는 실제 그림 검사 시 현재 +d4를 읽고 appendPicture는 같은 버퍼/dirty/Redraw에 연결한다.
    void ApplyScrolling(std::string_view command,std::array<std::uint32_t,8>& context,std::uint32_t& metadata,
        const std::function<bool()>& restricted,
        const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& appendPicture) const;
    // 기존 글꼴/팔레트/그림 가상 훅을 유지하면서 대괄호 처리를 이 객체에 연결한다.
    StyleTextHooks Hooks(StyleTextHooks hooks) const;
private:
    // 현재 타입 선택 필드를 조회한다. 호스트 배열 범위 밖 입력은 공급 경계가 거부한다.
    StyleTextMarkupType Type(std::int32_t type) const;
    // 캡처된 타입·프레임 식을 원본 signed 비교/감김/클램프로 해석하고 공통 체인 뒤에 추가한다.
    void Picture(std::string_view type,std::string_view frame,std::uint32_t& head,std::uint32_t metadata) const;
    // 문자열 파서와 같은 옵션을 처리한다. CD 다중 줄 W의 소리는 zone 허용 전역을 확인하지 않는다.
    void Option(char code,std::string_view first,std::string_view second,std::array<std::uint32_t,8>& context,
        std::uint32_t& metadata,bool unconditionalSound=false) const;
    // 다중 줄 그림은 suffix의 # 맞춤·숫자 타입 상한·+d4 제한과 판본별 헤더 문턱을 적용한다.
    void ScrollingPicture(std::string_view type,std::string_view frame,const std::function<bool()>& restricted,
        const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& appendPicture) const;
    // fragments_: 0은 null이며 map의 주소는 삽입 후에도 안정된다. nextToken=0은 자동 토큰 발급 소진 상태다 (StyleTextState).
    StyleTextState& fragments_;
    // pictures_: 본문에 삽입할 그림의 타입·프레임·배율 정보 목록.
    const Markup& pictures_;
    // state_: 원본 zone 옵션과 그림 맞춤 전역이다. 허용 값은 bool로 자르지 않고 0/비0로 읽는다 (StyleTextMarkupState).
    StyleTextMarkupState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (StyleTextMarkupHooks).
    StyleTextMarkupHooks hooks_;
    // edition_: 판본별 필드 배치와 동작을 선택하는 원본 버전.
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
