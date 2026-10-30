// 원본 00497580·00497900·00499fe0. 외부 목록 수집/구름/카메라 복사는 원본 Squid·지형 복원 뒤 확장한다.
#include "client/Renderer.h"
#include "client/DrawSort.h"
#include "client/GumpTextDraw.h"
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
Renderer::Renderer(int width, int height,o::OriginalEdition edition) : width_(width), height_(height),edition_(edition), dirty_(width, height) { dirty_.InvalidateAll(); }
// 같은 객체가 제출한 모든 그림의 합집합을 무효화한다. 특수 프레임은 그림 범위가 없다.
ScreenRect Renderer::Bounds(const RenderSprite& sprite) const {
    if (sprite.procedure) {
        if (sprite.database || !sprite.procedure->draw) throw std::invalid_argument("직접 표시 항목의 실제 출력 경계 오류");
        return ClipScreenRect(sprite.procedure->bounds,sprite.clip);
    }
    if (!sprite.database) throw std::invalid_argument("Null sprite database");
    if (sprite.block >= sprite.database->Blocks().size()) throw std::out_of_range("Invalid sprite block");
    ScreenRect bounds{};bool present=false;
    // 각 그림의 압축 영역을 같은 화면 기준점에 옮긴다.
    const auto include=[&](std::size_t number) {
        const auto& frame=sprite.database->Blocks()[sprite.block].frames.at(number);if (frame.IsSpecial()) return;
        const auto rect=ClipScreenRect({Position(sprite.x,frame.rect.left),Position(sprite.y,frame.rect.top),
            Position(sprite.x,frame.rect.right,1),Position(sprite.y,frame.rect.bottom,1)},sprite.clip);
        if (!Nonempty(rect)) return;
        bounds=present ? ScreenRect{std::min(bounds.left,rect.left),std::min(bounds.top,rect.top),std::max(bounds.right,rect.right),std::max(bounds.bottom,rect.bottom)} : rect;present=true;
    };
    include(sprite.frame);
    // 레이어를 별도 정렬하지 않고 전체 출력 영역에만 포함한다.
    for (const auto number:sprite.followingFrames) include(number);
    return bounds;
}
// 부분 갱신과 SceneImage 모두 같은 객체 내부 합성 순서를 사용한다.
void Renderer::DrawSprite(std::span<std::uint8_t> pixels,int pitch,ScreenRect clip,const RenderSprite& sprite) {
    const auto cut=ClipScreenRect(clip,sprite.clip);
    if (sprite.procedure) { sprite.procedure->draw(pixels,pitch,height_,cut);return; }
    // 압축 영역의 투명도와 색 치환 규칙은 기존 한 장 그리기를 그대로 사용한다.
    const auto draw=[&](std::size_t number) {
        const auto& frame=sprite.database->Blocks()[sprite.block].frames.at(number);if (frame.IsSpecial()) return;
        DrawIndexedImage(pixels,pitch,height_,Image(*sprite.database,sprite.block,number),Position(sprite.x,frame.rect.left),Position(sprite.y,frame.rect.top),cut,sprite.colors ? &*sprite.colors : nullptr,sprite.shadow);
    };
    draw(sprite.frame);
    // 0번 기반 그림 뒤 현재 물결과 상태별 그림을 원본 호출 순서로 덮는다.
    for (const auto number:sprite.followingFrames) draw(number);
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
    sourceSprites_=sprites; // dirty별 재정렬도 같은 원본 입력 순서에서 시작한다.
    if (sort) SortRenderSprites(sprites,edition_);
    // 글자 이동·삭제는 현재 전체를 무효화한다. 후속 Gump가 전용 변경 범위를 등록한다.
    if (!text_.empty() || !text.empty()) dirty_.InvalidateAll();
    sprites_ = std::move(sprites); text_ = std::move(text);
}
// 화면 모드 변경·전체 다시 그리기.
void Renderer::InvalidateAll() { dirty_.InvalidateAll(); }
// 원문 force redraw가 기존100칸을 비운 뒤 전체 사각형을 추가할 때 같은 표를 비운다.
void Renderer::ClearDirty() { dirty_.Clear(); }
// 수집기 교체/해제는 이전 캐시도 비워 반납된 월드 callback을 남기지 않는다.
void Renderer::SetExtraCollection(std::function<std::vector<RenderSprite>(ScreenRect,std::uint32_t)> collect) { extraCollection_=std::move(collect);extraCache_.clear();dirty_.InvalidateAll(); }
bool Renderer::HasExtraCollection() const { return static_cast<bool>(extraCollection_); } // 새 장치에 실제 수집기가 있는지 조회한다.
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
// 실제 메뉴는 자신의 현재 상태를 그리며 장치 변경/닫힘도 다음 실제 표면에 반영한다.
void Renderer::SetOverlayProcedure(std::optional<RenderProcedure> procedure) {
    if (procedure && !procedure->draw) throw std::invalid_argument("현재 UI 표면 출력 경계 누락");
    if (!procedure && !overlayProcedure_) return;
    overlayProcedure_=std::move(procedure);dirty_.InvalidateAll();
}
// 정지한 월드 위에 대화상자를 합성할 배경·스프라이트·선택 표시를 별도 버퍼에 그린다.
IndexedImage Renderer::SceneImage() {
    IndexedImage result{static_cast<std::size_t>(width_), static_cast<std::size_t>(height_),
        std::vector<std::uint8_t>(static_cast<std::size_t>(width_) * height_), std::vector<std::uint8_t>(static_cast<std::size_t>(width_) * height_,255)};
    const ScreenRect all{0,0,width_,height_};
    if (background_) DrawIndexedImage(result.indices,width_,height_,*background_,0,0,all);
    // 추가 수집의 SP/피드백을 다시 실행하지 않고 마지막 표시 목록만 같은 정렬로 합성한다.
    auto combined=extraCache_.empty() ? sprites_ : sourceSprites_;combined.insert(combined.end(),extraCache_.begin(),extraCache_.end());if (!extraCache_.empty()) SortRenderSprites(combined,edition_);
    // Renderer 장면 순서를 보존하며 메뉴 글자와 하드웨어 커서는 합성하지 않는다.
    for (const auto& sprite : combined) {
        const auto cut = ClipScreenRect(all, sprite.clip);
        if (!Nonempty(ClipScreenRect(Bounds(sprite), cut))) continue;
        DrawSprite(result.indices,width_,cut,sprite);
    }
    if (overlay_) DrawIndexedImage(result.indices,width_,height_,*overlay_,0,0,all);
    return result;
}
// 오브젝트 측의 변경 알림.
void Renderer::Invalidate(ScreenRect rect, std::uint32_t flags) { dirty_.Add(rect, flags); }
// 표시 억제/변경 표 넘침에 대응하는 현재 전체 갱신 상태다.
bool Renderer::FullRedrawPending() const { return dirty_.FullRedraw(); }
// 커서가 움직이지 않았으면 새 변경 영역을 만들지 않는다.
void Renderer::SetSoftwareCursor(const IndexedImage* image, ScreenPoint position,bool force) {
    if (!force && cursor_ == image && position.x == cursorPosition_.x && position.y == cursorPosition_.y) return;
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
// 기존 메뉴도 raw Gump와 같은 실제 글리프 반복/출력 순서를 사용한다. 해제 이미지 캐시는 유지한다.
void Renderer::Text(std::span<std::uint8_t> pixels, int pitch, ScreenRect clip, const RenderText& text) {
    if (text.clip) clip=ClipScreenRect(clip,*text.clip);
    if (!text.font) throw std::invalid_argument("Null bitmap font");
    if (text.bytes.size()>static_cast<std::size_t>(INT_MAX)) throw std::invalid_argument("Renderer 글자 길이 범위 오류");
    ColorMap map{};GumpGlyphHooks hooks;
    hooks.color=[&map](std::uint8_t color) { map[100]=color; };
    // VFX 프레임의 상대 그림 영역과 원본 DWORD 감김을 유지하고 기존 압축 해제 캐시를 재사용한다.
    hooks.glyph=[&](const BitmapFont& font,std::uint8_t code,ScreenPoint at) {
        const auto& shape=*font.Glyph(code).shape;const auto& frame=shape.Blocks()[0].frames[0];
        // 프레임 상대 좌표도 원본 x86 덧셈처럼 하위 DWORD로 만든다.
        const auto add=[](int a,int b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); };
        if (!frame.IsSpecial()) DrawIndexedImage(pixels,pitch,height_,Image(shape,0,0),add(at.x,frame.rect.left),add(at.y,frame.rect.top),clip,&map);
    };
    const GumpTextContext context{(text.shadow ? 2U : 0U)|(text.outline ? 4U : 0U),0,text.color,0,0,
        text.shadowColor,text.shadowOffset.x,text.shadowOffset.y};
    DrawBitmapGlyphText(text.bytes,text.font,{text.x,text.y},context,hooks,static_cast<std::int32_t>(text.bytes.size()));
}
// 변경 영역마다 배경→깊이 순 스프라이트→UI 글자 순으로 합성한다.
std::span<const ScreenRect> Renderer::Draw(std::span<std::uint8_t> pixels, int pitch, std::uint8_t background,const RendererWorldDrawing* world) {
    if (!painted_.empty()) throw std::logic_error("Present must follow Draw");
    if (pitch < width_ || pixels.size() < static_cast<std::size_t>(pitch) * static_cast<std::size_t>(height_)) throw std::invalid_argument("Invalid renderer surface");
    if (cameraRedraw_) { dirty_.InvalidateAll();cameraRedraw_=0; } // 원문 카메라 적용은 현재 Present 뒤에1을 쓰고 다음 Draw에서 전체를 다시 구성한다.
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
        if (world && (!world->collect || !world->background)) throw std::logic_error("Native Renderer의 현재 지형/배경 장치 누락");
        const auto covered=world ? world->collect(clip,paintFlags_[i]) : 0U; //실제 지형 수집 뒤 커서 kind4 미리보기를 수집한다. 별도487f10 건설 표시와 구별한다.
        if (!world) {
            // 독립 표시/10.72의 기존 합성은 추가 수집보다 먼저 배경을 복원한다.
            if ((paintFlags_[i]&kOpaque)==0) for (int y=clip.top;y<clip.bottom;++y) { //클립된 실제 행만 지운다.
                const auto row=static_cast<std::size_t>(y)*static_cast<std::size_t>(pitch);
                std::fill(pixels.begin()+row+clip.left,pixels.begin()+row+clip.right,background);
            }
            if (background_) DrawIndexedImage(pixels,pitch,height_,*background_,0,0,clip);
        }
        auto combined=extraCollection_ ? sourceSprites_ : sprites_; // 정적 월드와 dirty별 미리보기를 같은 원본 작업 순서로 병합한다.
        if (extraCollection_) {
            auto extra=extraCollection_(clip,paintFlags_[i]);if (!(paintFlags_[i]&4U)) extraCache_=extra;
            combined.insert(combined.end(),std::make_move_iterator(extra.begin()),std::make_move_iterator(extra.end()));SortRenderSprites(combined,edition_);
        }
        if (world) {
            world->background(clip,paintFlags_[i],covered);
            if (paintFlags_[i]&kOpaque) combined.clear(); //UI 전용4의 원문은 월드/미리보기 표시를 제출하지 않는다.
        }
        // 원본 깊이 키로 정렬한 모든 월드/미리보기 항목을 현재 dirty 안에서 출력한다.
        for (const auto& sprite : combined) {
            const auto bounds = Bounds(sprite); const auto cut = ClipScreenRect(clip, sprite.clip);
            if (!Nonempty(ClipScreenRect(bounds, cut))) continue;
            DrawSprite(pixels,pitch,cut,sprite);
        }
        if (overlay_ && (!world || !(paintFlags_[i]&kOpaque))) DrawIndexedImage(pixels,pitch,height_,*overlay_,0,0,clip);
        if (overlayProcedure_) {
            const auto cut=ClipScreenRect(clip,overlayProcedure_->bounds); // 현재 dirty 범위 밖의 세계 픽셀을 건드리지 않는다.
            if (Nonempty(cut)) overlayProcedure_->draw(pixels,pitch,height_,cut);
        }
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
    painted_.clear(); paintFlags_.clear(); dirty_.Clear();backgroundRedraw_=0;
}
// 변경 영역을 그린 프레임 수.
std::uint64_t Renderer::DrawCount() const { return draws_; }
// 내보낸 사각형 수.
std::uint64_t Renderer::PresentCount() const { return presents_; }
}
