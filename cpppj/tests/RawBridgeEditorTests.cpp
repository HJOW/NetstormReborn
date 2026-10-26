// 원본 10.78의 전체 다리 품질 명령을 독립 CPU fixture의 전체 슬롯·표시·난수로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBridgeEditor.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 재생용 합성 입력만 준비하며 기대 출력은 보호 PE fixture에서 읽는다.
// 원문 iterator의 시작 전 슬롯과 client/server 경계 및 비다리 슬롯이다.
constexpr std::array<std::uint16_t,8> kEditorObservedSids{1,5,6,7,8,9,10,15000};
// 독립 원문 입력과 같은 방향·번호·겹치는 Hard 플래그를 제공한다.
RiftTypeFrames EditorBridgeFrames() {
    return RiftTypeFrames({{'A','P',1,0},{'A','P',11,32},{'A','P',20,64},{'A','P',21,96},
        {'B','P',1,0},{'B','P',11,32},{'B','P',20,64},{'C','P',1,0}});
}
// 원문 fixture에 저장된 실제50BYTE를 값의 재계산 없이 해석한다.
std::vector<std::uint8_t> EditorBridgeHex(const std::string& text) {
    if (text.size()%2) throw std::logic_error("편집 다리 원문 hex 폭 오류");std::vector<std::uint8_t> bytes;
    // 한 원문 BYTE를 두 hex 글자로 복원한다.
    for (std::size_t i=0;i<text.size();i+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(i,2),nullptr,16)));
    return bytes;
}
// 제품의 할당 경로와 달리 예약SID1까지 포함한 원문 CPU 입력을 검사 저장소에 명시 설치한다.
void PrepareEditorBridgePool(SidPool& pool,std::uint32_t pattern) {
    auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Bytes().data()),pool.Bytes().size()); // 실제 가변 검사 풀에만 원문 입력을 쓰며 제품 API/원본 파일은 변경하지 않는다.
    std::fill(raw.begin(),raw.end(),std::uint8_t{});
    // 원문과 동일하게 모든 슬롯의 free bit만 켠다. SID1도 아래 명시 입력에서 비free로 바꾼다.
    for (std::uint32_t sid=0;sid<pool.Capacity();++sid) raw[sid*50U+11]=1;
    const std::array<std::uint32_t,8> frames{0,0,1,2,4,7,0,3}; // native 입력의 각 SID 현재 프레임이다.
    // 지정8개 슬롯의 원문 VT/TYPE/상태/WORD/프레임만 직접 준비한다.
    for (std::size_t index=0;index<kEditorObservedSids.size();++index) {
        const auto offset=static_cast<std::size_t>(kEditorObservedSids[index])*50U;Put(raw,offset,0x5034c8U);
        raw[offset+10]=static_cast<std::uint8_t>(kEditorObservedSids[index]==10 ? 70 : 82);
        raw[offset+11]=static_cast<std::uint8_t>(kEditorObservedSids[index]==6 ? 6 : 4);
        Put(raw,offset+12,0xa5ffU^pattern,2);Put(raw,offset+36,frames[index]);
    }
}
}
// mode0/1/2와 미지원 정수 두 개·seed3개·WORD2개를 원본30행 그대로 재생한다.
TEST_CASE(RawBridgeEditor_ReplaysNativeWholePoolTraceAndRandom) {
    const auto fixture=LoadFixture(NETSTORM_EDITOR_BRIDGES_FIXTURE);std::size_t checked=0;
    // 첫 표제 행 외에는 기대값을 고치거나 특정 SID를 생략하지 않는다.
    for (const auto& row:fixture.rows) {
        if (row[0]=="mode") continue;CHECK(row.size()==6);
        SidPool pool(OriginalEdition::Patch1078,24000);PrepareEditorBridgePool(pool,Number(row[2]));
        const std::uint32_t bridge=82;const std::int32_t crack=5;GameRandom random(Number(row[1]));const auto frames=EditorBridgeFrames();
        auto expected=std::vector<std::uint8_t>(pool.Bytes().begin(),pool.Bytes().end());std::string trace;std::uint32_t notified=0;
        const auto freeCount=pool.FreeCount();BridgeEditorHooks hooks;
        hooks.frames=[&](auto type)->const RiftTypeFrames& { CHECK(type==82);return frames; };
        hooks.notifySurface=[&](Sid sid) { CHECK(sid.value!=1 && sid.value!=10);++notified; };
        hooks.refresh=[&](Sid sid) { if (!trace.empty()) trace+=';';const auto raw=pool.Slot(sid);trace+=std::to_string(sid.value)+':'+std::to_string(Get(raw,12,2))+':'+std::to_string(Get(raw,36)); };
        RawBridgeEditor(pool,bridge,crack,random,std::move(hooks)).SetAll(Number(row[0]));
        // native가 관찰한8개 전체 슬롯을 원문 바이트로 기대 풀에 반영하고 나머지23992개도 보존 검사한다.
        for (const auto& item:Split(row[3],';')) {
            const auto fields=Split(item,':');CHECK(fields.size()==2);const auto sid=Number(fields[0]);const auto bytes=EditorBridgeHex(fields[1]);CHECK(bytes.size()==50);
            std::copy(bytes.begin(),bytes.end(),expected.begin()+static_cast<std::ptrdiff_t>(sid*50U));
        }
        const bool same=std::equal(expected.begin(),expected.end(),pool.Bytes().begin()) && trace==row[4] && random.State()==Number(row[5]);CHECK(same);
        if (!same) { std::fprintf(stderr,"EditorBridge mode=%s seed=%s pattern=%s trace=%s/%s random=%u/%s\n",row[0].c_str(),row[1].c_str(),row[2].c_str(),trace.c_str(),row[4].c_str(),random.State(),row[5].c_str());break; }
        CHECK(notified==(Number(row[0])<=2 ? 6U : 0U));CHECK(pool.FreeCount()==freeCount);++checked;
    }
    CHECK(checked==30);
}
// 원문 쓰기 뒤 실패는 이미 쓴 수명만 유지하며 미지원 정수는 미연결 효과를 요구하지 않는다.
TEST_CASE(RawBridgeEditor_PreservesPartialWriteAndUnsupportedNoop) {
    SidPool pool(OriginalEdition::Patch1078,24000);PrepareEditorBridgePool(pool,0);const std::uint32_t bridge=82;const std::int32_t crack=5;GameRandom random(1);
    const auto before=std::vector<std::uint8_t>(pool.Bytes().begin(),pool.Bytes().end());RawBridgeEditor editor(pool,bridge,crack,random,{});
    editor.SetAll(0xffffffffU);CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));CHECK(random.State()==1);
    CHECK(Throws([&] { editor.SetAll(0); }));CHECK(Get(pool.Slot(Sid{5}),12,2)==0xa5a7U);CHECK(Get(pool.Slot(Sid{5}),36)==0);CHECK(Get(pool.Slot(Sid{1}),12,2)==0xa5ffU);
}
