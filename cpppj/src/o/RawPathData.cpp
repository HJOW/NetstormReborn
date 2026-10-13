// Path 패킷은 미사용 헤더3바이트와 반환 길이 밖의 기존 공유 버퍼를 지우지 않는다.
#include "o/RawPathData.h"
#include "o/RawPathProcess.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::size_t kPayload=24; // 방향/거리 또는 직접 좌표 배열이 시작하는 원본 패킷 오프셋이다.
constexpr double kTimeScale=8; // 첫 double 도착 시각에 곱한 뒤 _ftol하는 실제 ROM 상수다.
// 비정렬 raw 필드는 호스트 엔디언과 무관하게 little endian으로 읽는다.
std::uint64_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset+width>bytes.size()) throw std::out_of_range("Path WriteData 이미지 범위");
    std::uint64_t result=0;
    // 원본 필드 폭만 읽으며 주변 여백을 합치지 않는다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint64_t>(bytes[offset+i])<<(i*8);
    return result;
}
// _ftol의 유효 signed32 입력 범위를 보존하고 정상 원본 범위 밖은 명시적으로 진단한다.
std::int32_t Truncate(double value) {
    const auto result=std::trunc(value);
    if (!std::isfinite(result) || result<std::numeric_limits<std::int32_t>::min() || result>std::numeric_limits<std::int32_t>::max())
        throw std::out_of_range("Path WriteData _ftol 범위");
    return static_cast<std::int32_t>(result);
}
// 좌표 배열의 실제 float32를 읽은 뒤 _ftol와 같은0쪽 절삭을 한다.
std::int32_t Coordinate(std::span<const std::uint8_t> image,std::size_t offset) {
    return Truncate(std::bit_cast<float>(static_cast<std::uint32_t>(Read(image,offset))));
}
}
// 현재 판본의 실제 원본 배열 위치를 사용한다.
RawPathData::RawPathData(OriginalEdition edition):edition_(edition) {}
// 원본 Runtime 저장소를 읽으며 Form 부착 콜백보다 앞서는 실제 base 번호는 호출자가 공급한다.
std::int32_t RawPathData::Write(const RawPathRuntime& runtime,Sid form,Sid parent,std::uint32_t parentGenus,
    const RawPathDataHeader& header,std::span<std::uint8_t> buffer) const {
    return Write(runtime.Bytes(),form,parent,parentGenus,header,buffer);
}
// 원본의 필드 쓰기 순서와 두 payload 분기를 그대로 유지한다.
std::int32_t RawPathData::Write(std::span<const std::uint8_t> image,Sid form,Sid parent,std::uint32_t parentGenus,
    const RawPathDataHeader& header,std::span<std::uint8_t> buffer) const {
    const auto layout=RawPathRuntime::Layout(edition_);
    if (image.size()!=layout.size || buffer.size()<kPayload) throw std::invalid_argument("Path WriteData 판본/버퍼 크기");
    // 실제 공통 헤더는 이 호출에서 fillLength0을 받는다. 따라서 기존 여백/나머지 버퍼를 보존한다.
    const auto put=[buffer](std::size_t offset,std::uint64_t value,std::size_t width=1) {
        if (offset+width>buffer.size()) throw std::out_of_range("Path WriteData 송신 버퍼 부족");
        // 원본 BYTE/WORD/DWORD의 하위 바이트만 저장한다.
        for (std::size_t i=0;i<width;++i) buffer[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
    };
    put(1,header.packetId);put(0,header.packetFlags&0x7f);put(2,std::max<std::uint16_t>(header.packetLength,4),2);
    put(4,form.value,2);put(6,parent.value,2);
    const auto count=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Read(image,0x10)));
    if (count<1 || static_cast<std::size_t>(count)>layout.capacity) throw std::out_of_range("Path WriteData i < nVert");
    auto x=Coordinate(image,0x14),y=Coordinate(image,0x18);
    put(15,static_cast<std::uint32_t>(x));put(16,static_cast<std::uint32_t>(y));
    put(17,static_cast<std::uint32_t>(Truncate(std::bit_cast<double>(Read(image,layout.times,8))*kTimeScale)),4);
    put(11,Read(image,layout.helper,2),2);
    const auto tail=edition_==OriginalEdition::Patch1078 ? 0x186e4U : 0x2f1cU;
    put(21,Read(image,tail,2),2);put(23,Read(image,tail+4,1));put(13,0,2);
    std::int32_t written=0;
    // 원본 Path는 부모 genus0x120000이면 방향/거리 대신 이후 꼭짓점의 절삭 좌표 두BYTE를 그대로 쓴다.
    for (std::int32_t i=1;i<count;++i) {
        const auto nextX=Coordinate(image,0x14+static_cast<std::size_t>(i)*8),nextY=Coordinate(image,0x18+static_cast<std::size_t>(i)*8);
        if (parentGenus&0x120000) {
            put(kPayload+written,static_cast<std::uint32_t>(nextX));++written;put(13,written,2);
            put(kPayload+written,static_cast<std::uint32_t>(nextY));++written;put(13,written,2);
        } else if (x!=nextX || y!=nextY) {
            const auto direction=static_cast<std::uint32_t>(Read(image,layout.directions+static_cast<std::size_t>(i)*4));
            auto distance=std::abs(static_cast<std::int64_t>(x)-nextX);
            if (!(direction&1)) distance+=std::abs(static_cast<std::int64_t>(y)-nextY);
            if (distance>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("Path WriteData 거리 범위");
            // 원본은31칸씩 분할하며 동일한 하위3비트 방향을 각 BYTE에 붙인다.
            while (distance>0) {
                const auto step=std::min<std::int64_t>(distance,31);put(kPayload+written,(step<<3)|(direction&7));
                ++written;put(13,written,2);distance-=step;
            }
        }
        x=nextX;y=nextY;
    }
    if (written>std::numeric_limits<std::int16_t>::max()) throw std::out_of_range("Path WriteData signed 길이 범위");
    return written+static_cast<std::int32_t>(kPayload);
}
// 같은 pool/hash를 사용하는 Host의 flags0 서버 부착이 실제 Path packet을 만들도록 분배한다.
ProcessFormTransmission MakePathFormTransmission(SidPool& pool,std::span<const RiftTypeRecord> types,
    RawPathDataHeader& header,std::span<std::uint8_t> buffer,std::function<void(std::int32_t)> transport,ProcessFormTransmission other) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || buffer.size()<kPayload || !transport)
        throw std::invalid_argument("Path 서버 직렬화 연결 누락");
    ProcessFormTransmission result;
    result.writeData=[&pool,types,&header,buffer,other](SquidProcess& process,bool parentChanged) {
        if (auto* path=dynamic_cast<RawPathProcess*>(&process)) {
            if (&path->Pool()!=&pool || !process.Form().value || !process.Parent().value) throw std::invalid_argument("Path 서버 직렬화 풀/부착 불일치");
            const auto type=pool.Slot(process.Parent())[10];
            return RawPathData(pool.Edition()).Write(path->Runtime(),process.Form(),process.Parent(),types[type].flags2,header,buffer);
        }
        if (!other.writeData) throw std::logic_error("다른 프로세스의 실제 WriteData 미복원 경계");
        return other.writeData(process,parentChanged);
    };
    result.transmit=std::move(transport);return result;
}
}
