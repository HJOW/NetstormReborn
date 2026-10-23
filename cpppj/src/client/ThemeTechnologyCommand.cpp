// 기술 지급은 창 수명과 분리하며 실제 플레이어 부모/타입표를 매 조회 시점에 읽는다.
#include "client/ThemeTechnologyCommand.h"
#include "client/RawCampaignSession.h"
#include "client/RawPriestWorldBindings.h"
#include "o/RawThemeTechnology.h"
#include <limits>
#include <stdexcept>
namespace netstorm::client {
// 원본00492ae0의 외부 경계를 기존 Session의 같은 객체들에만 연결한다.
void ApplyThemeTechnology(RawCampaignSession& session,std::int32_t theme) {
    o::ThemeTechnologyHooks hooks; // 현재 호출 동안만 세션을 빌리며 게임 상태 사본을 만들지 않는다.
    hooks.typeCount=[&session] { return static_cast<std::int32_t>(session.Core().types.size()); };
    hooks.typeAt=[&session](std::int32_t type)->const o::RiftTypeRecord& {
        const auto types=session.Core().types; // 현재 기록을 빌리며 원본 조회의 유효 범위를 호스트에서 보존한다.
        if (type<0 || static_cast<std::size_t>(type)>=types.size()) throw std::out_of_range("현재 기술 타입 기록 범위");
        return types[static_cast<std::size_t>(type)];
    };
    hooks.currentTechnology=[&session] {
        const auto parent=session.CurrentTechnologyParent(); // PlayerKnown0의 원본 반환0도 그대로 전달한다.
        if (parent>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("현재 기술 부모의 호스트 SID 표현 범위");
        return o::Sid{static_cast<std::uint16_t>(parent)};
    };
    hooks.findContained=[&session](o::Sid parent,std::uint32_t mask,std::uint32_t type,std::uint32_t index) { return session.Priests().Carrier().FindContained(parent,mask,type,index); };
    hooks.createContained=[&session](o::Sid parent,std::uint32_t type,std::uint32_t argument,std::uint32_t flags,std::uint32_t attach,std::uint32_t quality) { return session.CreateContained(parent,type,argument,flags,attach,quality); };
    o::RawThemeTechnology(std::move(hooks)).Apply(theme);
}
}
