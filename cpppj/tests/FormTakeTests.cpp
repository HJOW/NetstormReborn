// 두 실제 PE의 저장 관찰과 공유 Form 처리기의 기존 UI/프로세스 연결 및 오류 계약을 검사한다.
#include "TestSupport.h"
#include "o/SquidForms.h"
#include "o/GumpFormHost.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 원본 관찰 도구와 같은 초기 풀 크기 및 두 판본의 저장 사례 수다.
constexpr std::uint32_t kCapacity=32768,kCasesPerEdition=300;
// 마지막 빈 열도 포함하여 TSV/슬롯/통지 문자열을 분리한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> fields;std::size_t begin=0;
    // 구분자가 더 없으면 마지막 빈 문자열까지 반환한다.
    for (;;) {
        const auto end=text.find(separator,begin);fields.emplace_back(text.substr(begin,end==std::string_view::npos ? end : end-begin));
        if (end==std::string_view::npos) return fields;
        begin=end+1;
    }
}
// 전체 raw 슬롯을 소문자 16진수로 표현하여 fixture의 바이트에 직접 대조한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    // 원본 관찰의 두 자리 소문자 16진수 표다.
    constexpr char digits[]="0123456789abcdef";std::string result;
    // 모든 바이트를 축약하지 않고 기록한다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }
    return result;
}
// fixture의 명시적인 합성 초기 바이트만 놓는다. 생성/수신의 기대 결과는 PE 출력에서 읽는다.
void Input(SidPool& pool,const std::string& slots) {
    if (slots.empty()) return;
    // 입력 번호가 free여도 원본 Take를 검사해야 하므로 테스트에서만 실제 mutable 저장소를 직접 초기화한다.
    for (const auto& slot:Split(slots,';')) {
        const auto field=Split(slot,':');const Sid sid{static_cast<std::uint16_t>(std::stoul(field.at(0)))};
        const auto raw=pool.Slot(sid);CHECK(field.at(1).size()==raw.size()*2);
        auto* data=const_cast<std::uint8_t*>(raw.data());
        // 실제 수신 후의 결과를 만들지 않고 fixture에 저장된 초기 오염 바이트만 복원한다.
        for (std::size_t index=0;index<raw.size();++index)
            data[index]=static_cast<std::uint8_t>(std::stoul(field[1].substr(index*2,2),nullptr,16));
    }
}
// 표준 Adler-32는 watched 슬롯 밖의 전체 풀 변화까지 원본 출력과 대조한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // Python zlib와 같은 소수 및 넘침 방지 블록 크기다.
    constexpr std::uint64_t prime=65521;std::uint64_t a=1,b=0;std::size_t block=0;
    // raw 풀의 실제 순서로 모든 바이트를 누적한다.
    for (const auto byte:bytes) { a+=byte;b+=a;if (++block==4096) { a%=prime;b%=prime;block=0; } }
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
// 정렬되지 않은 Form WORD/DWORD를 검사할 때 같은 필드 폭으로 읽는다.
std::uint32_t Read(const SidPool& pool,Sid sid,std::size_t offset,std::size_t width) {
    const auto raw=pool.Slot(sid);std::uint32_t value=0;
    // 이웃 필드를 섞지 않고 little endian으로 조립한다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);
    return value;
}
// 수명 연결 검사에서 실제 할당 슬롯의 명시적인 부모/링크/payload 입력을 설정한다.
void Put(SidPool& pool,Sid sid,std::size_t offset,std::uint32_t value,std::size_t width) {
    auto raw=pool.AllocatedBytes(sid);
    // 지정 필드만 바꿔 보존 대상 payload를 함께 검사한다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 보호 계약의 오류가 발생하는지 확인한다. 성공을 무조건 인정하지 않는다.
template<class Function> bool Throws(Function function) {
    try { function(); } catch (const std::exception&) { return true; }return false;
}
}

// 생성/Take/Unpop/재확보의 실제 원본 600입력과 raw 전체·콜백 당시 상태·풀 메타데이터를 대조한다.
TEST_CASE(FormTake_IndependentPeFixture) {
    std::ifstream input(NETSTORM_FORMTAKE_FIXTURE);CHECK(input.good());
    if (!input) throw std::runtime_error("Form Take fixture 없음");
    std::string line;std::array<std::uint32_t,2> counts{};
    // 각 입력은 새로운 C++ 풀에서 실행하여 이전 사례의 상태가 섞이지 않게 한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');CHECK(row.size()==17);
        if (row.size()!=17) throw std::runtime_error("Form fixture 열 수 오류");
        const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");++counts[patch ? 0 : 1];
        SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,row[4]=="1");
        Input(pool,row[5]);std::string notices;
        // 콜백 당시 Form과 부모 전체 바이트도 원본의 명시 부모 통지 경계와 직접 대조한다.
        SquidForms forms(pool,[&](Sid parent,Sid form) {
            if (!notices.empty()) notices+=';';
            notices+=std::to_string(parent.value)+':'+std::to_string(form.value)+":2:"+Hex(pool.Slot(form))+':'+Hex(pool.Slot(parent));
        });
        const auto number=static_cast<std::uint32_t>(std::stoul(row[2]));const auto arg=std::stoi(row[3]);Sid result{};
        if (row[1]=="Create") result=forms.Create(number,static_cast<std::uint32_t>(arg));
        else if (row[1]=="Take") result=forms.Take(number,Sid{static_cast<std::uint16_t>(arg)});
        else if (row[1]=="Unpop") forms.Unpop(Sid{static_cast<std::uint16_t>(arg)});
        else if (row[1]=="Reset") pool.Reset(arg);
        else throw std::runtime_error("Form fixture 연산 오류");
        CHECK(result.value==std::stoul(row[7]));CHECK(notices==row[16]);
        // 각 watched 슬롯은 체크섬이 아닌 전체 실제 바이트를 비교한다.
        if (!row[6].empty()) for (const auto& slot:Split(row[6],';')) {
            const auto field=Split(slot,':');CHECK(Hex(pool.Slot(Sid{static_cast<std::uint16_t>(std::stoul(field[0]))}))==field[1]);
        }
        // 별도 풀 카운터/머리/꼬리도 raw 슬롯 출력과 함께 대조한다.
        const std::array<std::uint32_t,8> actual{pool.Capacity(),pool.FreeCount(),pool.PredictableCursor(),
            pool.FirstFree(true).value,pool.Tail(true).value,pool.FirstFree(false).value,pool.Tail(false).value,Adler(pool.Bytes())};
        // TSV는 원본 출력 순서와 같은 8개 메타데이터 열이다.
        for (std::size_t index=0;index<actual.size();++index) CHECK(actual[index]==std::stoul(row[index+8]));
    }
    CHECK(counts[0]==kCasesPerEdition);CHECK(counts[1]==kCasesPerEdition);
}

