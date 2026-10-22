// 원본 함수의 집계·순회·24칸 중단·signed 상태 감김 순서를 그대로 유지한다.
#include "o/RawAiBridgeCandidates.h"
#include "o/TerrainBuilder.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kBudget=24,kCapacity=6,kCandidates=0x648; //원본 호출 예산·후보 용량·Brain+5f0+58의 실제 배열 시작이다.
// DWORD의 signed 비교/산술 시프트를 호스트 signed overflow 없이 수행한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// raw 슬롯/본체의 비정렬 DWORD를 원본 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 다리 후보 DWORD 읽기 범위");std::uint32_t result{};
    for (std::size_t index=0;index<4;++index) result|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8); //지정한 네 바이트만 합친다.
    return result;
}
}
// 원본 준비는 다른 판본의 슬롯/TYPE 배치에서 실행하지 않는다.
RawAiBridgeCandidates::RawAiBridgeCandidates(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,std::span<const std::uint16_t> surfaces,std::span<const std::uint8_t> spots,AiBridgeCandidateTypes globals)
    :memory_(memory),pool_(pool),types_(types),frames_(frames),surfaces_(surfaces),spots_(spots),globals_(globals) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=188 || surfaces.size()!=65536 || spots.size()!=65536) throw std::invalid_argument("AI 다리 후보의 실제 PATCH188/지도 저장소 누락");
}
// 크기 없는 가짜 본체나 반납된 핸들을 처리하지 않는다.
std::span<std::uint8_t> RawAiBridgeCandidates::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 다리 후보의 실제0xb08 본체 누락");return raw; }
// 원본이 각 위치에서 다시 읽는 현재 필드를 캐시하지 않는다.
std::uint32_t RawAiBridgeCandidates::Field(std::uint32_t token,std::uint32_t offset) const { return Read(Body(token),offset); }
// 음수 후보 번호도 원본 DWORD 주소 감김 뒤 실제 본체 안이면 그 바이트를 쓴다.
void RawAiBridgeCandidates::Field(std::uint32_t token,std::uint32_t offset,std::uint32_t value) const {
    auto raw=Body(token);if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 다리 후보 DWORD 쓰기 범위");
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); //주변 필드는 그대로 보존한다.
}
// x255의 오른쪽처럼 같은 flat 지도 안의 인접 주소는 원본대로 다음 행을 읽는다.
std::uint8_t RawAiBridgeCandidates::Spot(std::int32_t x,std::int32_t y) const {
    const auto index=static_cast<std::int64_t>(y)*256+x;if (index<0 || index>=static_cast<std::int64_t>(spots_.size())) throw std::out_of_range("AI 다리 후보 spot 물리 범위");return spots_[static_cast<std::size_t>(index)];
}
// free/dead/void나 소유자 필터를 getter에 추가하지 않는다.
std::uint32_t RawAiBridgeCandidates::Graph(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto type=raw[10];if (type>=types_.size() || (!(types_[type].flags1&0x800U) && type!=static_cast<std::uint8_t>(globals_.fencemark) && type!=static_cast<std::uint8_t>(globals_.windArcher))) throw std::invalid_argument("AI 다리 후보 graph getter 타입 오류");return raw[30];
}
// 현재 코드의 A~P 연결표는 지면 생성과 같은 원본0052f910을 공유한다.
std::uint32_t RawAiBridgeCandidates::Connections(Sid sid) const {
    const auto raw=pool_.Slot(sid);const auto type=raw[10];const auto frame=Read(raw,36);if (type<70 || type>=frames_.size() || frame>=frames_[type].Codes().size()) throw std::out_of_range("AI 다리 후보의 현재 프레임 범위");return static_cast<std::uint32_t>(TerrainBuilder::Connection(static_cast<char>(frames_[type].Codes()[frame].side)));
}
// helper 효과가 없는 실제 읽기 순서와 후보 기록 뒤 나선 진행을 보존한다.
bool RawAiBridgeCandidates::Prepare(std::uint32_t token) const {
    const auto graph=Field(token,0x5f8),owner=Field(token,0x5fc);std::uint32_t budget=kBudget; //원본은 진입 시 그래프/소유자 두 DWORD만 스택에 보관한다.
    for (;;) { //한 나선 변이 끝날 때만 바깥 종료 조건을 다시 검사한다.
        const auto x=Signed(Field(token,0x6a0)),y=Signed(Field(token,0x6a4)),stage=Signed(Field(token,0x698));
        if (x<=10 || y<=10 || x>=246 || y>=246 || stage>=24 || (stage>=8 && Signed(Field(token,0x690))>=3)) return true;
        while (Field(token,0x69c)!=0) { //현재 변의 나머지는24칸 예산이 소진될 때까지 이어간다.
            if (!budget) return false;--budget;
            const auto px=Signed(Field(token,0x6a0)),py=Signed(Field(token,0x6a4));std::uint16_t number{}; //원본 map 조회는 두 축0..255를 먼저 검사한다.
            if (px>=0 && py>=0 && px<256 && py<256) number=surfaces_[static_cast<std::size_t>(py)*256+static_cast<std::size_t>(px)];
            if (number) {
                const Sid sid{number};if (Graph(sid)==graph && pool_.Slot(sid)[34]==owner && (!Spot(px-1,py) || !Spot(px+1,py) || !Spot(px,py-1) || !Spot(px,py+1))) {
                    const bool bridge=static_cast<std::uint32_t>(pool_.Slot(sid)[10])==globals_.bridge;const auto connections=Connections(sid);
                    for (std::uint32_t direction=0;direction<8;direction+=2) { //원본은 북/동/남/서만 이 순서로 최대6개 저장한다.
                        const auto count=Field(token,0x690);if (Signed(count)>=static_cast<std::int32_t>(kCapacity)) break;
                        const auto [dx,dy]=kNeighborCells[direction];if (((connections&(1U<<(direction/2)))!=0)==bridge && !Spot(px+dx,py+dy)) {
                            Field(token,0x690,count+1U);const auto at=kCandidates+count*12U; //원본 DWORD 곱셈/주소 감김과 count 증가 선행을 유지한다.
                            Field(token,at,Field(token,0x6a0));Field(token,at+4U,Field(token,0x6a4));Field(token,at+8U,direction);
                        }
                    }
                }
            }
            const auto direction=Field(token,0x694);if (direction>=kNeighborCells.size()) throw std::out_of_range("AI 다리 후보 나선 방향 범위");const auto [dx,dy]=kNeighborCells[direction]; //현재 변의 방향은 후보 방향과 다른 필드다.
            Field(token,0x6a0,Field(token,0x6a0)+static_cast<std::uint32_t>(dx));Field(token,0x6a4,Field(token,0x6a4)+static_cast<std::uint32_t>(dy));Field(token,0x69c,Field(token,0x69c)-1U);
        }
        const auto direction=static_cast<std::uint32_t>(Signed(Field(token,0x694)+2U)%8);const auto stageBits=Field(token,0x698)+1U; //나선 회전은 signed 나머지, 변 길이는 signed 산술시프트다.
        Field(token,0x694,direction);Field(token,0x698,stageBits);Field(token,0x69c,static_cast<std::uint32_t>(Signed(stageBits)>>1));
    }
}
// 공유 세계의 주소를 노출해 연결 계층이 같은 풀인지 확인한다.
const SidPool& RawAiBridgeCandidates::Pool() const { return pool_; }
}
