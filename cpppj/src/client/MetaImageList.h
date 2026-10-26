// 원본21칸 Image 목록의 등록·배치·적중·선택·삭제를 복원한다.
#pragma once
#include "client/MetaImageGump.h"

namespace netstorm::client { // 목록은 MetaImage 자식을 소유하고 각 Image는 창/호출자가 소유한다.
inline constexpr std::size_t kMetaImageListCapacity=21; // 원문 IML_MAX_SLOTS 저장소 크기다.
// 같은 기본 Gump 뒤의 원문 목록 필드이며 사용하지 않는 배열 끝도 보존한다.
struct MetaImageListNode: GumpCreateNode {
    std::uint32_t count{}; // 현재 유효한 자식/ID 접두부 길이다.
    std::array<std::uint32_t,kMetaImageListCapacity> images{},ids{}; // 실제 MetaImage 토큰과 별도 원문 ID다.
    std::int32_t selection{-1},columns{}; // 선택 인덱스와 열 수이며0이하는 가로 배치다.
    float cellHeight{},cellWidth{}; // 원문+154/+158의 배치 후 셀 크기다.
};
// 실제 공통 처리기와 가상 자식 효과는 같은 저장소를 사용한다.
struct MetaImageListHooks {
    std::function<MetaImageListNode*(std::uint32_t)> node; // 실제 목록 파생 저장소다.
    std::function<MetaImageGumpNode*(std::uint32_t)> image; // 현재 자식의 실제 비소유 Image/기하다.
    std::function<std::uint32_t(RawImage&)> createImage; // 실제0047f2a0 생성과 기본 등록이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach,baseAttach; // 현재 가상 Attach와 기본 Attach를 구별한다.
    std::function<void(std::uint32_t)> push,kill,destructBase; // 실제 분리/지연 Kill/기본 소멸이다.
    std::function<ScreenRect(std::uint32_t)> bounds; // 같은 공통 현재 화면 경계다.
    std::function<void(std::uint32_t,bool)> redraw; // 현재 목록 가상 redraw다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> notify; // 자신을 제외한 부모 Bubble이다.
    std::function<std::uint32_t()> green; // 현재 녹색 전역의 전체DWORD다.
    std::function<void(ScreenRect,std::uint32_t)> fill; // 원문 선택의 half-open 체크무늬 채우기다.
};
//0047ccb0부터0047d1f0 및0047f360의 실제 목록 몸체다.
class MetaImageList {
public:
    MetaImageList(const GumpCreateState& create,const GumpCreate& factory,MetaImageListHooks hooks); // 실제10.78 공통 factory를 빌린다.
    std::uint32_t Construct(std::uint32_t token,std::int32_t columns) const; // 자식 배열/count/선택만 초기화하며 ID/셀 크기는 보존한다.
    bool Add(std::uint32_t token,RawImage& image,std::uint32_t id) const; // ID→자식 생성→부착→목록 Push/재부착 순서다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // 실제 grid/가로 배치와 매 자식의 현재 count/셀 읽기다.
    std::int32_t Hit(std::uint32_t token,std::int32_t x,std::int32_t y) const; // 오른쪽/아래 제외와 원문 count 동등 경계를 유지한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; // 좌누름만 선택을 토글하고 원문 전체 인자로 부모에 통지한다.
    void Draw(std::uint32_t token) const; // 선택 셀의 원문 절삭 좌표를 체크무늬로 채운다.
    std::uint32_t Id(std::uint32_t token,std::int32_t index) const; // 원문 별도 ID이며 같은 인덱스와 구별한다.
    RawImage* Image(std::uint32_t token,std::int32_t index) const; // 현재 count 안의 자식이 빌린 실제 Image다.
    bool Remove(std::uint32_t token,std::int32_t index) const; // Kill만 예약하고 두 배열 접두부를 압축한다. 선택/끝 슬롯은 보존한다.
    void Destruct(std::uint32_t token) const; // 목록 가상 표 복원 뒤 실제 기본 소멸자를 호출한다.
private:
    MetaImageListNode& Node(std::uint32_t token) const; // 없는 저장소나 유효 범위를 벗어난 count는 거부한다.
    MetaImageGumpNode& Child(std::uint32_t token) const; // 없는 실제 자식을 호출 위치에서 진단한다.
    const GumpCreate& factory_; // 현재 공통 등록/기본 생성자다.
    MetaImageListHooks hooks_; // 같은 트리/수명/현재 출력 문맥이다.
};
}
