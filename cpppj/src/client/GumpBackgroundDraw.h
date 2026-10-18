// 배경 Gump의 현재 자산 조회·질감/단색/체크·디버그 출력과 실제 타입/SHP 공급을 복원한다.
#pragma once
#include "client/GumpBackground.h"
#include "client/GumpBackgroundCreate.h"
#include "client/GumpDraw.h"
#include "client/GameAssets.h"
#include <map>
#include <tuple>

namespace netstorm::client {
// debug는 패치 00565de0/CD 00535ea0, debugColor는 패치 00565e40/CD 00535ea4다.
// nearColor/farColor는 단색/체크/무배경의 1픽셀 왼쪽·위/오른쪽·아래 선 전역이다.
struct GumpBackgroundDrawState { /* debug: 원본 assert 경계와 진단 출력을 검사할지. */ bool debug{};/* debugColor: 진단 사각형 출력의 팔레트 색 인덱스. */ /* nearColor: 배경 명암 처리의 가까운 경계 색 인덱스. */ /* farColor: 배경 명암 처리의 먼 경계 색 인덱스. */ std::uint32_t debugColor{},nearColor{},farColor{}; };
// 마지막 파일 팔레트가 만든 기본 흰색/회색 결과 번호를 읽는다. 일시 팔레트에서는 재검색하지 않는다.
void SetGumpBackgroundColors(GumpBackgroundDrawState& state,const PaletteColorTable& palette);
// plain은 현재 색 표를 무시하는 원색 VFX, mapped는 마지막으로 설치한 명암 표의 소스 치환이다.
struct GumpBackgroundShapeRequest { /* type: 타입 표에서 객체 종류를 찾는 번호. */ /* frame: 타입에서 사용할 그림 프레임 번호. */ std::uint32_t type{},frame{};/* position: 현재 좌표 또는 저장소의 읽기 위치. */ ScreenPoint position;/* mapped: 색변환 여부. */ bool mapped{}; };
// frame는 매 질감 pass에서 현재 타입/SHP 헤더를 조회한다. 생성 노드의 cached shape/tile 크기를 쓰지 않는다.
// clip는 현재 half-open/VFX 전역을 동시에 갱신한다. 콜백이 바꾼 현재 클립/색을 이후 호출에서 다시 읽는다.
struct GumpBackgroundDrawHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<GumpBackgroundNode*(std::uint32_t)> node;
    // frame: 타입에서 사용할 그림 프레임 번호를 조회하는 콜백.
    std::function<GumpBackgroundFrame(std::uint32_t,std::uint32_t)> frame;
    // clip: 셰이프 출력의 클리핑 영역을 설정하는 콜백.
    std::function<void(ScreenRect)> clip;
    // shade: 명암 출력에 사용할 색 변환을 선택하는 콜백.
    std::function<void(unsigned)> shade;
    // shape: 요청한 셰이프 그림을 조회하거나 출력하는 콜백.
    std::function<void(GumpBackgroundShapeRequest)> shape;
    // fill: 지정 영역을 단색으로 채우는 콜백.
    std::function<void(ScreenRect,std::uint32_t,bool)> fill;
    // line: 지정한 두 점 사이의 선을 출력하는 콜백.
    std::function<void(GumpLine)> line;
};
// 00465c60/CD 00494e40 및 00465880/CD 00494a20의 전체 정상 출력 분배다.
class GumpBackgroundDraw {
public:
    // 수집/표면/전역/콜백의 수명은 분배기와 연결된 가상 Draw 동안 유지해야 한다.
    GumpBackgroundDraw(const GumpCollection& collection,GumpDrawState& clip,GumpBackgroundDrawState& state,GumpBackgroundDrawHooks hooks);
    // 현재 위치/배율 크기를 조회한 뒤 debug 또는 현재 노드의 flags/type/frame으로 출력한다.
    void Draw(std::uint32_t token) const;
    // 같은 전체 배경 분배를 명시 rect에서 실행한다. 단색/체크/무배경은 자산 조회가 필요 없다.
    void Background(ScreenRect rect,std::uint32_t flags,std::uint32_t type,std::uint32_t frame) const;
    // 각 질감 pass는 자산 조회 뒤 현재 클립을 저장하고 정상/예외 반환에서 복구한다. 색 표는 복구하지 않는다.
    void Texture(ScreenRect rect,std::uint32_t flags,std::uint32_t type,std::uint32_t frame) const;
private:
    // collection_: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (GumpCollection).
    const GumpCollection& collection_;
    // clip_: 출력을 제한하는 클리핑 영역 또는 상태.
    GumpDrawState& clip_;
    // state_: debug는 패치 00565de0/CD 00535ea0, debugColor는 패치 00565e40/CD 00535ea4다 (GumpBackgroundDrawState).
    GumpBackgroundDrawState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (GumpBackgroundDrawHooks).
    GumpBackgroundDrawHooks hooks_;
};
// 배경 토큰만 실제 가상 Draw로 분배하며 기존 돌 버튼/다른 파생 타입의 처리기를 보존한다.
void BindGumpBackgroundDraw(GumpDrawHooks& hooks,const GumpBackgroundDraw& draw,std::function<bool(std::uint32_t)> isBackground);
// 실제 SHP 블록과 원본 타입의 논리 프레임 수/flags1이다. 물리 레이어 수와 논리 수를 구분한다.
struct GumpBackgroundAssetBinding { /* type: 타입 표에서 객체 종류를 찾는 번호. */ std::uint32_t type{};/* block: 셰이프 DB에서 그림을 찾는 블록 번호. */ std::size_t block{};/* frameCount: 사용 가능한 프레임 개수. */ std::int32_t frameCount{};/* flags: 동작을 선택하는 플래그 비트. */ std::uint32_t flags{}; };
// 실제 GameAssets 또는 독립 검사 SHP를 기존 생성/출력의 자산 경계에 공급한다. 주소 토큰은 호스트 타입 번호다.
class GumpBackgroundAssets {
public:
    // 원본 타입 번호 70부터 실제 로딩 순서의 .type/SHP 블록을 연결한다. GameAssets는 이 객체보다 오래 살아야 한다.
    explicit GumpBackgroundAssets(const GameAssets& assets);
    // 검사/독립 SHP용 명시 바인딩이다. 데이터베이스는 어댑터보다 오래 살아야 한다.
    GumpBackgroundAssets(const ShapeDatabase& shapes,std::span<const GumpBackgroundAssetBinding> bindings);
    // 실제 타입 토큰/shape를 반환한다. 없는 타입/내장 무자산 타입은 호스트 예외다.
    const GumpBackgroundType& Type(std::uint32_t type) const;
    // 원본 frameCheck의 논리 수·추가 레이어 비트 0x440000·signed frame과 실제 물리 헤더를 확인한다.
    const GumpBackgroundFrame& Frame(std::uint32_t type,std::uint32_t abstract,std::uint32_t frame) const;
    // 정상 프레임의 실제 VFX 데이터베이스/블록을 제공한다. 생성 노드의 이전 shape 필드는 읽지 않는다.
    std::pair<const ShapeDatabase*,std::size_t> Shape(std::uint32_t type,std::uint32_t frame) const;
private:
    // 안정된 원본 메타데이터를 타입별로 소유한다. 여기의 shape 토큰을 물리 포인터로 재해석하지 않는다.
    struct View { /* type: 객체 종류를 구별하는 타입 자료. */ GumpBackgroundType type;/* block: 셰이프 DB에서 그림을 찾는 블록 번호. */ std::size_t block{};/* count: 현재 항목 또는 실행 횟수. */ std::int32_t count{};/* flags: 동작을 선택하는 플래그 비트. */ std::uint32_t flags{};/* frames: 타입별 프레임 자료. */ std::vector<GumpBackgroundFrame> frames; };
    // 중복 타입·잘못된 블록/논리 수·추가 헤더 누락을 거부하며 모든 물리 프레임 메타데이터를 보존한다.
    void Add(GumpBackgroundAssetBinding binding);
    // shapes_: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료.
    const ShapeDatabase& shapes_;
    // views_: 타입에서 프레임·셰이프 블록을 찾는 조회 정보 목록.
    std::map<std::uint32_t,View> views_;
};
// 동일 자산 어댑터를 배경 생성의 type/frame 경계와 매 pass의 프레임 조회에 연결한다.
void BindGumpBackgroundAssets(GumpBackgroundCreateHooks& create,GumpBackgroundDrawHooks& draw,const GumpBackgroundAssets& assets);
// 같은 GumpPixelTarget과 현재 VFX 색 표로 질감/선/fill을 출력한다. 반복 프레임의 해제 이미지는 캐시한다.
class GumpBackgroundPixelTarget {
public:
    // 모든 참조는 반환한 훅과 출력 대상보다 오래 살아야 한다. colors는 글자 출력과 같은 VFX 표를 공유할 수 있다.
    GumpBackgroundPixelTarget(const GumpPixelTarget& target,const GumpBackgroundAssets& assets,const GumpShadeMaps& maps,ColorMap& colors);
    // 규칙/노드/자산 조회 훅을 보존하고 실제 clip/shade/shape/fill/line 출력만 연결한다.
    GumpBackgroundDrawHooks Hooks(GumpDrawState& state,GumpBackgroundDrawHooks hooks={});
private:
    // 현재 타입/프레임의 상대 픽셀 영역을 원본 기준점에 더하여 원색 또는 현재 소스 색 표로 합성한다.
    void Shape(GumpBackgroundShapeRequest request);
    // target_: 버튼 선과 압축 해제한 배경/SHP 그림을 현재 VFX 클립으로 8비트 표면에 합성한다 (GumpPixelTarget).
    const GumpPixelTarget& target_;
    // assets_: 타입·셰이프·팔레트 자산 또는 해당 자산의 처리 경계.
    const GumpBackgroundAssets& assets_;
    // maps_: 0043c010/CD 00496530의 두 RGB 명암 표다. 소스 팔레트 번호를 목표 팔레트 번호로 변환한다 (GumpShadeMaps).
    const GumpShadeMaps& maps_;
    // colors_: 팔레트 색 또는 색 인덱스 변환 자료.
    ColorMap& colors_;
    // images_: 프레임이나 커서 번호별로 읽어 둔 그림 자료.
    std::map<std::tuple<const ShapeDatabase*,std::size_t,std::size_t>,IndexedImage> images_;
};
}
