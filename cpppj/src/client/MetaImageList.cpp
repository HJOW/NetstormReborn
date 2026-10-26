// 원문 목록의 부작용 순서와 float 저장/CRT 절삭을 보존한다.
#include "client/MetaImageList.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client { // 목록의 실제 자식은 원본 MetaImage이며 일반 메뉴 행으로 바꾸지 않는다.
namespace { // 원문 주소는 호스트에서 실행하지 않는 근거 식별자다.
constexpr std::uint32_t kListVtable=0x50d0d4,kListKind=22,kSelectionEvent=0x02000004; // 원본 가상 표/타입BYTE/부모 통지다.
// 도달한 실제 하위만 필수로 검사한다.
template<class T> const T& ListNeed(const T& hook) { if (!hook) throw std::logic_error("Image 목록 실제 하위 미연결");return hook; }
// 실제 CRT ftol의64비트 절삭 후 low DWORD다.
std::int32_t ListFtol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 원문 signed 산술의 DWORD 감김을 유지한다.
std::int32_t ListInteger(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
}
// 목록/공통 기본 생성은 같은10.78 자료를 사용한다.
MetaImageList::MetaImageList(const GumpCreateState& create,const GumpCreate& factory,MetaImageListHooks hooks):factory_(factory),hooks_(std::move(hooks)) {
    if (create.collection.edition!=o::OriginalEdition::Patch1078) throw std::invalid_argument("Image 목록 실제10.78 판본 필요");
}
// 비정상 포인터/저장소 밖 count를 성공으로 숨기지 않는다.
MetaImageListNode& MetaImageList::Node(std::uint32_t token) const {
    auto* node=token ? ListNeed(hooks_.node)(token) : nullptr;if (!node || node->count>kMetaImageListCapacity) throw std::out_of_range("Image 목록 실제 저장소/count 오류");return *node;
}
// 현재 배열이 가리키는 실제 MetaImage 파생을 빌린다.
MetaImageGumpNode& MetaImageList::Child(std::uint32_t token) const {
    auto* node=token ? ListNeed(hooks_.image)(token) : nullptr;if (!node) throw std::out_of_range("Image 목록 실제 자식 없음");return *node;
}
// 원문은 ID 배열과 두 셀 크기를 쓰지 않으므로 이전 입력 비트를 보존한다.
std::uint32_t MetaImageList::Construct(std::uint32_t token,std::int32_t columns) const {
    auto* current=ListNeed(hooks_.node)(token);if (!current) throw std::out_of_range("Image 목록 생성 저장소 없음");factory_.Construct(token,0,0,0,1);
    current->vtable=kListVtable;current->command=kListKind;current->count=0;current->images.fill(0);current->flags|=0x200;current->selection=-1;current->columns=columns;return token;
}
// 생성 중 현재 count가 바뀌어도 원문 각 읽기 시점을 유지한다.
bool MetaImageList::Add(std::uint32_t token,RawImage& image,std::uint32_t id) const {
    auto& node=Node(token);if (node.count>=kMetaImageListCapacity) return false;node.ids[node.count]=id;
    const auto child=ListNeed(hooks_.createImage)(image);node.images[node.count]=child;++node.count;
    ListNeed(hooks_.attach)(child,0,0,token,0);ListNeed(hooks_.push)(token);ListNeed(hooks_.attach)(token,node.x,node.y,0,0);return true;
}
// 기본 Attach 뒤의 현재 크기와 count를 읽으며 루프의 종료 count도 매번 읽는다.
void MetaImageList::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    ListNeed(hooks_.baseAttach)(token,x,y,parent,mode);auto& node=Node(token);const auto count=static_cast<std::int32_t>(node.count);if (count<=0) return;
    const auto columns=node.columns;float stepX{},stepY{};std::int32_t rowLimit=columns;
    if (columns<=0) {
        const double width=static_cast<double>(node.baseWidth)/count;stepX=static_cast<float>(width);node.cellWidth=stepX;
        node.cellHeight=static_cast<float>(width>node.baseHeight ? static_cast<double>(node.baseHeight) : width);rowLimit=count;
    } else {
        stepX=static_cast<float>(static_cast<double>(node.baseWidth)/columns);stepY=static_cast<float>(static_cast<double>(node.baseHeight)/(count/columns+(count%columns>0 ? 1 : 0)));
        node.cellWidth=stepX;node.cellHeight=stepY;
    }
    float cellX{},cellY{};std::int32_t column{}; // 각 셀의 누적 좌표는 원문처럼 매번float로 저장한다.
    for (std::uint32_t index=0;index<node.count;++index) { // 자식 통지 중 변경된 현재 count를 종료 조건으로 사용한다.
        ListNeed(hooks_.push)(node.images[index]);auto& child=Child(node.images[index]);child.baseWidth=node.cellWidth;child.width=static_cast<float>(static_cast<double>(node.cellWidth)*child.scale);
        auto& current=Child(node.images[index]);current.baseHeight=node.cellHeight;current.height=static_cast<float>(static_cast<double>(node.cellHeight)*current.scale);
        ListNeed(hooks_.attach)(node.images[index],cellX,cellY,token,0);cellX=static_cast<float>(static_cast<double>(cellX)+stepX);
        if (++column>=rowLimit) { column=0;cellX=0;cellY=static_cast<float>(static_cast<double>(cellY)+stepY); }
    }
}
// 표시 scale은 원문 적중 산술에 없으며 가로 분기는 count*dx를 정수로 먼저 계산한다.
std::int32_t MetaImageList::Hit(std::uint32_t token,std::int32_t x,std::int32_t y) const {
    const auto& node=Node(token);const auto bounds=ListNeed(hooks_.bounds)(token);if (x<bounds.left || x>=bounds.right || y<bounds.top || y>=bounds.bottom) return -1;
    const auto dx=ListInteger(static_cast<std::uint32_t>(x)-static_cast<std::uint32_t>(bounds.left));
    if (node.columns<=0) return ListFtol(static_cast<double>(ListInteger(node.count*static_cast<std::uint32_t>(dx)))/node.baseWidth);
    const auto dy=ListInteger(static_cast<std::uint32_t>(y)-static_cast<std::uint32_t>(bounds.top));
    const auto row=ListFtol(static_cast<double>(dy)/node.cellHeight),column=ListFtol(static_cast<double>(dx)/node.cellWidth);
    const auto index=ListInteger(static_cast<std::uint32_t>(row)*static_cast<std::uint32_t>(node.columns)+static_cast<std::uint32_t>(column));return static_cast<std::int32_t>(node.count)<index ? -1 : index;
}
// 같은 좌누름은 선택 해제도 허용하고 화면 밖 클릭도 현재 비교 결과로 통지한다.
std::uint32_t MetaImageList::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    if ((event&0x03ffffffU)!=0x10000U || (event&0x80000000U)) return 0;auto& node=Node(token);const auto index=Hit(token,ListInteger(first),ListInteger(second));
    if (index>=0) { node.selection=node.selection==index ? -1 : index;ListNeed(hooks_.redraw)(token,false); }
    ListNeed(hooks_.notify)(token,kSelectionEvent,static_cast<std::uint32_t>(index),node.selection==index ? 1U : 0U);return 1;
}
// 오른쪽/아래는절삭된왼쪽/위쪽에셀크기를더한뒤다시절삭한다.
void MetaImageList::Draw(std::uint32_t token) const {
    const auto& node=Node(token);if (node.selection<0) return;auto bounds=ListNeed(hooks_.bounds)(token);
    if (node.columns<=0) {
        bounds.left=ListFtol(static_cast<double>(node.selection)*node.baseWidth/node.count+bounds.left);bounds.right=ListFtol(static_cast<double>(bounds.left)+static_cast<double>(node.baseWidth)/node.count);
    } else {
        bounds.left=ListFtol(static_cast<double>(node.selection%node.columns)*node.cellWidth+bounds.left);bounds.right=ListFtol(static_cast<double>(bounds.left)+node.cellWidth);
        bounds.top=ListFtol(static_cast<double>(node.selection/node.columns)*node.cellHeight+bounds.top);bounds.bottom=ListFtol(static_cast<double>(bounds.top)+node.cellHeight);
    }
    const auto color=ListNeed(hooks_.green)();ListNeed(hooks_.fill)(bounds,color);
}
// 프로그래밍 오류인 저장소 밖 인덱스는 원문 진단에 대응하는 예외다.
std::uint32_t MetaImageList::Id(std::uint32_t token,std::int32_t index) const {
    if (index<0 || index>=static_cast<std::int32_t>(kMetaImageListCapacity)) throw std::out_of_range("Image 목록 ID 원문 index 진단");const auto& node=Node(token);return static_cast<std::uint32_t>(index)<node.count ? node.ids[index] : 0;
}
// 유효 접두부 밖은 Image가 없으며 selection/ID와 별도로 판정한다.
RawImage* MetaImageList::Image(std::uint32_t token,std::int32_t index) const {
    const auto& node=Node(token);return index>=0 && static_cast<std::uint32_t>(index)<node.count ? Child(node.images[index]).image : nullptr;
}
// 원문처럼 count만 줄이고 마지막 중복 포인터/ID와 현재 selection은 쓰지 않는다.
bool MetaImageList::Remove(std::uint32_t token,std::int32_t index) const {
    if (index<0 || index>=static_cast<std::int32_t>(kMetaImageListCapacity)) throw std::out_of_range("Image 목록 삭제 원문 index 진단");auto& node=Node(token);if (static_cast<std::uint32_t>(index)>=node.count || !node.count) return false;
    ListNeed(hooks_.kill)(node.images[index]);--node.count;
    for (auto current=static_cast<std::uint32_t>(index);current<node.count;++current) { node.images[current]=node.images[current+1];node.ids[current]=node.ids[current+1]; } // 두 접두부만 한 칸씩 압축한다.
    return true;
}
// Image 해제는 이 소멸자에 없으며 실제 기본 Gump 수명으로 넘어간다.
void MetaImageList::Destruct(std::uint32_t token) const { Node(token).vtable=kListVtable;ListNeed(hooks_.destructBase)(token); }
}
