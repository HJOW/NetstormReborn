// 원본 Graph 초기 표의 앞251행/예약0/틈3행/sentinel/reserved 보존을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGraph.h"
#include <cstring>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 다양한 전체1530BYTE 입력을 원본16개 출력과 대조하고 풀·스택 비변경도 확인한다.
TEST_CASE(GraphReset_Native16AndUnrelatedStorage) {
    const auto fixture=LoadFixture(NETSTORM_GRAPH_RESET_FIXTURE);CHECK(fixture.rows.size()==17 && fixture.rows.front()[0]=="seed");
    for (std::size_t rowIndex=1;rowIndex<fixture.rows.size();++rowIndex) { // 이전 표의 출력은 다음 초기 입력에 재사용하지 않는다.
        const auto& row=fixture.rows[rowIndex];CHECK(row.size()==2);const auto seed=Number(row[0]);std::array<std::uint8_t,1530> input{}; // 독립 관찰기에 공급한 동일 입력 패턴이다.
        for (std::size_t byte=0;byte<input.size();++byte) input[byte]=static_cast<std::uint8_t>(byte*37+seed); // reserved/gap도 서로 다른 바이트로 채운다.
        std::array<GraphRecord,Graph::kTableSize> records{};static_assert(sizeof(records)==1530);std::memcpy(records.data(),input.data(),input.size());
        SidPool pool{OriginalEdition::Patch1078,24000,false};SquidHash hash;std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeRecord> types(188);std::vector<std::vector<FrameCode>> frames(188); // 같은 물리 저장소의 크기 계약이다.
        std::vector<std::uint32_t> stack(Graph::kFloodSize,0x12345678);RawGraph graph(pool,hash,spots,types,frames,records,stack);const auto before=Hex(pool.Slot(Sid{5}));graph.ResetTable();
        const auto output=graph.Records();CHECK(Hex({reinterpret_cast<const std::uint8_t*>(output.data()),output.size_bytes()})==row[1]);
        CHECK(Hex(pool.Slot(Sid{5}))==before && std::ranges::all_of(graph.FloodStack(),[](std::uint32_t value) { return value==0x12345678; }));
    }
}
