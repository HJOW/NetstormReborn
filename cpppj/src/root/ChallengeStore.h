// R의 실제 CCh 물리/토큰/프로세스 배치를 Root와 통신 순회가 함께 빌린다.
#pragma once
#include "root/RootDirectory.h"
#include <list>

namespace netstorm::root { // R00477e10의 CCh 생성 부분과00478af0의 CCh 정리 부분을 소유한다.
struct Options; // 같은 R 설정의 현재 프로세스당 개수만 빌린다.
class ChallengeStore final { // 이 소유자는 Root/Network callback보다 오래 살아야 한다.
public:
    ChallengeStore(ConnectionState& state,TransceiverTransport& transport); // 생성은 최초 BSS 표만 만들며 소켓/게임을 실행하지 않는다.
    ~ChallengeStore(); // 예외 종료의 자기 물리/별칭만 반납한다. 정상 원문 닫힘은 명시 Shutdown에서 수행한다.
    ChallengeStore(const ChallengeStore&)=delete;ChallengeStore& operator=(const ChallengeStore&)=delete; // 고정 바이트/현재 토큰과 callback this를 복제하지 않는다.
    std::uint32_t Create(); // 실제 비제로 CRT 할당 입력→원문 생성자→새 비재사용 숫자 토큰이다. 표 공개는 호출자가 수행한다.
    std::span<std::uint8_t> Bytes(std::uint32_t token);std::span<const std::uint8_t> Bytes(std::uint32_t token) const; // 다음 해당 소유권 반납 전까지 같은 실제2b80BYTE를 빌린다.
    ChallengeGeometry& Geometry() { return geometry_; }const ChallengeGeometry& Geometry() const { return geometry_; } // 프로세스 재초기화에서도 같은 최초 표를 보존한다.
    std::array<std::uint32_t,51>& ProcessOwners() { return processOwners_; } //004d1f88의 현재51개 프로세스 소유 번호다. 전체 초기화의0쓰기는 상위 부모가 수행한다.
    void InitializeRoot(Options& options,const std::function<void(std::int32_t,std::string_view)>& log); //00477e10의 강제 단일 프로세스/현재1..count-1 생성·index/flags/모드 쓰기다.
    NetworkHooks BindNetwork(NetworkHooks hooks={}); // 현재40토큰 조회/수신/flush를 같은 실제 CCh와 TransceiverTransport에 연결한다. 보조 객체는 상위 장치다.
    void Shutdown(); //00478af0의 CCh1..50 소멸/표0만 수행한다. 일반256/보조 객체의 앞뒤 정리는 상위 부모다.
private:
    struct Record; // 현재 CRT 물리와 숫자 토큰의 소유권은 구현 파일에 둔다.
    ConnectionState& state_;TransceiverTransport& transport_; // 같은 현재 R 표와 실제 노드 닫힘/수신/flush다.
    std::list<std::unique_ptr<Record>> records_;std::uint32_t serial_{}; // 포인터를 절삭하지 않는 안정된 소유자/비재사용 세대 번호다.
    ChallengeGeometry geometry_;std::array<std::uint32_t,51> processOwners_{}; // 객체 교체와 별개인 같은 프로세스 BSS다.
    void Assert(std::string_view expression,std::uint32_t line); // 실제 r.cpp 진단과 복귀 뒤 원문 쓰기다.
    void Receive(std::uint32_t token);void Flush(std::uint32_t token); //004578f0/004579d0의0..39 현재 토큰 순회다.
    void Destroy(std::uint32_t token); //0044cf30의 현재 연결40개 close/0 뒤 해당 실제 CCh 물리를 반납한다.
};
}
