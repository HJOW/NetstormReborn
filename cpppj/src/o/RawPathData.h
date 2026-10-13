// 실제 Path 가상 WriteData와 동일한 공유 버퍼를 서버 ProcessForm 송신에 연결한다.
#pragma once
#include "o/RawPathRuntime.h"
#include "o/SquidProcess.h"

namespace netstorm::o {
// 현재 원본 패킷 전역이다. 길이는 Path 본체의 반환 길이와 별개이며 버퍼 재사용 시에도 다시 기록한다.
struct RawPathDataHeader {
    std::uint8_t packetId{0x30}; // 패치0052edce/CD0050ff68: Path 패킷 종류 번호다.
    std::uint8_t packetFlags{1}; // 패치0052edcf/CD0050ff6c: 공통 헤더의 하위7비트다.
    std::uint16_t packetLength{}; // 패치00544c50/CD0050ff70: 공통 헤더에 기록할 현재 WORD 길이다.
};
// 패치004895b0/CD0047d630의 실제 필드 폭·좌표 압축·기존 버퍼 보존을 수행한다.
class RawPathData {
public:
    // 판본은 Path 이미지의 실제 배열/끝 필드 위치를 선택하며 호출 사이에 바뀌지 않는다.
    explicit RawPathData(OriginalEdition edition);
    // 부모 변경·force 인자는 원본 두 WriteData에서 읽지 않는다. form/parent는 OnAttached 전 현재 base 번호다.
    std::int32_t Write(const RawPathRuntime& runtime,Sid form,Sid parent,std::uint32_t parentGenus,
        const RawPathDataHeader& header,std::span<std::uint8_t> buffer) const;
    // 독립 native raw 입력과 같은 이미지에서 읽는다. 추가 경로/시각/상태 복사본은 만들지 않는다.
    std::int32_t Write(std::span<const std::uint8_t> image,Sid form,Sid parent,std::uint32_t parentGenus,
        const RawPathDataHeader& header,std::span<std::uint8_t> buffer) const;
private:
    OriginalEdition edition_; // 현재 원본 판본이며 모든 raw 필드 조회에 사용한다.
};
// 같은 SID 풀의 Path만 실제 직렬화한다. 다른 프로세스는 지정한 기존 송신기로 분배하며 누락을 거부한다.
// header/buffer/types/pool/송신 효과는 Host보다 오래 살아야 한다. transport는 이미 쓴 같은 buffer를 전송해야 한다.
ProcessFormTransmission MakePathFormTransmission(SidPool& pool,std::span<const RiftTypeRecord> types,
    RawPathDataHeader& header,std::span<std::uint8_t> buffer,std::function<void(std::int32_t)> transport,
    ProcessFormTransmission other={});
}
