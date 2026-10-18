// 원본 생산 아이콘의 SHP Gump 생성·배율·색·그림자·투명 적중을 복원한다.
#pragma once
#include "client/GumpCreate.h"
#include "client/GumpDraw.h"
#include "client/GameAssets.h"
#include <functional>
#include <map>

namespace netstorm::client {
// 실제501bb0의 +a0~b0다. 기본 저장소/등록/수명은 기존 Gump와 공유한다.
struct ImageGumpNode: GumpCreateNode {
    std::uint32_t type{},frame{},mode{}; // +a0/+a4/+a8: 실제 TYPE/SHP 프레임과 출력 비트다.
    float hotspotX{},hotspotY{}; // +ac/+b0: SHP signed기준점−TYPE signed기준점의 최종float이다.
};
// 원본 TYPE+dc/+1dc/+1e0의 현재 자산 정보다.
struct ImageGumpType { std::uint32_t shape{};std::int32_t hotspotX{},hotspotY{}; }; // SHP 토큰과 두 정수 기준점이다.
// 원본 추가 헤더 +18/+1a/+1c/+1e의 signed WORD다.
struct ImageGumpHeader { std::int16_t width{},height{},hotspotX{},hotspotY{}; }; // 크기와 프레임 기준점이다.
// 실제 VFX 일반/변환 호출의 같은 의미다. 회전 인자는 항상0이다.
struct ImageGumpDrawRequest {
    std::uint32_t type{},shape{},frame{}; // 같은 TYPE/실제 SHP 토큰/물리 프레임이다.
    ScreenPoint position; // VFX 기준점의 signed DWORD 좌표다.
    std::int32_t scale{}; // 같은 두 축의 signed16.16 배율이다.
    bool scaled{},mapped{}; // 실제 일반/변환과 현재 색표 사용 분기다.
};
// 4daca0의 로컬 검사점과 임시 버퍼의 그림 기준점/배율이다.
struct ImageGumpHitRequest {
    std::uint32_t type{},shape{},frame{}; // 같은 실제 그림 선택이다.
    ScreenPoint point,origin; // 항목 로컬 검사점과 이미지 자식 위치를 포함한 기준점이다.
    std::int32_t scale{}; // 원본 두 축의 같은 signed16.16 값이다.
};
// 조회 시 현재 원본 전역을 읽으며 생성 상태와 구별한다.
struct ImageGumpState {
    const std::uint32_t& localOwner; // 40da40의 현재 로컬 owner DWORD다.
    std::span<const std::uint32_t> ownerMaps; // 557940: TYPE별9색표의 현재 기본 토큰이다.
    const ScreenPoint& shadowOffset; // 52f434/52f444: 선행 그림의 현재 정수 오프셋이다.
};
// 같은 공통 저장소/자산/현재 표면으로 연결하는 실제 외부 경계다.
struct ImageGumpHooks {
    std::function<ImageGumpNode*(std::uint32_t)> node; // 안정된 파생 노드다.
    std::function<ImageGumpType(std::uint32_t)> type; // 현재 TYPE의 실제 SHP와 기준점이다.
    std::function<ImageGumpHeader(std::uint32_t,std::uint32_t)> header; // 419850의 실제 프레임 헤더다.
    std::function<ScreenPoint(std::uint32_t)> position; // 같은 GumpCollection::Position이다.
    std::function<void(std::uint32_t)> color; // 4dabe0의 현재256색표 설치다.
    std::function<void(ImageGumpDrawRequest)> draw; // 같은 표면의 실제 일반/변환 VFX다.
    std::function<std::uint32_t(ImageGumpHitRequest)> hit; // 4daca0의 실제 임시 그림 적중이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 419cb0의 같은 기본 부착이다.
};
// 원본419fc0/4198f0/419990/419cd0/419cb0의 전체 몸체다. 파생 소유 저장소는 호출자가 제공한다.
class ImageGump {
public:
    // 기본 생성/현재 전역·자산·노드는 처리기보다 오래 살아야 한다. 장치나 창을 만들지 않는다.
    ImageGump(const GumpCreateState& create,const GumpCreate& factory,ImageGumpState state,ImageGumpHooks hooks);
    std::uint32_t Construct(std::uint32_t token,std::uint32_t type,std::uint32_t frame,std::uint32_t mode) const; // 실제 기본 생성→TYPE BYTE3→프레임 설정이다.
    void SetFrame(std::uint32_t token,std::uint32_t type,std::uint32_t frame,std::uint32_t mode) const; // 현재 배율로 크기·정수 기준점 차이를 저장한다.
    void Draw(std::uint32_t token) const; // 선행 그림→모드색/현재owner/일반·배율 분기를 유지한다.
    std::uint32_t Hit(std::uint32_t token,float x,float y) const; // 실제 항목 로컬 좌표의 투명 영역을 판정한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // 같은 GumpTree로 그대로 전달한다.
private:
    ImageGumpNode& Node(std::uint32_t token) const; // 없는 파생 저장소를 호출 위치에서 거부한다.
    const GumpCreate& factory_; // 단일 실제 등록표의 기본 생성자다.
    ImageGumpState state_; // 현재 owner/색표 토큰/선행 위치의 안정 참조다.
    ImageGumpHooks hooks_; // 실제 하위 효과와 자료 공급이다.
};
// 생성/프레임 조회를 보호 GameAssets의 실제 TYPE/SHP에 연결한다.
void BindImageGumpAssets(ImageGumpHooks& hooks,const GameAssets& assets);
// 기존 VFX 일반/직교 배율을 현재 UI 표면과 실제200×200 적중 버퍼에 연결한다.
class ImageGumpPixelTarget {
public:
    // 색표 공급은 현재 실제 토큰을 해석한다. 누락/미복원 표를 항등으로 바꾸지 않는다.
    ImageGumpPixelTarget(const GumpPixelTarget& target,const GameAssets& assets,ColorMap& currentColors,
        std::function<ColorMap(std::uint32_t)> colors);
    ImageGumpHooks Hooks(ImageGumpHooks hooks); // 기존 저장소/부착 훅을 보존하고 실제 그리기·색·적중만 연결한다.
private:
    const IndexedImage& Image(std::uint32_t type,std::uint32_t frame); // 실제 일반 프레임의 해제 픽셀을 캐시한다.
    std::size_t Block(std::uint32_t type) const; // 현재 실제 TYPE 번호를 보호 SHP 블록에 연결한다.
    void Draw(ImageGumpDrawRequest request); // 현재 표면/클립에 실제 VFX를 출력한다.
    std::uint32_t Hit(ImageGumpHitRequest request); // 원본 임시 버퍼/기준점 클램프와255 판정을 실행한다.
    const GumpPixelTarget& target_;const GameAssets& assets_; // 같은 UI 표면과 실제 자산의 안정 참조다.
    ColorMap& currentColors_; // 이후 mapped 그림이 사용하는 현재256색 문맥이다.
    std::function<ColorMap(std::uint32_t)> colors_; // 실제 원본 색표 토큰의 공급자다.
    std::map<std::pair<std::uint32_t,std::uint32_t>,IndexedImage> images_; // 실제 TYPE/프레임별 해제 픽셀이다.
};
}
