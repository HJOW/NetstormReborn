// 원본 섬 추가 창의 자식 배치와 드래그 미리보기 갱신을 같은 Gump 저장소에 적용한다.
#pragma once
#include "client/EditorIslandDrag.h"
#include "client/MetaImageGump.h"

namespace netstorm::client { // 창 생성/Refresh의 소유권과 배치 중 빌리는 현재 필드를 구별한다.
// 실제 창의 Image·일곱 자식·입력 상태에 대한 비소유 연결이며 별도 후보/지도 사본을 만들지 않는다.
struct EditorIslandLayoutFields {
    RawImage* image{}; // 원본+c4의 현재 지도 Image다.
    std::uint32_t background{},map{},done{},hidden{},title{},list{},tell{}; // 원본+d8..+f0의 현재 자식 토큰이다.
    EditorIslandDragState* interaction{}; // 같은+d84 드래그와+d88 record6BYTE를 빌린다.
};
// 가상 자식 효과를 호출한 뒤 실제 현재 필드를 다시 읽는 원문 경계다.
struct EditorIslandLayoutHooks {
    std::function<EditorIslandLayoutFields&(std::uint32_t)> fields; // 같은 창의 현재 자식/이미지/입력 필드를 공급한다.
    std::function<GumpCreateNode*(std::uint32_t)> node; // 공통 트리의 실제 크기/배율/깊이다.
    std::function<MetaImageGumpNode*(std::uint32_t)> image; // 같은 실제 MetaImage 파생 저장소다.
    std::function<void(std::uint32_t)> resizeImage,push; // 실제0047cbd0과 가상 Push다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> baseAttach,attach; // 기본 Attach와 자식 가상 Attach를 구별한다.
    std::function<ScreenRect(std::uint32_t)> bounds; // 현재 지도 이미지의 화면 경계다.
    std::function<ScreenPoint(std::uint32_t)> position; // 충돌 검사 뒤 다시 읽는 같은 지도 위치다.
    std::function<bool(std::span<const std::uint8_t,6>)> canPlace; // 현재 독립 preview의 실제0046de50이며 월드에는 쓰지 않는다.
    std::function<std::uint32_t()> root; // 매 부착 때 현재 실제 루트 토큰을 읽는다.
    std::function<void(const char*)> assertion; // 원문 drag!=NULL 진단 뒤의 계속 실행을 보존한다.
};
//0047d280 배치와0047d5f0 미리보기 Broadcast 몸체다. 전체 창 생성/Refresh는 별도 부모에서 호출한다.
class EditorIslandLayout {
public:
    explicit EditorIslandLayout(EditorIslandLayoutHooks hooks); // 같은 공통 트리/현재 Session 효과를 빌린다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // Image 측정→기본 Attach→현재 자식 Push/배치 순서다.
    void Motion(std::uint32_t token,std::int32_t x,std::int32_t y) const; // 반크기/지도 원점을 함께 절삭하고 격자·충돌 색상·루트 깊이를 갱신한다.
private:
    EditorIslandLayoutFields& Fields(std::uint32_t token) const; // 효과 뒤 같은 현재 필드를 재조회한다.
    GumpCreateNode& Node(std::uint32_t token) const; // 없는 공통 저장소를 진단한다.
    MetaImageGumpNode& Image(std::uint32_t token) const; // 실제 MetaImage 이외의 잘못된 자식을 거부한다.
    void Width(std::uint32_t token,double value) const; // 기본 float 저장과 배율 곱셈 전의 원문 중간 정밀도를 구별한다.
    void Height(std::uint32_t token,double value) const; // 높이도 같은 원문 두 저장 순서다.
    EditorIslandLayoutHooks hooks_; // 창보다 짧은 처리기 수명 동안 빌린 실제 하위 효과다.
};
}
