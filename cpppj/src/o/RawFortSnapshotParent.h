// 10.78의 전체 Fort/Battle snapshot과 현재칸 두 wrapper의 순서·재조회를 보존한다.
#pragma once
#include <cstdint>
#include <functional>
#include <string_view>

namespace netstorm::o {
// 같은 Template.cpp 진단의 실제 행 번호다. 진단이 복귀하면 원문 위치에서 계속한다.
enum class FortSnapshotParentAssertion : std::uint32_t {
    MissingFortTemplate=0x530, // 004bf9a0 진입의 manager0이다.
    NotFortMode=0x531, // 004bf9a0의 현재 inFortMode0이다.
    FortNotMine=0x532, // 004bf9a0의 고정 this/현재 선택 주소 불일치다.
    MissingBattleTemplate=0x50e, // 004bf880 진입의 manager0이다.
    NotBattleMode=0x50f, // 004bf880의 현재 inBattleMode0이다.
    BattleNotMine=0x510, // 004bf880의 고정 this/현재 선택 주소 불일치다.
    MissingTechnologyTemplate=0x4a6, // 현재 기술 부모를 조회하기 직전의 manager0이다.
    MissingMissionTemplate=0x1e8 // 새 Mission 섹션을 만들기 직전의 manager0이다.
};
// bool/DWORD 소유자의 현재 값을 각 원문 읽기 위치에서 빌린다. getter 결과를 장기 복제하지 않는다.
struct FortSnapshotParentGlobals {
    std::function<std::uint32_t()> templateOwner; // 00540cac: 현재 Template 선택이다. Player owner00540c70과 구별한다.
    std::function<std::uint32_t()> fort; // 00594fb8: 부모 진단 및 wrapper의 두 번째 분기다.
    std::function<std::uint32_t()> battle; // 00594fbc: 부모 진단 및 wrapper의 첫 번째 분기다.
};
// 부모의 inline Create와 원본 child 진입을 별개로 연결한다. 각 child의 본체는 재구현하지 않는다.
struct FortSnapshotParentHooks {
    std::function<std::uint32_t(std::int32_t)> manager; // 고정 this 슬롯의 현재 첫 DWORD를 매 원본 위치에서 읽는다.
    std::function<void(FortSnapshotParentAssertion)> assertion; // 도달한 원문 진단이며 복귀 후 현재 상태를 계속 읽는다.
    std::function<void(std::int32_t)> typeNames; // 004bd6f0: Fort에서만 먼저 실행한다.
    std::function<std::uint32_t()> technologyParent; // 0048fc60: Technology Create 전에 조회해 반환값을 보관한다.
    std::function<std::uint32_t(std::uint32_t,std::string_view)> create; // 0044d1b0: 현재 manager와 원문 이름으로 stream을 만들고 명시 참조를 반환한다.
    std::function<void(std::uint32_t,std::uint32_t)> technology; // 004bcea0(parent,stream): Create가 다른 current/기술 부모를 만들어도 앞서 받은 두 값을 사용한다.
    std::function<void(std::int32_t)> objects; // 004be430: 같은 this의 전체 Terr/Chaff 저장이다.
    std::function<void(std::int32_t)> territory; // 004bf6d0: 같은 this의 현재 Territory 저장이다.
    std::function<void(std::int32_t)> deck; // 004befd0: Fort는 Territory 뒤, Battle은 Objects 전에 호출한다.
    std::function<void(std::uint32_t)> money; // 004832c0(stream): 부모가 만든 같은 stream이며 실제 공유 난수 writer는 외부 child다.
    std::function<void(std::uint32_t)> mission; // 00463a80(Guide00562ca0,stream): 실제 현재 미션 유무는 Guide child가 결정한다.
    std::function<void(std::int32_t)> badges; // 004be7f0: Fort의 Badges 저장이다. Battle에는 호출하지 않는다.
    std::function<void(std::int32_t)> coreData; // 004beb30: Battle의 조건부 CoreData 저장이다. Badges로 대체하지 않는다.
    std::function<void(std::int32_t)> notify; // 004bd360: 마지막 같은 this의 변경/실제 파일 저장 관문이다.
};
// 004bf9a0: 고정 this의 Fort 진단→TypeNames/기술/Objects/Territory/Deck/Money/Mission/Badges→Notify다.
void SnapshotFortTemplate(std::int32_t slot,const FortSnapshotParentGlobals& globals,const FortSnapshotParentHooks& hooks);
// 004bf880: 고정 this의 Battle 진단→기술/Deck/Objects/Territory/Money/Mission/CoreData→Notify다.
void SnapshotBattleTemplate(std::int32_t slot,const FortSnapshotParentGlobals& globals,const FortSnapshotParentHooks& hooks);
// 004bfac0: Battle와 Fort의 두 독립 if에서 각각 당시의 현재칸을 읽는다.
void SnapshotCurrentTemplate(const FortSnapshotParentGlobals& globals,const FortSnapshotParentHooks& hooks);
// 004bfb50: 최초 현재칸 manager0이면 반환한다. Battle 복귀 때만 owner를 다시 읽어 두 번째 Fort에 사용한다.
void SnapshotCurrentTemplateIfPresent(const FortSnapshotParentGlobals& globals,const FortSnapshotParentHooks& hooks);
}
