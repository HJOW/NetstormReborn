// PATCH Player의 실제 기술contained 조회/삭제/ensure와40개 문자열 목록 사슬을 같은 raw 세계에 연결한다.
#pragma once
#include "o/RawPlayerWorld.h"
#include "o/RawCarrierPreDestroy.h"
#include "o/RawContainedCreate.h"

namespace netstorm::o {
// 원본의 서로 다른 타입 범위 assertion 위치다. assert 후 조회/생성을 계속한다.
enum class PlayerTechnologyAssertion { FindTypeRange,EnsureTypeRange }; // 00490330/00490420의 typeNum 진단이다.
// 원본 DWORD 목록의 실제40개 capacity와 parser index를 보존한다.
struct PlayerTechnologyList {
    std::array<std::int32_t,40> types{}; // operator new(0xa0)의실제원소저장소다. count뒤의바이트는목록으로사용하지않는다.
    std::size_t count{}; // 원본목록+8의현재개수이며최대40이다.
    std::int32_t index{}; // 004914e0의현재세미콜론항목번호다.
};
// 원본 parser의 한 반복에서 나타나는 실제 종료/진행/같은 항목 반복이다.
enum class PlayerTechnologyParseStep { Complete,Progress,Stalled }; // 미등록비빈이름은Stalled이며원본은같은항목을무한반복한다.
// 기존 실제 타입 표 조회/가상 조각 삭제/진단을 연결한다.
struct PlayerTechnologyHooks {
    std::function<std::int32_t(std::string_view)> findType; // 실제0049a860 또는 RiftTypeTable::Find다.
    std::function<void(PlayerTechnologyAssertion)> assertion; // 타입진단후계속진행하는원본assert경계다.
    std::function<void(Sid,std::uint32_t)> destroy; // 실제가상+10(sid,flags)이며같은raw내용물체인을변경해야한다.
};
// 기존 Player 기술부모/contained finder/create를 재사용하며 SP/AI/별도 Player 사본을 만들지 않는다.
class RawPlayerTechnology {
public:
    // 모든 하위는 동일 PATCH Pool이어야 한다. types와 참조 대상은 모듈보다 오래 살아야 한다.
    RawPlayerTechnology(RawPlayerWorld& player,const RawCarrierPreDestroy& finder,const RawContainedCreate& create,
        std::span<const RiftTypeRecord> types,PlayerTechnologyHooks hooks);
    // 00490330: 타입assert→PlayerKnown진단/현재부모→실제FindContained(mask4,type,index0)다.
    Sid Find(std::int32_t owner,std::int32_t type) const;
    // 004903b0: 부모를한번캡처한뒤현재flags4내용물을매회다시찾아실제가상삭제flags0을반복한다.
    void Clear(std::int32_t owner) const;
    // 00490420: 타입assert→Find의중첩진단→없으면현재부모재조회→실제contained생성(0,4,10,0)이다.
    void Ensure(std::int32_t owner,std::int32_t type) const;
    // 004914e0의한반복이다. 미등록비빈입력을건너뛰지않으며caller가무진행을관찰할수있다.
    PlayerTechnologyParseStep ParseStep(std::string_view text,PlayerTechnologyList& list) const;
    // 004915c0의정상종료목록전체를먼저만든뒤순서대로Ensure한다. 원본무한반복입력은명시경계오류다.
    void AddList(std::int32_t owner,std::string_view text) const;
    // 같은 raw 세계 연결의 동일성을 검증한다.
    const SidPool& Pool() const;
private:
    // 타입범위진단은false성공이나조기return으로대체하지않는다.
    void CheckType(std::int32_t type,PlayerTechnologyAssertion where) const;
    // 원본목록의capacity확인→중복검사→끝추가순서다.
    static void Append(PlayerTechnologyList& list,std::int32_t type);
    // PlayerTechnology DWORD를실제SID폭과같은Pool범위로변환한다.
    Sid Parent(std::int32_t owner) const;
    RawPlayerWorld& player_; // 현재9 Player와PlayerKnown진단을공유한다.
    const RawCarrierPreDestroy& finder_; // 기존004ac9f0의실제contained체인조회다.
    const RawContainedCreate& create_; // 기존Factory/Form/부모부착의실제생성사슬이다.
    std::span<const RiftTypeRecord> types_; // 실제번호순group표이며all분류에직접사용한다.
    PlayerTechnologyHooks hooks_; // 실제이름해석/가상삭제/타입진단경계다.
};
// 원본이름표조회에기존RiftTypeTable::Find를직접연결한다.
PlayerTechnologyHooks BindPlayerTechnologyTypes(const RiftTypeTable& types,PlayerTechnologyHooks hooks);
}
