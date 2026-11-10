// Client 원문의 지연 분포와 각 실제 연결의 FIFO를 복원하며 소켓/Session을 새로 만들지 않는다.
#pragma once
#include "o/RawPacketDispatch.h"
#include <functional>
#include <span>
#include <vector>

namespace netstorm::o { // 원문 포인터 대신 같은 현재 연결과 불변 receiver 값의 수명을 유지한다.
struct PacketDelayGlobals { //005e4784의 선택 분포와 같은 Client의 시계/CRT 장치다.
    PacketDelayGlobals(); // 새로운 Client에서만 기본01ff 분포를 선택한다.
    void ResetDefault(); //439863의 최초 지도 모드에서만 기본 분포를 다시 선택한다.
    void SelectLag(std::int32_t lag); //435220의1/2/3만 분포를 교체하고 다른 값은 현재 선택을 보존한다.
    std::span<const std::int8_t> distribution; // 실제 정적 signed BYTE 분포의 주소이며 기존 큐의 분포 사본을 재설정하지 않는다.
    std::function<double()> wall; // 원문의 현재 벽시각을 실제 장기 시계에서 읽는다.
    std::function<std::uint32_t()> random; // 같은 Client CRT Next를 소비하며 새 난수 스트림을 만들지 않는다.
};
class RawPacketDelayQueue final { //4def40의100칸 ring과4df010/4df5e0의 순서/주소 복원을 보유한다.
public:
    explicit RawPacketDelayQueue(std::shared_ptr<PacketDelayGlobals> globals); //현재 선택 분포를 한 번 빌리고 해당 연결의 빈100칸을 만든다.
    ~RawPacketDelayQueue(); //반납 시 남은 packet/receiver를 해제하고 미처리 callback을 실행하지 않는다.
    std::int32_t Enqueue(PacketConnection& connection,std::shared_ptr<const CanonicalPacketReceiver> receiver,const RawPacketDispatch& dispatch,std::span<const std::uint8_t> packet,std::int32_t delay); //현재 난수·observer1·선언 길이·주소를 원문 순서로 저장한다.
    std::int32_t Pump(PacketConnection& connection); //선두 deadline<now만 배송하며 주소/깊이를 복구하고 닫히면-1로 멈춘다.
private:
    struct Entry; //실제 원문30BYTE 노드의 논리 필드/packet owner다.
    void Grow(); //빈칸2미만일 때 ring을 두 배로 확장하고 기존 읽기/쓰기 순서를 유지한다.
    std::shared_ptr<PacketDelayGlobals> globals_; //같은 실제 Client 시계/CRT owner이며 Session을 보유하지 않는다.
    std::span<const std::int8_t> distribution_; //설치 당시 원문+74의 같은 불변 분포 주소다.
    std::int32_t weight_{}; //최대50 signed BYTE를ff 전까지 더한 실제+78 합계다.
    std::vector<std::unique_ptr<Entry>> ring_; //현재+7c/80의 안정된 Entry owner 배열이다.
    std::size_t write_{},read_{}; //현재+84/88의 서로 구별되는 쓰기/읽기 위치다.
};
void PreparePacketDelay(PacketConnection& connection,const std::shared_ptr<PacketDelayGlobals>& globals); //실제4dfc00 Poll에서만 현재 분포가 있고 큐가 없으면 설치한다.
}
