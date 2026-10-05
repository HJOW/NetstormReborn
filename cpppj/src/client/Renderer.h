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
struct DrawOrder { float x{}, y{}; std::int16_t depth{}; };
// 00497900 ↔ CD 00458240: 깊이 내림차순, y·x 오름차순. 같은 좌표도 원본은 1을 반환한다.
int CompareDrawOrder(DrawOrder a, DrawOrder b);
struct DirtyRegion { ScreenRect rect; std::uint32_t flags{}; };
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
    int width_, height_;
    bool full_{};
    std::vector<DirtyRegion> entries_;
};
struct RenderSprite {
    // 자산과 글꼴·커서 이미지는 Renderer보다 오래 살아야 한다.
    const ShapeDatabase* database{};
    std::size_t block{}, frame{};
    int x{}, y{};
    ScreenRect clip{};
    DrawOrder order{};
    std::optional<ColorMap> colors;
    bool shadow{};
};
// 004a3f70의 글자 색·그림자·네 방향 외곽선 문맥. 문자열은 원본 코드 페이지 바이트다.
struct RenderText {
    const BitmapFont* font{};
    std::string bytes;
    int x{}, y{};
    std::uint8_t color{255}, shadowColor{};
    ScreenPoint shadowOffset{};
    bool shadow{}, outline{};
};
class Renderer {
public:
    // 원본 창 모드의 8비트 화면 크기로 만들고 첫 화면을 전체 갱신한다.
    Renderer(int width, int height);
    // 현재 장면을 바꾼다. 기존/새 사각형을 모두 무효화하여 삭제된 그림도 지운다.
    void SetScene(std::vector<RenderSprite> sprites, std::vector<RenderText> text = {}, bool sort = true);
    // 메뉴가 합성한 화면 크기의 8비트 배경을 소유하여 프레임 사이의 자산 수명을 보장한다.
    void SetBackground(std::shared_ptr<const IndexedImage> image);
    // 선택 표시처럼 스프라이트 위에 오는 투명 UI 픽셀을 소유한다.
    void SetOverlay(std::shared_ptr<const IndexedImage> image);
    // 정지한 미션 위에 대화상자를 합성하는 배경을 만든다. 변경 표·출력 상태·커서는 건드리지 않는다.
    IndexedImage SceneImage();
    // 배경이나 WM_PAINT 등 화면 전체 변화.
    void InvalidateAll();
    // 외부 오브젝트의 변화 범위를 원본 변경 표에 추가한다.
    void Invalidate(ScreenRect rect, std::uint32_t flags = 0);
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
    // 현재 변경 영역 안에 바이트 글자를 원본 전진 폭으로 그린다.
    void Text(std::span<std::uint8_t> pixels, int pitch, ScreenRect clip, const RenderText& text);
    int width_, height_;
    DirtyRegions dirty_;
    std::vector<RenderSprite> sprites_;
    std::vector<RenderText> text_;
    std::shared_ptr<const IndexedImage> background_;
    std::shared_ptr<const IndexedImage> overlay_;
    std::vector<ScreenRect> painted_;
    std::vector<std::uint32_t> paintFlags_;
    const IndexedImage* cursor_{};
    ScreenPoint cursorPosition_{};
    std::map<std::tuple<const ShapeDatabase*, std::size_t, std::size_t>, IndexedImage> images_;
    std::uint64_t draws_{}, presents_{};
};
// 압축 해제한 이미지의 불투명 픽셀만 합성한다. 그림자는 소스 색 대신 대상 화면 색을 변환한다.
void DrawIndexedImage(std::span<std::uint8_t> pixels, int pitch, int height, const IndexedImage& image,
    int left, int top, ScreenRect clip, const ColorMap* colors = nullptr, bool shadow = false);
}
