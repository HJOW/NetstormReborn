// 원본 부착의 float 임시 저장과 reciprocal을 유지한 마지막 크기 쓰기를 구별한다.
#include "client/MetaImageGump.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client { // 동적 Image는 전용 가상 분배에서 같은 실제 표면으로 출력한다.
namespace { // 원문 주소는 실제 호스트 포인터로 실행하지 않고 근거 식별자로만 쓴다.
constexpr std::uint32_t kMetaVtable=0x50d290,kMetaKind=21,kDisabledColors=0x556038; // 실제 가상 표/TYPE BYTE/고정 색표다.
// 도달한 가상 효과만 필수로 검사한다.
template<class T> const T& MetaNeed(const T& value) { if (!value) throw std::logic_error("MetaImage 실제 하위 효과 미연결");return value; }
// 원본 CRT ftol의64비트 절삭과 반환 하위DWORD를 보존한다.
std::int32_t MetaFtol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 정수 기준점의 DWORD 덧셈 감김이다.
std::int32_t MetaAdd(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b)); }
}
// 기본 생성/등록은 이미 같은 판본의 실제 공통 factory를 사용한다.
MetaImageGump::MetaImageGump(const GumpCreateState& create,const GumpCreate& factory,MetaImageGumpHooks hooks):factory_(factory),hooks_(std::move(hooks)) {
    if (create.collection.edition!=o::OriginalEdition::Patch1078) throw std::invalid_argument("MetaImage 실제10.78 판본 필요");
}
// 각 효과가 현재 저장소를 다시 조회하므로 기본 부착 중의 Image 교체도 보인다.
MetaImageGumpNode& MetaImageGump::Node(std::uint32_t token) const {
    auto* node=token ? MetaNeed(hooks_.node)(token) : nullptr;if (!node) throw std::out_of_range("MetaImage 실제 파생 저장소 없음");return *node;
}
// 원본 생성은 Image 크기를 아직 측정하지 않고 기본 크기0/배율1을 보존한다.
std::uint32_t MetaImageGump::Construct(std::uint32_t token,RawImage& image) const {
    auto& node=Node(token);factory_.Construct(token,0,0,0,1);node.vtable=kMetaVtable;node.command=kMetaKind;node.image=&image;node.scale=1;node.width=node.baseWidth;node.height=node.baseHeight;node.colorMode=0;return token;
}
// 원문 fild→fst(float)→fmul은 정수+1을 먼저 float로 좁히지 않는다.
void MetaImageGump::Resize(std::uint32_t token) const {
    auto& node=Node(token);if (!node.image) throw std::logic_error("MetaImage 현재 Image 없음");
    const auto width=static_cast<double>(MetaAdd(node.image->Width(),1)),height=static_cast<double>(MetaAdd(node.image->Height(),1));
    node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(width*node.scale);node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(height*node.scale);
}
// 두 비율은 각각float로 저장하지만 reciprocal은 크기 곱셈까지 원문 레지스터 정밀도를 유지한다.
void MetaImageGump::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    MetaNeed(hooks_.attach)(token,x,y,parent,mode);auto& node=Node(token);if (!node.image) throw std::logic_error("MetaImage 부착 후 Image 없음");double scale=1;
    if (node.image->Width()!=0 && node.image->Height()!=0) {
        const auto width=static_cast<float>(static_cast<double>(node.image->Width())/node.baseWidth),height=static_cast<float>(static_cast<double>(node.image->Height())/node.baseHeight);
        if (width>1 || height>1) scale=1.0/static_cast<double>(height<width ? width : height);
    }
    node.scale=static_cast<float>(scale);node.width=static_cast<float>(scale*node.baseWidth);node.height=static_cast<float>(scale*node.baseHeight);
}
// 배율 비트가 정확히1일 때만 mapped를 쓰며 다른 배율은 plain 변환을 사용한다.
void MetaImageGump::Draw(std::uint32_t token) const {
    const auto& node=Node(token);if (!node.image) throw std::logic_error("MetaImage 출력 Image 없음");if (node.image->Shape().empty()) return;
    const auto point=MetaNeed(hooks_.position)(token);const bool unit=std::bit_cast<std::uint32_t>(node.scale)==0x3f800000U;
    if (unit) {
        if (node.colorMode) MetaNeed(hooks_.color)(kDisabledColors);
        MetaNeed(hooks_.draw)({node.image,{MetaAdd(point.x,node.image->Width()),MetaAdd(point.y,node.image->Height())},65536,false,node.colorMode!=0});return;
    }
    const auto scale=MetaFtol(static_cast<double>(node.scale)*65536.0);
    const auto y=MetaFtol(static_cast<double>(node.image->Height())*node.scale+point.y),x=MetaFtol(static_cast<double>(node.image->Width())*node.scale+point.x);
    MetaNeed(hooks_.draw)({node.image,{x,y},scale,true,false});
}
// 동적 shape의 현재 내용은 매 출력에서 읽고, 공통 클립/색표는 원문처럼 공유한다.
MetaImageGumpHooks BindMetaImagePixels(MetaImageGumpHooks hooks,const GumpPixelTarget& target,ColorMap& current,const ColorMap& disabled) {
    hooks.color=[&current,&disabled](auto token) { if (token!=kDisabledColors) throw std::out_of_range("MetaImage 고정 색표 주소 오류");current=disabled; };
    hooks.draw=[&target,&current](MetaImageDrawRequest request) {
        if (!request.image) throw std::logic_error("MetaImage 실제 픽셀 Image 없음");const auto raw=request.image->Shape();if (raw.empty()) return;
        ShapeDatabase database(std::vector<std::uint8_t>(raw.begin(),raw.end()));const auto& frame=database.Blocks()[0].frames[0];if (frame.IsSpecial()) return;
        const auto image=database.Decode(0,0);const auto* colors=request.mapped ? &current : nullptr;
        if (request.scaled) target.ScaledImage(image,frame,request.position,request.scale,request.scale,colors);
        else target.Image(image,{MetaAdd(request.position.x,frame.rect.left),MetaAdd(request.position.y,frame.rect.top)},colors);
    };
    return hooks;
}
}
