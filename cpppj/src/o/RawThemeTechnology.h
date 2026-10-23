// 10.78 CheatAllTechnology00492ae0의 현재 기술 부모 조회와 테마별 contained 지급 순서를 보존한다.
#pragma once
#include "o/RiftType.h"
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 본체의 네 원본 하위 호출과 현재 타입 개수 읽기를 실제 월드에 연결한다.
struct ThemeTechnologyHooks {
    std::function<std::int32_t()> typeCount; // 00541348의 현재 signed 개수다. 시작과 매 반복 끝에 다시 읽는다.
    std::function<const RiftTypeRecord&(std::int32_t)> typeAt; // 0049a840의 번호순 현재 기록이며 0번도 허용한다.
    std::function<Sid()> currentTechnology; // 0048fc60의 현재 기술 부모다. 명시 owner 조회와 대체하지 않는다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> findContained; // 부모·mask4·type·index0의 실제004acac0이다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> createContained; // 실제004afa70(parent,type,0,4,0x10,0)이며 반환은 본체에서 무시한다.
};
// 타입 필터와 현재 getter 재조회만 담당한다. 안내창 종료·PlayerKnown/타입 assertion을 추가하지 않는다.
class RawThemeTechnology {
public:
    // 모든 실제 getter/contained 경계가 필수이며 참조하는 월드는 이 객체보다 오래 살아야 한다.
    explicit RawThemeTechnology(ThemeTechnologyHooks hooks);
    // group10·theme5를 제외하고 선택 theme의 없는 기술을 타입0부터 원본 순서로 생성한다.
    void Apply(std::int32_t theme) const;
private:
    ThemeTechnologyHooks hooks_; // 현재 값과 외부효과를 같은 월드로 연결하는 명시 경계다.
};
}
