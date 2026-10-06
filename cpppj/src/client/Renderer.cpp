// 원본 00497580·00497900·00499fe0. 외부 목록 수집/구름/카메라 복사는 원본 Squid·지형 복원 뒤 확장한다.
#include "client/Renderer.h"
#include <algorithm>
#include <cmath>
#include <climits>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 변경 사각형 표의 최대 개수(this+0x68의 사각형, this+0x6a8의 플래그).
constexpr std::size_t kDirtyCapacity = 100;
// 원본 변경 표 플래그: 무시한 항목, 배경을 다시 지우지 않는 영역.
constexpr std::uint32_t kDiscarded = 1, kOpaque = 4;
// 양 끝을 포함하지 않는 사각형 면적. 복원 API의 큰 좌표에도 부호 곱셈 오버플로를 피한다.
std::int64_t Area(ScreenRect rect) { return (static_cast<std::int64_t>(rect.right) - rect.left) * (static_cast<std::int64_t>(rect.bottom) - rect.top); }
// 두 변경 영역의 둘레를 포함하는 합집합 상자를 구한다.
ScreenRect Union(ScreenRect a, ScreenRect b) {
    return {std::min(a.left, b.left), std::min(a.top, b.top), std::max(a.right, b.right), std::max(a.bottom, b.bottom)};
}
// 교집합의 크기가 양수인지 검사한다.
bool Nonempty(ScreenRect rect) { return rect.left < rect.right && rect.top < rect.bottom; }
// VFX 상대 좌표를 화면 좌표로 옮길 때 범위 밖 정수의 래핑을 거부한다.
int Position(int origin, int offset, int edge = 0) {
    const auto value = static_cast<std::int64_t>(origin) + offset + edge;
    if (value < INT_MIN || value > INT_MAX) throw std::overflow_error("Renderer coordinate overflow");
    return static_cast<int>(value);
}
}
// 원본의 부호 있는 깊이 비교와 float 좌표 차이를 그대로 사용한다.
int CompareDrawOrder(DrawOrder a, DrawOrder b) {
    const int depth = static_cast<int>(b.depth) - static_cast<int>(a.depth);
    if (depth != 0) return depth;
    const float delta = a.y - b.y;
    return (delta == 0 ? a.x - b.x : delta) < 0 ? -1 : 1;
}
// 원본 화면 밖 사각형의 추가 조건에 쓰는 경계다.
DirtyRegions::DirtyRegions(int width, int height) : width_(width), height_(height) {
    if (width <= 0 || height <= 0) throw std::invalid_argument("Invalid dirty region dimensions");
}
// 병합 전에 자르지 않는다. 원본도 Draw/Present 때 화면 경계로 자른다.
void DirtyRegions::Add(ScreenRect rect, std::uint32_t flags) {
    if (!(rect.left < width_ && rect.top < height_ && rect.right > 0 && rect.bottom > 0)) return;
    if (entries_.size() >= kDirtyCapacity) { full_ = true; return; }
    const auto area = Area(rect);
    // 원본은 첫 번째 병합 대상에서 반환하며 뒤 항목과 다시 병합하지 않는다.
    for (auto& entry : entries_) if ((entry.flags & kDiscarded) == 0) {
        const auto oldArea = Area(entry.rect);
        const auto united = Union(entry.rect, rect);
        const auto unitedArea = Area(united);
        if ((flags & kOpaque) != 0 && unitedArea == area) entry.flags |= kDiscarded;
        else {
            if ((entry.flags & kOpaque) != 0 && unitedArea == oldArea) return;
            if (unitedArea - unitedArea / 8 < oldArea + area) { entry.rect = united; return; }
        }
    }
    entries_.push_back({rect, flags & ~kDiscarded});
}
// 넘침·전체 다시 그리기 플래그.
void DirtyRegions::InvalidateAll() { full_ = true; }
// 출력이 끝나면 다음 프레임은 변화가 생길 때까지 비어 있다.
void DirtyRegions::Clear() { full_ = false; entries_.clear(); }
// 원본 순서의 표.
std::span<const DirtyRegion> DirtyRegions::Entries() const { return entries_; }
// 넘침/전체 변화 여부.
bool DirtyRegions::FullRedraw() const { return full_; }

