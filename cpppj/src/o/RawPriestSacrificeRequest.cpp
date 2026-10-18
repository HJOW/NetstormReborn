// 원본 wrapper의 하위 요청 순서와 현재 owner 재독을 보존한다.
#include "o/RawPriestSacrificeRequest.h"
#include <stdexcept>
#include <utility>
namespace netstorm::o {
// 패치10.78의 실제 원문만 검증했으므로 CD를 추정 실행하지 않는다.
RawPriestSacrificeRequest::RawPriestSacrificeRequest(const SidPool& pool,const std::uint32_t& nuggetType,const std::uint32_t& localOwner,PriestSacrificeRequestHooks hooks)
    :pool_(pool),nuggetType_(nuggetType),localOwner_(localOwner),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || !hooks_.allowed || !hooks_.remove || !hooks_.command || !hooks_.music) throw std::invalid_argument("사제 희생 요청 판본/필수 효과 오류");
}
// 실제 원본4498e0은 SID를 현재 슬롯 주소로 바꾸고449710의 결과를 그대로 전달한다.
bool RawPriestSacrificeRequest::Allowed(Sid dais,Sid priest) const { return hooks_.allowed(dais,priest); }
// 원본494eb0은 별도 자격 검사를 덧붙이지 않는다. 명령 자체가 현재 자격을 다시 확인한다.
void RawPriestSacrificeRequest::Begin(Sid priest,Sid dais) const {
    hooks_.remove(priest,nuggetType_,0,1);hooks_.command(dais,priest,1);
    if (pool_.Slot(priest)[34]==localOwner_) hooks_.music("sacrifice.mus");
}
}
