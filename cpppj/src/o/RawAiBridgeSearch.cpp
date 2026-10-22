// 끝점 열거 순서·signed32 산술·비대칭 x87 저장·동점 난수와8개 예산을 보존한다.
#include "o/RawAiBridgeSearch.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
//0052f83c/0052f85c의 원본 이웃 좌표다.
constexpr std::array<std::int32_t,8> kDx{0,1,1,1,0,-1,-1,-1},kDy{-1,-1,0,1,1,1,0,-1};
//0052f8bc의 반대 방향이며 -1 조회는 바로 앞의float -1 비트도 실제로 읽는다.
constexpr std::array<std::int32_t,9> kOpposite{-1082130432,4,5,6,7,0,1,2,3};
// 비정렬 DWORD의 주변 바이트를 읽지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 다리 검색 DWORD 읽기 범위");
    std::uint32_t value{};
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8); //little endian 네BYTE를 조합한다.
    return value;
}
// 지정 네BYTE만 갱신한다.
void Write(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 다리 검색 DWORD 쓰기 범위");
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); //원본 순서대로 낮은 바이트부터 쓴다.
}
// 원문 DWORD의 signed 비교를 보존한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원본__ftol signed64 절삭의 낮은DWORD이며 indefinite 하위는0이다.
std::uint32_t Truncate(double value) { if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(value)); }
// INT_MIN도 원본 xor/sub의32비트 감김으로 처리한다.
std::int32_t Absolute(std::uint32_t value) { return Signed(Signed(value)<0 ? 0U-value : value); }
// 전체 방향 번호를 검사한 뒤 원본 이웃 표를 조회한다.
std::size_t Direction(std::uint32_t value) { if (value>=8) throw std::out_of_range("AI 다리 검색 방향 표 범위");return value; }
}
// 별도 월드 사본이나 난수 스트림을 만들지 않는다.
RawAiBridgeSearch::RawAiBridgeSearch(RawAiBrainMemory& memory,std::span<const RiftTypeFrames> frames,std::span<const std::uint8_t> spots,const std::uint32_t& bridge,GameRandom& random):memory_(memory),frames_(frames),spots_(spots),bridge_(bridge),random_(random) {
    if (spots_.size()!=65536) throw std::invalid_argument("AI 다리 검색 실제spot65536 누락");
}
// 매 호출마다 현재 소유 블록을 다시 조회한다.
std::span<std::uint8_t> RawAiBridgeSearch::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 다리 검색 실제0xb08 본체 누락");return raw; }
// 원본 생성자의 bridge 경로이며 패턴 번호에 임의 마스크를 추가하지 않는다.
CanonDecoder RawAiBridgeSearch::Decoder(const AiBridgePlacement& placement) const {
    const auto patterns=BridgePatterns();if (bridge_>=frames_.size() || placement.pattern>=patterns.size()) throw std::out_of_range("AI 다리 검색 현재TYPE/패턴 범위");
    return {frames_[bridge_],patterns[placement.pattern],Signed(placement.direction),placement.point.x,placement.point.y};
}
// 원본0040f400은 caller가 이미 비운64BYTE grid를 다시 비우지 않는다.
void RawAiBridgeSearch::Shape(std::uint32_t pattern,std::uint32_t direction,std::span<std::uint8_t> raw) const {
    if (raw.size()!=kShapeBytes) throw std::invalid_argument("AI 다리 모양 실제0xbc 크기");
    auto decoder=Decoder({{1,1},pattern,direction});std::vector<std::array<std::int32_t,2>> cells;
    for (;decoder.Valid();decoder.Advance()) { //실제 decoder가 내는 칸 순서를 끝점 검사에도 유지한다.
        const auto x=Signed(Truncate(decoder.X())),y=Signed(Truncate(decoder.Y()));if (x<1 || x>6 || y<1 || y>6) throw std::out_of_range("AI 다리 모양1..6 좌표");
        raw[static_cast<std::size_t>(x*8+y)]=static_cast<std::uint8_t>(decoder.Side());cells.push_back({x,y});
    }
    Write(raw,0xb8,0);
    for (const auto& cell:cells) { //같은 칸의 북→남 또는 동→서 순서로만 열린 끝점을 기록한다.
        const auto side=raw[static_cast<std::size_t>(cell[0]*8+cell[1])];if (side!='J' && side!='K') continue;
        for (const auto edge:(side=='J' ? std::array<std::uint32_t,2>{0,4} : std::array<std::uint32_t,2>{2,6})) { //J는수직/K는수평 두 끝만 검사한다.
            if (raw[static_cast<std::size_t>((cell[0]+kDx[edge])*8+cell[1]+kDy[edge])]!=0) continue;
            const auto count=Read(raw,0xb8),offset=0x40+count*12;Write(raw,0xb8,count+1);Write(raw,offset,static_cast<std::uint32_t>(cell[0]));Write(raw,offset+4,static_cast<std::uint32_t>(cell[1]));Write(raw,offset+8,edge);
        }
    }
}
// 원본0040f880은 y절삭→x절삭→flat spot 조회 순서다.
bool RawAiBridgeSearch::Clear(const AiBridgePlacement& placement) const {
    auto decoder=Decoder(placement);
    for (;decoder.Valid();decoder.Advance()) { //첫 점유 칸에서 즉시false이며 빈 decoder는true다.
        const auto y=Truncate(decoder.Y()),x=Truncate(decoder.X()),index=y*256U+x;if (index>=spots_.size()) throw std::out_of_range("AI 다리 검색spot 물리 범위");if (spots_[index]!=0) return false;
    }
    return true;
}
// 원본0040f910은 X 절대점은x87에 유지하고 Y 절대점만float에 먼저 저장한다.
std::int32_t RawAiBridgeSearch::Score(const AiBridgePlacement& placement,std::int32_t targetX,std::int32_t targetY,std::span<const std::uint8_t> shape,std::uint32_t excluded,RawPathPoint& endpointScratch) const {
    if (shape.size()!=kShapeBytes) throw std::invalid_argument("AI 다리 끝점 실제0xbc 크기");std::int32_t best=65536;
    for (std::uint32_t index=0;Signed(index)<Signed(Read(shape,0xb8));++index) { //원본 순회와 strict 최소 비교이므로 끝점 자체 동률은 바꾸지 않는다.
        if (index==excluded) continue;const auto offset=0x40+index*12;
        const double x=static_cast<double>(Signed(Read(shape,offset)-1U))+placement.point.x;const float y=static_cast<float>(static_cast<double>(Signed(Read(shape,offset+4)-1U))+placement.point.y);
        const auto dx=Truncate(static_cast<double>(targetX)-x),dy=Truncate(static_cast<double>(targetY)-y);const auto absX=Absolute(dx),absY=Absolute(dy);auto score=Signed(static_cast<std::uint32_t>(absX)+static_cast<std::uint32_t>(absY));
        const auto direction=absX==absY ? -1 : (absX>absY || absX==2) ? (Signed(dx)<=0 ? 6 : 2) : (Signed(dy)<=0 ? 0 : 4);
        const auto edge=Signed(Read(shape,offset+8));
        if (edge!=direction) {
            if (edge==kOpposite[static_cast<std::size_t>(direction+1)] || absX==2 || absY==2) score=65536;
            else { score=Signed(static_cast<std::uint32_t>(score)+4U);if (score<=6) score=6; }
        }
        if (score<best) { best=score;endpointScratch={static_cast<float>(x),y}; }
    }
    return best;
}
// 본체의+d4회전→+d8역순 재고→+dc정순 연결점 상태를 그대로 재개한다.
bool RawAiBridgeSearch::Search(std::uint32_t token,RawPathPoint& endpointScratch) const {
    auto raw=Body(token);int budget=8;
    if (Signed(Read(raw,0x6c8))>=Signed(Read(raw,0x628))) Write(raw,0x6c8,Read(raw,0x628)-1U);
    while (Signed(Read(raw,0x6c4))<=7) { //네 회전이 끝날 때만true이며 예산 중단은 현재 진행 필드를 유지한다.
        while (Signed(Read(raw,0x6c8))>=0) { //재고 마지막 조각부터 첫 조각으로 내려간다.
            while (Signed(Read(raw,0x6cc))<Signed(Read(raw,0x690))) { //같은 회전/조각에 각 연결점을 정순으로 대입한다.
                if (budget<1) return false;const auto candidate=0x648+Read(raw,0x6cc)*12;const auto edge=Direction(Read(raw,candidate+8));const auto desired=kOpposite[edge+1];
                std::array<std::uint8_t,kShapeBytes> shape{};const auto pattern=Read(raw,0x608+Read(raw,0x6c8)*4),rotation=Read(raw,0x6c4);--budget;Shape(pattern,rotation,shape);
                for (std::uint32_t endpoint=0;Signed(endpoint)<Signed(Read(shape,0xb8));++endpoint) { //8개 예산은 끝점 수와 별개이며 같은 모양의 모든 일치 끝점을 검사한다.
                    const auto offset=0x40+endpoint*12;if (Signed(Read(shape,offset+8))!=desired) continue;
                    AiBridgePlacement placement{{static_cast<float>(Signed(Read(raw,candidate)+1U+static_cast<std::uint32_t>(kDx[edge])-Read(shape,offset))),static_cast<float>(Signed(Read(raw,candidate+4)+1U+static_cast<std::uint32_t>(kDy[edge])-Read(shape,offset+4)))},pattern,rotation};
                    if (!Clear(placement)) continue;const auto score=Score(placement,Signed(Read(raw,0x640)),Signed(Read(raw,0x644)),shape,endpoint,endpointScratch);const auto best=Signed(Read(raw,0x6a8));
                    if (score>best || (score==best && random_.Next(100)>=50)) continue;
                    Write(raw,0x6ac,std::bit_cast<std::uint32_t>(placement.point.x));Write(raw,0x6b0,std::bit_cast<std::uint32_t>(placement.point.y));Write(raw,0x6b4,pattern);Write(raw,0x6b8,rotation);Write(raw,0x6a8,static_cast<std::uint32_t>(score));Write(raw,0x6bc,std::bit_cast<std::uint32_t>(endpointScratch.x));Write(raw,0x6c0,std::bit_cast<std::uint32_t>(endpointScratch.y));
                }
                Write(raw,0x6cc,Read(raw,0x6cc)+1U);
            }
            Write(raw,0x6cc,0);Write(raw,0x6c8,Read(raw,0x6c8)-1U);
        }
        Write(raw,0x6c8,Read(raw,0x628)-1U);Write(raw,0x6c4,Read(raw,0x6c4)+2U);
    }
    return true;
}
}
