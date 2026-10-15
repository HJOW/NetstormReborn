// 실제AI본체/목록의소유메모리와PATCH00416860/00410100의생성·소멸을복원한다.
#pragma once
#include "o/RawPlayerWorld.h"
#include "o/RawPlayerParticipation.h"
#include "o/RawIslandRuntime.h"
#include <map>
#include <set>

namespace netstorm::o {
// 실제소유블록에대응하는opaque32핸들과요청크기다. 원본가상주소와같음을주장하지않는다.
struct AiBrainAllocation { std::uint32_t token{};std::size_t size{}; }; // 본체/목록포인터가이등록부의같은저장소를조회한다.
// 원본32비트pointer필드를호스트64비트포인터로역참조하지않는실제소유메모리등록부다.
class RawAiBrainMemory {
public:
    // 각할당의미초기화바이트입력을준비한다. 생성자는이후실제로쓰는바이트만변경한다.
    using Fill=std::function<void(std::span<std::uint8_t>)>;
    // 첫핸들은비제로이며각실제블록크기를8바이트경계로정렬해다음핸들을정한다.
    RawAiBrainMemory(std::uint32_t firstToken,Fill fill);
    // 실제host소유블록을만들고등록된핸들을반환한다. OOM은예외이며NULL성공을합성하지않는다.
    std::uint32_t Allocate(std::size_t size);
    // NULL은CRTfree처럼무동작이며유효블록은해제뒤다시조회할수없다.
    void Free(std::uint32_t token);
    // 현재살아있는실제블록전체를빌린다. 크기없는가짜brain/해제토큰은거부한다.
    std::span<std::uint8_t> Bytes(std::uint32_t token);
    std::span<const std::uint8_t> Bytes(std::uint32_t token) const;
    // 실제소유블록의존재/할당요청/해제순서를검증한다. game판단을대신하는상태는아니다.
    bool Contains(std::uint32_t token) const;
    const std::vector<AiBrainAllocation>& Allocations() const;
    const std::vector<std::uint32_t>& Frees() const;
private:
    std::uint32_t next_; // 다음실제소유블록의opaque32핸들이다.
    Fill fill_; // 할당직후의미초기화byte입력공급자다.
    std::map<std::uint32_t,std::vector<std::uint8_t>> blocks_; // 각핸들의유일한실제바이트소유자다.
    std::vector<AiBrainAllocation> allocations_; // 실제성공한할당의순서/크기다.
    std::vector<std::uint32_t> frees_; // 실제요청한free의순서다.
};
// 같은현재전역을참조한다. 값의사본을별도초기값으로가정하지않는다.
struct AiBrainConstructorGlobals {
    const std::uint32_t& localOwner; //00540c70의현재로컬owner다.
    const std::int32_t& noRegion; //0050c038의현재첫지역없음DWORD다.
    std::function<std::uint32_t()> invalidGraph; // 같은 장부의 현재 값을 각 00540410 읽기 지점에서 받아 본체 DWORD에 쓴다. BYTE backing도 안전하게 연결한다.
};
// 생성자가실제로호출하는사제/좌표지역/요구에너지metadata의명시경계다.
struct AiBrainConstructorHooks {
    std::function<Sid(std::uint32_t)> priest; //004921f0의같은실제Pool사제SID다.
    std::function<std::int32_t(float,float)> regionAt; //0046f530의현재hash0/실제지면지역이다.
    std::function<std::string(std::uint32_t)> requiredEnergy; //TYPE+a0실제문자열이다. type.name으로대체하지않는다.
};
// 아직전략전체/초기update를대체하지않는실제생성자와분리된소멸자다.
class RawAiBrainConstructor {
public:
    static constexpr std::size_t kBytes=0xb08; //00417e50이요청하는실제본체크기다.
    // 본체/목록메모리와Player9/방향별동맹/지역/TYPE는같은실제세계의저장소여야한다.
    RawAiBrainConstructor(RawAiBrainMemory& memory,RawPlayerWorld& player,RawPlayerParticipation& participation,
        RawIslandRuntime& islands,std::span<std::uint32_t> allies,std::span<const RiftTypeRecord> types,
        AiBrainConstructorGlobals globals,AiBrainConstructorHooks hooks);
    // 실제본체할당→00416860을수행한다. RawAiCreate::construct경계가요청한bytes도확인한다.
    std::uint32_t Construct(std::uint32_t owner,std::span<const std::uint8_t> settings,std::size_t bytes=kBytes);
    // caller가이미할당한0xb08블록에원본생성자만적용한다. native대조/실제allocator분리에사용한다.
    void Initialize(std::uint32_t token,std::uint32_t owner,std::span<const std::uint8_t> settings);
    //00410100은ae4조건부free후16개목록을역순free하며본체는아직살아있다.
    void Destruct(std::uint32_t token);
    // 상위00413ae0의CRTfree에해당하며이미소멸한본체만해제한다.
    void FreeBody(std::uint32_t token);
    // 후속초기update/전략이동일실제원문과등록된목록을읽고쓰게한다.
    std::span<std::uint8_t> Bytes(std::uint32_t token);
    const SidPool& Pool() const;
private:
    RawAiBrainMemory& memory_; // 같은본체/각목록의실제소유자다.
    RawPlayerWorld& player_; // 같은Player9의첫지역/기술부모저장소다.
    RawPlayerParticipation& participation_; // 기존실제방향별동맹/Chat/서버경계다.
    RawIslandRuntime& islands_; // 같은섬존재/소유/Player첫지역몸체다.
    std::span<std::uint32_t> allies_; // 같은실제81DWORD방향별표다.
    std::span<const RiftTypeRecord> types_; //현재TYPE번호순flags2표다.
    AiBrainConstructorGlobals globals_; // 현재세전역의참조다.
    AiBrainConstructorHooks hooks_; // 선택한실제지면/사제/요구에너지경계다.
    std::set<std::uint32_t> live_,destructed_; // 실제초기화/소멸완료본체의수명계약이다.
};
}
