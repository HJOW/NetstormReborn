// 원본 동적 Image를 빌리는 전용 MetaImage의 생성·기하·가상 출력을 복원한다.
#pragma once
#include "client/GumpCreate.h"
#include "client/GumpDraw.h"
#include "client/RawImage.h"

namespace netstorm::client { // 자산 TYPE ImageGump와 독립된 원본 MetaImage 파생이다.
// Image 수명은 목록/창이 소유하며 이 노드는 그 주소만 빌린다.
struct MetaImageGumpNode: GumpCreateNode {
    RawImage* image{}; // 원본+a0의 현재 Image이며 파생 노드 소멸이 Image를 해제하지 않는다.
    std::uint32_t colorMode{}; // 원본+a4 비영은 단위배율에서00556038 비활성 색표를 선택한다.
};
// 실제 VFX frame0/angle0 출력의 기준점·16.16 배율·plain/mapped 분기다.
struct MetaImageDrawRequest {
    const RawImage* image{}; // 현재 Image의 실제1.10 shape를 출력 동안만 빌린다.
    ScreenPoint position{}; // 원문 Image 폭/높이를 반영한 VFX 기준점이다.
    std::int32_t scale{65536}; // 두 축의 동일한 signed16.16 값이다.
    bool scaled{},mapped{}; // 확대/축소는 colorMode와 무관한 plain 변환이다.
};
// 같은 공통 저장소/부착/위치와 현재 픽셀·색표 효과를 빌린다.
struct MetaImageGumpHooks {
    std::function<MetaImageGumpNode*(std::uint32_t)> node; // 실제 파생 저장소다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 기본 Attach가 먼저 실행된다.
    std::function<ScreenPoint(std::uint32_t)> position; // 같은 공통 위치 getter다.
    std::function<void(std::uint32_t)> color; // 실제00556038 색표 설치를 요청한다.
    std::function<void(MetaImageDrawRequest)> draw; // 현재 표면과 같은 클립에서 shape를 소비한다.
};
//0047f2a0/0047cbd0/0047cc00/0047cae0의 실제 몸체다.
class MetaImageGump {
public:
    MetaImageGump(const GumpCreateState& create,const GumpCreate& factory,MetaImageGumpHooks hooks); //10.78 원문과 같은 단일 등록표를 사용한다.
    std::uint32_t Construct(std::uint32_t token,RawImage& image) const; // 기본0크기 생성 후 Image 주소만 빌린다.
    void Resize(std::uint32_t token) const; // 현재 image 크기+1로 기본/배율 크기를 갱신한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // 기본 부착 뒤 현재 Image/기본 크기의 비율로 축소한다.
    void Draw(std::uint32_t token) const; // shape가 없으면 위치 조회 전 반환하며 원문 분기/ftol 순서를 유지한다.
private:
    MetaImageGumpNode& Node(std::uint32_t token) const; // 없는 파생/현재 Image는 진단으로 구별한다.
    const GumpCreate& factory_; // 실제 기본 생성자/등록표다.
    MetaImageGumpHooks hooks_; // 현재 공통 트리와 Draw 문맥의 빌린 효과다.
};
// 기존 VFX 해제/변환 커널을 동적 shape와 현재 UI 클립/공유 색표에 연결한다.
MetaImageGumpHooks BindMetaImagePixels(MetaImageGumpHooks hooks,const GumpPixelTarget& target,ColorMap& current,const ColorMap& disabled);
}
