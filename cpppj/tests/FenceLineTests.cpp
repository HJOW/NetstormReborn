// 원본 mode0 검색점과 현재 프레임/가상 삭제 효과를 검증한다.
#include "RawSceneSupport.h"
#include "o/RawFenceLine.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 좌표 보정과 끝점/100회 상한은 두 원본 정밀도에서 같은99개 입력으로 전수 대조한다.
TEST_CASE(FenceLine_Native99) {
    const auto fixture=LoadFixture(NETSTORM_FENCE_LINE_FIXTURE);CHECK(fixture.rows.size()==100 && fixture.rows.front()[0]=="input");
    for (std::size_t index=1;index<fixture.rows.size();++index) { // 각 독립 네float 입력을 그대로 재생한다.
        const auto& row=fixture.rows[index];CHECK(row.size()==2);const auto input=Split(row[0],',');CHECK(input.size()==4);std::array<float,4> endpoints{}; // 원본 인자 비트다.
        for (std::size_t coordinate=0;coordinate<4;++coordinate) endpoints[coordinate]=std::bit_cast<float>(Number(input[coordinate])); // 계산 없이 binary32로 복원한다.
        const auto points=RawFenceLine::SearchPoints(endpoints);std::string text; // 제품의 실제 순서만 기록한다.
        for (const auto& point:points) { if (!text.empty()) text+=';';text+=std::to_string(point[0])+','+std::to_string(point[1]); } // 원문 검색점과 순서를 전수 대조한다.
        CHECK(text==row[1]);
    }
}
// 현재 해시/방향 코드가 맞는 mark만 삭제하며 누락한 삭제 소비자는 선택 위치에서 거부한다.
TEST_CASE(FenceLine_CurrentHashFrameAndRequiredDeletion) {
    SidPool pool{OriginalEdition::Patch1078,24000,false};SquidHash hash; // 같은 실제 입력 슬롯/해시다.
    std::vector<RiftTypeRecord> types(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::uint32_t markType=170; //실제00541300은 포스 필드170이며 내부 fencemark90과 다르다.
    types[170].footX=types[170].footY=1;types[90].footX=types[90].footY=1;frames[170]=RiftTypeFrames({{'J',0,0,0},{'K',0,0,0}});
    for (;;) { const auto sid=pool.Allocate(2);if (sid.value==53) break;if (!sid.value || sid.value>53) throw std::logic_error("펜스 선 검사 슬롯 확보 실패"); } // 네 후보를 실제 할당한다.
    for (std::uint16_t number=50;number<=53;++number) { // 세mark와 타입이 다른 객체를 실제 해시에 등록한다.
        auto raw=pool.AllocatedBytes(Sid{number});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=number==53 ? 90 : 170;Put(raw,14,std::bit_cast<std::uint32_t>(49.0f));Put(raw,18,std::bit_cast<std::uint32_t>(40.0f+number-50));Put(raw,36,number==51 ? 1U : 0U);
        auto& head=hash.Bucket(1,49,40.0f+number-50);Put(raw,4,head,2);head=number;
    }
    std::vector<Sid> removed; // 외부 가상 삭제의 실제 요청을 관찰한다.
    RawFenceLine body(pool,hash,types,frames,markType,[&](Sid sid,std::uint32_t flags) { CHECK(flags==0x40);removed.push_back(sid);pool.AllocatedBytes(sid)[40]|=8; });
    body.Clear({50,40,50,48});CHECK(removed.size()==2 && removed[0].value==50 && removed[1].value==52);CHECK(pool.Slot(Sid{51})[40]==0 && pool.Slot(Sid{53})[40]==0);
    pool.AllocatedBytes(Sid{50})[40]=0;RawFenceLine missing(pool,hash,types,frames,markType,{});CHECK(Throws([&] { missing.Clear({50,40,50,48}); }));
}
