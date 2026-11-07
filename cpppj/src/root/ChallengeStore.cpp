// R 초기화의 CCh 배치와 실제40개 전송/정리 수명을 복원한다.
#include "root/ChallengeStore.h"
#include "root/Configuration.h"
#include <bit>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace netstorm::root { // Native CCh 메모리와 연결표의 숫자 정체성을 구분한다.
namespace { // 비정렬 실제 DWORD를 호스트 포인터 캐스팅 없이 읽고 쓴다.
// 실제 물리의 선택 DWORD만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) {
    if (offset>bytes.size() || 4>bytes.size()-offset) throw std::out_of_range("R CCh DWORD 읽기 범위");std::uint32_t result{};
    for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[offset+byte])<<(byte*8);return result; // x86 낮은 BYTE부터 조립한다.
}
// 현재 물리의 DWORD만 덮고 미작성 tail은 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    if (offset>bytes.size() || 4>bytes.size()-offset) throw std::out_of_range("R CCh DWORD 쓰기 범위");
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 원문의 비정렬 DWORD 폭이다.
}
}
struct ChallengeStore::Record { // 실제 malloc 입력을 지우지 않고 생성자에 전달하는 안정된 소유권이다.
    std::uint32_t token{}; // 재사용하지 않는 같은 프로세스의 숫자 세대다.
    std::unique_ptr<std::uint8_t,decltype(&std::free)> bytes{nullptr,&std::free}; // 마지막 미작성DWORD를 포함한 실제 CRT 할당이다. nullptr 초기 소유자만 구성한다.
};
ChallengeStore::ChallengeStore(ConnectionState& state,TransceiverTransport& transport):state_(state),transport_(transport) {}
ChallengeStore::~ChallengeStore() {
    for (const auto& record:records_) for (auto& token:state_.challenges) if (token==record->token) token=0; // 최종 C++ 소유권 정리에서는 자기 별칭만 비우고 임의 사용자 callback을 재호출하지 않는다.
}
std::uint32_t ChallengeStore::Create() {
    if (serial_==std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("R CCh 숫자 토큰 소진");auto record=std::make_unique<Record>();record->bytes.reset(static_cast<std::uint8_t*>(std::malloc(kRootChallengeBytes)));
    if (!record->bytes) throw std::bad_alloc();ConstructChallengeState({record->bytes.get(),kRootChallengeBytes},geometry_);record->token=++serial_;const auto token=record->token;records_.push_back(std::move(record));return token;
}
std::span<std::uint8_t> ChallengeStore::Bytes(std::uint32_t token) {
    for (const auto& record:records_) if (record->token==token) return {record->bytes.get(),kRootChallengeBytes}; // 같은 생존 물리만 반환한다.
    throw std::out_of_range("R 생존 CCh 토큰 범위");
}
std::span<const std::uint8_t> ChallengeStore::Bytes(std::uint32_t token) const {
    for (const auto& record:records_) if (record->token==token) return {record->bytes.get(),kRootChallengeBytes}; // const 조회도 현재 소유 물리의 사본을 만들지 않는다.
    throw std::out_of_range("R 생존 CCh 토큰 범위");
}
void ChallengeStore::Assert(std::string_view expression,std::uint32_t line) { const auto hooks=transport_.Lifecycle();if (!hooks.assertion) throw std::logic_error("R CCh 실제 진단 장치 누락");hooks.assertion(expression,"c:\\ns\\r\\r.cpp",line); }
void ChallengeStore::InitializeRoot(Options& options,const std::function<void(std::int32_t,std::string_view)>& log) {
    if (!(state_.modeFlags&kRootMode)) return;
    if (!log) throw std::logic_error("R CCh 초기화의 실제 로그 장치 누락");log(0,"rootServerNumChalStates = "+std::to_string(state_.challengeStateCount)+"\n");log(0,"rootServerNumChalStatesPP = "+std::to_string(options.challengeStatesPerProcess)+"\n");
    if (state_.challengeStateCount>50) Assert("rootServerNumChalStates < NUM_CHALLENGE_STATES",2015);options.challengeStatesPerProcess=state_.challengeStateCount;
    if (!state_.challengeStateCount) throw std::domain_error("R numProcesses 계산의 원문0 나누기 거부"); // 원문은 count/count로 단일 프로세스를 강제하고0이면 실제 나누기 오류다.
    for (std::int32_t index=0;index<state_.challengeStateCount;++index) processOwners_.at(static_cast<std::size_t>(index))=0; // count/count=1이며 유효 index/count는0이므로 원문 지역 process 배열의0번만 읽는다.
    processOwners_[0]=0;if (state_.challengeStateCount>50) Assert("rootServerNumChalStates < NUM_CHALLENGE_STATES",2115);
    for (std::int32_t index=1;index<state_.challengeStateCount;++index) { // 원문의 객체 생성→표 공개→index/flags→모드 OR 순서다.
        const auto token=Create();state_.challenges.at(static_cast<std::size_t>(index))=token;const auto bytes=Bytes(token);Put(bytes,0,static_cast<std::uint32_t>(index));
        if (index<options.challengeStatesPerProcess) Put(bytes,4,Read(bytes,4)|2U);if (index>=0 && index<options.challengeStatesPerProcess) Put(bytes,4,Read(bytes,4)|1U);state_.modeFlags|=kChallengeMode;
    }
}
void ChallengeStore::Receive(std::uint32_t token) {
    for (std::size_t participant=0;participant<kChallengeParticipantCount;++participant) { // 각 수신 callback 뒤 현재 CCh의 다음 슬롯을 다시 읽는다.
        const auto current=Read(Bytes(token),kChallengeConnectionOffset+participant*4);if (current) static_cast<void>(transport_.ReceiveOne(current,true));
    }
}
void ChallengeStore::Flush(std::uint32_t token) {
    for (std::size_t participant=0;participant<kChallengeParticipantCount;++participant) { // 원문은0번도 포함한 현재 비NULL 슬롯만 가상 flush한다.
        const auto current=Read(Bytes(token),kChallengeConnectionOffset+participant*4);if (current) transport_.Flush(current);
    }
}
NetworkHooks ChallengeStore::BindNetwork(NetworkHooks hooks) {
    hooks.challengeConnection=[this](auto token,auto participant) { return ChallengeDirectoryView(Bytes(token)).Transceiver(participant); };hooks.receiveChallenge=[this](auto token) { Receive(token); };hooks.sendChallenge=[this](auto token) { Flush(token); };return hooks; // 실제 보조 객체/시계/로그 등 나머지 상위 장치는 그대로 빌린다.
}
void ChallengeStore::Destroy(std::uint32_t token) {
    for (auto record=records_.begin();record!=records_.end();++record) { // 현재 실제 소유자만 찾고 close callback이 끝나기 전에는 물리를 반납하지 않는다.
        if ((*record)->token!=token) continue;
        for (std::size_t participant=0;participant<kChallengeParticipantCount;++participant) { //0044cf30의 각 현재 연결 close→같은 슬롯0 순서다.
            const auto current=Read(Bytes(token),kChallengeConnectionOffset+participant*4);if (current) { transport_.Close(current);Put(Bytes(token),kChallengeConnectionOffset+participant*4,0); }
        }
        records_.erase(record);return;
    }
    throw std::out_of_range("R 반납할 생존 CCh 토큰 없음");
}
void ChallengeStore::Shutdown() {
    if (transport_.CallbacksActive()) throw std::logic_error("R 실제 전송 callback 중 CCh 소유권 반납 거부");
    for (std::size_t index=1;index<state_.challenges.size();++index) { // 원문 cleanup은0번을 제외한1..50의 현재 CCh를 소멸한 뒤 같은 칸0을 쓴다.
        const auto token=state_.challenges[index];if (token) { Destroy(token);state_.challenges[index]=0; }
    }
}
}
