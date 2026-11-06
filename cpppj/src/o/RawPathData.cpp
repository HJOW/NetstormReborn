// Path 패킷은 미사용 헤더3바이트와 반환 길이 밖의 기존 공유 버퍼를 지우지 않는다.
#include "o/RawPathData.h"
#include "o/RawPathProcess.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio> // 원문의 두 경로 trace 서식을 같은 signed DWORD로 작성한다.
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::size_t kPayload=24; // 방향/거리 또는 직접 좌표 배열이 시작하는 원본 패킷 오프셋이다.
constexpr double kTimeScale=8; // 첫 double 도착 시각에 곱한 뒤 _ftol하는 실제 ROM 상수다.
constexpr double kReadTimeScale=0.125,kStageSnapDistance=1.5; // 보호 PE0050df38/00503d78의 시각 복원 배율과 의식 시작점 보정 거리다.
// 비정렬 raw 필드는 호스트 엔디언과 무관하게 little endian으로 읽는다.
std::uint64_t ReadBits(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
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
    return Truncate(std::bit_cast<float>(static_cast<std::uint32_t>(ReadBits(image,offset))));
}
}
// 현재 판본의 실제 원본 배열 위치를 사용한다.
RawPathData::RawPathData(OriginalEdition edition):edition_(edition) {}
// 각 필드/꼭짓점의 실제 쓰기를 끝낸 뒤 진단하며 뒤 실패에서 앞선 상태를 되돌리지 않는다.
void RawPathData::Read(RawPathRuntime& runtime,std::span<const std::uint8_t> packet,const SidPool& pool,std::span<const RiftTypeRecord> types,const RawPathDataReadHooks& hooks) const {
    if (edition_!=OriginalEdition::Patch1078 || runtime.edition_!=edition_ || pool.Edition()!=edition_ || types.size()!=188) throw std::invalid_argument("Path 수신의 실제 PATCH 저장소/판본 불일치");
    if (packet.size()<kPayload) throw std::out_of_range("Path 수신의 실제24BYTE 머리 부족");
    const auto need=[](const auto& callback)->const auto& { if (!callback) throw std::logic_error("Path 수신의 실제 선택된 장치 미연결");return callback; }; // 미선택 하위는 요구하지 않으며 실제 호출을 빈 성공으로 대신하지 않는다.
    const auto parent=Sid{static_cast<std::uint16_t>(ReadBits(packet,6,2))};if (parent.value>=pool.Capacity()) throw std::out_of_range("Path 수신의 부모 물리 범위");
    auto x=static_cast<std::uint32_t>(packet[15]),y=static_cast<std::uint32_t>(packet[16]);const auto startTime=static_cast<double>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(ReadBits(packet,17))))*kReadTimeScale;
    const auto& layout=runtime.layout_;runtime.Write(layout.finished,0);runtime.Write(layout.targetA,0);runtime.Write(layout.targetB,0);runtime.Write(layout.helper,ReadBits(packet,11,2));runtime.Write(0x186e4,ReadBits(packet,21,2));runtime.Write(0x186e8,packet[23]);
    const auto byte=[&](std::size_t index) { if (index>=packet.size()-kPayload) throw std::out_of_range("Path 수신의 선택된 payload 부족");return packet[kPayload+index]; }; // 고정 헤더 뒤 기존 payload만 읽는다.
    const auto nodeCount=[&] { return static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReadBits(packet,13,2)))); }; // 직접 좌표 루프는 원문처럼 조건마다 현재 WORD를 재조회한다.
    const auto append=[&](RawPathPoint point,std::uint32_t direction) { const auto index=runtime.Count();if (index<0 || static_cast<std::size_t>(index)>=layout.capacity) throw std::out_of_range("Path 수신 append의 실제 배열 범위");const auto at=static_cast<std::size_t>(index);runtime.Write(0x14+at*8,std::bit_cast<std::uint32_t>(point.x));runtime.Write(0x18+at*8,std::bit_cast<std::uint32_t>(point.y));runtime.Write(layout.directions+at*4,direction);runtime.Write(layout.times+at*8,0,8);runtime.Write(0x10,static_cast<std::uint32_t>(index)+1U);if (static_cast<std::size_t>(index)+1>=layout.capacity) need(hooks.assertion)("nVert < MAX_PATH_LENGTH","c:\\ns\\o\\path.h",0x16); }; // 원문의 현재 count의 point/direction/time 쓰기→count++→진단이다.
    const auto tracePoint=[&](std::int32_t index,std::uint32_t px,std::uint32_t py,std::uint32_t direction) { std::array<char,128> text{};std::snprintf(text.data(),text.size(),"[%d] (%d,%d) , %d\n",index,std::bit_cast<std::int32_t>(px),std::bit_cast<std::int32_t>(py),static_cast<std::int32_t>(direction));need(hooks.trace)(text.data()); }; // Native printf의 DWORD 비트를 signed %d로 해석한다.
    const auto point=[&] { return RawPathPoint{static_cast<float>(std::bit_cast<std::int32_t>(x)),static_cast<float>(std::bit_cast<std::int32_t>(y))}; }; // 좌표 DWORD 감김 뒤 signed int→float 저장이다.
    const auto type=pool.Slot(parent)[10];if (type>=types.size()) throw std::out_of_range("Path 수신의 현재 부모 TYPE 범위");std::uint32_t firstDirection{};
    if (!(types[type].flags2&0x120000U)) {
        const auto count=nodeCount();firstDirection=count>0 ? byte(0)&7U : 0U;tracePoint(0,x,y,firstDirection);append(point(),firstDirection);auto previous=firstDirection,last=firstDirection;
        constexpr std::array<std::int32_t,8> dx{0,1,1,1,0,-1,-1,-1},dy{-1,-1,0,1,1,1,0,-1}; // 보호 PE0052f83c/5c의 실제8방향 DWORD다.
        for (std::int32_t index=0;index<count;++index) { const auto packed=byte(static_cast<std::size_t>(index));const auto length=std::uint32_t(packed>>3);last=packed&7U;std::array<char,128> text{};std::snprintf(text.data(),text.size(),"n[%d] %d | %d\n",index,static_cast<std::int32_t>(last),static_cast<std::int32_t>(length));need(hooks.trace)(text.data()); // 매 run의 방향/거리 로그가 실제 append보다 앞선다.
            if (last!=previous) { append(point(),previous);tracePoint(runtime.Count()-1,x,y,previous);append(point(),last);tracePoint(runtime.Count()-1,x,y,last); }
            x+=static_cast<std::uint32_t>(dx[last])*length;y+=static_cast<std::uint32_t>(dy[last])*length;previous=last;
        }
        if (count!=0) append(point(),last);tracePoint(runtime.Count()-1,x,y,last);
    } else {
        append(point(),0);if (packet[13]&1U) need(hooks.assertion)("(zwp->nodeCount & 1) == 0","\\Ns\\O\\Pathprocess.cpp",0x1ca);
        for (std::int32_t index=0;index<nodeCount();index+=2) { const auto px=byte(static_cast<std::size_t>(index)),py=byte(static_cast<std::size_t>(index)+1);append({static_cast<float>(px),static_cast<float>(py)},0); } // 짝수 좌표쌍을 순서대로 추가하며 방향/시간은0이다.
    }
    const auto target=static_cast<std::uint32_t>(runtime.Read(0x186e4));if (target && target<pool.Capacity() && !(pool.Slot(Sid{static_cast<std::uint16_t>(target)})[11]&1U)) {
        auto stage=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(runtime.Read(0x186e8)));const Sid destination{static_cast<std::uint16_t>(target)};
        if (stage>0 && stage<5) { const auto corner=need(hooks.stagePoint)(destination,static_cast<std::uint32_t>(stage-1));const auto raw=pool.Slot(parent);const double differenceX=static_cast<double>(std::bit_cast<float>(static_cast<std::uint32_t>(ReadBits(raw,14))))-corner.x,differenceY=static_cast<double>(std::bit_cast<float>(static_cast<std::uint32_t>(ReadBits(raw,18))))-corner.y;if (std::sqrt(differenceX*differenceX+differenceY*differenceY)<kStageSnapDistance) { runtime.Write(0x14,std::bit_cast<std::uint32_t>(corner.x));runtime.Write(0x18,std::bit_cast<std::uint32_t>(corner.y)); } }
        stage=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(runtime.Read(0x186e8)));if (stage>=0 && stage<5) { const auto corner=need(hooks.stagePoint)(destination,static_cast<std::uint32_t>(stage));const auto at=static_cast<std::int64_t>(runtime.Count())*8+0xc;if (at<0) throw std::out_of_range("Path 수신 마지막 점의 물리 범위");runtime.Write(static_cast<std::size_t>(at),std::bit_cast<std::uint32_t>(corner.x));runtime.Write(static_cast<std::size_t>(at+4),std::bit_cast<std::uint32_t>(corner.y)); }
    }
    const auto turning=need(hooks.metadata)(parent)[1];const auto speed=need(hooks.metadata)(parent)[0];runtime.Prepare(need(hooks.now)(),startTime,speed,turning,static_cast<std::int32_t>(firstDirection));need(hooks.clearAnimation)(parent);
}
// 수신 방향은 무동작이며 송신은 원문의 signed WORD 덧셈/하위WORD 저장을 유지한다.
void ObservePathData(std::span<std::uint8_t> packet,std::uint32_t sending,const std::function<void(std::string_view,std::string_view,std::uint32_t)>& assertion) {
    if (!sending) return;if (packet.size()<15) throw std::out_of_range("Path observer의 실제 nodeCount 부족");const auto count=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReadBits(packet,13,2)));if (count==-1) { if (!assertion) throw std::logic_error("Path observer의 실제 진단 미연결");assertion("nodeCount != (short)-1","\\Ns\\O\\_p_packets.cpp",0x716); }const auto length=static_cast<std::uint16_t>(static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReadBits(packet,13,2))))+24);packet[2]=static_cast<std::uint8_t>(length);packet[3]=static_cast<std::uint8_t>(length>>8);
}
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
    const auto count=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(ReadBits(image,0x10)));
    if (count<1 || static_cast<std::size_t>(count)>layout.capacity) throw std::out_of_range("Path WriteData i < nVert");
    auto x=Coordinate(image,0x14),y=Coordinate(image,0x18);
    put(15,static_cast<std::uint32_t>(x));put(16,static_cast<std::uint32_t>(y));
    put(17,static_cast<std::uint32_t>(Truncate(std::bit_cast<double>(ReadBits(image,layout.times,8))*kTimeScale)),4);
    put(11,ReadBits(image,layout.helper,2),2);
    const auto tail=edition_==OriginalEdition::Patch1078 ? 0x186e4U : 0x2f1cU;
    put(21,ReadBits(image,tail,2),2);put(23,ReadBits(image,tail+4,1));put(13,0,2);
    std::int32_t written=0;
    // 원본 Path는 부모 genus0x120000이면 방향/거리 대신 이후 꼭짓점의 절삭 좌표 두BYTE를 그대로 쓴다.
    for (std::int32_t i=1;i<count;++i) {
        const auto nextX=Coordinate(image,0x14+static_cast<std::size_t>(i)*8),nextY=Coordinate(image,0x18+static_cast<std::size_t>(i)*8);
        if (parentGenus&0x120000) {
            put(kPayload+written,static_cast<std::uint32_t>(nextX));++written;put(13,written,2);
            put(kPayload+written,static_cast<std::uint32_t>(nextY));++written;put(13,written,2);
        } else if (x!=nextX || y!=nextY) {
            const auto direction=static_cast<std::uint32_t>(ReadBits(image,layout.directions+static_cast<std::size_t>(i)*4));
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
    const auto pathSelected=std::make_shared<bool>(); // 반환된 writer와 버퍼 조회가 마지막 실제 파생 선택을 공유한다.
    result.writeData=[&pool,types,&header,buffer,other,pathSelected](SquidProcess& process,bool parentChanged) {
        *pathSelected=dynamic_cast<RawPathProcess*>(&process)!=nullptr;
        if (auto* path=dynamic_cast<RawPathProcess*>(&process)) {
            if (&path->Pool()!=&pool || !process.Form().value || !process.Parent().value) throw std::invalid_argument("Path 서버 직렬화 풀/부착 불일치");
            const auto type=pool.Slot(process.Parent())[10];
            return RawPathData(pool.Edition()).Write(path->Runtime(),process.Form(),process.Parent(),types[type].flags2,header,buffer);
        }
        if (!other.writeData) throw std::logic_error("다른 프로세스의 실제 WriteData 미복원 경계");
        return other.writeData(process,parentChanged);
    };
    result.packet=[buffer,other,pathSelected](std::int32_t size)->std::span<const std::uint8_t> {
        if (!*pathSelected) { if (!other.packet) throw std::logic_error("다른 Process의 실제 작성 버퍼 미연결");return other.packet(size); }
        if (size<0 || static_cast<std::size_t>(size)>buffer.size()) throw std::out_of_range("Path 실제 작성 버퍼 범위");return buffer.first(static_cast<std::size_t>(size));
    };
    result.writeFullData=[writer=result.writeData,other,pathSelected](SquidProcess& process,std::uint32_t force) {
        if (dynamic_cast<RawPathProcess*>(&process)) return writer(process,false); // 원문 Path writer는 세 번째 force 인자로 필드를 선별하지 않고 현재 전체 경로를 작성한다.
        *pathSelected=false; // 파생 fallback 뒤 버퍼 조회가 앞선 Path 버퍼를 잘못 선택하지 않게 같은 writer 선택을 갱신한다.
        if (!other.writeFullData) throw std::logic_error("다른 Process의 실제 전체 writer 미연결");return other.writeFullData(process,force);
    };
    result.transmit=[transport=std::move(transport),other,pathSelected](std::int32_t size) { if (*pathSelected) transport(size);else { if (!other.transmit) throw std::logic_error("다른 Process의 실제 송신 장치 미연결");other.transmit(size); } };return result; // 버퍼 조회와 같은 직전 writer 선택을 송신에서도 사용한다.
}
}
