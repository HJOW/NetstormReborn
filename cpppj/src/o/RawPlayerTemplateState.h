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
    // PATCH4bd550/CD427920의기존nonemptyState다. 처음pos0, 짧은원문은44d290처럼확장뒤pos/end28, 그외동일데이터를반환한다.
    std::span<std::uint8_t> Get(std::uint32_t slot) const;
    // PATCH4bd540/4bd360·CD427910/4275d0의Notify다. 실제isMine1 또는preventSave에서자연반환한다.
    // 없는State초기화/실제파일저장은명시거부하며짧은기존State는실제할당이연결됐을때만28BYTE로확장한다.
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
    std::function<void(PlayerTemplateStateAssertion)> assertion_; // 원본보고후현재조건을재조회한다.
    std::function<std::span<std::uint8_t>(std::uint32_t,std::uint32_t)> allocate_; //shortState의실제새MemStream할당이다. 기존원문은본문이복사/0채움한다.
    std::function<void(std::uint32_t)> depthChanged_; // CD즉시증감값을실제소비자에전달한다.
    std::function<void(std::uint32_t)> allocationComplete_; // 실제 원문 복사가 끝난 뒤 이전 State 할당을 반납한다.
};
}
