// Client 수명의 Player/factory 할당과 세션별 고정뷰 대여를 한 소유자에 보관한다.
#pragma once
#include "o/RawPlayerInitialization.h"
#include "o/SquidPostPop.h"
#include "o/RawProductionDeck.h"
#include <map>
#include <memory>

namespace netstorm::client {
class ClientCoreDataGlobals; // 같은 현재 Core/Player 뷰 슬롯의 장기 소유자다.
// opaque32 주소는 실제 보관 블록의 식별자이며 호스트 포인터를 DWORD로 절삭하지 않는다.
class ClientPlayerRecords {
public:
    using AllocationBytes=std::function<void(std::span<std::uint8_t>)>; // new가 초기화하지 않는 실제120BYTE의 외부 할당 입력이다.
    explicit ClientPlayerRecords(AllocationBytes allocationBytes={}); // 할당은 실제 원문 초기화 호출 때 시작한다.
    ClientPlayerRecords(const ClientPlayerRecords&)=delete; // 살아 있는 Player/factory alias를 복제하지 않는다.
    ClientPlayerRecords& operator=(const ClientPlayerRecords&)=delete; // 실제 할당 소유권의 값 대입을 막는다.
    ClientPlayerRecords(ClientPlayerRecords&&)=delete; // 대여자가 참조하는 owner 주소를 고정한다.
    ClientPlayerRecords& operator=(ClientPlayerRecords&&)=delete; // 사용 중인 원시 장치의 주소를 바꾸지 않는다.
    // 고정 Player span의 모든 소비자보다 먼저 만들고 나중에 반납하는 단일 세션 대여다.
    class Use {
    public:
        ~Use(); // 소비자 소멸 후 다음 세션의 재할당을 허용한다. 원문 정리를 암묵 실행하지 않는다.
        Use(const Use&)=delete; // 단일 대여를 복제하지 않는다.
        Use& operator=(const Use&)=delete; // 대여의 중복 반납을 막는다.
        void Initialize(std::function<void(std::uint32_t,std::uint32_t)> alliance); // 같은 대여의 첫 호출만 원문236BYTE를 실행한다.
        void AdoptCurrent(); // 앞선 실제 Native 부모가 초기화한 기록만 대여한다. 할당/동맹/필드 초기화를 다시 실행하지 않는다.
        void BindFactory(o::SquidPostPopList& list,std::uint32_t owner); // 실제 Player+0/+4/+8에 목록을 연결한다.
    private:
        friend class ClientPlayerRecords;
        explicit Use(ClientPlayerRecords& records); // 동시 세션이 기존 span을 무효화하기 전에 거부한다.
        ClientPlayerRecords& records_; // 모든 실제 할당과 두 카운터의 유일한 owner다.
        bool attempted_{},initialized_{}; // 실패한 부모를 같은 고정뷰 대여 중 재진입하지 않는다.
    };
    std::unique_ptr<Use> Acquire(); // 현재 대여가 없어야 하며 조회 자체는 할당/초기화를 하지 않는다.
    void Initialize(std::function<void(std::uint32_t,std::uint32_t)> alliance); // 고정뷰 소비자가 없을 때 원문 공통 시작/State 부모의 실제 초기화를 실행한다.
    void BindCoreData(std::shared_ptr<ClientCoreDataGlobals> core); // 같은 Player base 변경을 실제 Core의 빌린 Player 슬롯에 연결한다. 현재 선택을 임의로 바꾸지 않는다.
    std::shared_ptr<ClientCoreDataGlobals> BoundCoreData() const; // 같은 실제 장기 소유자 정체성을 순수 조회한다.
    bool Initialized() const; // 전체 실제 초기화가 정상 복귀했는지 관측한다. 부분 실패는 초기화 완료로 대여하지 않는다.
    std::span<std::uint8_t> Records(); // 현재 base가 가리키는 실제1620BYTE다. 재할당을 넘어 보관하려면 Use가 필요하다.
    std::span<const std::uint8_t> Records() const; // 같은 실제 Player 물리의 읽기뷰다.
    std::uint32_t Base() const; // 원시0059534c의 현재 opaque32 주소다.
    std::uint32_t& FortCount(); // 초기화와 실제 MissionLogin이 함께 쓰는005951f8이다.
    std::uint32_t& LastLoginPlayer(); // 같은005951fc의 실제 현재 DWORD다.
    std::array<std::uint32_t,81>& Alliances(); // 실제 동맹 자식과 모든 세션 소비자가 공유하는 방향별 표다.
    const std::shared_ptr<o::RawProductionDeckStorage>& ProductionDeckMemory() const { return productionDeck_; } // World 없는 HUD와 현재 Session이 같은 Player+68 블록을 빌린다.
private:
    struct Block {
        std::vector<std::uint32_t> words; // 실제 DWORD 정렬 저장소이며 Player405칸 또는 factory30칸이다.
        bool player{}; // 잘못된 해제 종류로 다른 생존 블록을 지우지 않는다.
    };
    std::uint32_t Allocate(std::uint32_t bytes,bool player); // 실제 메모리를 확보하고 겹치지 않는32비트 물리 주소를 발급한다.
    void Free(std::uint32_t token,bool player); // NULL free를 허용하며 해당 실제 할당을 반납한다.
    std::span<std::uint8_t> Memory(std::uint32_t address,std::uint32_t bytes); // 현재 생존 블록만 정확한 폭으로 해석한다.
    std::uint32_t Read(std::uint32_t owner,std::uint32_t offset); // 현재 Player 행의 little endian DWORD다.
    void Write(std::uint32_t owner,std::uint32_t offset,std::uint32_t value); // 같은 현재 행에4BYTE만 쓴다.
    std::span<std::uint32_t> Factory(std::uint32_t owner); // 현재 Player의 포인터/용량으로 같은 실제 factory 배열을 해석한다.
    void InitializeBody(std::function<void(std::uint32_t,std::uint32_t)> alliance); // 공통/대여 경로가 같은 실제 Native 부모와 메모리 장치를 공유한다.
    void WriteBase(std::uint32_t base); // 원문 현재 주소를 쓰고 같은 실제 할당의 host span 슬롯도 즉시 갱신한다.
    std::map<std::uint32_t,Block> blocks_; // 실제 생존 할당만 유지하며 해제된 주소로의 조회는 실패한다.
    std::uint32_t next_{0x30000000},base_{},fortCount_{},lastLoginPlayer_{}; // 겹치지 않는 발급 주소와 원문의 실제 세 DWORD다.
    std::array<std::uint32_t,81> alliances_{}; // 최초 BSS0이며 원문 자식이 각 행을 직접 갱신한다.
    AllocationBytes allocationBytes_; // 명시 검사는 고정 입력, 실제 Client는 현재 CRT의 할당 BYTE를 공급한다.
    bool inUse_{}; // 기존 고정span 소비자가 살아 있는 동안 재할당을 막는 호스트 수명 관문이다.
    bool initialized_{};std::weak_ptr<ClientCoreDataGlobals> core_; // 실제 초기화 정상 복귀와 외부 장기 Core 슬롯의 약한 연결이다.
    std::shared_ptr<o::RawProductionDeckStorage> productionDeck_{std::make_shared<o::RawProductionDeckStorage>()}; // 초기화/대여/반납으로 재생성하지 않는 실제 Deck 할당 소유자다.
};
}
