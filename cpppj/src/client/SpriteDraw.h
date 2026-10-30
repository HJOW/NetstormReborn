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
    // shape: 프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료.
    o::SquidDisplayShape shape;
    // gumpFrame: Gump 프레임.
    std::uint32_t gumpFrame{};
    int capacity{},constructionTime{},duration{}; // 패치 타입 +0x1b8/+0x1bc/+0x1cc.
};
// 현재 선택 프로세스 목록의 한 슬롯이다. 0번 슬롯은 원본처럼 건너뛴다.
struct SpriteStatusProcess {
    // type: 타입 표에서 객체 종류를 찾는 번호.
    // flags: 동작을 선택하는 플래그 비트.
    std::uint32_t type{},flags{};
    // members: 멤버.
    std::span<const std::uint32_t> members;
    // endTime: 끝 시각.
    double endTime{};
};
// 자원 프로세스/내용물 조회가 제공하는 실제 원본 값이며 막대 좌표는 Draw에서 계산한다.
struct SpriteResourceStatus { /* capacityType: 용량 타입. */ std::uint32_t capacityType{};/* remaining: 남은 수량 또는 남은 수명. */ int remaining{}; };
// Draw가 읽는 원본 전역을 외부에서 현재 값으로 공급한다. 선택 SID/그림자 alias만 쓰기 대상이다.
struct SpriteDrawState {
    // cameraX: 월드에서 화면에 투영할 카메라 X 원점.
    // cameraY: 월드에서 화면에 투영할 카메라 Y 원점.
    int cameraX{},cameraY{};
    // localPlayer: 로컬 플레이어.
    // specialMode: 특수 모드.
    // filteredFlags: 필터된 플래그.
    std::uint32_t localPlayer{},specialMode{},filteredFlags{};
    // wideOwnerType: 넓은 소유자 타입.
    // unfilteredType: 필터전 타입.
    // shadowAliasType: 그림자 별칭 타입.
    std::uint32_t wideOwnerType{},unfilteredType{},shadowAliasType{};
    // shadowAliasPinned: 그림자 별칭 고정된 여부.
    // quarterHitPoints: 4분의1 체력 점 여부.
    // selectionDisabled: 선택 비활성 여부.
    // markerEnabled: 표식 활성 여부.
    // markerVisible: 표식 표시 여부.
    bool shadowAliasPinned{},quarterHitPoints{},selectionDisabled{},markerEnabled{},markerVisible{};
    // markerExcludeType: 표식 대상에서 제외할 타입 번호.
    // markerType: 선택·체크 표식을 그릴 타입 번호.
    // statusType: 상태 타입.
    // abstractStatusType: 추상 상태 타입.
    std::uint32_t markerExcludeType{},markerType{},statusType{},abstractStatusType{};
    // timerProcessType: 타이머 프로세스 타입.
    // statusProcessType: 상태 프로세스 타입.
    // costType: 비용 타입.
    std::uint32_t timerProcessType{},statusProcessType{},costType{};
    std::array<std::uint32_t,3> timerTypes{}; // 20/60/60 최대값, 색 0xd9/9/0x6b 순서.
    // ownerMaps: 플레이어별 미리보기 색 변환표.
    std::vector<std::uint8_t> ownerMaps;
    std::span<const std::uint32_t> sharedOwnerMaps; // 실제 Client의557940이며 기존 독립 입력만 위 지역 표를 사용한다.
    const std::uint32_t* sharedUseBanners{}; // 실제5319c0을 표식 출력 순간에 다시 읽는다.
    // ownerPalettes: 소유자 팔레트.
    std::vector<int> ownerPalettes;
    // selectionColors: 선택 색.
    std::vector<std::uint32_t> selectionColors;
    std::array<std::uint32_t,3> hitPointColors{}; // 정상/절반/1/4 이하.
    // barBackground: 막대 배경.
    // selectedSid: 선택된 SID.
    std::uint32_t barBackground{},selectedSid{};
    // shadowShapeType: 그림자 셰이프 타입.
    std::optional<std::uint32_t> shadowShapeType;
    // now: 현재 판정에 사용하는 시각(초).
    double now{};
    // processes: 공유 커널에 연결한 객체 프로세스 호스트.
    std::span<const SpriteStatusProcess> processes;
    const std::uint32_t* sharedPatchDiagnostic{}; // 실제005318ec을 그림자 분기에서 읽는다. 독립 fixture의 NULL만 shadowAliasPinned 입력을 쓴다.
};
// 한 그림 요청은 색 문맥 변경과 분리되어 전달된다. 그림자는 명시 소스 치환을 사용한다.
struct SpriteShapeRequest {
    std::uint32_t type{},frame{}; // 같은 자산 표의 TYPE/SHP 식별자와 실제 물리 프레임이다.
    ScreenPoint position; // 원문 VFX 기준점이며 프레임 rect 원점은 출력기가 적용한다.
    SpriteShapeMode mode{}; // 원색/소유자/고정 소스 치환표의 선택이다.
    bool scaled{}; // true는 원문4daaf0의 회전 없는 VFX 배율 출력으로 분배한다.
    std::int32_t scaleX{65536},scaleY{65536}; // signed16.16 두 축 배율이며 기본은1배다.
};
// 출력 경계와 게임 규칙 조회 경계를 구분한다. 필요한 분기의 빠진 경계는 오류다.
struct SpriteDrawHooks {
    // setOwnerColors: 타입과 소유자에 맞춰 스프라이트 색 변환을 설정하는 콜백.
    std::function<void(std::uint32_t,std::uint32_t)> setOwnerColors;
    // shape: 요청한 셰이프 그림을 조회하거나 출력하는 콜백.
    std::function<void(SpriteShapeRequest)> shape;
    // line: 지정한 두 점 사이의 선을 출력하는 콜백.
    std::function<void(ScreenPoint,ScreenPoint,std::uint32_t)> line;
    // fill: 지정 영역을 단색으로 채우는 콜백.
    std::function<void(ScreenRect,std::uint32_t)> fill;
    // virtualFrame: 대상의 가상 프레임 번호를 조회하는 콜백.
    std::function<int(o::Sid)> virtualFrame;
    // virtualDraw: 대상의 파생 가상 그리기 함수를 호출하는 콜백.
    std::function<void(o::Sid,ScreenPoint)> virtualDraw;
    // surface: 지정 월드 좌표의 표면 SID를 조회하는 콜백.
    std::function<o::Sid(float,float)> surface;
    // constructionActive: 대상에 진행 중인 건설 표시가 있는지 조회하는 콜백.
    std::function<bool(o::Sid)> constructionActive;
    // resource: 대상의 자원 수량·표시 상태를 조회하는 콜백.
    std::function<std::optional<SpriteResourceStatus>(o::Sid)> resource;
    // timers: 대상의 표시용 타이머 시각을 조회하는 콜백.
    std::function<std::optional<std::array<double,3>>(o::Sid,std::uint32_t)> timers;
    // statusProcesses: 선택 막대 직전에 실제 Kernel 순서의 현재 상태 프로세스 목록을 조회한다.
    std::function<std::span<const SpriteStatusProcess>()> statusProcesses;
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
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const o::SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const o::RiftTypeRecord> types_;
    // views_: 타입에서 프레임·셰이프 블록을 찾는 조회 정보 목록.
    std::span<const SpriteTypeView> views_;
    // state_: Draw가 읽는 원본 전역을 외부에서 현재 값으로 공급한다. 선택 SID/그림자 alias만 쓰기 대상이다 (SpriteDrawState).
    SpriteDrawState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (SpriteDrawHooks).
    SpriteDrawHooks hooks_;
    // tokenBase_: 토큰 기본.
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
    // pixels_: 팔레트 인덱스나 색을 직접 쓰는 출력 픽셀 버퍼.
    std::span<std::uint8_t> pixels_;
    // pitch_: 출력 버퍼에서 한 행을 이동할 바이트 간격.
    // height_: 세로 크기.
    int pitch_,height_;
    // clip_: 출력을 제한하는 클리핑 영역 또는 상태.
    ScreenRect clip_;
    // shape_: 요청한 셰이프 그림을 조회하거나 출력하는 콜백.
    std::function<PreviewShapeBinding(std::uint32_t)> shape_;
    // colors_: 셰이프 출력 모드·타입·소유자별 색 변환표를 조회하는 콜백.
    std::function<ColorMap(SpriteShapeMode,std::uint32_t,std::uint32_t)> colors_;
    // current_: 현재 처리 위치 또는 현재 선택 상태.
    std::optional<ColorMap> current_;
    // images_: 프레임이나 커서 번호별로 읽어 둔 그림 자료.
    std::map<std::tuple<const ShapeDatabase*,std::size_t,std::size_t>,IndexedImage> images_;
};
}
