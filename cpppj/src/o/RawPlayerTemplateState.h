// 기존 State28BYTE와 parent가쓴 isMine을 실제 Player 소어루저 후처리에서 직접 조회한다.
#pragma once
#include "o/SidPool.h"
#include <array>
#include <functional>
#include <string>

namespace netstorm::o {
// DataManager 현재State의물리BYTE·MemStream현재cursor를원본저장소로소유한다.
struct PlayerTemplateStateStream {
    std::span<std::uint8_t> bytes; // 실제현재State 데이터이며외부수명이서비스보다길어야한다.
    std::uint32_t position{},end{}; // lookup의pos0·existing최소28 검사를같은DWORD로보존한다.
    bool present{true}; // 원본data pointer0을길이0과구별한다.
};
// 원본 빈 State의 미정 스택 입력과 실제 섹션 재생성/28BYTE 쓰기를 분리한다.
struct PlayerTemplateStateInitialization {
    std::function<void(std::uint32_t,std::span<std::uint8_t,28>)> bytes; // 원본이 보존하는 미정 바이트를 현재 외부 입력으로 공급한다.
    std::function<PlayerTemplateStateStream&(std::uint32_t,std::span<const std::uint8_t,28>)> replace; // 기존 State를 반납하고 새 현재 섹션에 같은28BYTE를 쓴다.
};
// 004bd550이 실제로 쓰는 필드만 초기화한다. 입력의 다른 바이트와 flags 상위5비트는 보존한다.
void InitializePlayerTemplateStateBytes(std::span<std::uint8_t,28> bytes);
// 원본 getter/Notify가보고후계속하는 진단종류다.
enum class PlayerTemplateStateAssertion {
    MissingStateTemplate, // Get의Template첫DWORD가0이다.
    DotFileName, // Notify의현재manager.fileName첫문자가'.'이다.
    MineTransientFlag, // Notify에서isMine이0이아닌데Template+12가0이아니다.
    InMissionSave, // isMine자연반환이없는데현재inMission이0이아니다.
    MissingNotifyTemplate // Notify의isMine/미션진단뒤Template첫DWORD가0이다.
};
// 실제Template9 header/이름/Statecursor를같은원본전역으로조립하는좁은서비스다.
class RawPlayerTemplateState {
public:
    // Template180/State9/현재filename9·loadingDepth·inMission은서비스보다오래살아야한다.
    RawPlayerTemplateState(OriginalEdition edition,std::span<std::uint8_t> headers,
        std::array<PlayerTemplateStateStream,9>& states,std::array<std::string,9>& fileNames,
        std::uint32_t& loadingDepth,const bool& inMission,std::function<void(PlayerTemplateStateAssertion)> assertion,
        std::function<void(std::uint32_t)> depthChanged,
        std::function<std::span<std::uint8_t>(std::uint32_t,std::uint32_t)> allocate={},std::function<void(std::uint32_t)> allocationComplete={});
    // 실제 Client의 빌린 헤더도 같은 manager의 State/name을 매번 조회하도록 연결한다.
    RawPlayerTemplateState(OriginalEdition edition,std::span<std::uint8_t> headers,
        std::function<PlayerTemplateStateStream&(std::uint32_t)> state,std::function<const std::string&(std::uint32_t)> name,
        std::uint32_t& loadingDepth,const bool& inMission,std::function<void(PlayerTemplateStateAssertion)> assertion,
        std::function<void(std::uint32_t)> depthChanged,std::function<std::span<std::uint8_t>(std::uint32_t,std::uint32_t)> allocate={},std::function<void(std::uint32_t)> allocationComplete={});
    // PATCH4bd550/CD427920: 처음pos0, 짧은원문은확장뒤pos/end28이다. PATCH 빈State는 연결된 생성 소비자를 사용한다.
    std::span<std::uint8_t> Get(std::uint32_t slot) const;
    void BindInitialization(PlayerTemplateStateInitialization initialization); // PATCH 빈 State를 실제 미정 입력/섹션 생성 소비자에 연결한다.
    void BindMinimumSize(std::function<PlayerTemplateStateStream&(std::uint32_t,std::int32_t)> ensure); // PATCH 실제4d290이 만든 새 현재 State를 직접 받는다.
    void BindFileSave(std::function<void(std::uint32_t)> save); //PATCH Notify의 실제0044d8d0 저장 부모를 같은 현재 Template 칸에 연결한다.
    // PATCH4bd540/4bd360·CD427910/4275d0의Notify다. 실제isMine1 또는preventSave에서자연반환한다.
    // 연결된 실제 저장 부모만 호출하며 짧은 State는 최소 확장 부모 또는 분리 할당 소비자가 있을 때만 확장한다.
    void Changed(std::uint32_t slot) const;
private:
    // 현재Template한기록의DWORD를원본little endian폭으로조회한다.
    std::uint32_t Header(std::uint32_t slot,std::size_t offset) const;
    OriginalEdition edition_; // CD의즉시depth증감과각진단줄을분리한다.
    std::span<std::uint8_t> headers_; // 실제9×20BYTE/currentisMine/flags를원본자료로빌린다.
    std::function<PlayerTemplateStateStream&(std::uint32_t)> states_; // 현재 헤더가 빌린 manager의 같은 mutableState/cursor를 조회한다.
    std::function<const std::string&(std::uint32_t)> names_; // 같은 현재 DataManager의 실제 파일 이름을 조회한다.
    std::uint32_t& depth_; // 같은Template/Object/Priest/Meta로딩깊이다.
    const bool& mission_; // 실제미션ctor가설정한594fa0/CD52e16c를공유한다.
    PlayerTemplateStateInitialization initialization_; // 실제로 빈 State가 선택됐을 때만 사용하는 필수 생성 경계다.
    std::function<PlayerTemplateStateStream&(std::uint32_t,std::int32_t)> ensureMinimum_; // 실제 임시 섹션/이름 교체까지 수행하는 현재 최소 확장 소비자다.
    std::function<void(PlayerTemplateStateAssertion)> assertion_; // 원본보고후현재조건을재조회한다.
    std::function<std::span<std::uint8_t>(std::uint32_t,std::uint32_t)> allocate_; // 분리 stream 입력의 확장용 할당이다. 실제 Session은 ensureMinimum_ 부모를 사용한다.
    std::function<void(std::uint32_t)> depthChanged_; // CD즉시증감값을실제소비자에전달한다.
    std::function<void(std::uint32_t)> allocationComplete_; // 분리 할당 어댑터의 원문 복사가 끝난 뒤 이전 State 할당을 반납한다.
    std::function<void(std::uint32_t)> save_; //isMine/미션/누락/preventSave 관문 뒤에만 선택하는 실제 파일 저장 소비자다.
};
}
