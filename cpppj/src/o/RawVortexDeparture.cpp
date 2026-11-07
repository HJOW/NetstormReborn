// 섬 상승의 지형 기록·현재 공간 순회·파생 삭제를 일반 DropIsland와 구별한다.
#include "o/RawVortexDeparture.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // PATCH 실제 신전/섬 부모의 상태 변경 순서를 보존한다.
namespace { // 원문 비정렬 읽기/정수 변환/선택 장치 검사를 이 모듈에 한정한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("신전 섬 상승의 실제 장치 미연결");return function; } // 효과 직전에만 장치를 요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { // 지정한 물리 바이트만 합친다.
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("신전 상승의 실제 필드 범위");std::uint32_t result{};
    for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[at+byte])<<(byte*8);return result; // 현재 폭만 little endian으로 읽는다.
}
std::int32_t Cell(float value) { // _ftol의 하위 DWORD를 보존하며 유한 호스트 정수 범위 밖은 거부한다.
    const double wide=value;if (!(wide>=-9223372036854775808.0 && wide<9223372036854775808.0)) throw std::out_of_range("신전 상승 좌표의 CRT 변환 범위");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(wide)));
}
}
RawVortexDeparture::RawVortexDeparture(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    ChunkMap& chunks,RawIslandRuntime& islands,const RawVortexAscend& ascend,
    const std::uint32_t& localOwner,const bool& server,std::uint32_t& revision,VortexDepartureHooks hooks)
    :pool_(pool),hash_(hash),types_(types),chunks_(chunks),islands_(islands),ascend_(ascend),localOwner_(localOwner),server_(server),revision_(revision),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || &islands.Pool()!=&pool || &ascend.Pool()!=&pool) throw std::invalid_argument("신전 섬 상승의 같은 PATCH World 필요");
}
std::uint32_t RawVortexDeparture::Genus(Sid sid) const { const auto type=pool_.Slot(sid)[10];if (type>=types_.size()) throw std::out_of_range("신전 상승의 현재 TYPE 범위");return types_[type].flags2; } // 첫 TYPE 사본을 후속 효과에 재사용하지 않는다.
std::array<float,2> RawVortexDeparture::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; } // 원문 float32 입력이다.
std::int32_t RawVortexDeparture::IslandAt(float x,float y) const { return std::bit_cast<std::int8_t>(chunks_.At(Cell(x)/16,Cell(y)/16).island); } // C++ signed 나눗셈도 원문처럼0방향으로 자른다.
bool RawVortexDeparture::AdditionalCandidate(Sid sid,std::int32_t island) const { // 첫 genus는 추가 좌표 조회 후에도 보관한다.
    const auto genus=Genus(sid);
    if (genus&0x1000U) { const auto point=Position(sid);return IslandAt(point[0],static_cast<float>(static_cast<double>(point[1])-4.0))==island; }
    if (!(genus&0x28a300U)) return false;
    const auto point=Position(sid);bool found=Need(hooks_.islandNumber)(point[0],point[1])==island;
    if (genus&0x80000U) found=IslandAt(point[0],point[1])==island; // 첫0046f530 뒤의 직접 청크 조회도 생략하지 않는다.
    return !(genus&0x1000000U) && found;
}
void RawVortexDeparture::Surfaces(std::int32_t island,bool neighbors) const { //004db3e0의 현재커서는 반환 SID의x보다1큰 값을 보관한다.
    std::int32_t x{},y{};const auto surface=hash_.Entries(0);
    const auto at=[&](std::int32_t px,std::int32_t py)->Sid { if (px<0 || px>255 || py<0 || py>255) return {};return Sid{surface[static_cast<std::size_t>(py)*256+px]}; }; // 범위 밖 표면은 원문의 SID0이다.
    const auto next=[&]()->Sid { // 고정된 목록 사본 없이 현재 청크/해시를 다시 읽는다.
        Sid current{};
        do {
            if (std::bit_cast<std::int8_t>(chunks_.At(x/16,y/16).island)==island) { current=at(x,y);++x; }
            else x+=16;
            if (x>255) { x=0;++y; }
        } while (!current.value && y<256);
        return current;
    };
    for (auto current=next();current.value;current=next()) { // 삭제가 다음 청크/표면을 바꾸면 다음 조회에 반영한다.
        const auto genus=Genus(current);
        if (neighbors) {
            if (!(genus&2U)) continue;
            const std::array<std::array<std::int32_t,2>,4> offsets{{{-1,0},{1,0},{0,-1},{0,1}}}; // 원문은 보관된 다음x를 기준으로 네 방향을 조회한다.
            for (const auto& offset:offsets) { const auto candidate=at(x+offset[0],y+offset[1]);if (Genus(candidate)&4U) Need(hooks_.destroy)(candidate,0x100048); } // 각 삭제 뒤 다음 이웃의 현재 genus를 읽는다.
        } else if ((genus&0x1000002U)==2U) Need(hooks_.destroy)(current,0x848);
    }
}
void RawVortexDeparture::Remove(Sid temple,std::int32_t island) const { //004dbce0의 고정 신전 주소와 조건부 자기 저장을 구별한다.
    if (pool_.Slot(temple)[11]&1U) Need(hooks_.assertion)("!vortex.isFree()","\\Ns\\O\\Vortex.cpp",0xea);
    Need(hooks_.state)(temple,3);bool mine{};
    if (pool_.Slot(temple)[34]==localOwner_) {
        mine=true;const auto point=Position(temple);auto record=chunks_.At(Cell(point[0])/16,Cell(point[1])/16).record;
        if (!(record[1]&1U)) Need(hooks_.assertion)("newTerr.exists","\\Ns\\O\\Vortex.cpp",0xf7);
        std::array<std::uint8_t,120> records{};const auto owner=pool_.Slot(temple)[34];Need(hooks_.readTerritory)(owner,records);
        if (!(record[1]&4U)) {
            record[1]=static_cast<std::uint8_t>((record[1]&0xf7U)|2U);
            for (std::size_t index=0;index<20;++index) { if (!(records[index*6+1]&1U)) { std::copy(record.begin(),record.end(),records.begin()+index*6);break; } } // 첫 빈 기록만 새 섬으로 채우며 없으면 그대로 저장한다.
        } else {
            record[1]&=0xfbU;
            for (std::size_t index=0;index<20;++index) if (records[index*6+1]&4U) records[index*6+1]&=0xfbU; // 기존20개 home 비트만 지운다.
        }
        Need(hooks_.writeTerritory)(owner,records);
        if (!islands_.Exists(island)) Need(hooks_.assertion)("islandList.exists(islandId)","\\Ns\\O\\Chunkmap.cpp",0x139);
        for (int cy=0;cy<chunks_.YLength();++cy) { // 현재 y 행을 먼저 순회한다.
            for (int cx=0;cx<chunks_.XLength();++cx) { auto& entry=chunks_.At(cx,cy);if (std::bit_cast<std::int8_t>(entry.island)==island) entry.record=record; } // 같은 섬의6BYTE만 바꾼다.
        }
        Need(hooks_.refreshProduction)();Need(hooks_.snapshot)();
    }
    Need(hooks_.owner)(temple,0);Surfaces(island,true);
    if (!islands_.Exists(island)) Need(hooks_.assertion)("islandList.exists(islandId)","\\Ns\\O\\Chunkmap.cpp",0xc2);
    const auto bounds=chunks_.Bounds(island);RawSquidFinder finder(pool_,hash_,types_);
    auto current=finder.Begin({bounds.left*16,bounds.top*16,bounds.right*16,bounds.bottom*16+4});
    while (current.value) { const auto sid=current;current=finder.Next();if (ascend_.Candidate(sid,island,9999) || AdditionalCandidate(sid,island)) Need(hooks_.destroy)(sid,0x848); } // 원문은 후보 효과 전에 다음 SID를 확보한다.
    Surfaces(island,false);islands_.Delete(island);++revision_;
    if (mine && !Need(hooks_.ownedIslands)(localOwner_)) Need(hooks_.requestEdit)();
}
void RawVortexDeparture::Receive(std::int32_t sender,std::span<const std::uint8_t> packet) const { //004dc050은 처음 선택한 신전 주소를 마지막 owner 조회까지 유지한다.
    if (packet.size()<8) throw std::out_of_range("신전51h 수신8BYTE 부족");const Sid original{static_cast<std::uint16_t>(Read(packet,5,2))};
    if (pool_.Slot(original)[11]&1U) Need(hooks_.assertion)("!vortex.isFree()","\\Ns\\O\\Vortex.cpp",0x14c);
    if (!islands_.Exists(packet[4])) { std::array<char,64> text{};std::snprintf(text.data(),text.size(),"Can't ascend island %d\n",packet[4]);Need(hooks_.log)(text.data());return; }
    if (packet[7]) { std::array<char,64> text{};std::snprintf(text.data(),text.size(),"Removing isle %d\n",packet[4]);Need(hooks_.log)(text.data());Remove(Sid{static_cast<std::uint16_t>(Read(packet,5,2))},packet[4]);return; }
    if (server_) Need(hooks_.sendExcept)(sender,packet);
    Need(hooks_.regular)(Sid{static_cast<std::uint16_t>(Read(packet,5,2))});const auto owner=pool_.Slot(original)[34];
    if (owner!=localOwner_) { Need(hooks_.warnAscend)(owner);Need(hooks_.sound)("enemyIslandPreparing.wav");Need(hooks_.sound)("islandRise.wav");return; }
    Need(hooks_.sound)("preparingAscend.wav");
}
}
