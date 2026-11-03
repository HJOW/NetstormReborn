// ContentForm 부착 뒤의 원문 사제 참여 갱신·음향·제한 안내를 연결한다.
#pragma once
#include "o/SidPool.h"
#include <array>
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::o { // 포획 자체와 분리된 10.78의004927f0 안내 부모다.
// 현재 참여/시각/이름/음향/Tell을 소유한 상위 Client가 선택 경계를 제공한다.
struct PriestTakenNoticeHooks {
    std::function<void(std::uint32_t)> refresh; //00495060의 현재 사제 참여 상태를 먼저 갱신한다.
    std::function<double()> wallSeconds; //00460d70의 정지와 무관한 현재 벽시각이다.
    std::function<std::string(std::uint32_t)> name; //00484480의 현재 Player/Chat 이름을 원본 바이트로 읽는다.
    std::function<void(std::string_view)> sound; //004a9cb0의 전역 재생0/0/0/1/0이다.
    std::function<void(std::string_view)> tell; //004cf960의 인자 포함 경고 섹션을 같은 실제 UI에 전달한다.
};
// 일반 ContentForm 부모가 조건을 판정한 뒤 이 알림만 호출하며 게임 포획 동작은 수행하지 않는다.
class RawPriestTakenNotice {
public:
    // 같은 풀·현재 로컬 owner·Client 수명의9개 기한을 빌린다. 생성은 어떤 게임 효과도 내지 않는다.
    RawPriestTakenNotice(const SidPool& pool,const std::uint32_t& localOwner,std::array<double,9>& deadlines,PriestTakenNoticeHooks hooks);
    // 참여 갱신→현재 소유자 분기→음향→기한별 안내→현재 벽시각+20 저장 순서를 보존한다.
    void Notify(Sid parent,std::uint32_t previousOwner) const;
private:
    const SidPool& pool_; // 음향/참여 콜백 뒤에도 같은 부모의 현재 owner BYTE를 읽는다.
    const std::uint32_t& localOwner_; //00540c70의 현재 DWORD이며 호출 전 사본으로 고정하지 않는다.
    std::array<double,9>& deadlines_; //00595450의 이전 소유자별 기한이며 Session 교체에서 지우지 않는다.
    PriestTakenNoticeHooks hooks_; // 실제 선택한 외부 효과만 요구하는 현재 장치다.
};
}
