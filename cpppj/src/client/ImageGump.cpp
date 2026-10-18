// 원본 x87의 최종float 저장·CRT하위DWORD·배율 비트 비교·색표 설치 순서를 보존한다.
#include "client/ImageGump.h"
#include "client/VFXTransform.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kVtable=0x501bb0,kShadow=0x556338,kSelected=0x556438,kDisabled=0x556038; // 원본 가상 표와 세 현재 색표 주소다.
constexpr std::int32_t kFixedUnit=65536,kHitSize=200; // 실제16.16과 적중 버퍼의 변 길이다.
// 도달한 실제 효과만 필수로 검사한다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("이미지 Gump 실제 하위 미연결");return callback; }
// 정수 좌표/색표 주소의 low DWORD 덧셈이다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
// 원본 CRT의64비트 절삭 후 하위 signedDWORD다. 비유한/범위 밖의 하위값은0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 10.78의 원본 필드/색표 계약으로만 생성한다.
ImageGump::ImageGump(const GumpCreateState& create,const GumpCreate& factory,ImageGumpState state,ImageGumpHooks hooks)
    :factory_(factory),state_(state),hooks_(std::move(hooks)) {
    if (create.collection.edition!=o::OriginalEdition::Patch1078) throw std::invalid_argument("이미지 Gump PATCH 자료 필요");
}
// 기본 생성자가 쓰는 동일 파생 주소를 반환한다.
ImageGumpNode& ImageGump::Node(std::uint32_t token) const {
    auto* node=token ? Need(hooks_.node)(token) : nullptr;if (!node) throw std::out_of_range("이미지 Gump 저장소 없음");return *node;
}
// 같은 기본 생성/등록 뒤 원본 타입 BYTE와 가상 표를 쓴다.
std::uint32_t ImageGump::Construct(std::uint32_t token,std::uint32_t type,std::uint32_t frame,std::uint32_t mode) const {
    auto& node=Node(token);factory_.Construct(token,0,0,0,1);node.vtable=kVtable;node.command=3;SetFrame(token,type,frame,mode);return token;
}
// fisub는 TYPE 정수를 float으로 먼저 좁히지 않는다. 마지막 차이만float으로 저장한다.
void ImageGump::SetFrame(std::uint32_t token,std::uint32_t type,std::uint32_t frame,std::uint32_t mode) const {
    auto& node=Node(token);node.type=type;node.frame=frame;node.mode=mode;const auto data=Need(hooks_.type)(type);const auto header=Need(hooks_.header)(type,node.frame);
    node.baseWidth=static_cast<float>(header.width);node.width=static_cast<float>(static_cast<double>(header.width)*node.scale);
    node.baseHeight=static_cast<float>(header.height);node.height=static_cast<float>(static_cast<double>(header.height)*node.scale);
    node.hotspotX=static_cast<float>(static_cast<double>(header.hotspotX)-data.hotspotX);node.hotspotY=static_cast<float>(static_cast<double>(header.hotspotY)-data.hotspotY);
}
// 모드4는 먼저 그리며 뒤의 일반/지정색 본체를 생략하지 않는다.
void ImageGump::Draw(std::uint32_t token) const {
    const auto& node=Node(token);const auto point=Need(hooks_.position)(token);const auto data=Need(hooks_.type)(node.type);
    const bool unit=std::bit_cast<std::uint32_t>(node.scale)==0x3f800000U;
    // 각 출력의 현재 기준점/16.16값과 색표 설치 순서를 보존한다.
    const auto draw=[&](bool mapped,bool shadow) {
        const auto px=shadow ? Add(point.x,state_.shadowOffset.x) : point.x,py=shadow ? Add(point.y,state_.shadowOffset.y) : point.y;
        const auto x=Ftol(static_cast<double>(px)+static_cast<double>(node.hotspotX)*(unit ? 1.0 : node.scale));
        const auto y=Ftol(static_cast<double>(py)+static_cast<double>(node.hotspotY)*(unit ? 1.0 : node.scale)-(shadow && unit ? 1.0 : 0.0));
        Need(hooks_.draw)({node.type,data.shape,node.frame,{x,y},unit ? kFixedUnit : Ftol(static_cast<double>(node.scale)*kFixedUnit),!unit,mapped});
    };
    if (node.mode&4U) { Need(hooks_.color)(kShadow);draw(true,true); }
    if (node.mode&3U) { Need(hooks_.color)((node.mode&1U) ? kSelected : kDisabled);draw(true,false);return; }
    if (node.type>=state_.ownerMaps.size()) throw std::out_of_range("이미지 Gump 현재TYPE 색표 범위 오류");
    const auto owner=std::bit_cast<std::int32_t>(state_.localOwner);const auto table=state_.ownerMaps[node.type];
    if ((!unit || !(node.mode&8U)) && table && owner>0) { Need(hooks_.color)(table+(state_.localOwner<<8));draw(true,false); }
    else draw(false,false);
}
// 그림 자식의 로컬 위치를 포함한 기준점을 만든 뒤 실제 버퍼 판정에 전달한다.
std::uint32_t ImageGump::Hit(std::uint32_t token,float x,float y) const {
    const auto& node=Node(token);const auto type=Need(hooks_.type)(node.type);const auto scale=Ftol(static_cast<double>(node.scale)*kFixedUnit);
    return Need(hooks_.hit)({node.type,type.shape,node.frame,{Ftol(x),Ftol(y)},
        {Ftol(static_cast<double>(node.hotspotX)*node.scale+node.x),Ftol(static_cast<double>(node.hotspotY)*node.scale+node.y)},scale});
}
// 기본 부착을 그대로 호출하며 추가 좌표 보정/자식 생성은 없다.
void ImageGump::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const { Need(hooks_.attach)(token,x,y,parent,mode); }
// TYPE 번호와 실제 로딩 블록 순서를 혼용하지 않는다.
void BindImageGumpAssets(ImageGumpHooks& hooks,const GameAssets& assets) {
    if (assets.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("이미지 Gump 실제PATCH 자산 필요");
    hooks.type=[&assets](std::uint32_t type) { const auto types=assets.TypeTable().Types();if (type<o::kFirstAssetTypeNumber || type>=types.size()) throw std::out_of_range("이미지 Gump 실제TYPE 자산 범위 오류");const auto& record=types[type];return ImageGumpType{type-o::kFirstAssetTypeNumber+1U,record.hotspotX,record.hotspotY}; };
    hooks.header=[&assets](std::uint32_t type,std::uint32_t frame) {
        if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=assets.Types().size()) throw std::out_of_range("이미지 Gump 실제SHP 타입 범위 오류");const auto block=type-o::kFirstAssetTypeNumber;const auto& record=assets.TypeTable().Types()[type];
        const auto logical=assets.Types()[block].definition.FrameTable().Codes().size()*((record.flags1&0x440000U) ? 2U : 1U);
        if (frame>=logical) throw std::out_of_range("이미지 Gump 실제frameCheck 범위 오류");const auto data=assets.Shapes().SquidMetrics(block,frame);return ImageGumpHeader{data.width,data.height,data.hotspotX,data.hotspotY};
    };
}
// 표면과 실제 자산은 공통 UI가 소유하며 색표 조회만 현재 소비자가 공급한다.
ImageGumpPixelTarget::ImageGumpPixelTarget(const GumpPixelTarget& target,const GameAssets& assets,ColorMap& currentColors,std::function<ColorMap(std::uint32_t)> colors)
    :target_(target),assets_(assets),currentColors_(currentColors),colors_(std::move(colors)) {
    if (assets.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("이미지 Gump 픽셀PATCH 자산 필요");
}
// 다른 생성/좌표/부착 분배를 보존한다.
ImageGumpHooks ImageGumpPixelTarget::Hooks(ImageGumpHooks hooks) {
    hooks.color=[this](auto token) { currentColors_=Need(colors_)(token); };hooks.draw=[this](auto request) { Draw(request); };hooks.hit=[this](auto request) { return Hit(request); };return hooks;
}
// 실제 타입 범위만 확인하고 표시 이름으로 그림을 바꾸지 않는다.
std::size_t ImageGumpPixelTarget::Block(std::uint32_t type) const {
    if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=assets_.Types().size()) throw std::out_of_range("이미지 Gump 픽셀TYPE 오류");return type-o::kFirstAssetTypeNumber;
}
// 같은 실제 프레임만 한 번 해제한다.
const IndexedImage& ImageGumpPixelTarget::Image(std::uint32_t type,std::uint32_t frame) {
    const auto key=std::make_pair(type,frame);auto it=images_.find(key);if (it==images_.end()) it=images_.emplace(key,assets_.Shapes().Decode(Block(type),frame)).first;return it->second;
}
// 특수 메타프레임은 기존 VFX와 같이 그림을 만들지 않는다.
void ImageGumpPixelTarget::Draw(ImageGumpDrawRequest request) {
    const auto block=Block(request.type);const auto& frame=assets_.Shapes().Blocks()[block].frames.at(request.frame);if (frame.IsSpecial()) return;
    const auto& image=Image(request.type,request.frame);const auto* colors=request.mapped ? &currentColors_ : nullptr;
    if (request.scaled) target_.ScaledImage(image,frame,request.position,request.scale,request.scale,colors);
    else target_.Image(image,{Add(request.position.x,frame.rect.left),Add(request.position.y,frame.rect.top)},colors);
}
// 원본4daca0→4dabf0의 버퍼/클램프·검사점 보정 뒤 실제 VFX 출력 색255를 판정한다.
std::uint32_t ImageGumpPixelTarget::Hit(ImageGumpHitRequest request) {
    const auto block=Block(request.type);const auto& frame=assets_.Shapes().Blocks()[block].frames.at(request.frame);
    auto point=request.point;const auto origin=request.origin;
    if (origin.x<0) point.x=Add(point.x,std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(origin.x)));
    if (origin.x>199) point.x=Add(point.x,Add(199,std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(origin.x))));
    if (origin.y<0) point.y=Add(point.y,std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(origin.y)));
    if (origin.y>199) point.y=Add(point.y,Add(199,std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(origin.y))));
    const auto index=static_cast<std::int64_t>(point.x)+static_cast<std::int64_t>(point.y)*kHitSize;
    if (index<0 || index>=kHitSize*kHitSize) throw std::out_of_range("이미지 Gump 원본 적중 버퍼 밖 읽기");
    std::array<std::uint8_t,kHitSize*kHitSize> scratch;scratch.fill(255);
    if (!frame.IsSpecial()) DrawScaledVfxImage(scratch,kHitSize,kHitSize,Image(request.type,request.frame),frame,{std::clamp(origin.x,0,199),std::clamp(origin.y,0,199)},request.scale,request.scale,{0,0,kHitSize,kHitSize});
    return scratch[static_cast<std::size_t>(index)]!=255 ? 1U : 0U;
}
}
