// 일반 Squid Draw의 본체·그림자·선택 표시를 실제 raw 풀과 연결한다.
#pragma once
#include "client/DrawSubmission.h"
#include "client/PreviewDraw.h"
#include "o/SquidDisplay.h"
#include "o/SquidHash.h"

namespace netstorm::client {
// VFX 호출의 원색·현재 소유자 표·명시 변환 표를 서로 구분한다.
enum class SpriteShapeMode { Plain, Owner, Abstract, Dead, Shadow };
// 실제 타입의 SHP와 선택 헤더/기본 프레임 및 상태 막대용 타입 필드다.
struct SpriteTypeView {
    o::SquidDisplayShape shape;
    std::uint32_t gumpFrame{};
    int capacity{},constructionTime{},duration{}; // 패치 타입 +0x1b8/+0x1bc/+0x1cc.
};
// 현재 선택 프로세스 목록의 한 슬롯이다. 0번 슬롯은 원본처럼 건너뛴다.
struct SpriteStatusProcess {
    std::uint32_t type{},flags{};
    std::span<const std::uint32_t> members;
    double endTime{};
};
// 자원 프로세스/내용물 조회가 제공하는 실제 원본 값이며 막대 좌표는 Draw에서 계산한다.
struct SpriteResourceStatus { std::uint32_t capacityType{};int remaining{}; };
// Draw가 읽는 원본 전역을 외부에서 현재 값으로 공급한다. 선택 SID/그림자 alias만 쓰기 대상이다.
struct SpriteDrawState {
    int cameraX{},cameraY{};
    std::uint32_t localPlayer{},specialMode{},filteredFlags{};
    std::uint32_t wideOwnerType{},unfilteredType{},shadowAliasType{};
    bool shadowAliasPinned{},quarterHitPoints{},selectionDisabled{},markerEnabled{},markerVisible{};
    std::uint32_t markerExcludeType{},markerType{},statusType{},abstractStatusType{};
    std::uint32_t timerProcessType{},statusProcessType{},costType{};
    std::array<std::uint32_t,3> timerTypes{}; // 20/60/60 최대값, 색 0xd9/9/0x6b 순서.
    std::vector<std::uint8_t> ownerMaps;
    std::vector<int> ownerPalettes;
    std::vector<std::uint32_t> selectionColors;
    std::array<std::uint32_t,3> hitPointColors{}; // 정상/절반/1/4 이하.
    std::uint32_t barBackground{},selectedSid{};
    std::optional<std::uint32_t> shadowShapeType;
    double now{};
    std::span<const SpriteStatusProcess> processes;
};
// 한 그림 요청은 색 문맥 변경과 분리되어 전달된다. 그림자는 명시 소스 치환을 사용한다.
struct SpriteShapeRequest { std::uint32_t type{},frame{};ScreenPoint position;SpriteShapeMode mode{}; };
// 출력 경계와 게임 규칙 조회 경계를 구분한다. 필요한 분기의 빠진 경계는 오류다.
struct SpriteDrawHooks {
    std::function<void(std::uint32_t,std::uint32_t)> setOwnerColors;
    std::function<void(SpriteShapeRequest)> shape;
    std::function<void(ScreenPoint,ScreenPoint,std::uint32_t)> line;
    std::function<void(ScreenRect,std::uint32_t)> fill;
    std::function<int(o::Sid)> virtualFrame;
    std::function<void(o::Sid,ScreenPoint)> virtualDraw;
    std::function<o::Sid(float,float)> surface;
    std::function<bool(o::Sid)> constructionActive;
    std::function<std::optional<SpriteResourceStatus>(o::Sid)> resource;
    std::function<std::optional<std::array<double,3>>(o::Sid,std::uint32_t)> timers;
};
// 00498220 ↔ CD 00455f60의 분기·프레임·색 선택·표식·선택/상태 막대를 복원한다.
class SpriteDraw {
public:
    // 풀/타입/그림/전역의 수명은 호출자가 유지한다. tokenBase는 수집기와 같은 값이어야 한다.
    SpriteDraw(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::span<const SpriteTypeView> views,
        SpriteDrawState& state,SpriteDrawHooks hooks,std::uint32_t tokenBase);
    // 화면 WORD와 월드 투영이 다르면 원본 assert에 해당하는 계약 오류를 알린다.
    void Draw(const o::RenderCollectionEntry& entry) const;
private:
    // alias가 바뀐 경우 물리 SHP 헤더만 현재 그림 타입에서 읽는다. 논리 프레임 수는 원래 타입에 남긴다.
    const o::SquidDisplayShape& Shape(std::uint32_t type) const;
    // 선택 헤더와 현재 raw HP를 읽고 판본별 막대 배치를 유지한다.
    void Selection(o::Sid sid,std::uint32_t type,std::uint32_t owner,std::uint32_t frame,ScreenPoint screen) const;
    const o::SidPool& pool_;
    std::span<const o::RiftTypeRecord> types_;
    std::span<const SpriteTypeView> views_;
    SpriteDrawState& state_;
    SpriteDrawHooks hooks_;
    std::uint32_t tokenBase_;
};
// 공통 kind 0과 kind 1 묶음의 각 원소가 같은 일반 Draw를 사용하게 한다.
void BindSpriteDraw(DrawDispatchHooks& hooks,const SpriteDraw& draw);
// 실제 자산의 선택 헤더/프레임 수와 .type 기본 프레임을 준비한다. 추가 상태 필드는 별도 공급한다.
std::vector<SpriteTypeView> MakeSpriteAssetViews(const GameAssets& assets);
// 그림자 alias의 SHP 포인터만 현재 원본 타입에 연결하고 나머지 타입 정보는 그대로 둔다.
std::function<PreviewShapeBinding(std::uint32_t)> MakeSpriteAssetShapes(const GameAssets& assets,const SpriteDrawState& state);
// 표식의 +0.9999/CRT 좌표와 실제 0단계 표면 SID 표를 연결한다.
std::function<o::Sid(float,float)> MakeSpriteSurfaceLookup(const o::SquidHash& hash);
// 실제 SHP 합성·직교 선택 선·막대 fill을 클립된 8비트 표면으로 출력한다.
class SpritePixelTarget {
public:
    // shape는 alias를 포함한 현재 바인딩, colors는 실제 원본 색 표를 반환해야 한다.
    SpritePixelTarget(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,
        std::function<PreviewShapeBinding(std::uint32_t)> shape,
        std::function<ColorMap(SpriteShapeMode,std::uint32_t,std::uint32_t)> colors);
    // 규칙 조회/가상 Draw를 받은 hooks에 실제 픽셀 출력을 연결한다.
    SpriteDrawHooks Hooks(SpriteDrawHooks rules={});
private:
    // SHP 투명 픽셀을 유지하면서 원색 또는 명시/현재 색 표로 합성한다.
    void Shape(SpriteShapeRequest request);
    // 양 끝 포함 직교 선을 원래 끝점에서 클립한다.
    void Line(ScreenPoint first,ScreenPoint last,std::uint32_t color);
    // 원본 half-open fill의 빈/역전 사각형은 픽셀을 바꾸지 않는다.
    void Fill(ScreenRect rect,std::uint32_t color);
    std::span<std::uint8_t> pixels_;
    int pitch_,height_;
    ScreenRect clip_;
    std::function<PreviewShapeBinding(std::uint32_t)> shape_;
    std::function<ColorMap(SpriteShapeMode,std::uint32_t,std::uint32_t)> colors_;
    std::optional<ColorMap> current_;
    std::map<std::tuple<const ShapeDatabase*,std::size_t,std::size_t>,IndexedImage> images_;
};
}
