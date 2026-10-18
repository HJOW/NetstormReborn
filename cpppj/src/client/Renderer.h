// 원본 Renderer.cpp의 정렬·변경 사각형·창 모드 그리기 기반. GameWorld가 실제 지형/객체 목록을 제출한다.
#pragma once
#include "client/BitmapFont.h"
#include <functional>
#include <map>
#include <optional>
#include <tuple>

namespace netstorm::client {
// VFX 색 변환표. 소스 색 치환과 그림자의 대상 화면 색 치환에 각각 사용한다.
using ColorMap = std::array<std::uint8_t, 256>;
// 원본 16바이트 draw entry의 x·y·signed short 깊이(나머지는 객체 포인터와 종류).
struct DrawOrder { /* x: X 좌표. */ /* y: Y 좌표. */ float x{}, y{}; /* depth: 출력이나 이벤트 후보의 깊이 값. */ std::int16_t depth{}; };
// 00497900 ↔ CD 00458240: 깊이 내림차순, y·x 오름차순. 같은 좌표도 원본은 1을 반환한다.
int CompareDrawOrder(DrawOrder a, DrawOrder b);
// 다시 그릴 화면 영역과 그 영역의 출력 플래그이다.
struct DirtyRegion { /* rect: 출력·적중·배치 검사에 사용할 사각형. */ ScreenRect rect; /* flags: 동작을 선택하는 플래그 비트. */ std::uint32_t flags{}; };
// 부분 갱신 사각형을 수집하고 화면 경계로 제한하는 관리자이다.
class DirtyRegions {
public:
    // 원본 화면 경계와 변경 사각형 표를 초기화한다.
    DirtyRegions(int width, int height);
    // 00497580 ↔ CD 00457320: 합친 면적의 1/8 허용치와 플래그 1/4를 보존한다.
    void Add(ScreenRect rect, std::uint32_t flags = 0);
    // 원본 100항목 넘침과 화면 모드 변경은 전체 다시 그리기로 이어진다.
    void InvalidateAll();
    // 00499fe0의 출력 뒤 표 초기화. 화면 크기는 유지한다.
    void Clear();
    // 검증과 그리기 단계에서 원본 순서의 변경 표를 읽는다.
    std::span<const DirtyRegion> Entries() const;
    // 전체 화면 변화나 100항목 넘침이 등록됐는지 읽는다.
    bool FullRedraw() const;
private:
    // width_: 가로 크기.
    // height_: 세로 크기.
    int width_, height_;
    // full_: 전체 여부.
    bool full_{};
    // entries_: 현재 등록하거나 읽은 항목 목록.
    std::vector<DirtyRegion> entries_;
};
// 원본 kind3 입자처럼 SHP 없이 현재 표면에 직접 그리는 한 정렬 항목이다.
struct RenderProcedure {
    ScreenRect bounds; // 같은 시점의 현재 표시 영역이다. 실제 그리기 클립은 renderer의 dirty 사각형이다.
    std::function<void(std::span<std::uint8_t>,int,int,ScreenRect)> draw; // pixels/pitch/height/clip의 실제 표면 소비자다.
};
// 출력할 스프라이트 프레임·좌표·클립·정렬·색 변환 정보이다.
struct RenderSprite {
    // 자산과 글꼴·커서 이미지는 Renderer보다 오래 살아야 한다.
    const ShapeDatabase* database{};
    // block: 셰이프 DB에서 그림을 찾는 블록 번호.
    // frame: 타입에서 사용할 그림 프레임 번호.
    std::size_t block{}, frame{};
    // x: X 좌표.
    // y: Y 좌표.
    int x{}, y{};
    // clip: 출력을 제한하는 클리핑 영역 또는 상태.
    ScreenRect clip{};
    // order: 출력 순서를 비교하는 깊이·좌표 키.
    DrawOrder order{};
    // colors: 팔레트 색 또는 색 인덱스 변환 자료.
    std::optional<ColorMap> colors;
    // shadow: 그림자 여부.
    bool shadow{};
    // 같은 객체 Draw 안에서 본 프레임 뒤에 순서대로 그릴 그림이다. 정렬은 객체당 한 번만 한다.
    std::vector<std::size_t> followingFrames;
    std::optional<RenderProcedure> procedure; // SHP 대신 같은 정렬 위치에서 실행할 실제 직접 그리기다. 소유자는 Draw 동안 살아 있어야 한다.
};
// 004a3f70의 글자 색·그림자·네 방향 외곽선 문맥. 문자열은 원본 코드 페이지 바이트다.
struct RenderText {
    // font: 문자 출력에 사용할 글꼴 또는 글꼴 핸들.
    const BitmapFont* font{};
    // bytes: 필드 접근이나 파일 읽기에 사용하는 원시 바이트.
    std::string bytes;
    // x: X 좌표.
    // y: Y 좌표.
    int x{}, y{};
    // color: 그림·글자 출력에 사용할 팔레트 색 인덱스 또는 색 상태.
    // shadowColor: 그림자에 적용할 팔레트 색 인덱스.
    std::uint8_t color{255}, shadowColor{};
    // shadowOffset: 본문 위치에서 그림자를 이동할 픽셀 오프셋.
    ScreenPoint shadowOffset{};
    // shadow: 그림자 여부.
    // outline: 윤곽 여부.
    bool shadow{}, outline{};
    // TextGump 등 파생 글자의 내부 클립이다. 비어 있으면 기존 화면 클립만 적용한다.
    std::optional<ScreenRect> clip;
};
// 수집한 스프라이트·텍스트·배경을 갱신 영역에 그리는 출력 관리자이다.
class Renderer {
public:
    // 원본 창 모드의 8비트 화면 크기/정렬 판본으로 만들고 첫 화면을 전체 갱신한다. 기본은 10.78이다.
    Renderer(int width, int height,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // 현재 장면을 바꾼다. 기존/새 사각형을 모두 무효화하여 삭제된 그림도 지운다.
    void SetScene(std::vector<RenderSprite> sprites, std::vector<RenderText> text = {}, bool sort = true);
    // 원본 미리보기처럼 dirty마다 수집하는 표시를 정적 월드 뒤에 병합해 같은 원본 키로 정렬한다.
    void SetExtraCollection(std::function<std::vector<RenderSprite>(ScreenRect,std::uint32_t)> collect);
    bool HasExtraCollection() const; // 장치 교체 뒤 실제 수집기 연결 여부를 확인한다.
    // 메뉴가 합성한 화면 크기의 8비트 배경을 소유하여 프레임 사이의 자산 수명을 보장한다.
    void SetBackground(std::shared_ptr<const IndexedImage> image);
    // 선택 표시처럼 스프라이트 위에 오는 투명 UI 픽셀을 소유한다.
    void SetOverlay(std::shared_ptr<const IndexedImage> image);
    // 현재 메뉴 Gump를 월드/선택 뒤, 소프트웨어 커서 앞의 같은 dirty clip에 직접 그린다.
    void SetOverlayProcedure(std::optional<RenderProcedure> procedure);
    // 정지한 미션 위에 대화상자를 합성하는 배경을 만든다. 변경 표·출력 상태·커서는 건드리지 않는다.
    IndexedImage SceneImage();
    // 배경이나 WM_PAINT 등 화면 전체 변화.
    void InvalidateAll();
    // 외부 오브젝트의 변화 범위를 원본 변경 표에 추가한다.
    void Invalidate(ScreenRect rect, std::uint32_t flags = 0);
    // raw Squid 표시가 원본 전체 갱신 전역처럼 변경 표 추가를 생략할지 확인한다.
    bool FullRedrawPending() const;
    // 소프트웨어 커서의 이전·새 범위를 모두 무효화한다. nullptr이면 표시를 지운다.
    void SetSoftwareCursor(const IndexedImage* image, ScreenPoint position);
    // 변경 영역만 합성한다. 반환 사각형은 Present 이전까지 유효하다.
    std::span<const ScreenRect> Draw(std::span<std::uint8_t> pixels, int pitch, std::uint8_t background = 0);
    // 00499fe0: 그린 사각형만 내보내고 변경 표를 비운다. 콜백은 Screen::Update에 연결한다.
    void Present(const std::function<void(ScreenRect)>& update);
    // 실제 그리기·출력 여부를 스모크에서 검사할 누적 횟수.
    std::uint64_t DrawCount() const;
    std::uint64_t PresentCount() const;
private:
    // 프레임을 한 번만 압축 해제하고 이후 그리기에 재사용한다.
    const IndexedImage& Image(const ShapeDatabase& database, std::size_t block, std::size_t frame);
    // 원본 기준점과 VFX 상대 사각형에서 화면상의 그림 범위를 구한다.
    ScreenRect Bounds(const RenderSprite& sprite) const;
    // 본 그림과 후속 그림을 하나의 정렬 항목 안에서 동일 위치/색/클립으로 그린다.
    void DrawSprite(std::span<std::uint8_t> pixels,int pitch,ScreenRect clip,const RenderSprite& sprite);
    // 현재 변경 영역 안에 바이트 글자를 원본 전진 폭으로 그린다.
    void Text(std::span<std::uint8_t> pixels, int pitch, ScreenRect clip, const RenderText& text);
    // width_: 가로 크기.
    // height_: 세로 크기.
    int width_, height_;
    o::OriginalEdition edition_; // 실제 PE마다 다른 CRT 표시 목록 정렬을 선택한다.
    // dirty_: 부분 갱신 사각형을 수집하고 화면 경계로 제한하는 관리자 (DirtyRegions).
    DirtyRegions dirty_;
    // sprites_: 스프라이트.
    std::vector<RenderSprite> sprites_;
    std::vector<RenderSprite> sourceSprites_; // 추가 수집을 병합할 때 정렬 전의 원본 제출 순서를 유지한다.
    std::function<std::vector<RenderSprite>(ScreenRect,std::uint32_t)> extraCollection_; // 현재 dirty마다 원본 추가 수집을 호출한다.
    std::vector<RenderSprite> extraCache_; // 정지 장면 합성은 새 SP 수집 없이 마지막 표시 목록을 사용한다.
    // text_: 표시하거나 해석할 본문 문자열.
    std::vector<RenderText> text_;
    // background_: 배경 출력 자료 또는 배경 Gump 토큰.
    std::shared_ptr<const IndexedImage> background_;
    // overlay_: 화면 위 계층의 출력·입력 대상.
    std::shared_ptr<const IndexedImage> overlay_;
    std::optional<RenderProcedure> overlayProcedure_; // 현재 실제 UI 트리의 픽셀 소비자이며 별도 장면 사본을 만들지 않는다.
    // painted_: 출력된.
    std::vector<ScreenRect> painted_;
    // paintFlags_: 출력 플래그.
    std::vector<std::uint32_t> paintFlags_;
    // cursor_: 팔레트 인덱스 픽셀로 구성한 그림 (IndexedImage).
    const IndexedImage* cursor_{};
    // cursorPosition_: 원본 004a4fe0의 창 위치 계산(설정 `initWindowPos`): 1 왼쪽 위, 2 오른쪽 위, 3 오른쪽 아래, 4 왼쪽 아래, 5 가운데 (ScreenPoint).
    ScreenPoint cursorPosition_{};
    // images_: 프레임이나 커서 번호별로 읽어 둔 그림 자료.
    std::map<std::tuple<const ShapeDatabase*, std::size_t, std::size_t>, IndexedImage> images_;
    // draws_: 그리기.
    // presents_: 화면전송.
    std::uint64_t draws_{}, presents_{};
};
// 압축 해제한 이미지의 불투명 픽셀만 합성한다. 그림자는 소스 색 대신 대상 화면 색을 변환한다.
void DrawIndexedImage(std::span<std::uint8_t> pixels, int pitch, int height, const IndexedImage& image,
    int left, int top, ScreenRect clip, const ColorMap* colors = nullptr, bool shadow = false);
}
