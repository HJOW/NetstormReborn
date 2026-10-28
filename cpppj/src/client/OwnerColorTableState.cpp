// 43bc40/43be30의 실제 분기·store 순서와 공통 owner 색표 저장소다.
#include "client/OwnerColorTableState.h"
#include "o/RawColorSortPrimary.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client { // Client 수명과 독립 장치 검사 모두 같은 원문 몸체를 사용한다.
namespace { // 토큰 별칭과 필수 효과 검사는 게임 옵션을 새로 해석하지 않는다.
constexpr std::uint32_t kTokenBase=0x100000U,kTableBytes=2304U; // 호스트 주소 크기와 무관한 기존 ImageGump별칭 규칙이다.
template<class T> const T& Required(const T& value) { // 실제 도달한 외부 효과가 없으면 빈 성공을 만들지 않는다.
    if (!value) throw std::logic_error("공통 소유자 색표 장치 미연결");return value;
}
std::uint32_t TableToken(std::uint32_t type) { return kTokenBase+type*kTableBytes; } // 같은 타입9표의 고정 주소 별칭이다.
}
// 준비0/같은 값이면 포인터/장치를 읽지 않는다. 높은 비트와 dirty DWORD 감김도 보존한다.
void SetBannerOption(OwnerColorTableGlobals globals,std::uint32_t requested,const OwnerColorTableTypes& types,const OwnerColorTableHooks& hooks) {
    if (!globals.ready || globals.enabled==requested) return;
    const auto dirty=globals.redraw+1U;globals.enabled=requested;globals.redraw=dirty; // 실제43bc69/43bc6e의 쓰기 순서다.
    if (!requested) {
        Required(hooks.special)(types.banner[3],0.0,SpecialOwnerColorSources(97));
        Required(hooks.special)(types.banner[4],0.0,SpecialOwnerColorSources(94));return;
    }
    // 원문은 각 현재 TYPE/포인터를 차례로 다시 읽고 존재하는 표만 free 뒤0으로 쓴다.
    for (const auto type:types.banner) {
        if (type>=globals.tables.size()) throw std::out_of_range("배너 현재 TYPE 색표 범위");
        if (const auto pointer=globals.tables[type]) { Required(hooks.release)(pointer);globals.tables[type]=0; }
    }
}
// 최초 owner 표 등록은 색표가 만들어질 때만 수행하며 일반 팔레트 교체에서 다시 호출하지 않는다.
void InitializeOwnerColorTables(OwnerColorTableGlobals globals,std::uint32_t skip,const OwnerColorTableTypes& types,const OwnerColorTableHooks& hooks) {
    const auto previous=globals.enabled;globals.ready=1;globals.enabled=0xffffffffU;SetBannerOption(globals,previous,types,hooks);
    if (!skip) { Required(hooks.standard)(types.standard[0]);Required(hooks.standard)(types.standard[1]); }
    Required(hooks.special)(types.special[0],0.0,SpecialOwnerColorSources(111));
    Required(hooks.special)(types.special[1],-0.15000000596046448,SpecialOwnerColorSources(141));
    // 중간 네 일반 표 다음137 전용 표를 만드는 원본 순서를 유지한다.
    for (std::size_t index=2;index<6;++index) Required(hooks.standard)(types.standard[index]);
    Required(hooks.special)(types.special[2],0.0,SpecialOwnerColorSources(137));
    // 나머지 여섯 일반 표도 원본 호출 순서다.
    for (std::size_t index=6;index<types.standard.size();++index) Required(hooks.standard)(types.standard[index]);
}
void OwnerColorTableState::Initialize(std::shared_ptr<o::RawColorSortPrimary> primary,std::size_t typeCount,std::uint32_t bridgeType,std::uint32_t& redraw,std::uint32_t skip) { // 최초 Client 연결의 필수 물리를 검증한다.
    if (ready_ || !primary || !primary->Constructed() || typeCount<=170) throw std::invalid_argument("최초 공통 소유자 색표 준비 계약");
    primary_=std::move(primary);bridgeType_=bridgeType;tokens_.resize(typeCount);maps_.resize(typeCount);
    InitializeOwnerColorTables({ready_,enabled_,redraw,tokens_},skip,types_,Hooks());
}
void OwnerColorTableState::SetEnabled(std::uint32_t value,std::uint32_t& redraw) { SetBannerOption({ready_,enabled_,redraw,tokens_},value,types_,Hooks()); } // 같은 현재 포인터 표를 동기 변경한다.
const std::uint32_t& OwnerColorTableState::Enabled() const { return enabled_; } // 현재 DWORD의 주소를 그대로 빌린다.
bool OwnerColorTableState::Ready() const { return ready_!=0; } // 원문 준비 관문만 읽는다.
std::span<const std::uint32_t> OwnerColorTableState::Tokens() const { if (!ready_) throw std::logic_error("공통 소유자 색표 최초 등록 전 조회");return tokens_; } // 빈 표로 정상 렌더를 가장하지 않는다.
const OwnerColorMaps::value_type& OwnerColorTableState::Color(std::uint32_t type,std::uint32_t palette) const { // 현재 비제로 표와0..8 색 번호를 함께 검증한다.
    if (type>=maps_.size() || !tokens_[type] || !maps_[type] || palette>8) throw std::out_of_range("현재 공통 소유자 색표 조회 계약");return (*maps_[type])[palette];
}
const OwnerColorMaps::value_type& OwnerColorTableState::ColorToken(std::uint32_t token) const { // 원본처럼 행 오프셋은256 BYTE 배수다.
    if (token<kTokenBase || (token-kTokenBase)%256U) throw std::out_of_range("공통 색표 토큰 정렬");
    const auto offset=token-kTokenBase;return Color(offset/kTableBytes,(offset%kTableBytes)/256U);
}
OwnerColorTableHooks OwnerColorTableState::Hooks() { // 네이티브 free 호출 뒤의 포인터0쓰기는 SetBannerOption이 수행한다.
    return {[this](auto type) { Standard(type); },[this](auto type,auto offset,auto sources) { Special(type,offset,sources); },[this](auto token) {
        if (token<kTokenBase || (token-kTokenBase)%kTableBytes) throw std::out_of_range("공통 색표 반납 토큰");maps_.at((token-kTokenBase)/kTableBytes).reset();
    }};
}
void OwnerColorTableState::Standard(std::uint32_t type) { // 현재 타입/다리 특례의 같은 일반9색띠를 설치한다.
    maps_.at(type)=std::make_unique<OwnerColorMaps>(BuildStandardOwnerColorMaps(type,bridgeType_));tokens_.at(type)=TableToken(type);
}
void OwnerColorTableState::Special(std::uint32_t type,double offset,std::span<const std::uint32_t> sources) { // 가변 인자 계약을 확인하고 기존 primary 본체를 사용한다.
    const auto expected=SpecialOwnerColorSources(type);const auto wanted=type==141 ? -0.15000000596046448 : 0.0;
    if (!std::equal(sources.begin(),sources.end(),expected.begin(),expected.end()) || std::bit_cast<std::uint64_t>(offset)!=std::bit_cast<std::uint64_t>(wanted)) throw std::logic_error("공통 전용 색표의 원본 소스/명도 계약");
    maps_.at(type)=std::make_unique<OwnerColorMaps>(BuildSpecialOwnerColorMaps(*primary_,type));tokens_.at(type)=TableToken(type);
}
}
