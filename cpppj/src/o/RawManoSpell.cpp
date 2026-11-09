// 현재 운반체·생성 대상·spawns 재조회와 원본 float 저장 경계를 유지한다.
#include "o/RawManoSpell.h"
#include "o/RawDeckDraw.h"
#include "o/RawWindCircle.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o { //원본 주소는 가상 표 선택 정보이며 호스트 함수 포인터로 실행하지 않는다.
namespace { //세 선택 주소와 누락 장치 진단을 이 몸체 안에 한정한다.
constexpr std::array<std::uint32_t,3> kTables{0x5026a0U,0x502778U,0x502840U}; //원문 함수의 선택 순서다.
template<class F> F ManoNeed(const F& function) { if (!function) throw std::logic_error("맨 오 워 주문의 선택된 실제 하위 미연결");return function; } //선택한 함수 사본을 정상/예외 복귀까지 유지한다.
}
RawManoSpell::RawManoSpell(SidPool& pool,const bool& server,std::span<const std::int32_t> counts,ManoSpellHooks hooks)
    :pool_(pool),server_(server),counts_(counts),hooks_(std::move(hooks)) { //별도 Pool/현재 TYPE/권한을 복제하지 않는다.
    if (pool.Edition()!=OriginalEdition::Patch1078 || counts.size()!=188) throw std::invalid_argument("맨 오 워 주문 PATCH/현재188TYPE 계약 오류");
}
std::uint32_t RawManoSpell::Read(Sid sid,std::size_t offset,std::size_t width) const { //이웃 필드와 free/dead 상태의 추가 관문을 넣지 않는다.
    const auto raw=pool_.Slot(sid);std::uint32_t result{};
    for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(raw[offset+byte])<<(8*byte); //필요한 폭만 little endian으로 조합한다.
    return result;
}
std::int32_t RawManoSpell::Count(std::uint32_t type) const { //외부 효과 뒤 같은 TYPE 행의 현재 spawns를 읽는다.
    if (type>=counts_.size()) throw std::out_of_range("맨 오 워 주문 현재 TYPE spawns 범위 오류");return counts_[type];
}
bool RawManoSpell::MayTarget(Sid parent,Sid target) const { //선택된 세 함수는 같은 순서를 쓰며 헤더 진단 위치만 다르다.
    if (!server_ || target.value!=Read(parent,12,2)) return false;
    const auto table=Read(parent,0);std::uint32_t level{};
    while (level<kTables.size() && kTables[level]!=table) ++level; //현재 실제 가상 표만 선택하고 다른 자격으로 대체하지 않는다.
    if (level==kTables.size()) throw std::invalid_argument("맨 오 워 주문 자격의 실제 가상 표 불일치");
    const auto localSeed=RawDeckDraw::WallSeed(ManoNeed(hooks_.wallSeconds)());static_cast<void>(localSeed); //원문 지역 난수는 생성만 하고 전역 게임 난수를 소비하지 않는다.
    const auto type=ManoNeed(hooks_.spellType)(level); //원문은 이 TYPE 포인터를 고정하고 그 행의 spawns만 반복해서 읽는다.
    for (std::int32_t index=0;index<Count(type);index=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(index)+1U)) { //현재 signed spawns와 DWORD 증가/재조회를 보존한다.
        const auto created=ManoNeed(hooks_.create)(ManoNeed(hooks_.flyerType)(),0);const auto raw=pool_.Slot(created);
        for (std::size_t byte=0;byte<4;++byte) const_cast<std::uint8_t*>(raw.data())[36+byte]=0; //가상 프레임 지정/표시 없이 실제 DWORD 프레임0만 쓴다.
        ManoNeed(hooks_.owner)(created,Read(created,0),static_cast<std::uint8_t>(Read(target,34,1)));
        const auto shape=ManoNeed(hooks_.header)(created,Read(created,40,1)&1U,level!=0);const auto center=ManoNeed(hooks_.center)(target);
        const auto x=static_cast<float>(static_cast<double>(center[0])+1.0); //원문5012d0을 더한 결과를 단정도에 저장한다.
        const auto y=static_cast<float>(static_cast<double>(center[1])+static_cast<float>(int(shape[0])-int(shape[1])+1)); //signed WORD 높이 보정 뒤 단정도에 저장한다.
        const auto point=WindCirclePoint(x,y,Count(type),index); //기존 전체41e020의 현재 개수/동일 index를 사용한다.
        ManoNeed(hooks_.pop)(created,Read(created,0),point[0],point[1],0);
    }
    return false;
}
const SidPool& RawManoSpell::Pool() const { return pool_; } //같은 현재 물리 자료를 빌리는지 확인한다.
}
