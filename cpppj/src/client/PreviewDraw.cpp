// 00446270 ↔ CD 0043dbd0: VFX 호출 전의 현재 커서 상태/배치 격자/문구 위치를 보존한다.
#include "client/PreviewDraw.h"
#include "client/RawCursorPlacement.h"
#include "o/RawBridgeConnect.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 빠진 필수 출력은 조용히 생략하지 않고 해당 분기에서 계약 오류를 알린다.
template<class F> const F& RequiredPreview(const F& hook) {
    if (!hook) throw std::invalid_argument("Preview Draw hook required");return hook;
}
// 원본 DWORD 덧셈·뺄셈의 하위 비트를 C++ signed 넘침 없이 유지한다.
int AddPreview(int a,int b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b)); }
int SubPreview(int a,int b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b)); }
// 작은 원본 타입 footprint 곱의 DWORD 넘침도 정의된 비트로 보존한다.
int MulPreview(int a,int b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b)); }
// raw little endian 필드를 주변 미사용 바이트와 분리해 읽는다.
std::uint32_t ReadPreviewDraw(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value=0;
    // 필요한 필드 폭만 합친다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// finite x87 결과를 signed 64비트 절삭 후 하위 DWORD로 바꾼다. 비유한 격자 좌표는 거부한다.
int TruncatePreview(double value) {
    if (!std::isfinite(value) || value< -0x1p63 || value>=0x1p63) throw std::out_of_range("Preview Draw coordinate");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 타입 footprint가 뒤집힌 경우도 원본의 하한 우선 독립 clamp를 유지한다.
int ClampPreview(int value,int lower,int upper) { return std::max(lower,std::min(value,upper)); }
}
// 실제 저장소가 같은 범위의 raw 토큰과 타입 표를 사용하도록 검증한다.
PreviewDraw::PreviewDraw(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
    const PreviewCollectionState& collection,const PreviewDrawState& state,PreviewDrawHooks hooks,std::uint32_t tokenBase)
    :pool_(pool),types_(types),collection_(collection),state_(state),hooks_(std::move(hooks)),tokenBase_(tokenBase) {
    if (types.empty() || static_cast<std::uint64_t>(tokenBase)+static_cast<std::uint64_t>(pool.Capacity())*pool.Layout().stride>0x100000000ULL)
        throw std::invalid_argument("Preview Draw tables/token range");
}
// 같은 PATCH 풀에 속한 커서만 연결하여 raw SID 해석을 보존한다.
void PreviewDraw::BindCursor(CursorPreviewSource source) {
    if (&source.pool!=&pool_ || pool_.Edition()!=o::OriginalEdition::Patch1078)
        throw std::invalid_argument("Preview Draw cursor pool/edition");
    cursor_.emplace(source);
}
// 본체와 보조의 공통 화면 WORD/footprint를 먼저 저장하고 출력 분기를 나눈다.
void PreviewDraw::Draw(const o::RenderCollectionEntry& entry) const {
    if (entry.token<tokenBase_) throw std::out_of_range("Preview Draw token");
    const auto offset=entry.token-tokenBase_,stride=pool_.Layout().stride;
    if (!offset || offset%stride || offset/stride>=pool_.Capacity()) throw std::out_of_range("Preview Draw SID");
    const auto raw=pool_.Slot({static_cast<std::uint16_t>(offset/stride)});const auto type=static_cast<std::uint32_t>(raw[10]);
    if (type>=types_.size()) throw std::out_of_range("Preview Draw type");
    const auto& record=types_[type];
    // 출력 경계가 바꾼 값은 다음 원본 접근에서 참조를 통해 다시 읽는다.
    const auto draw=[&](const auto& c,const auto& s) {
        const int x=SubPreview(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReadPreviewDraw(raw,22,2))),s.cameraX);
        const int y=SubPreview(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReadPreviewDraw(raw,24,2))),s.cameraY);
        const int left=SubPreview(x,MulPreview(record.footX,16)),top=SubPreview(y,MulPreview(record.footY,11));
        const bool outlined=!c.parentGump && !(record.flags2&0x100004U) && type!=s.noOutlineType;
        // 표 선택과 각 그림은 원본 호출 순서 그대로 전달한다.
        const auto colors=[&](PreviewColors mode) { RequiredPreview(hooks_.setColors)({mode,type,c.localPlayer}); };
        const auto shape=[&](std::uint32_t key,std::uint32_t frame,int sx,int sy,PreviewShapeMode mode) {
            RequiredPreview(hooks_.shape)({key,frame,{sx,sy},mode});
        };
        const auto rectangle=[&](ScreenRect rect,std::uint8_t color,bool checker) { RequiredPreview(hooks_.rectangle)(rect,color,checker); };
        if (!(entry.tail&2U)) {
            if (outlined) rectangle({left,top,x,y},c.legal ? s.goodColor : s.badColor,false);
            const auto frame=ReadPreviewDraw(raw,pool_.Edition()==o::OriginalEdition::Patch1078 ? 36 : 34,pool_.Edition()==o::OriginalEdition::Patch1078 ? 4 : 1);
            const int sx=SubPreview(x,record.hotspotX),sy=SubPreview(y,record.hotspotY);
            if (c.legal) {
                if (type>=s.ownerMaps.size()) throw std::out_of_range("Preview Draw owner maps");
                if (s.ownerMaps[type] && std::bit_cast<std::int32_t>(c.localPlayer)>0) { colors(PreviewColors::Owner);shape(type,frame,sx,sy,PreviewShapeMode::CurrentColors); }
                else shape(type,frame,sx,sy,PreviewShapeMode::Plain);
                if (type==s.overlayTrigger) shape(s.overlayType,0,sx,sy,PreviewShapeMode::Plain);
            } else if (s.selectedType==s.destroyType) { colors(PreviewColors::Destroy);shape(type,frame,sx,sy,PreviewShapeMode::CurrentColors); }
            else shape(type,frame,sx,sy,PreviewShapeMode::Illegal);
            return;
        }
        if (!c.legal && outlined && ((s.gridOption&1U) || s.showLetters)) {
            if (record.footX>10 || record.footY>10) throw std::out_of_range("Preview Draw grid footprint");
            const auto wx=std::bit_cast<float>(ReadPreviewDraw(raw,14,4)),wy=std::bit_cast<float>(ReadPreviewDraw(raw,18,4));
            const float snappedX=static_cast<float>(std::floor(static_cast<double>(wx)+0.5)),snappedY=static_cast<float>(std::floor(static_cast<double>(wy)+0.5));
            const int anchorX=SubPreview(TruncatePreview(static_cast<double>(snappedX)*16+0.5),s.cameraX);
            const int anchorY=SubPreview(TruncatePreview(static_cast<double>(snappedY)*11+0.5),s.cameraY);
            const int firstX=static_cast<double>(wx)-TruncatePreview(wx)<0.5 ? -1 : 0;
            const int firstY=static_cast<double>(wy)-TruncatePreview(wy)<0.5 ? -1 : 0;
            // 원본은 행을 먼저 순회하고 표는 열*12+행으로 읽는다. 빈/퇴화 사각형도 출력 경계에 전달한다.
            for (int row=firstY;row<=record.footY;++row) {
                // 한 행의 화면 x는 16씩 줄어들며 다음 행의 y는 11씩 줄어든다.
                for (int column=firstX;column<=record.footX;++column) {
                    if (!s.grid.at(static_cast<std::size_t>((column+1)*12+row+1))) continue;
                    const int right=SubPreview(anchorX,MulPreview(column,16)),bottom=SubPreview(anchorY,MulPreview(row,11));
                    rectangle({ClampPreview(SubPreview(right,16),left,x),ClampPreview(SubPreview(bottom,11),top,y),
                        ClampPreview(right,left,x),ClampPreview(bottom,top,y)},s.badColor,true);
                }
            }
        }
        if (c.parentGump) return;
        const int labelX=AddPreview(x,SubPreview(s.spacing,s.labelOffsetX));int labelY=SubPreview(y,2);
        // MSVC signed char로 문자를 비교하며 숨긴 글자도 한 칸씩 아래쪽 기준을 이동한다.
        for (const auto letter:s.labels) {
            if (!letter) break;const auto code=static_cast<std::uint8_t>(letter);
            if (std::bit_cast<std::int8_t>(code)<'a' || s.showLetters) {
                colors(PreviewColors::LabelShadow);shape(s.labelType,s.labelFrames[code],AddPreview(labelX,1),AddPreview(labelY,1),PreviewShapeMode::CurrentColors);
                shape(s.labelType,s.labelFrames[code],labelX,labelY,PreviewShapeMode::Plain);
            }
            labelY=SubPreview(labelY,s.spacing);
        }
        std::uint32_t message=0;
        if (!(s.flags&0x20U) || s.messageOverride) {
            if (!c.legal && s.blocked) message=2;
            else if (!s.powered) message=3;
            else if (!c.legal && !s.reachable) message=4;
        }
        if (message) {
            const int mx=AddPreview(x,SubPreview(SubPreview(s.labelWidth/2,s.messageWidths[message]/2),s.labelOffsetX));
            // 실제 호출의 스택 인수로 확인한 y 기준이다. CD 디컴파일의 겹친 지역 변수와 구분한다.
            const int my=AddPreview(y,
                SubPreview(SubPreview(s.labelHeight/2,s.messageHeights[message]/2),s.labelOffsetY));
            RequiredPreview(hooks_.text)({message,{mx,my}});
        }
        if (s.flags&0x40U) {
            const int half=SubPreview(x,left)/2;RequiredPreview(hooks_.text)({5,{SubPreview(SubPreview(x,s.priceWidth/2),half),y}});
            // 두 판본 모두 실제 스택에서는 원래 화면 x에 비용 이미지 보정을 더한다.
            const int iconX=AddPreview(AddPreview(x,
                SubPreview(SubPreview(s.priceIconOffset,s.priceWidth/2),half)),s.priceIconX);
            shape(s.priceType,3,iconX,AddPreview(AddPreview(y,3),s.priceIconY),PreviewShapeMode::Plain);
        }
    };
    if (!cursor_) { draw(collection_,state_);return; }
    const auto& source=*cursor_;
    // 표시 허용은 최종 내려놓기 허용이 아닌55a290을 참조한다.
    struct CollectionInput {
        const std::uint32_t &parentGump,&legal,&localPlayer; // 실제 부모·표시 판정·플레이어 전역이다.
    };
    // 고정 출력 자료도 참조하여 같은 출력 중 변경을 숨기지 않는다.
    struct DrawInput {
        const std::uint32_t &selectedType,&destroyType,&noOutlineType,&overlayTrigger,&overlayType,&labelType,&priceType; // 선택 타입과 그림 종류다.
        const std::int32_t &cameraX,&cameraY,&spacing,&labelOffsetX,&labelOffsetY,&labelWidth,&labelHeight; // 화면 원점과 안내 영역이다.
        const std::int32_t &priceWidth,&priceIconOffset,&priceIconX,&priceIconY; // 금액과 그림 기준점이다.
        const std::uint32_t &gridOption,&showLetters,&flags,&messageOverride,&blocked,&powered,&reachable; // 서로 다른 옵션·호출 플래그·배치 판정이다.
        const std::uint8_t &goodColor,&badColor; // 허용·거부 윤곽 색이다.
        const std::array<std::uint8_t,144>& grid; // 실제 배치 검사 격자다.
        const std::array<std::uint32_t,256>& labelFrames; // signed 문자 그림 프레임 표다.
        const std::string& labels; // 현재 배치 설명이다.
        const std::array<std::int32_t,5> &messageWidths,&messageHeights; // Take에서 측정한 경고 문구 치수다.
        const std::vector<bool>& ownerMaps; // 타입별 소유자 변환표 존재 여부다.
    };
    const auto& d=source.drop;const auto& l=source.lifecycle;const auto& p=source.placement;const auto& s=state_;
    draw(CollectionInput{l.currentGump,p.placementAllowed,d.localPlayer},DrawInput{
        d.type,source.surfaces.bridgeType,source.surfaces.connectorType,s.overlayTrigger,s.overlayType,s.labelType,s.priceType,
        p.screenOrigin.x,p.screenOrigin.y,p.labelSpacing,p.bounds[0],p.bounds[1],p.bounds[2],p.bounds[3],
        d.cursorWidth,l.costTextWidth,l.costHotspotX,l.costHotspotY,s.gridOption,s.showLetters,d.costFlags,
        p.sourceAllowed,p.placementFlags,p.descriptionAllowed,p.spaceAllowed,s.goodColor,s.badColor,
        source.grid,s.labelFrames,p.description,l.warningWidths,l.warningHeights,s.ownerMaps});
}
// 종류 분배가 tail을 잃지 않고 실제 Draw를 호출하도록 연결한다.
void BindPreviewDraw(DrawDispatchHooks& hooks,const PreviewDraw& draw) { hooks.preview=[&draw](const auto& entry) { draw.Draw(entry); }; }
// 실제 로딩 타입의 블록과 별도 비용 이미지 바인딩을 반환한다.
std::function<PreviewShapeBinding(std::uint32_t)> MakePreviewAssetShapes(const GameAssets& assets,std::uint32_t priceType,PreviewShapeBinding price) {
    return [&assets,priceType,price](std::uint32_t type) {
        if (type==priceType) return price;
        if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=assets.Types().size()) throw std::out_of_range("Preview Draw asset type");
        return PreviewShapeBinding{&assets.Shapes(),assets.Types()[type-o::kFirstAssetTypeNumber].block};
    };
}
// 비용 그림은 Take가 실제 자산에서 얻은 현재 토큰으로 선택하며 일반 타입은 기존 저장소를 공유한다.
std::function<PreviewShapeBinding(std::uint32_t)> MakeCursorPreviewAssetShapes(const GameAssets& assets,
    std::uint32_t priceType,const CursorLifecycleState& lifecycle) {
    if (assets.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("Preview cursor asset edition");
    const auto ordinary=MakePreviewAssetShapes(assets,priceType,{});
    return [&assets,priceType,&lifecycle,ordinary](std::uint32_t type) {
        if (type!=priceType) return ordinary(type);
        const auto token=lifecycle.costImage;
        if (!token || token>assets.Shapes().Blocks().size()) throw std::out_of_range("Preview cursor cost image");
        return PreviewShapeBinding{&assets.Shapes(),token-1U};
    };
}
// 표면 범위를 먼저 검증하고 원본 dirty clip으로 제한한다.
PreviewPixelTarget::PreviewPixelTarget(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip,
    std::function<PreviewShapeBinding(std::uint32_t)> shape,std::function<ColorMap(PreviewColorRequest)> color,std::function<void(PreviewTextRequest)> text)
    :pixels_(pixels),pitch_(pitch),height_(height),clip_(clip),shape_(std::move(shape)),color_(std::move(color)),text_(std::move(text)) {
    if (pitch<=0 || height<=0 || static_cast<std::uint64_t>(pitch)*height>pixels.size() || clip.left>clip.right || clip.top>clip.bottom)
        throw std::invalid_argument("Preview Draw surface/clip");
    clip_=ClipScreenRect(clip,{0,0,pitch,height});
}
// 색 문맥 선택은 이후 mapped 그림에만 적용하고 plain 그림은 항상 원색을 쓴다.
PreviewDrawHooks PreviewPixelTarget::Hooks() {
    return {[this](auto request) { current_=RequiredPreview(color_)(request); },[this](auto request) { Shape(request); },
        [this](auto rect,auto color,auto checker) { Rectangle(rect,color,checker); },text_};
}
// 기존 VFX 해제/투명 합성을 사용하고 화면 기준점에 프레임 상대 사각형을 더한다.
void PreviewPixelTarget::Shape(PreviewShapeRequest request) {
    const auto binding=RequiredPreview(shape_)(request.type);if (!binding.database) throw std::invalid_argument("Preview Draw shape binding");
    const auto blocks=binding.database->Blocks();if (binding.block>=blocks.size() || request.frame>=blocks[binding.block].frames.size()) throw std::out_of_range("Preview Draw frame");
    const auto& frame=blocks[binding.block].frames[request.frame];if (frame.IsSpecial()) return;
    const auto key=std::make_tuple(binding.database,binding.block,static_cast<std::size_t>(request.frame));auto image=images_.find(key);
    if (image==images_.end()) image=images_.emplace(key,binding.database->Decode(binding.block,request.frame)).first;
    std::optional<ColorMap> translated;const ColorMap* colors=nullptr;
    if (request.mode==PreviewShapeMode::CurrentColors) {
        if (!current_) throw std::logic_error("Preview Draw current colors missing");colors=&*current_;
    } else if (request.mode==PreviewShapeMode::Illegal) { translated=RequiredPreview(color_)({PreviewColors::Illegal,request.type,0});colors=&*translated; }
    DrawIndexedImage(pixels_,pitch_,height_,image->second,AddPreview(request.position.x,frame.rect.left),AddPreview(request.position.y,frame.rect.top),clip_,colors);
}
// 체크 무늬의 위상은 원본 (~y^x)&1을 사용한다. 잘린 윤곽에 새 선을 만들지 않는다.
void PreviewPixelTarget::Rectangle(ScreenRect rect,std::uint8_t color,bool checker) {
    if (!checker) {
        const int right=SubPreview(rect.right,1),bottom=SubPreview(rect.bottom,1);
        // 원본은 폭/높이 0도 four line 호출로 전달한다. 역전 끝점은 선분 구간으로 정규화한다.
        const auto horizontal=[&](int y) {
            if (y<clip_.top || y>=clip_.bottom) return;
            const int low=std::max(std::min(rect.left,right),clip_.left),high=std::min(std::max(rect.left,right),clip_.right-1);
            // 가로 선분의 양 끝 픽셀을 포함한다.
            for (int x=low;x<=high;++x) pixels_[static_cast<std::size_t>(y)*pitch_+x]=color;
        };
        const auto vertical=[&](int x) {
            if (x<clip_.left || x>=clip_.right) return;
            const int low=std::max(std::min(rect.top,bottom),clip_.top),high=std::min(std::max(rect.top,bottom),clip_.bottom-1);
            // 세로 선분의 양 끝 픽셀을 포함한다.
            for (int y=low;y<=high;++y) pixels_[static_cast<std::size_t>(y)*pitch_+x]=color;
        };
        horizontal(rect.top);horizontal(bottom);vertical(rect.left);vertical(right);return;
    }
    const auto visible=ClipScreenRect(rect,clip_);if (visible.left>=visible.right || visible.top>=visible.bottom) return;
    // 장치 밖 좌표를 건드리지 않고 현재 dirty의 실제 픽셀만 갱신한다.
    for (int y=visible.top;y<visible.bottom;++y) {
        // 원본의 오른/아래 모서리는 제외 경계에서 한 픽셀 안쪽이다.
        for (int x=visible.left;x<visible.right;++x) {
            if (((x^y)&1)!=0)
                pixels_[static_cast<std::size_t>(y)*pitch_+x]=color;
        }
    }
}
}
