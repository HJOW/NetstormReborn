// 설명 복사/정제 뒤 같은 현재 World의 실제88과 특정 전투의 기존 표시 본체를 실행한다.
#include "client/ClientMetaBattleDescription.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawCampaignSession.h"
#include "client/ScriptCommand.h" //실제80칸 문자열의 같은 수명 검사를 사용한다.
#include <algorithm>
#include <stdexcept>

namespace netstorm::client { //일반 World/메타 World를 새로 만들거나 성공 효과로 대체하지 않는다.
std::uint32_t EncodeClientMetaDescriptionAddress(const ClientChallengeGlobals& challenge,std::uint32_t offset) {
    const auto address=std::uint64_t(challenge.Current())+offset;if (address>=kMetaDescriptionAddressTag) throw std::overflow_error("설명 인자의 CS 논리 주소 표식 범위 소진");return kMetaDescriptionAddressTag|static_cast<std::uint32_t>(address);
} //호스트 포인터 절삭/고리 슬롯 예약 없이 원문 메뉴 작성 위치의 논리 주소만 보존한다.
std::optional<std::string_view> ResolveClientMetaDescriptionText(const ClientChallengeGlobals& challenge,const ScriptStringRing& strings,std::uint32_t parameter) {
    if (!parameter) return {};if (!(parameter&kMetaDescriptionAddressTag)) return strings.Resolve(parameter);
    const auto address=parameter&~kMetaDescriptionAddressTag;const auto bytes=challenge.Bytes();const auto offset=address-challenge.Current();if (offset>=bytes.size()) throw std::out_of_range("설명 창의 현재 CS 논리 문자열 물리 범위");const auto tail=bytes.subspan(offset);const auto end=std::find(tail.begin(),tail.end(),0);
    if (end==tail.end()) throw std::out_of_range("설명 창의 현재 CS 문자열 NUL 누락");return std::string_view(reinterpret_cast<const char*>(tail.data()),static_cast<std::size_t>(end-tail.begin()));
} //CS 내용은 메뉴 작성 때 복사하지 않으며 HTML 고리는 덮어쓴 토큰을 다른 주소로 재해석하지 않는다.
NativeMetaBattleDescriptionHooks MakeClientMetaBattleDescriptionHooks(Client& client) {
    NativeMetaBattleDescriptionHooks hooks;hooks.log=[&client](auto text) { client.WriteDiagnosticLog(9,text); };
    hooks.refreshObject=[&client](auto number) { auto& storage=*client.SurfaceStorage();if (number>65535U || number>=storage.pool.Capacity()) throw std::out_of_range("전투 설명의 실제 객체 SID 물리 범위");const auto raw=storage.pool.Slot(o::Sid{static_cast<std::uint16_t>(number)});client.GameplayGlobals()->RequireSession().Core().display.Update(raw,0); };
    hooks.updateBattle=[&client](auto battle) { client.UpdateNativeMetaBattle(battle); };return hooks; //전체 Poll/다른 참가자 갱신으로 대체하지 않는다.
}
}
