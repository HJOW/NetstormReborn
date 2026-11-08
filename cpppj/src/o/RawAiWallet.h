// 원본00413bb0의 현재 Player→실제 AI 본체 지갑 가산을 복원한다.
#pragma once
#include "o/RawAiBrain.h"
#include <memory> // 선택된 실제 본체 owner를 지갑 호출이 끝날 때까지 보유한다.

namespace netstorm::o {
// 원본이 각각의 읽기 위치에서 조회하는 두 전역/미션 조건이다.
struct AiWalletReads {
    std::function<bool()> disabled; // 00594fc8이 비영이면 Player/brain을 읽기 전에 반환한다.
    std::function<bool()> demo; // 가산 뒤00463b50이 현재 미션 포인터/+68에서 판정하는 데모 여부다.
};
// 현재 실제 메모리와 그 전체 Runtime 수명을 한 호출 동안 함께 빌린다. 새 brain을 만들지 않는다.
struct AiWalletMemoryAccess {
    RawAiBrainMemory* memory{}; // 선택된 Player+b0가 가리키는 실제 본체 저장소다.
    std::shared_ptr<void> owner; // 실제 Runtime 소유권이며 독립 메모리는 호출자의 수명을 사용한다.
};
// 값형 지갑 사본 없이 같은 Player9의+b0와 같은0xb08 본체를 사용한다.
class RawAiWallet {
public:
    // 참조/현재 조회기의 수명은 이 처리기보다 길어야 한다. 이 몸체는10.78용이다.
    RawAiWallet(RawAiBrainMemory& memory,std::span<const std::uint8_t> players,AiWalletReads reads);
    RawAiWallet(std::function<AiWalletMemoryAccess()> acquire,std::span<const std::uint8_t> players,AiWalletReads reads); // 모드/owner/NULL brain의 조기 반환 뒤에만 실제 본체 owner를 선택한다.
    RawAiWallet(std::function<AiWalletMemoryAccess()> acquire,std::function<std::span<const std::uint8_t>()> players,AiWalletReads reads); // Native의 재할당 가능한 실제 Player base를 원문 조회 위치에서만 읽는다.
    // signed owner>0 및 현재 brain 비영인 경우만 DWORD 감김 가산 뒤 데모 하한1190을 적용한다.
    // 원본의 표 밖 접근/해제 포인터는 호스트 예외로 거부한다. SP/난수/화면 자체는 갱신하지 않는다.
    void Credit(std::uint32_t owner,std::int32_t amount) const;
    std::int32_t Balance(std::uint32_t owner) const; //00413c20:현재모드/Player+b0/실제본체의signed지갑을읽는다.데모하한/분산SP는조회하지않는다.
private:
    std::span<const std::uint8_t> CurrentPlayers() const; // 조기 반환 뒤 현재 Player 물리를 조회하고 실제9행 폭을 확인한다.
    AiWalletMemoryAccess AcquireMemory() const; // 선택된 비영 brain에만 현재 실제 메모리/수명을 요구한다.
    std::span<std::uint8_t> Body(std::uint32_t token,const AiWalletMemoryAccess& memory) const; // 같은 실제 본체의 물리 폭을 확인하고 빌린다.
    RawAiBrainMemory* memory_{}; // 생성/부착/소멸과 같은 opaque 핸들 소유자다.
    std::span<const std::uint8_t> players_; // 현재 Player9의1620BYTE이며 별도 부착 bool을 사용하지 않는다.
    std::function<std::span<const std::uint8_t>()> playersRead_; // Client의 실제 현재 base getter이며 초기화/재할당을 넘어 span을 보관하지 않는다.
    std::function<AiWalletMemoryAccess()> acquire_; // Native의 현재 전체 owner를 필요한 위치에서만 빌리는 장치다.
    AiWalletReads reads_; // 부작용 뒤 다음 읽기 위치의 현재 값을 전달한다.
};
}
