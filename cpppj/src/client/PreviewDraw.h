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
struct PreviewColorRequest { PreviewColors colors;std::uint32_t type{},owner{}; };
// SHP 타입/frame과 화면 기준점이다. 별도 비용 이미지는 priceType으로 지정한다.
struct PreviewShapeRequest { std::uint32_t type{},frame{};ScreenPoint position;PreviewShapeMode mode{}; };
// 메시지 번호 2~4 또는 비용 문자열 5를 상위 번역/글꼴 계층으로 전달한다.
struct PreviewTextRequest { std::uint32_t index{};ScreenPoint position; };
// 준비한 실제 커서 전역과 격자/글자 표다. legal/parent/플레이어는 수집 상태를 공유한다.
struct PreviewDrawState {
    std::uint32_t selectedType{},destroyType{},noOutlineType{},overlayTrigger{},overlayType{},labelType{},priceType{};
    std::int32_t cameraX{},cameraY{},spacing{},labelOffsetX{},labelOffsetY{},labelWidth{},labelHeight{};
    std::int32_t priceWidth{},priceIconOffset{},priceIconX{},priceIconY{};
    std::uint32_t gridOption{},showLetters{},flags{},messageOverride{},blocked{},powered{},reachable{};
    std::uint8_t goodColor{},badColor{};
    std::array<std::uint8_t,144> grid{}; // 원본 (열+1)*12+(행+1), 최대 footprint 10까지.
    std::array<std::uint32_t,256> labelFrames{}; // MSVC signed char의 원본 표 조회를 호스트 바이트로 연결한다.
    std::string labels;
    std::array<std::int32_t,5> messageWidths{},messageHeights{};
    std::vector<bool> ownerMaps;
};
// 실제 SHP/색 표/장치 경계다. rectangle의 true는 체크 무늬, false는 윤곽이다.
struct PreviewDrawHooks {
    std::function<void(PreviewColorRequest)> setColors;
    std::function<void(PreviewShapeRequest)> shape;
    std::function<void(ScreenRect,std::uint8_t,bool)> rectangle;
    std::function<void(PreviewTextRequest)> text;
};
// 수집에서 받은 대리 토큰을 같은 raw 풀로 되돌리고 두 판본의 전체 Draw 흐름을 복원한다.
class PreviewDraw {
public:
    // 풀/타입/현재 상태와 콜백은 이 객체보다 오래 살아야 한다.
    PreviewDraw(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
        const PreviewCollectionState& collection,const PreviewDrawState& state,PreviewDrawHooks hooks,std::uint32_t tokenBase=0);
    // entry의 x/y/depth 대신 raw signed 화면 WORD/프레임과 tail bit 2를 읽는다.
    void Draw(const o::RenderCollectionEntry& entry) const;
private:
    const o::SidPool& pool_;std::span<const o::RiftTypeRecord> types_;
    const PreviewCollectionState& collection_;const PreviewDrawState& state_;PreviewDrawHooks hooks_;std::uint32_t tokenBase_{};
};
// 공통 kind 4 분배의 전체 항목을 실제 미리보기 Draw로 전달한다.
void BindPreviewDraw(DrawDispatchHooks& hooks,const PreviewDraw& draw);
// 실제 픽셀 압축/블록 바인딩이다. 자산은 출력 대상보다 오래 살아야 한다.
struct PreviewShapeBinding { const ShapeDatabase* database{};std::size_t block{}; };
// 원본 타입 번호를 실제 GameAssets SHP 블록으로 연결한다. 비용 이미지는 별도 바인딩으로 제공한다.
std::function<PreviewShapeBinding(std::uint32_t)> MakePreviewAssetShapes(const GameAssets& assets,
    std::uint32_t priceType,PreviewShapeBinding price);
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
    std::span<std::uint8_t> pixels_;int pitch_,height_;ScreenRect clip_;
    std::function<PreviewShapeBinding(std::uint32_t)> shape_;
    std::function<ColorMap(PreviewColorRequest)> color_;std::function<void(PreviewTextRequest)> text_;
    std::optional<ColorMap> current_;std::map<std::tuple<const ShapeDatabase*,std::size_t,std::size_t>,IndexedImage> images_;
};
}
