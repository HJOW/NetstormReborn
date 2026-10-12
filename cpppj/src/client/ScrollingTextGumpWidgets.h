// 다중 줄 TextGump의 정상 생성·버퍼 소유·Slider 재부착·태그/스크롤/커서 입력과 프레임 드래그를 결합한다.
#pragma once
#include "client/TextGump.h"
#include "client/ScrollGumpWidgets.h"
#include <memory>

namespace netstorm::client {
// 버퍼는 Gump 등록 표에 넣지 않는 별도 소유 객체다. 동일 조각/글꼴/그림 처리기와 안정된 상태를 유지한다.
class StyleTextBufferStore {
public:
    // release는 원본 버퍼 free에 해당하는 호스트 토큰 반납이다. 상태/문맥/반납 경계는 저장소보다 오래 살아야 한다.
    StyleTextBufferStore(StyleTextState& fragments,const StyleText& style,StyleTextBufferHooks hooks,
        std::function<void(std::uint32_t)> release);
    // 원본 버퍼 생성자를 실제 새 상태에 호출한다. token은 외부 new(0x38)가 성공한 별도 식별자다.
    std::uint32_t Construct(std::uint32_t token,std::int32_t maxLines);
    // 기존 TextGump의 표시/입력에 같은 상태와 실제 버퍼 처리기를 공급한다. 없는 토큰은 거부한다.
    TextGumpBufferBinding Binding(std::uint32_t token) const;
    // 완료/표시 조각 소멸→호스트 free→기록 반납 순서다. 원본의 미완료 끝 칸 누락도 유지한다.
    void Destroy(std::uint32_t token);
private:
    // handler는 같은 record.state를 참조하므로 레코드 주소를 이동하거나 상태 복사본을 만들지 않는다.
    struct Record { StyleTextBufferState state;std::unique_ptr<StyleTextBuffer> handler; };
    // 안정된 현재 레코드를 실제 도달 시 읽는다. release 콜백은 현재 버퍼를 재귀 반납하면 안 된다.
    Record& Entry(std::uint32_t token) const;
    StyleTextState& fragments_;
    const StyleText& style_;
    StyleTextBufferHooks hooks_;
    std::function<void(std::uint32_t)> release_;
    std::map<std::uint32_t,std::unique_ptr<Record>> records_;
};
// 현재 signed 정렬과 Slider 배경 인자·실제 문자열 사건 ID 전역이다. 사건 ID는 원본 초기화 결과를 공급한다.
// 초기 정렬은 46, 나머지 ID/type은 실행 초기화 후 값이며 PE의 미초기화 0으로 대체하면 안 된다.
struct ScrollingTextGumpWidgetState {
    std::int8_t alignment{46};
    std::uint32_t sliderType{},sliderFrame{};
    std::uint32_t changeEvent{},stepEvent{},refreshEvent{},cursorEvent{},leftTagEvent{},rightTagEvent{};
};
// 같은 노드·Slider 조회와 new(0x38 버퍼/0xd8 Slider)·현재 가상 Push/Attach/Redraw·커서 선택 경계다.
// 두 allocate 크기의 반환 저장소 종류는 이 생성 계층의 문맥에 맞춰 준비한다. 모든 노드는 콜백 동안 안정돼야 한다.
struct ScrollingTextGumpWidgetHooks {
    std::function<ScrollingTextGumpNode*(std::uint32_t)> node;
    std::function<ScrollGumpNode*(std::uint32_t)> slider;
    std::function<GumpCreateNode*(std::uint32_t)> base;
    std::function<std::uint32_t(std::size_t)> allocate;
    std::function<void(std::uint32_t)> push;
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
    std::function<void(std::uint32_t,bool)> redraw;
    std::function<void(std::uint32_t)> cursor;
};
// 기존 다중 줄 표시/스크롤 처리기에 실제 생성/수명/입력과 추가 설정을 연결한다. 자동 Client 조립은 후속이다.
class ScrollingTextGumpWidgets {
public:
    // 저장소/생성/트리/수명/통지/캡처/조각/표시/Slider의 수명은 이 처리기와 연결한 분배보다 길어야 한다.
    ScrollingTextGumpWidgets(GumpCreateState& create,ScrollingTextGumpWidgetState& state,const GumpCreate& factory,
        const GumpTree& tree,const GumpLifetime& lifetime,const GumpEvents& events,const GumpInput& input,
        StyleTextState& fragments,StyleTextBufferStore& buffers,const ScrollingTextGump& text,
        const ScrollGumpWidgets& sliderWidgets,ScrollingTextGumpWidgetHooks hooks);
    // 004bc060/CD 004b5c30: 기본 등록/버퍼 128/문맥·크기/overlay→숨긴 Slider 9×100/부착→맨 아래 요청/기본 플래그다.
    // 미작성 +ac/dragY와 기존 textFlags의 다른 비트는 보존한다. 전체 생성 EH 정리는 별도 후속이다.
    std::uint32_t Construct(std::uint32_t token,std::int32_t width,std::int32_t height,std::uint32_t font,std::uint32_t color) const;
    // 004bc6b0/CD 004b6bd0: 기본 Attach→현재 Slider Push/높이·visible→우측 Attach(mode 1)→기존 scroll 요청/폭 dirty다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
    // 004bc450/CD 004b6560: Slider 변화/step 5·페이지·refresh와 클릭 태그/캡처/커서 1·15·16을 처리한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 004ba5e0/CD 004b6910: scroll 허용 bit 4와 전달 y delta만 사용해 반대 방향의 상대 요청을 한다. 폴링하지 않는다.
    void Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 004ba180/CD 004b6180: 비0이면 기본 Gump의 입력 제외 0x200을 지우고 0이면 설치한다.
    void SetSelectable(std::uint32_t token,std::int32_t value) const;
    // 004ba1b0/CD 004b61a0: 값의 최하위 비트로 slider bit 2를 바꾸고 필요 시 scroll bit 4를 켠다.
    // 기본 void bit 8이 없으면 현재 가상 Push→현재 x/y로 자신 Attach(parent 0,mode 0)를 호출한다.
    void SetSlider(std::uint32_t token,std::uint32_t value) const;
    // 004ba210/CD 004b6200: 현재 버퍼의 noWrap=(value==0)만 저장하며 dirty/Redraw는 호출하지 않는다.
    void SetWrap(std::uint32_t token,std::int32_t value) const;
    // 004ba230/CD 004b6220: 최하위 비트로 scroll bit 4를 바꾸며 끄면서 slider가 켜져 있으면 SetSlider(0)다.
    void SetScrollable(std::uint32_t token,std::uint32_t value) const;
    // 004bc3b0/CD 004b64f0: 현재 소유 버퍼를 Destroy/free/포인터 0 처리한 뒤 새 포인터/소유 DWORD만 쓴다.
    // 원본처럼 dirty/Redraw/스크롤/태그를 갱신하지 않는다. 같은 소유 토큰 교체는 소멸된 참조를 남긴다.
    void ReplaceBuffer(std::uint32_t token,std::uint32_t buffer,std::uint32_t owns) const;
    // 004bc890/004bc930/CD 004b5e70/004b7e70: 파생 표→현재 소유 버퍼 소멸/free/포인터 0→기본 scalar 소멸이다.
    // 빌린 버퍼는 포인터도 그대로 보존한다. 자식 Slider 삭제는 기존 자식 우선 Destroy가 맡는다.
    std::uint32_t DeleteScalar(std::uint32_t token,std::uint32_t flags) const;
private:
    // 기본/파생/Slider의 동일 주소와 실제 도달 토큰을 검사한다.
    ScrollingTextGumpNode& Node(std::uint32_t token) const;
    ScrollGumpNode& Slider(std::uint32_t token) const;
    GumpCreateNode& Base(std::uint32_t token) const;
    // 가상 +8(false)/+18/+20의 경계는 해당 분기에 도달했을 때 요구한다.
    void Redraw(std::uint32_t token) const;
    void Push(std::uint32_t token) const;
    void AttachSlider(std::uint32_t token,float x,std::uint32_t parent) const;
    // 현재 표시 조각의 공통 T 태그를 읽고 원본 반환 뒤 삭제한 조각은 호스트 오류로 거부한다.
    std::uint32_t Tag(std::uint32_t fragment) const;
    // 원본 화면 좌표의 하위 DWORD를 signed로 해석해 기존 조각 선택 함수에 전달한다.
    std::uint32_t Hit(std::uint32_t token,std::uint32_t x,std::uint32_t y) const;
    GumpCreateState& create_;
    ScrollingTextGumpWidgetState& state_;
    const GumpCreate& factory_;
    const GumpTree& tree_;
    const GumpLifetime& lifetime_;
    const GumpEvents& events_;
    const GumpInput& input_;
    StyleTextState& fragments_;
    StyleTextBufferStore& buffers_;
    const ScrollingTextGump& text_;
    const ScrollGumpWidgets& sliderWidgets_;
    ScrollingTextGumpWidgetHooks hooks_;
};
// 128칸 생성 저장소와 기존 다중 줄 표시/입력이 같은 실제 버퍼를 조회하게 한다.
void BindScrollingTextBuffers(ScrollingTextGumpHooks& hooks,const StyleTextBufferStore& buffers);
// 가상 Attach/재귀·부모 통지/직접 캡처/프레임/scalar를 실제 다중 줄 Widget에 연결하고 이전 분배를 보존한다.
void BindScrollingTextGumpWidgets(GumpTreeHooks& tree,GumpEventHooks& events,GumpInputHooks& input,
    GumpFrameHooks& frame,GumpLifetimeHooks& lifetime,std::function<const ScrollingTextGumpWidgets*(std::uint32_t)> resolve);
}
