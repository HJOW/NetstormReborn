// 원본 Markup 그림 조각의 현재 자산/기준점·축소·측정·가상 출력·복제/소멸을 복원한다.
#pragma once
#include "client/StyleText.h"
#include "client/GameAssets.h"
#include <tuple>

namespace netstorm::client {
// 원본 +c 타입/+10 프레임/+14 float 배율/+18 색 치환 BYTE다. 공통 next/+8은 StyleText 표에 있다.
struct MarkupPicture {
    // type: 타입 표에서 객체 종류를 찾는 번호.
    // frame: 타입에서 사용할 그림 프레임 번호.
    std::uint32_t type{},frame{};
    // scale: 좌표와 그림 크기에 적용하는 배율.
    float scale{1};
    // mapped: 색변환.
    std::uint8_t mapped{};
};
// 원본 현재 타입 수 전역과 그림 조각의 안정된 파생 저장소다. 공통 조각 표와 같은 토큰을 쓴다.
struct MarkupState {
    // typeCount: 현재 타입 표에 등록된 항목의 개수.
    std::int32_t typeCount{};
    // pictures: 본문에 삽입할 그림의 타입·프레임·배율 정보 목록.
    std::map<std::uint32_t,MarkupPicture> pictures;
};
// token은 안정된 타입 식별자, shape는 현재 SHP 식별자다. hotspot은 원본 타입의 signed DWORD다.
struct MarkupType {
    // token: 저장소나 분배기에서 대상을 구별하는 토큰.
    // shape: 프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료.
    std::uint32_t token{},shape{};
    // frameCount: 사용 가능한 프레임 개수.
    // hotspotX: 프레임 그림 기준점의 X 오프셋.
    // hotspotY: 프레임 그림 기준점의 Y 오프셋.
    std::int32_t frameCount{},hotspotX{},hotspotY{};
};
// 원본 36바이트 메타 헤더 중 +18/+1a 폭/높이, +1c/+1e 기준점, +20/+22 두 WORD다.
struct MarkupFrame {
    // width: 가로 크기.
    // height: 세로 크기.
    // hotspotX: 프레임 그림 기준점의 X 오프셋.
    // hotspotY: 프레임 그림 기준점의 Y 오프셋.
    // extraX: 추가 X.
    // extraY: 추가 Y.
    std::int16_t width{},height{},hotspotX{},hotspotY{},extraX{},extraY{};
};
// Lookup은 타입 기준점 차이를 signed WORD 감김으로 반영한 헤더와 현재 SHP를 반환한다.
struct MarkupFrameView {
    // type: 타입 표에서 객체 종류를 찾는 번호.
    // shape: 프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료.
    std::uint32_t type{},shape{};
    // frame: 사용할 그림 프레임 또는 프레임 처리 자료.
    MarkupFrame frame;
};
// scaled가 false면 원색/현재 색 표의 일반 VFX 출력, true면 angle=0·16.16 배율의 원본 확대/축소 요청이다.
struct MarkupShapeRequest {
    // type: 타입 표에서 객체 종류를 찾는 번호.
    // shape: 프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료.
    // frame: 타입에서 사용할 그림 프레임 번호.
    std::uint32_t type{},shape{},frame{};
    // position: 현재 좌표 또는 저장소의 읽기 위치.
    ScreenPoint position;
    // mapped: 색변환 여부.
    // scaled: 배율적용 여부.
    bool mapped{},scaled{};
    // angle: 방향 또는 회전에 사용하는 각도.
    // scaleX: 그림의 가로 출력 배율.
    // scaleY: 그림의 세로 출력 배율.
    std::int32_t angle{},scaleX{},scaleY{};
};
// type/frame 조회는 현재 저장소를 공급한다. 조회 중 현재 그림 필드/타입 기준점이 바뀔 수 있다.
// color는 원본 그림 변환 표 설치이며 shape는 설치 후 현재 frame을 다시 읽은 요청을 받는다.
struct MarkupHooks {
    // type: 타입 표에서 객체 종류를 찾는 번호를 조회하는 콜백.
    std::function<const MarkupType*(std::uint32_t)> type;
    // frame: 타입에서 사용할 그림 프레임 번호를 조회하는 콜백.
    std::function<MarkupFrame(std::uint32_t,std::uint32_t,std::uint32_t)> frame;
    // color: 출력 색 또는 색 변환표를 선택하는 콜백.
    std::function<void()> color;
    // shape: 요청한 셰이프 그림을 조회하거나 출력하는 콜백.
    std::function<void(MarkupShapeRequest)> shape;
};
// StyleText의 공통 체인·scalar 반납과 같은 저장소에서 그림 조각을 처리한다.
class Markup {
public:
    // 상태/자산/출력 경계는 처리기와 연결한 StyleText 훅보다 오래 살아야 한다.
    Markup(StyleTextState& fragments,MarkupState& state,MarkupHooks hooks,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // 00475810/CD 004506f0: 공통 next=0/+8, type/frame, scale=1, mapped=0을 초기화한다.
    std::uint32_t NewPicture(std::uint32_t type,std::uint32_t frame,std::uint32_t metadata=0) const;
    // 00475b80/CD 00450b80: type/frame/+8을 복사하지만 배율/색 치환은 1/0으로 다시 만든다.
    std::uint32_t ClonePicture(std::uint32_t token) const;
    // 00475bf0/CD 00450b70: 공통 가상 분류를 Base로 바꾸고 그림 필드는 보존한다.
    // 실제 scalar 저장소 반납은 StyleText::Destroy와 BindMarkup의 release 분배가 수행한다.
    void Destruct(std::uint32_t token) const;
    // scalar 반납 직전 저장소 정리다. 그림 외의 토큰이면 아무것도 하지 않는다.
    void Release(std::uint32_t token) const;
    // 다른 외부 조각의 기존 분배를 보존할 때 사용하는 토큰 소속 조회다.
    bool Contains(std::uint32_t token) const;
    // 004758e0/CD 00450800: 전달한 DWORD의 하위 BYTE만 저장한다.
    void SetMapped(std::uint32_t token,std::uint32_t mapped) const;
    // 00475840/CD 00450770: 범위/shape/frame 확인 후 현재 헤더·타입 기준점을 읽는다. 무자산이면 여섯 WORD가 0이다.
    MarkupFrameView Lookup(std::uint32_t token) const;
    // 004758f0/CD 00450810: 지정 상자의 두 비율 중 원본 순서로 선택해 1 미만만 저장한다.
    // 10.78은 세로 비율을 float32로 먼저 저장하며 CD는 unordered도 최종 배율에 저장한다.
    void Fit(std::uint32_t token,std::int32_t width,std::int32_t height) const;
    // 가상 +4: 높이/보정된 y 기준점에 현재 배율과 CRT 절삭을 적용한다.
    std::array<std::int32_t,2> Baseline(std::uint32_t token) const;
    // 가상 +8: 폭/높이에 현재 배율과 CRT 절삭을 적용한다.
    std::array<std::int32_t,2> Measure(std::uint32_t token) const;
    // 가상 +c: 무자산은 생략하고 현재 기준점/배율/색 치환으로 VFX 요청을 만든다. 텍스트 override는 사용하지 않는다.
    void Draw(std::uint32_t token,ScreenPoint position) const;
private:
    // 기본/파생 저장소가 모두 있는 토큰만 허용한다. 소멸 후 직접 파생 호출은 거부한다.
    MarkupPicture& Picture(std::uint32_t token) const;
    // 공통 소유 필드 조회는 소멸자의 Base 전환 직전에 수행한다.
    StyleTextFragment& Fragment(std::uint32_t token) const;
    // fragments_: 0은 null이며 map의 주소는 삽입 후에도 안정된다. nextToken=0은 자동 토큰 발급 소진 상태다 (StyleTextState).
    StyleTextState& fragments_;
    // state_: 원본 현재 타입 수 전역과 그림 조각의 안정된 파생 저장소다. 공통 조각 표와 같은 토큰을 쓴다 (MarkupState).
    MarkupState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (MarkupHooks).
    MarkupHooks hooks_;
    // edition_: 판본별 필드 배치와 동작을 선택하는 원본 버전.
    o::OriginalEdition edition_;
};
// 그림에 해당하는 외부 가상 처리/반납만 연결하고 기존 다른 조각 분배를 보존한다.
void BindMarkup(StyleTextHooks& hooks,const Markup& markup);
// 실제 GameAssets의 원본 타입 수·논리 프레임/타입 기준점·물리 SHP 헤더를 공급한다.
class MarkupAssets {
public:
    // 원본 자산은 읽기만 하며 모든 참조는 이 어댑터와 반환 훅보다 오래 살아야 한다.
    explicit MarkupAssets(const GameAssets& assets);
    // 현재 원본 타입 수를 MarkupState에 공급할 때 사용한다.
    std::int32_t TypeCount() const;
    // 무자산/내장 타입은 null을 반환한다. 주소는 다른 조회 후에도 안정된다.
    const MarkupType* Type(std::uint32_t token) const;
    // abstract=0의 논리 프레임을 원본 signed 메타 헤더로 읽는다. 타입 기준점은 Lookup에서 별도로 뺀다.
    MarkupFrame Frame(std::uint32_t type,std::uint32_t abstract,std::uint32_t frame) const;
    // 출력에는 실제 VFX 물리 프레임을 사용한다. 색 설치 콜백 후 바뀐 frame도 다시 검사한다.
    std::pair<const ShapeDatabase*,std::size_t> Shape(std::uint32_t shape,std::uint32_t frame) const;
private:
    // 안정된 타입 필드와 로딩 순서의 SHP 블록이다.
    struct View { /* type: 객체 종류를 구별하는 타입 자료. */ MarkupType type;/* block: 셰이프 DB에서 그림을 찾는 블록 번호. */ std::size_t block{}; };
    // shapes_: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료.
    const ShapeDatabase& shapes_;
    // typeCount_: 현재 타입 표에 등록된 항목의 개수.
    std::int32_t typeCount_{};
    // views_: 타입에서 프레임·셰이프 블록을 찾는 조회 정보 목록.
    std::map<std::uint32_t,View> views_;
};
// 원본 현재 타입 수와 두 자산 조회 경계를 같은 어댑터에 연결한다.
void BindMarkupAssets(MarkupState& state,MarkupHooks& hooks,const MarkupAssets& assets);
// 그림의 원색/현재 변환 표 및 회전 없는 배율 출력을 실제 GumpPixelTarget에 연결한다.
class MarkupPixelTarget {
public:
    // pictureColors는 원본 그림 변환 표의 공급 값이고 currentColors는 글자/배경과 공유하는 현재 VFX 표다.
    MarkupPixelTarget(const GumpPixelTarget& target,const MarkupAssets& assets,ColorMap& currentColors,const ColorMap& pictureColors);
    // 색 설치/직교 배율을 공급한다. 회전 요청은 기존 shape 경계로 넘기며 없으면 실제 도달 시 거부한다.
    MarkupHooks Hooks(MarkupHooks hooks);
private:
    // 현재 실제 프레임을 해제/캐시하고 원본 기준점·배율·투명/색 치환 규칙으로 출력한다.
    void Image(MarkupShapeRequest request);
    // target_: 버튼 선과 압축 해제한 배경/SHP 그림을 현재 VFX 클립으로 8비트 표면에 합성한다 (GumpPixelTarget).
    const GumpPixelTarget& target_;
    // assets_: 타입·셰이프·팔레트 자산 또는 해당 자산의 처리 경계.
    const MarkupAssets& assets_;
    // colors_: 팔레트 색 또는 색 인덱스 변환 자료.
    ColorMap& colors_;
    // pictureColors_: 그림 색.
    const ColorMap& pictureColors_;
    // images_: 프레임이나 커서 번호별로 읽어 둔 그림 자료.
    std::map<std::tuple<const ShapeDatabase*,std::size_t,std::size_t>,IndexedImage> images_;
};
}
