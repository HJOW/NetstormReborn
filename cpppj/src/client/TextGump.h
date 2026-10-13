// 한 줄 TextGump와 다중 줄 표시기의 원본 크기·클립·기준선·스크롤·조각 선택을 복원한다.
#pragma once
#include "client/GumpDraw.h"
#include "client/GumpLifetime.h"
#include "client/StyleTextBuffer.h"

namespace netstorm::client {
// 원본 TextGump +a0의 소유 체인이다. 기본 생성/표시/수명 저장소와 같은 객체여야 한다.
struct TextGumpNode: GumpCreateNode { /* head: 연결 목록에서 순회를 시작할 머리 번호. */ std::uint32_t head{}; };
// 실제 생성자가 signed로 확장하는 정렬 BYTE 전역이다. 기본값은 두 배포 PE의 초기값이며 호출자가 변경할 수 있다.
struct TextGumpState { /* alignment: 항목의 좌우·상하 정렬을 선택하는 값. */ std::int8_t alignment{45}; };
// 파생 노드와 기본 노드는 같은 주소를 반환해야 한다. 실제 저장소의 반납은 GumpLifetime이 맡는다.
struct TextGumpHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<TextGumpNode*(std::uint32_t)> node;
    // baseNode: 파생 노드와 공유하는 기본 Gump 노드를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> baseNode;
};
// 메뉴 제목 등에 사용하는 원본 한 줄 클래스다. StyleText 조각/폰트/그림과 실제 Gump 클립을 공유한다.
class TextGump {
public:
    // 모든 처리기/전역/안정된 노드의 수명은 가상 Draw/소멸 분배보다 길어야 한다.
    TextGump(GumpCreateState& create,TextGumpState& state,const GumpCreate& factory,
        const GumpLifetime& lifetime,const GumpCollection& collection,GumpDrawState& clip,
        const StyleText& style,TextGumpHooks hooks);
    // 004bb170/CD 004b7280: 기본 (0,0,0,1) 생성 뒤 head/정렬/크기/32바이트 문맥/클립 플래그를 쓴다.
    std::uint32_t Construct(std::uint32_t token,std::int32_t width,std::int32_t height,
        std::uint32_t font,std::uint32_t color) const;
    // 004bb240/CD 004b73f0: 현재 소유 체인을 scalar 삭제하고 head를 0으로 쓴다.
    void Clear(std::uint32_t token) const;
    // 004bb250/CD 004b7440: -1만 NUL 탐색이다. 길이 0은 조각을 만들지 않으며 나머지는 명시 길이다.
    void AppendText(std::uint32_t token,std::span<const char> bytes,const std::array<std::uint32_t,8>& context,
        std::int32_t length=-1) const;
    // 004bb310/CD 004b75c0: 전체 조각 측정 뒤 정수 여백을 더하고 저장 전 정수 값으로 배율 크기를 계산한다.
    void Fit(std::uint32_t token,std::int32_t padWidth=0,std::int32_t padHeight=0) const;
    // 004bb380/CD 004b76c0: 빈 head는 조기 반환한다. 독립 내부 클립→현재 위치→현재 head 출력 순서다.
    void Draw(std::uint32_t token) const;
    // 004bc910/CD 004b7e90: 파생 가상 표→Clear 두 번→기본 scalar 소멸 순서다. 반환은 입력 토큰이다.
    std::uint32_t DeleteScalar(std::uint32_t token,std::uint32_t flags) const;
private:
    // null/미등록/서로 다른 기본·파생 저장소를 호스트 오류로 거부한다.
    TextGumpNode& Node(std::uint32_t token) const;
    // create_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& create_;
    // state_: 실제 생성자가 signed로 확장하는 정렬 BYTE 전역이다. 기본값은 두 배포 PE의 초기값이며 호출자가 변경할 수 있다 (TextGumpState).
    TextGumpState& state_;
    // factory_: 타입에 맞는 객체를 생성하는 처리기.
    const GumpCreate& factory_;
    // lifetime_: 등록된 객체의 삭제·해제를 처리하는 수명 관리자.
    const GumpLifetime& lifetime_;
    // collection_: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (GumpCollection).
    const GumpCollection& collection_;
    // clip_: 출력을 제한하는 클리핑 영역 또는 상태.
    GumpDrawState& clip_;
    // style_: 글꼴·색·정렬을 선택하는 텍스트 출력 문맥.
    const StyleText& style_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (TextGumpHooks).
    TextGumpHooks hooks_;
};
// 원본 다중 줄 TextGump의 Slider/버퍼/소유·표시·입력 필드다. 생성/수명/입력은 ScrollingTextGumpWidgets가 맡는다.
struct ScrollingTextGumpNode: GumpCreateNode {
    // slider: 스크롤바.
    // buffer: 읽기·쓰기·출력에 재사용하는 버퍼.
    std::uint32_t slider{},buffer{};
    // scroll: 스크롤.
    // scrollRequest: 스크롤 요청.
    // bottomPadding: 아래 여백.
    std::int32_t scroll{},scrollRequest{-1},bottomPadding{};
    // textFlags: 텍스트 플래그.
    std::uint32_t textFlags{};
    // +a8 소유 여부/+ac 미작성/+b0..b4 보조/+c0 마지막 배치 폭/+c4 이전 drag y/+c8 태그/+d4 콜백 토큰이다.
    std::uint32_t ownsBuffer{},unwritten{};
    // reserved: 예약영역.
    std::array<std::uint32_t,2> reserved{};
    // layoutWidth: 배치 너비.
    float layoutWidth{};
    // dragY: 드래그 Y.
    std::int32_t dragY{};
    // metadata: 메타정보.
    // callback: 콜백.
    std::uint32_t metadata{},callback{};
};
// 같은 StyleTextBuffer 생성에 사용한 상태와 처리기를 묶는다. 소유권은 원래 생성 계층에 있다.
struct TextGumpBufferBinding { /* state: +0 max/+4 완료 줄 수/+8 전체 칸/+c 센티널/+24 표시 줄 수/+28 미사용/+2c 변경/+30 줄바꿈 금지다 (StyleTextBufferState). */ StyleTextBufferState* state{};/* handler: 외부 이벤트·프로세스 동작을 실제 코드에 전달하는 처리기. */ const StyleTextBuffer* handler{}; };
// 스크롤바의 range/page/position 가상 경계다. position 쓰기 뒤 현재 slider를 다시 읽어 값을 얻는다.
// 조회는 안정된 저장소를 반환해야 하며 출력 도중 현재 표시 줄/조각을 반납하거나 buffer를 교체하면 안 된다.
struct ScrollingTextGumpHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<ScrollingTextGumpNode*(std::uint32_t)> node;
    // baseNode: 파생 노드와 공유하는 기본 Gump 노드를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> baseNode;
    // buffer: 토큰에 연결된 텍스트 버퍼와 상태를 조회하는 콜백.
    std::function<TextGumpBufferBinding(std::uint32_t)> buffer;
    // sliderRange: 스크롤바의 최소·최대 범위를 설정하는 콜백.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> sliderRange;
    // sliderPage: 스크롤바의 페이지 크기를 설정하는 콜백.
    // sliderSetPosition: 스크롤바의 현재 위치를 설정하는 콜백.
    std::function<void(std::uint32_t,std::int32_t)> sliderPage,sliderSetPosition;
    // sliderPosition: 스크롤바의 현재 위치를 조회하는 콜백.
    std::function<std::int32_t(std::uint32_t)> sliderPosition;
    // redraw: 대상의 화면 갱신을 요청하는 콜백.
    std::function<void(std::uint32_t,bool)> redraw;
};
// 다중 줄 TextGump 표시/스크롤 처리기다. 이름은 호스트 구분이며 원본 클래스 이름을 확정한 것은 아니다.
class ScrollingTextGump {
public:
    // 조각 표는 style/모든 연결 버퍼와 같아야 한다. 스크롤바 가상 경계는 실제 도달 시 검사한다.
    ScrollingTextGump(GumpCreateState& create,const GumpCollection& collection,GumpDrawState& clip,
        StyleTextState& fragments,const StyleText& style,ScrollingTextGumpHooks hooks);
    // 004ba630/CD 004b6940: 두 dirty 조건 중 하나가 참일 때만 플래그를 먼저 비우고 현재 폭으로 Layout한다.
    void EnsureLayout(std::uint32_t token) const;
    // 004b9f20/CD 004b5f50: 강제 Layout 뒤 전체 표시 높이+1을 기본/배율 높이에 저장한다.
    void FitHeight(std::uint32_t token) const;
    // 004b9fb0/CD 004b5fa0: 임시 폭 10000으로 배치한 뒤 최대 폭+여백+선택적 slider 폭+1로 맞춘다.
    void FitContent(std::uint32_t token,std::int32_t padWidth=0,std::int32_t padHeight=0) const;
    // 004ba0f0의 명시 문맥 입력이다. 버퍼 AppendText→현재 textFlags|1→가상 Redraw(false) 순서다.
    void AppendText(std::uint32_t token,std::span<const char> bytes,const std::array<std::uint32_t,8>& context,
        std::int32_t length=-1,std::uint32_t metadata=0) const;
    // 004ba090: 현재 기본 32바이트 문맥의 복사본을 명시 문맥 입력에 넘긴다.
    void AppendText(std::uint32_t token,std::span<const char> bytes,std::int32_t length=-1,std::uint32_t metadata=0) const;
    // 004ba130/CD 004b6140: 현재 버퍼에 그림을 추가하고 dirty 비트/Redraw(false)를 문자열과 같은 순서로 쓴다.
    void AppendPicture(std::uint32_t token,const Markup& pictures,std::uint32_t type,std::uint32_t frame,
        std::uint32_t metadata=0,std::int32_t fitWidth=0,std::int32_t fitHeight=0) const;
    // 004bc250/CD 004b6050: 기본 문맥을 먼저 복사하고 완료 입력 줄만 제거한 뒤 일반 문자열을 추가한다.
    // 미완료 끝 줄·기존 표시 복제·현재 태그/스크롤 요청은 보존하며 스타일 명령을 해석하지 않는다.
    void ReplaceText(std::uint32_t token,std::span<const char> bytes,std::uint32_t metadata=0) const;
    // 004ba270/CD 004b6250: 현재 높이/내용으로 제한한 뒤 slider range→page→position→현재 값 재조회를 한다.
    void SetScroll(std::uint32_t token,std::int32_t position) const;
    // 004ba310/CD 004b62e0: 내용 높이-기본 높이를 CRT 절삭한다. 원본은 전달된 extra 인자를 읽지 않는다.
    void ScrollBottom(std::uint32_t token,std::int32_t extra=0) const;
    // 004ba360/CD 004b6330: 요청을 저장하고 Redraw(false)를 호출한다. 적용은 ProcessRequest에서 한다.
    void RequestScroll(std::uint32_t token,std::int32_t position) const;
    // 004ba380/CD 004b6350: 현재 scroll+delta를 DWORD로 더하고 음수이면 요청 0으로 제한한다.
    void RequestRelativeScroll(std::uint32_t token,std::int32_t delta) const;
    // 004ba3b0/3d0/CD 004b6370/6390: -2 요청/추가 여백을 저장한 뒤 Redraw(false)를 호출한다.
    void RequestBottom(std::uint32_t token,std::int32_t extra=0) const;
    // 004ba690/CD 004b6980: -1은 무동작, -2는 맨 아래, 나머지는 SetScroll 뒤 현재 요청을 -1로 쓴다.
    void ProcessRequest(std::uint32_t token) const;
    // 004ba400/CD 004b63b0: 내용 높이-기본 높이-10 <= scroll의 ordered x87 비교다. NaN은 false다.
    bool AtBottom(std::uint32_t token) const;
    // 004ba450/CD 004b6770: 현재 half-open 경계/표시 줄/조각 폭·높이로 화면 좌표의 조각 토큰을 반환한다.
    // Layout/스크롤 요청을 자동 적용하지 않으며 조각의 위쪽 경계는 원본처럼 엄격히 제외한다.
    std::uint32_t HitTest(std::uint32_t token,ScreenPoint position) const;
    // 004ba6f0/CD 004b69e0: Layout→스크롤→내부 클립→위치→가시 줄의 기준선 두 번 순회 출력이다.
    void Draw(std::uint32_t token) const;
private:
    // 동일 기본/파생 저장소와 살아 있는 연결 버퍼를 실제 사용 시 조회한다.
    ScrollingTextGumpNode& Node(std::uint32_t token) const;
    GumpCreateNode& BaseNode(std::uint32_t token) const;
    TextGumpBufferBinding Buffer(std::uint32_t token) const;
    // 두 dirty 저장의 판본별 순서를 유지한다. Layout 자체는 dirty를 지우지 않는다.
    void ClearDirty(std::uint32_t token) const;
    // slider 사용 플래그일 때만 현재 slider의 기본 폭을 빼고 CD의 float32 중간 저장을 반영한다.
    void Layout(std::uint32_t token) const;
    // 필수 가상 Redraw(false)를 현재 노드 요청 저장 뒤 호출한다.
    void Redraw(std::uint32_t token) const;
    // 저장소의 현재 줄/조각을 조회하며 소멸·미등록 토큰을 거부한다. 줄 0은 센티널이다.
    StyleTextBufferLine& Line(StyleTextBufferState& storage,std::uint32_t line) const;
    StyleTextFragment& Fragment(std::uint32_t fragment) const;
    // y+height가 scroll보다 작은 줄만 건너뛴다. 센티널/순환으로 끝나지 않는 잘못된 목록은 거부한다.
    std::uint32_t FirstVisible(std::uint32_t token,StyleTextBufferState& storage) const;
    // create_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& create_;
    // collection_: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (GumpCollection).
    const GumpCollection& collection_;
    // clip_: 출력을 제한하는 클리핑 영역 또는 상태.
    GumpDrawState& clip_;
    // fragments_: 0은 null이며 map의 주소는 삽입 후에도 안정된다. nextToken=0은 자동 토큰 발급 소진 상태다 (StyleTextState).
    StyleTextState& fragments_;
    // style_: 글꼴·색·정렬을 선택하는 텍스트 출력 문맥.
    const StyleText& style_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (ScrollingTextGumpHooks).
    ScrollingTextGumpHooks hooks_;
};
// 같은 kind 2 가상 Draw에 한 줄/다중 줄 처리를 연결한다. 나머지는 이전 분배기를 호출한다.
void BindTextGumpDraw(GumpDrawHooks& hooks,std::function<const TextGump*(std::uint32_t)> single,
    std::function<const ScrollingTextGump*(std::uint32_t)> scrolling);
// 한 줄 파생 scalar 소멸자만 같은 GumpLifetime에 연결하고 이전 분배를 보존한다.
void BindTextGumpLifetime(GumpLifetimeHooks& hooks,std::function<const TextGump*(std::uint32_t)> resolve);
}
