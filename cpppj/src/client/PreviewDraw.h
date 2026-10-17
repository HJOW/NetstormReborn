// 원본 kind 4의 본체/격자·문구를 실제 raw 화면 위치와 현재 커서 상태로 그린다.
#pragma once
#include "client/PreviewCollection.h"
#include "client/GameAssets.h"

namespace netstorm::client {
// 표 선택은 원본 전역 VFX 문맥을 바꾸고 명시 불법 표는 해당 그림에만 적용한다.
enum class PreviewColors { Owner=1,Destroy=2,Illegal=3,LabelShadow=4 };
// 원색·현재 색 문맥·한 그림에만 적용하는 불법 표를 구분한다.
enum class PreviewShapeMode { Plain,CurrentColors,Illegal };
// 소유자 색 표는 raw 타입과 현재 플레이어 번호로 선택한다.
struct PreviewColorRequest { /* colors: 팔레트 색 또는 색 인덱스 변환 자료. */ PreviewColors colors;/* type: 타입 표에서 객체 종류를 찾는 번호. */ /* owner: 객체나 지역을 소유한 플레이어 번호. */ std::uint32_t type{},owner{}; };
// SHP 타입/frame과 화면 기준점이다. 별도 비용 이미지는 priceType으로 지정한다.
struct PreviewShapeRequest { /* type: 타입 표에서 객체 종류를 찾는 번호. */ /* frame: 타입에서 사용할 그림 프레임 번호. */ std::uint32_t type{},frame{};/* position: 현재 좌표 또는 저장소의 읽기 위치. */ ScreenPoint position;/* mode: 동작 분기를 선택하는 모드. */ PreviewShapeMode mode{}; };
// 메시지 번호 2~4 또는 비용 문자열 5를 상위 번역/글꼴 계층으로 전달한다.
struct PreviewTextRequest { /* index: 목록이나 표에서 현재 항목을 찾는 인덱스. */ std::uint32_t index{};/* position: 현재 좌표 또는 저장소의 읽기 위치. */ ScreenPoint position; };
// 준비한 실제 커서 전역과 격자/글자 표다. legal/parent/플레이어는 수집 상태를 공유한다.
struct PreviewDrawState {
    // selectedType: 선택된 타입.
    // destroyType: 삭제 타입.
    // noOutlineType: 없음 윤곽 타입.
    // overlayTrigger: 오버레이 발동조건.
    // overlayType: 오버레이 타입.
    // labelType: 미리보기 안내 라벨의 타입 번호.
    // priceType: 가격 표시의 글꼴·그림 타입 번호.
    std::uint32_t selectedType{},destroyType{},noOutlineType{},overlayTrigger{},overlayType{},labelType{},priceType{};
    // cameraX: 월드에서 화면에 투영할 카메라 X 원점.
    // cameraY: 월드에서 화면에 투영할 카메라 Y 원점.
    // spacing: 배치 항목 사이의 간격.
    // labelOffsetX: 안내 라벨 기준점의 X 픽셀 오프셋.
    // labelOffsetY: 안내 라벨 기준점의 Y 픽셀 오프셋.
    // labelWidth: 안내 라벨의 픽셀 너비.
    // labelHeight: 안내 라벨의 픽셀 높이.
    std::int32_t cameraX{},cameraY{},spacing{},labelOffsetX{},labelOffsetY{},labelWidth{},labelHeight{};
    // priceWidth: 가격 표시 문자열의 픽셀 너비.
    // priceIconOffset: 가격 옆 아이콘 배치의 기준 오프셋.
    // priceIconX: 가격 아이콘의 X 픽셀 위치.
    // priceIconY: 가격 아이콘의 Y 픽셀 위치.
    std::int32_t priceWidth{},priceIconOffset{},priceIconX{},priceIconY{};
    // gridOption: 격자 표시 옵션을 고르는 값.
    // showLetters: 배치 패턴의 번호 글자를 표시할지.
    // flags: 동작을 선택하는 플래그 비트.
    // messageOverride: 메시지 덮어쓰기.
    // blocked: 공간·소유권 조건으로 진행이 차단됐는지.
    // powered: 현재 배치 위치에 필요한 에너지가 공급되는지.
    // reachable: 현재 배치 위치에 다리 연결이 닿는지.
    std::uint32_t gridOption{},showLetters{},flags{},messageOverride{},blocked{},powered{},reachable{};
    // goodColor: 배치 가능 표시의 팔레트 색 인덱스.
    // badColor: 배치 불가 표시의 팔레트 색 인덱스.
    std::uint8_t goodColor{},badColor{};
    std::array<std::uint8_t,144> grid{}; // 원본 (열+1)*12+(행+1), 최대 footprint 10까지.
    std::array<std::uint32_t,256> labelFrames{}; // MSVC signed char의 원본 표 조회를 호스트 바이트로 연결한다.
    // labels: 라벨.
    std::string labels;
    // messageWidths: 미리보기 안내 문자열별 출력 너비 표.
    // messageHeights: 미리보기 안내 문자열별 출력 높이 표.
    std::array<std::int32_t,5> messageWidths{},messageHeights{};
    // ownerMaps: 플레이어별 미리보기 색 변환표.
    std::vector<bool> ownerMaps;
};
// 실제 SHP/색 표/장치 경계다. rectangle의 true는 체크 무늬, false는 윤곽이다.
struct PreviewDrawHooks {
    // setColors: 요청한 타입·소유자에 맞는 출력 색 변환을 설정하는 콜백.
    std::function<void(PreviewColorRequest)> setColors;
    // shape: 요청한 셰이프 그림을 조회하거나 출력하는 콜백.
    std::function<void(PreviewShapeRequest)> shape;
    // rectangle: 요청한 사각형을 출력하는 콜백.
    std::function<void(ScreenRect,std::uint8_t,bool)> rectangle;
    // text: 요청한 본문을 실제 텍스트 처리기에 전달하는 콜백.
    std::function<void(PreviewTextRequest)> text;
};
// 수집에서 받은 대리 토큰을 같은 raw 풀로 되돌리고 두 판본의 전체 Draw 흐름을 복원한다.
class PreviewDraw {
public:
    // 풀/타입/현재 상태와 콜백은 이 객체보다 오래 살아야 한다.
    PreviewDraw(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
        const PreviewCollectionState& collection,const PreviewDrawState& state,PreviewDrawHooks hooks,std::uint32_t tokenBase=0);
    // PATCH의 실제 전역을 참조한다. 팔레트·기호 타입·표시 옵션은 기존 state에서 읽는다.
    void BindCursor(CursorPreviewSource source);
    // entry의 x/y/depth 대신 raw signed 화면 WORD/프레임과 tail bit 2를 읽는다.
    void Draw(const o::RenderCollectionEntry& entry) const;
private:
    std::optional<CursorPreviewSource> cursor_; // 수집기와 같은 커서 전역 및 실제 격자를 참조한다.
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const o::SidPool& pool_;/* types_: 타입 번호로 속성·플래그를 조회하는 타입 표. */ std::span<const o::RiftTypeRecord> types_;
    // collection_: 원본 전역의 현재 목록/선택 타입/부모와 배치 칸, 패치 전용 표시 상태다 (PreviewCollectionState).
    const PreviewCollectionState& collection_;/* state_: 준비한 실제 커서 전역과 격자/글자 표다. legal/parent/플레이어는 수집 상태를 공유한다 (PreviewDrawState). */ const PreviewDrawState& state_;/* hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (PreviewDrawHooks). */ PreviewDrawHooks hooks_;/* tokenBase_: 토큰 기본. */ std::uint32_t tokenBase_{};
};
// 공통 kind 4 분배의 전체 항목을 실제 미리보기 Draw로 전달한다.
void BindPreviewDraw(DrawDispatchHooks& hooks,const PreviewDraw& draw);
// 실제 픽셀 압축/블록 바인딩이다. 자산은 출력 대상보다 오래 살아야 한다.
struct PreviewShapeBinding { /* database: 셰이프 블록과 프레임을 읽어 그림으로 변환하는 저장소 (ShapeDatabase). */ const ShapeDatabase* database{};/* block: 셰이프 DB에서 그림을 찾는 블록 번호. */ std::size_t block{}; };
// 원본 타입 번호를 실제 GameAssets SHP 블록으로 연결한다. 비용 이미지는 별도 바인딩으로 제공한다.
std::function<PreviewShapeBinding(std::uint32_t)> MakePreviewAssetShapes(const GameAssets& assets,
    std::uint32_t priceType,PreviewShapeBinding price);
// 실제 Take가 저장한 block+1 토큰을 매 그림 호출 때 읽는다. 금액 문자열 경계 뒤의 변경도 반영한다.
std::function<PreviewShapeBinding(std::uint32_t)> MakeCursorPreviewAssetShapes(const GameAssets& assets,
    std::uint32_t priceType,const CursorLifecycleState& lifecycle);
// 현재 8비트 출력 표면에서 실제 SHP 투명/색 표/격자·윤곽을 즉시 합성한다.
// text는 기존 번역/비트맵 글꼴에 연결한다. 전체 Client 프레임·커서 입력은 별도다.
class PreviewPixelTarget {
public:
    // color는 이미 만든 실제 256색 표를 반환한다. 화면/clip은 right·bottom 제외다.
    PreviewPixelTarget(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,
        std::function<PreviewShapeBinding(std::uint32_t)> shape,
        std::function<ColorMap(PreviewColorRequest)> color,std::function<void(PreviewTextRequest)> text);
    // 이 객체를 캡처한 실제 출력 콜백을 돌려준다. 사용 중 객체를 이동/삭제하지 않는다.
    PreviewDrawHooks Hooks();
private:
    // SHP를 한 번 해제하고 원본 VFX 상대 사각형/기준점으로 합성한다.
    void Shape(PreviewShapeRequest request);
    // 원본 격자는 오른/아래 제외 체크 무늬이며 윤곽은 모서리 선 네 개다.
    void Rectangle(ScreenRect rect,std::uint8_t color,bool checker);
    // pixels_: 팔레트 인덱스나 색을 직접 쓰는 출력 픽셀 버퍼.
    std::span<std::uint8_t> pixels_;/* pitch_: 출력 버퍼에서 한 행을 이동할 바이트 간격. */ /* height_: 세로 크기. */ int pitch_,height_;/* clip_: 출력을 제한하는 클리핑 영역 또는 상태. */ ScreenRect clip_;
    // shape_: 요청한 셰이프 그림을 조회하거나 출력하는 콜백.
    std::function<PreviewShapeBinding(std::uint32_t)> shape_;
    // color_: 출력 색 또는 색 변환표를 선택하는 콜백.
    std::function<ColorMap(PreviewColorRequest)> color_;/* text_: 요청한 본문을 실제 텍스트 처리기에 전달하는 콜백. */ std::function<void(PreviewTextRequest)> text_;
    // current_: 현재 처리 위치 또는 현재 선택 상태.
    std::optional<ColorMap> current_;/* images_: 프레임이나 커서 번호별로 읽어 둔 그림 자료. */ std::map<std::tuple<const ShapeDatabase*,std::size_t,std::size_t>,IndexedImage> images_;
};
}
