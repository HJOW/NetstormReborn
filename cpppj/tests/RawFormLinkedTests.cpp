//원문 Form 연결 전체120입력과 호스트의 현재 풀 대여/순환 경계를 구별한다.
#include "TestSupport.h"
#include "RawSceneSupport.h"
#include "o/RawFormLinked.h"
#include <array>
#include <fstream>

using namespace netstorm::o;
namespace { //기대값 대신 원문에 공급한 체인 입력만 보관한다.
const std::array<std::vector<std::uint16_t>,12> kFormChains{{{},{20},{50},{20,50},{50,20},{20,30,50,63},{20,64},{50,64},{64},{65535},{20,50,65535},{63,30,20}}}; //논리 범위64 전후와 자기 뒤 실패 입력이다.
void PutFormWord(std::span<std::uint8_t> bytes,std::uint16_t sid,std::size_t offset,std::uint16_t value) { const auto at=static_cast<std::size_t>(sid)*50+offset;bytes[at]=static_cast<std::uint8_t>(value);bytes[at+1]=static_cast<std::uint8_t>(value>>8); } //소유 입력의 지정 WORD만 쓴다.
std::span<std::uint8_t> FormPoolInput(SidPool& pool) { const auto bytes=pool.Bytes();return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; } //검사 소유 풀의 물리 입력만 수정하며 원본 파일과 무관하다.
template<class Function> bool FormFailed(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; } //정상 false와 명시 호스트 중단을 구별한다.
}
TEST_CASE(RawFormLinked_Native120WholeChainAndReadOnlyBytes) { //자기를 찾은 뒤에도 범위를 검사하며 타입/상태 필터를 더하지 않는다.
    using namespace netstorm::test::rawscene;std::ifstream file(NETSTORM_FORM_LINKED_FIXTURE);CHECK(file.good());std::string line;std::getline(file,line);std::size_t rows{};
    while (std::getline(file,line)) { //원문 고유 입력120개를 전부 대조한다.
        const auto row=Split(line,'\t');CHECK(row.size()==4);if (row.size()!=4) return;const auto p=Split(row[0],',');CHECK(p.size()==3);if (p.size()!=3) return;const Sid form{static_cast<std::uint16_t>(Number(p[0]))},parent{static_cast<std::uint16_t>(Number(p[1]))};const auto& chain=kFormChains.at(Number(p[2]));std::vector<std::uint8_t> bytes(64*50);
        PutFormWord(bytes,parent.value,6,chain.empty() ? 0 : chain[0]);for (std::size_t index=0;index<chain.size();++index) if (chain[index]<64) PutFormWord(bytes,chain[index],4,index+1<chain.size() ? chain[index+1] : 0); //논리 밖 번호는 next/head 값으로만 배치한다.
        const auto before=bytes;const bool result=CheckRawFormChain(bytes,50,64,form,parent);CHECK(std::to_string(result)==row[1] && row[2]=="P:"+std::to_string(form.value)+":"+std::to_string(parent.value) && row[3]=="1" && bytes==before);++rows;
    }
    CHECK(rows==120);
}
TEST_CASE(RawFormLinked_CurrentPoolAfterParentAndExplicitCycle) { //가상 부모 반환 중 풀 재확보 뒤의 현재 물리와 명시 순환 오류를 확인한다.
    SidPool pool(OriginalEdition::Patch1078,32768);std::size_t calls{};const auto parent=[&](Sid form) { CHECK(form==Sid{50});++calls;pool.Reset(32770);auto raw=FormPoolInput(pool);PutFormWord(raw,1,6,50);PutFormWord(raw,50,4,0);return Sid{1}; };
    CHECK(CheckRawFormLinked(pool,Sid{50},parent) && calls==1 && pool.Capacity()==32770);auto raw=FormPoolInput(pool);PutFormWord(raw,50,4,50);const auto fixed=[](Sid) { return Sid{1}; };CHECK(FormFailed([&] { CheckRawFormLinked(pool,Sid{50},fixed); })); //원문 비반환 고리는 새 호스트에서 별도로 중단한다.
    PutFormWord(raw,50,4,32770);CHECK(!CheckRawFormLinked(pool,Sid{50},fixed));CHECK(FormFailed([&] { CheckRawFormLinked(pool,Sid{50},{}); }));
}
TEST_CASE(RawFormLinked_ProcessGetterKeepsWordAndIgnoresStateType) { //양판본 알려진 가상 getter는 원문처럼 부모 WORD 전체만 반환한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { //실제5/7BYTE leaf와 동일한 두 물리 배치만 사용한다.
        SidPool pool(edition,32768);const auto bytes=FormPoolInput(pool);const auto stride=pool.Layout().stride;auto raw=bytes.subspan(50U*stride,stride);const auto table=edition==OriginalEdition::Patch1078 ? 0x512098U : 0x505990U; //생성자의 raw 가상 표 토큰이다.
        for (std::size_t index=0;index<4;++index) raw[index]=static_cast<std::uint8_t>(table>>(index*8)); //호스트의 실제 포인터를 쓰지 않는다.
        raw[10]=0;raw[11]=0xff;raw[16]=0xcd;raw[17]=0xab;CHECK(ReadRawProcessFormParent(pool,Sid{50})==Sid{0xabcd});raw[0]=0;CHECK(FormFailed([&] { ReadRawProcessFormParent(pool,Sid{50}); })); //범위가 확인되지 않은 다른 가상 표는 임의 부모 값으로 대체하지 않는다.
    }
}