// 양 끝을 포함하지 않는 clip과 이미지의 opacity를 함께 적용한다.
void DrawIndexedImage(std::span<std::uint8_t> pixels, int pitch, int height, const IndexedImage& image,
    int left, int top, ScreenRect clip, const ColorMap* colors, bool shadow) {
    if (pitch <= 0 || height <= 0 || pixels.size() < static_cast<std::size_t>(pitch) * static_cast<std::size_t>(height))
        throw std::invalid_argument("Invalid renderer surface");
    if (image.width > static_cast<std::size_t>(INT_MAX) || image.height > static_cast<std::size_t>(INT_MAX) ||
        image.width * image.height != image.indices.size() || image.opacity.size() != image.indices.size())
        throw std::invalid_argument("Invalid renderer image");
    if (shadow && !colors) throw std::invalid_argument("Shadow requires destination color map");
    clip = ClipScreenRect(clip, {0, 0, pitch, height});
    // 클리핑된 실제 행만 순회한다.
    for (int y = std::max(top, clip.top); static_cast<std::int64_t>(y) < std::min<std::int64_t>(static_cast<std::int64_t>(top) + static_cast<std::int64_t>(image.height), clip.bottom); ++y) {
        // 투명 픽셀에서는 기존 대상 색을 유지한다.
        for (int x = std::max(left, clip.left); static_cast<std::int64_t>(x) < std::min<std::int64_t>(static_cast<std::int64_t>(left) + static_cast<std::int64_t>(image.width), clip.right); ++x) {
            const auto input = static_cast<std::size_t>(static_cast<std::int64_t>(y) - top) * image.width + static_cast<std::size_t>(static_cast<std::int64_t>(x) - left);
            if (image.opacity[input]) {
                auto& output = pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(pitch) + static_cast<std::size_t>(x)];
                output = colors ? (*colors)[shadow ? output : image.indices[input]] : image.indices[input];
            }
        }
    }
}
// 첫 프레임을 원본처럼 전체 화면으로 그린다.
Renderer::Renderer(int width, int height) : width_(width), height_(height), dirty_(width, height) { dirty_.InvalidateAll(); }
// 특수 프레임은 이미지가 아니므로 변경 범위도 갖지 않는다.
ScreenRect Renderer::Bounds(const RenderSprite& sprite) const {
    if (!sprite.database) throw std::invalid_argument("Null sprite database");
    if (sprite.block >= sprite.database->Blocks().size()) throw std::out_of_range("Invalid sprite block");
    const auto& frame = sprite.database->Blocks()[sprite.block].frames.at(sprite.frame);
    if (frame.IsSpecial()) return {};
    return ClipScreenRect({Position(sprite.x, frame.rect.left), Position(sprite.y, frame.rect.top),
        Position(sprite.x, frame.rect.right, 1), Position(sprite.y, frame.rect.bottom, 1)}, sprite.clip);
}
// 이동 전후 그림을 같은 변경 표에 넣어 사라진 그림의 배경도 복구한다.
void Renderer::SetScene(std::vector<RenderSprite> sprites, std::vector<RenderText> text, bool sort) {
    // 이전 스프라이트의 범위.
    for (const auto& sprite : sprites_) dirty_.Add(Bounds(sprite));
    // 새 스프라이트의 범위와 유한 좌표 검사.
    for (const auto& sprite : sprites) {
        if (!std::isfinite(sprite.order.x) || !std::isfinite(sprite.order.y)) throw std::invalid_argument("Nonfinite draw order");
        dirty_.Add(Bounds(sprite));
    }
    if (sort) std::stable_sort(sprites.begin(), sprites.end(), [](const RenderSprite& a, const RenderSprite& b) { return CompareDrawOrder(a.order, b.order) < 0; });
    // 글자 이동·삭제는 현재 전체를 무효화한다. 후속 Gump가 전용 변경 범위를 등록한다.
    if (!text_.empty() || !text.empty()) dirty_.InvalidateAll();
    sprites_ = std::move(sprites); text_ = std::move(text);
}
// 화면 모드 변경·전체 다시 그리기.
void Renderer::InvalidateAll() { dirty_.InvalidateAll(); }
// 배경 팔레트 번호를 변환하지 않고 변경 영역의 합성에서 재사용한다.
void Renderer::SetBackground(std::shared_ptr<const IndexedImage> image) {
    if (image && (image->width != static_cast<std::size_t>(width_) || image->height != static_cast<std::size_t>(height_) ||
        image->indices.size() != image->width * image->height || image->opacity.size() != image->indices.size()))
        throw std::invalid_argument("Invalid renderer background");
    background_ = std::move(image); dirty_.InvalidateAll();
}
// 오브젝트 선택 표시는 UI 글자와 같이 월드 스프라이트 뒤에 합성한다.
void Renderer::SetOverlay(std::shared_ptr<const IndexedImage> image) {
    if (image && (image->width != static_cast<std::size_t>(width_) || image->height != static_cast<std::size_t>(height_) ||
        image->indices.size() != image->width*image->height || image->opacity.size() != image->indices.size()))
        throw std::invalid_argument("Invalid renderer overlay");
    overlay_=std::move(image); dirty_.InvalidateAll();
}
// 정지한 월드 위에 대화상자를 합성할 배경·스프라이트·선택 표시를 별도 버퍼에 그린다.
IndexedImage Renderer::SceneImage() {
    IndexedImage result{static_cast<std::size_t>(width_), static_cast<std::size_t>(height_),
        std::vector<std::uint8_t>(static_cast<std::size_t>(width_) * height_), std::vector<std::uint8_t>(static_cast<std::size_t>(width_) * height_,255)};
    const ScreenRect all{0,0,width_,height_};
    if (background_) DrawIndexedImage(result.indices,width_,height_,*background_,0,0,all);
    // Renderer 장면 순서를 보존하며 메뉴 글자와 커서는 합성하지 않는다.
    for (const auto& sprite : sprites_) {
        const auto cut = ClipScreenRect(all, sprite.clip);
        if (!Nonempty(ClipScreenRect(Bounds(sprite), cut))) continue;
        const auto& frame = sprite.database->Blocks()[sprite.block].frames.at(sprite.frame);
        DrawIndexedImage(result.indices,width_,height_,Image(*sprite.database,sprite.block,sprite.frame),
            Position(sprite.x,frame.rect.left),Position(sprite.y,frame.rect.top),cut,sprite.colors ? &*sprite.colors : nullptr,sprite.shadow);
    }
    if (overlay_) DrawIndexedImage(result.indices,width_,height_,*overlay_,0,0,all);
    return result;
}
// 오브젝트 측의 변경 알림.
void Renderer::Invalidate(ScreenRect rect, std::uint32_t flags) { dirty_.Add(rect, flags); }
// 표시 억제/변경 표 넘침에 대응하는 현재 전체 갱신 상태다.
bool Renderer::FullRedrawPending() const { return dirty_.FullRedraw(); }
// 커서가 움직이지 않았으면 새 변경 영역을 만들지 않는다.
void Renderer::SetSoftwareCursor(const IndexedImage* image, ScreenPoint position) {
    if (cursor_ == image && position.x == cursorPosition_.x && position.y == cursorPosition_.y) return;
    // 이전·새 프레임의 사각형을 각각 등록한다.
    const auto invalidate = [this](const IndexedImage* item, ScreenPoint at) {
        if (item) {
            if (item->width > static_cast<std::size_t>(INT_MAX) || item->height > static_cast<std::size_t>(INT_MAX))
                throw std::invalid_argument("Invalid cursor image dimensions");
            dirty_.Add({at.x, at.y, Position(at.x, static_cast<int>(item->width)), Position(at.y, static_cast<int>(item->height))});
        }
    };
    invalidate(cursor_, cursorPosition_); invalidate(image, position);
    cursor_ = image; cursorPosition_ = position;
}
// 같은 VFX 프레임을 반복해서 압축 해제하지 않는다.
const IndexedImage& Renderer::Image(const ShapeDatabase& database, std::size_t block, std::size_t frame) {
    const auto key = std::make_tuple(&database, block, frame);
    auto found = images_.find(key);
    if (found == images_.end()) found = images_.emplace(key, database.Decode(block, frame)).first;
    return found->second;
}
// 004a3430은 ABC A를 좌표에 더하지 않고 세 번째 표의 B+C로 다음 글자로 진행한다.
void Renderer::Text(std::span<std::uint8_t> pixels, int pitch, ScreenRect clip, const RenderText& text) {
    if (!text.font) throw std::invalid_argument("Null bitmap font");
    // 그림자·외곽선·본문을 한 번씩 그리는 공통 경로.
    const auto draw = [&](int offsetX, int offsetY, std::uint8_t color) {
        ColorMap map{}; map[100] = color;
        int x = Position(text.x, offsetX);
        // 코드 페이지 바이트열을 원본 글꼴 표의 폭으로 진행한다.
        for (const unsigned char c : text.bytes) {
            const auto& glyph = text.font->Glyph(c);
            if (glyph.shape) {
                const auto& frame = glyph.shape->Blocks()[0].frames[0];
                if (!frame.IsSpecial()) DrawIndexedImage(pixels, pitch, height_, Image(*glyph.shape, 0, 0), Position(x, frame.rect.left), Position(Position(text.y, offsetY), frame.rect.top), clip, &map);
                x = Position(x, glyph.drawAdvance);
            }
        }
    };
    if (text.shadow) draw(text.shadowOffset.x, text.shadowOffset.y, text.shadowColor);
    if (text.outline) {
        draw(0, -1, text.shadowColor); draw(0, 1, text.shadowColor); draw(-1, 0, text.shadowColor); draw(1, 0, text.shadowColor);
    }
    draw(0, 0, text.color);
}
// 변경 영역마다 배경→깊이 순 스프라이트→UI 글자 순으로 합성한다.
std::span<const ScreenRect> Renderer::Draw(std::span<std::uint8_t> pixels, int pitch, std::uint8_t background) {
    if (!painted_.empty()) throw std::logic_error("Present must follow Draw");
    if (pitch < width_ || pixels.size() < static_cast<std::size_t>(pitch) * static_cast<std::size_t>(height_)) throw std::invalid_argument("Invalid renderer surface");
    if (dirty_.FullRedraw()) { painted_.push_back({0, 0, width_, height_}); paintFlags_.push_back(0); }
    else {
        // 무시 플래그가 없는 영역만 화면 안으로 자른다.
        for (const auto& entry : dirty_.Entries()) if ((entry.flags & kDiscarded) == 0) {
            const auto rect = ClipScreenRect(entry.rect, {0, 0, width_, height_});
            if (Nonempty(rect)) { painted_.push_back(rect); paintFlags_.push_back(entry.flags); }
        }
    }
    // 각 변경 영역을 같은 장면에서 다시 합성한다.
    for (std::size_t i = 0; i < painted_.size(); ++i) {
        const auto clip = painted_[i];
        // 변경 영역의 배경 행만 지운다.
        if ((paintFlags_[i] & kOpaque) == 0)
            // 원본 플래그 4는 기존 배경을 보존한다.
            for (int y = clip.top; y < clip.bottom; ++y) {
                const auto row = static_cast<std::size_t>(y) * static_cast<std::size_t>(pitch);
                std::fill(pixels.begin() + row + clip.left, pixels.begin() + row + clip.right, background);
            }
        if (background_) DrawIndexedImage(pixels, pitch, height_, *background_, 0, 0, clip);
        // 원본 깊이 키로 정렬한 스프라이트.
        for (const auto& sprite : sprites_) {
            const auto bounds = Bounds(sprite); const auto cut = ClipScreenRect(clip, sprite.clip);
            if (!Nonempty(ClipScreenRect(bounds, cut))) continue;
            const auto& frame = sprite.database->Blocks()[sprite.block].frames.at(sprite.frame);
            DrawIndexedImage(pixels, pitch, height_, Image(*sprite.database, sprite.block, sprite.frame), Position(sprite.x, frame.rect.left), Position(sprite.y, frame.rect.top), cut,
                sprite.colors ? &*sprite.colors : nullptr, sprite.shadow);
        }
        if (overlay_) DrawIndexedImage(pixels,pitch,height_,*overlay_,0,0,clip);
        // UI 글자는 지형 위에 낸다. Gump 깊이 통합은 메뉴 복원 단계에서 진행한다.
        for (const auto& text : text_) Text(pixels, pitch, clip, text);
        if (cursor_) DrawIndexedImage(pixels, pitch, height_, *cursor_, cursorPosition_.x, cursorPosition_.y, clip);
    }
    if (!painted_.empty()) ++draws_;
    return painted_;
}
// 그린 영역만 Win32로 내보내고 성공했을 때 변경 표를 비운다.
void Renderer::Present(const std::function<void(ScreenRect)>& update) {
    // 원본과 같이 표의 순서로 내보낸다.
    for (const auto rect : painted_) { update(rect); ++presents_; }
    painted_.clear(); paintFlags_.clear(); dirty_.Clear();
}
// 변경 영역을 그린 프레임 수.
std::uint64_t Renderer::DrawCount() const { return draws_; }
// 내보낸 사각형 수.
std::uint64_t Renderer::PresentCount() const { return presents_; }
}