// 양수 풀 재확보 후에도 같은 Form 처리기를 쓸 수 있고 삭제 기록·권한·잘못된 크기의 기존 상태를 보존한다.
TEST_CASE(FormTake_ResizePreservesRecordsAndRejectsBadCapacity) {
    // 두 판본의 크기 경계와 client 권한을 각각 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,false);SquidForms forms(pool);const auto sid=forms.Create(3,2);pool.Release(sid);
        const auto deletion=pool.Deletions(true)[0];pool.Reset(static_cast<std::int32_t>(kCapacity+17));
        CHECK(!pool.IsServer());CHECK(pool.Capacity()==kCapacity+17);CHECK(pool.Deletions(true)[0].sid==deletion.sid);
        CHECK(pool.Deletions(true)[0].type==deletion.type);CHECK(forms.Create(1,2).value==5);
        pool.Reset(-1);CHECK(pool.Capacity()==kCapacity+17);CHECK(pool.FirstFree(true).value==5);
        const auto before=std::vector<std::uint8_t>(pool.Bytes().begin(),pool.Bytes().end());
        CHECK(Throws([&] { pool.Reset(1); }));CHECK(Throws([&] { pool.Reset(65536); }));
        CHECK(std::equal(before.begin(),before.end(),pool.Bytes().begin()));CHECK(pool.Deletions(true)[0].sid==deletion.sid);
    }
}

// 실제 GumpForm 부착→지정 Take→재부착→공통 삭제는 소유 DWORD/payload와 같은 SID 풀/깊이를 유지한다.
TEST_CASE(FormTake_GumpAttachmentAndDestroyIntegration) {
    // 같은 연결을 두 판본의 실제 가상 표/stride에서 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);
        SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);GumpFormHost forms(pool,factory,types,destroy);
        const auto parent=factory.Create(90,2),form=forms.Create(0),head=forms.Create(2);
        forms.SetOwner(form,0xabcd5678);forms.Attach(form,parent,0x10);forms.Attach(head,parent,0x10);
        const auto count=pool.FreeCount();CHECK(Read(pool,parent,6,2)==head.value);
        CHECK(forms.Take(form)==form);CHECK(forms.Owner(form)==0xabcd5678);CHECK(forms.Parent(form)==parent);
        CHECK(Read(pool,parent,6,2)==head.value);CHECK(Read(pool,head,4,2)==0);CHECK(pool.FreeCount()==count);
        CHECK((pool.Slot(form)[11]&12)==4);forms.Attach(form,parent,0x10);forms.Destroy(form,0x10);
        CHECK((pool.Slot(form)[11]&3)==3);CHECK(Read(pool,parent,6,2)==head.value);
        CHECK(destroy.PreDepth()==0);CHECK(destroy.PostDepth()==0);CHECK(pool.Deletions(false)[0].type==3);
    }
}

// ContentForm 알림은 링크/contained 해제 뒤 발생하며 알림이 고친 현재 payload가 Take 생성 뒤에도 보존된다.
TEST_CASE(FormTake_ContentNoticeTimingAndPayloadMutation) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity);SquidForms forms(pool);
    const auto parent=forms.Create(1,2),form=forms.Create(6,0);
    Put(pool,parent,6,form.value,2);Put(pool,form,16,parent.value,2);pool.AllocatedBytes(form)[11]=8;
    const auto before=Hex(pool.Slot(form));CHECK(Throws([&] { forms.Take(6,form); }));CHECK(Hex(pool.Slot(form))==before);
    std::uint32_t notices=0;
    // 부모 가상 +24 경계에서 현재 해제 상태를 읽고 payload를 바꾼다.
    SquidForms bound(pool,[&](Sid notifiedParent,Sid notifiedForm) {
        ++notices;CHECK(notifiedParent==parent);CHECK(notifiedForm==form);CHECK((pool.Slot(form)[11]&12)==4);
        CHECK(Read(pool,parent,6,2)==0);Put(pool,form,18,0xfedc5678,4);
    });
    CHECK(bound.Take(6,form)==form);CHECK(notices==1);CHECK(Read(pool,form,18,4)==0xfedc5678);
    CHECK(Throws([&] { bound.Take(6,form); }));CHECK(Throws([&] { bound.Take(3,form); }));
    const auto free=pool.FreeCount();CHECK(Throws([&] { bound.Create(4,2); }));CHECK(pool.FreeCount()==free);
}
