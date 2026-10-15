// 기술목록의원본40개제한/중복/무진행과현재raw부모재조회를보존한다.
#include "o/RawPlayerTechnology.h"
#include "o/ConfigInterface.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제이름all에사용하는원본ASCII대소문자무시비교다.
bool IsAll(std::string_view text) { return text.size()==3 && (text[0]=='a' || text[0]=='A') && (text[1]=='l' || text[1]=='L') && (text[2]=='l' || text[2]=='L'); }
}
// 기존세몸체가같은PATCH풀을직접바라보게한다.
RawPlayerTechnology::RawPlayerTechnology(RawPlayerWorld& player,const RawCarrierPreDestroy& finder,const RawContainedCreate& create,std::span<const RiftTypeRecord> types,PlayerTechnologyHooks hooks)
    :player_(player),finder_(finder),create_(create),types_(types),hooks_(std::move(hooks)) {
    if (player.Pool().Edition()!=OriginalEdition::Patch1078 || &player.Pool()!=&finder.Pool() || &player.Pool()!=&create.Pool() || types.empty() || types.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) || !hooks_.findType)
        throw std::invalid_argument("Player기술은같은PATCH실제Pool/타입조회가필요합니다");
}
// 원본진단을보고한후계속한다. host메모리밖으로나가는경로는하위raw계약에서별도로거부한다.
void RawPlayerTechnology::CheckType(std::int32_t type,PlayerTechnologyAssertion where) const {
    if (type<1 || static_cast<std::size_t>(type)>=types_.size()) { if (!hooks_.assertion) throw std::logic_error("Player기술실제타입assert경계누락");hooks_.assertion(where); }
}
// 실제PlayerKnown진단과현재Technology DWORD읽기를기존getter에서수행한다.
Sid RawPlayerTechnology::Parent(std::int32_t owner) const {
    const auto value=player_.Technology(owner);if (value>std::numeric_limits<std::uint16_t>::max() || value>=player_.Pool().Capacity()) throw std::out_of_range("Player기술실제부모SID범위오류");return {static_cast<std::uint16_t>(value)};
}
// 현재부모기술중kind WORD와flags4가모두맞는첫실제SID를돌려준다.
Sid RawPlayerTechnology::Find(std::int32_t owner,std::int32_t type) const { CheckType(type,PlayerTechnologyAssertion::FindTypeRange);return finder_.FindContained(Parent(owner),4,static_cast<std::uint32_t>(type),0); }
// 삭제가변경한같은체인을매회처음부터조회하며다음SID를미리캡처하지않는다.
void RawPlayerTechnology::Clear(std::int32_t owner) const {
    const auto parent=Parent(owner);
    // 실제가상삭제는해당내용물을체인에서제거해야한다. 실패/미복원은callback에서보고한다.
    for (;;) { const auto found=finder_.FindContained(parent,4,0,0);if (!found.value) return;if (!hooks_.destroy) throw std::logic_error("Player기술실제가상삭제경계누락");hooks_.destroy(found,0); }
}
// 원본Ensure의중첩Find는두타입assert위치와없는경우두PlayerKnown진단을그대로발생시킨다.
void RawPlayerTechnology::Ensure(std::int32_t owner,std::int32_t type) const {
    CheckType(type,PlayerTechnologyAssertion::EnsureTypeRange);if (!Find(owner,type).value) create_.Create(Parent(owner),static_cast<std::uint32_t>(type),0,4,0x10,0);
}
// capacity가가득찬때는중복검사를하지않고끝낸다.
void RawPlayerTechnology::Append(PlayerTechnologyList& list,std::int32_t type) {
    if (list.count>=list.types.size()) return;const auto end=list.types.begin()+static_cast<std::ptrdiff_t>(list.count);if (std::find(list.types.begin(),end,type)==end) list.types[list.count++]=type;
}
// 원본은빈항목을만나면끝나며미등록비빈항목에서는index를증가시키지않는다.
PlayerTechnologyParseStep RawPlayerTechnology::ParseStep(std::string_view text,PlayerTechnologyList& list) const {
    if (list.count>list.types.size()) throw std::out_of_range("Player기술목록실제40개범위오류");
    const auto previous=list.index;const auto value=ConfigListItem(text,list.index);const auto type=hooks_.findType(value);
    if (!value.empty() && IsAll(value)) {
        // 원본은0번부터전체TYPE을순서대로조회하고group10만제외한다.
        for (std::size_t index=0;index<types_.size();++index) if (types_[index].group!=10) Append(list,static_cast<std::int32_t>(index));
        list.index=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(list.index)+1U);
    }
    if (type>0) { Append(list,type);list.index=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(list.index)+1U); }
    if (value.empty()) return PlayerTechnologyParseStep::Complete;return list.index==previous ? PlayerTechnologyParseStep::Stalled : PlayerTechnologyParseStep::Progress;
}
// 전체목록생성이끝나기전에는어떤Ensure도실행하지않는다.
void RawPlayerTechnology::AddList(std::int32_t owner,std::string_view text) const {
    PlayerTechnologyList list;
    // 실제미등록항목의무한반복을skip성공으로바꾸지않고상위미복원/잘못된스크립트경계로보고한다.
    for (;;) { const auto step=ParseStep(text,list);if (step==PlayerTechnologyParseStep::Complete) break;if (step==PlayerTechnologyParseStep::Stalled) throw std::logic_error("원본004914e0이미등록기술이름에서같은항목을무한반복합니다"); }
    // 먼저만든40개까지의순서/중복제거목록을변경하지않고실제Ensure에전달한다.
    for (std::size_t index=0;index<list.count;++index) Ensure(owner,list.types[index]);
}
// 연결어댑터의동일raw세계검증용이다.
const SidPool& RawPlayerTechnology::Pool() const { return player_.Pool(); }
// 타입이름의별도합성검색표를만들지않는다.
PlayerTechnologyHooks BindPlayerTechnologyTypes(const RiftTypeTable& types,PlayerTechnologyHooks hooks) { hooks.findType=[&types](std::string_view name) { return types.Find(name); };return hooks; }
}
