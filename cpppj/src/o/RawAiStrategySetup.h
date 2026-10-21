// 원본 AI의 첫 영역·다리·생산 준비와 전략 목표 선택을 같은 본체에서 실행한다.
#pragma once
#include "o/RawAiPreparation.h"
#include "o/RawCarrierPreDestroy.h"
#include "o/RawContainedCreate.h"
#include "o/RawPlayerWorld.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 실제 원본 전역의 현재 저장소를 빌린다. 생성 시점의 값을 복제하지 않는다.
struct AiStrategySetupGlobals {
    const std::uint32_t& localOwner; //00540c70의 현재 사람/데모 로컬 소유자다.
    const std::int32_t& noRegion; //0050c038의 지역 없음 번호다.
    const std::uint32_t& templeProduct; //0054126c의 첫 신전 생산 TYPE다.
    const std::uint32_t& bridgeType; //005411a0의 다리 내용물 TYPE다.
    const std::int32_t& mapLimit; //00531928의 signed 지도 좌표 상한이다.
    const double& now; //0055b4d0의 현재 QWORD 시각이다.
};
// 월드가 이미 소유한 지역/삭제와 원본 진단 장치만 연결한다.
struct AiStrategySetupHooks {
    std::function<std::array<float,4>(std::int32_t)> regionBounds; //00432210→0040d900의 실제 자기 청크 경계다.
    std::function<std::uint32_t(std::int32_t)> temple; //0046f260의 현재 지역 신전 조회다.
    std::function<void(Sid,std::uint32_t)> destroy; //가상+10의 실제 내용물 삭제다.
    std::function<void(std::string_view)> assertion; //원본 assert 이후 계속하는 진단이며 미공급이면 명시 실패한다.
};
// 초기화258과 최초 월드 스캔25c를 분리하고 실제 Player 기술부모/내용물/게임 난수를 공유한다.
class RawAiStrategySetup {
public:
    RawAiStrategySetup(RawAiBrainMemory& memory,RawPlayerWorld& player,const RawAiPreparation& preparation,
        const RawCarrierPreDestroy& finder,const RawContainedCreate& create,const ContainedFinderState& finderState,
        std::span<const RiftTypeRecord> types,GameRandom& random,std::span<const std::uint32_t> allies,
        AiStrategySetupGlobals globals,AiStrategySetupHooks hooks); //같은 PATCH 풀과 Player/동맹 물리 계약을 확인한다.
    void Initialize(std::uint32_t token) const; //00414af0: 양쪽 지역 확인 뒤 자기 영역·첫 재고·수집자 생산과258을 기록한다.
    void SelectGoal(std::uint32_t token) const; //004139f0: 자금/기능/그래프/특정 좌표로 목표1~4를 고른다.
    Sid FindProduct(std::uint32_t token,std::uint32_t type) const; //00410360과004103c0은 모두 같은 mask8 조회다.
    void AddProduct(std::uint32_t token,Sid parent,std::uint32_t type) const; //00410420의 로컬/원격 생산 내용물 생성이다.
    void SeedBridges(std::uint32_t token,Sid parent,std::uint32_t count) const; //0040fc60의 기존 다리 스캔과 부족분 생성이다.
    const SidPool& Pool() const; //같은 실제 월드 조립 여부를 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재 살아 있는0xb08 본체를 다시 빌린다.
    std::uint32_t Field(std::uint32_t token,std::size_t offset) const; //현재 본체 DWORD를 읽는다.
    void Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const; //선택된 DWORD만 쓰고 주변을 보존한다.
    Sid PhysicalSid(std::uint32_t value) const; //풀 범위 검사 뒤 WORD 번호로 좁힌다.
    std::int32_t Region(std::uint32_t owner) const; //같은 현재 Player+78을 읽는다.
    void AssertType(std::uint32_t type) const; //원본 타입 범위 진단 후 계속한다.
    void Assert(std::string_view text) const; //선택된 원본 진단의 누락을 거부한다.
    void StoreBridge(std::uint32_t token,std::uint32_t pattern) const; //실제8DWORD 재고의 현재+628 칸에 쓴다.
    void AddBridge(std::uint32_t token,Sid parent) const; //0040fb30의 난수 소비·네 번째 직선·내용물·QWORD 기한이다.
    RawAiBrainMemory& memory_; //기존 생성/삭제/지갑과 같은 본체 저장소다.
    RawPlayerWorld& player_; //같은9Player의 현재 기술부모와 지역을 읽는다.
    const RawAiPreparation& preparation_; //공유00410300 적 선택 몸체다.
    const RawCarrierPreDestroy& finder_; //공유004ac9f0 내용물 조건 검색이다.
    const RawContainedCreate& create_; //공유004afa70 실제 생산 내용물 생성이다.
    const ContainedFinderState& finderState_; //004b2270 순회의 현재 권한/내용물 TYPE다.
    std::span<const RiftTypeRecord> types_; //신전의 현재 BYTE TYPE에서 theme을 읽는다.
    GameRandom& random_; //다른 게임 계산과 같은 원본 난수 스트림이다.
    std::span<const std::uint32_t> allies_; //방향별 현재9×9 동맹 표다.
    AiStrategySetupGlobals globals_; //현재 원본 전역의 참조다.
    AiStrategySetupHooks hooks_; //선택된 실제 월드/장치 효과다.
};
}
