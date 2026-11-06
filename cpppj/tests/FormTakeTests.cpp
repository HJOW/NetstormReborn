// 두 실제 PE의 저장 관찰과 공유 Form 처리기의 기존 UI/프로세스 연결 및 오류 계약을 검사한다.
#include "FormTypeInputs.h" // 원문 공통 Create가 읽는 내장 Form 생성자 주소를 입력에 제공한다.
#include "TestSupport.h"
#include "o/SquidForms.h"
#include "o/GumpFormHost.h"
#include "o/RawContentParentNotice.h" // 내장 Form의 실제 빈 부모 통지가 인자를 읽지 않는지 확인한다.
#include "o/SquidFormDispatch.h" // 부모삭제가현재Form클래스의알림/취소를사용하는지같은실제호스트로대조한다.
#include <algorithm>
#include <bit>
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
        SidPool pool(edition,kCapacity);std::vector<RiftTypeRecord> types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);
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
    auto detached=std::vector<std::uint8_t>(pool.Slot(form).begin(),pool.Slot(form).end());detached[11]=4; // 원문004ac6b0은 부모 알림 전에 void/contained와 부모 head를 이미 쓴다. 나머지 전체 바이트는 보존한다.
    CHECK(Throws([&] { forms.Take(6,form); }));CHECK(Hex(pool.Slot(form))==Hex(detached) && Read(pool,parent,6,2)==0);
    Put(pool,parent,6,form.value,2);pool.AllocatedBytes(form)[11]=8; // 다음 알림/payload 사례는 별도의 동일한 정상 부착 입력에서 시작한다.
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

// TYPE별칭에도현재Gump/Content VT의부모삭제알림·커서취소·공통깊이반납을보존한다.
TEST_CASE(FormClassLifetime_CurrentVtableKeepsGumpAndContentEffects) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두판본의실제생성자/가상표를각각사용한다.
        for (const auto formType:{3U,6U}) { // Gump의owner알림과Content의빈알림/커서취소를구별한다.
            for (const auto sourceType:{3U,6U,7U}) { // 변경가능한TYPE가현재클래스를대신할수없다.
                SidPool pool(edition,kCapacity,false);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188:171);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
                SquidDestroyHooks assets;assets.selected=[] { return Sid{}; };assets.emit=[](const SquidDestroyEvent&) { throw std::logic_error("Form삭제검사에서자산효과선택"); }; // 이검사는자산/전송/통신경계를선택하지않는다.
                SquidFormDispatch dispatch(pool,destroy,assets);Sid parent{},form{};std::size_t notices{},cancels{};
                GumpFormHost gumps(pool,factory,types,destroy,[&](auto owner,Sid notifiedParent) { CHECK(owner==0x12345678 && notifiedParent==parent);++notices; },dispatch.Hooks()); // 원본Gump의비정렬ownerDWORD와부모WORD를확인한다.
                ContentFormHost content(pool,types,[](Sid,Sid,std::uint32_t) { throw std::logic_error("삭제중Content부모통지선택"); });ContentLifetimeState state;ContentLifetimeHooks hooks;
                hooks.cancelCursor=[&] { CHECK(pool.Slot(form)[10]==sourceType && (pool.Slot(form)[11]&2U));++cancels; }; // 원본Content preDestroy는dead설정뒤실제커서와비교한다.
                ContentFormLifetime lifetime(pool,types,content,destroy,state,std::move(hooks),dispatch.Hooks());dispatch.Bind(gumps);dispatch.Bind(lifetime);
                parent=factory.CreateGumpForm(2);form=factory.Create(formType,2);state.cursor=form.value;
                if (formType==3) gumps.SetOwner(form,0x12345678);else Put(pool,form,18,75,2); // 너겟/경제/전투효과를선택하지않는일반kind다.
                Put(pool,parent,6,form.value,2);Put(pool,form,16,parent.value,2);pool.AllocatedBytes(form)[11]=8;pool.AllocatedBytes(form)[10]=static_cast<std::uint8_t>(sourceType);const auto free=pool.FreeCount();
                bool failed=false;try { CHECK(dispatch.Destroy(parent,0)); } catch (const std::exception& error) { failed=true;std::printf("FormClassLifetime edition=%u form=%u type=%u: %s\n",static_cast<unsigned>(edition),formType,sourceType,error.what()); }
                CHECK(!failed && notices==(formType==3 ? 1U:0U) && cancels==(formType==6 ? 1U:0U));CHECK((pool.Slot(parent)[11]&1U) && (pool.Slot(form)[11]&1U));CHECK(pool.FreeCount()==free+2 && destroy.PreDepth()==0 && destroy.PostDepth()==0);CHECK(gumps.IsGumpForm(form)==(sourceType==3) && lifetime.IsContentForm(form)==(sourceType==6)); // 반납뒤타입기록조회는현재클래스판정과별도계약이다.
            }
        }
    }
}

// 직접 필드 조회는 TYPE/상태 선행 필터를 추가하지 않고 실제 부모 getter와 전체 체인을 사용한다.
TEST_CASE(ContentFieldCurrent_AliasesStatesAndWholeChain) {
    std::size_t inputs{};
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두 물리 stride와 실제 Form getter를 각각 사용한다.
        SidPool pool(edition,kCapacity);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188:171);SquidFactory factory(pool,types);ContentFormHost content(pool,types,[](Sid,Sid,std::uint32_t) {});const auto parent=factory.CreateContained(2);
        for (const auto formType:{1U,2U,3U,6U}) { // 공통 Linked의 네 실제 부모 getter를 사용한다.
            const auto form=factory.Create(formType,2);auto raw=pool.AllocatedBytes(form);Put(pool,form,16,parent.value,2);Put(pool,form,20,0x20,2);Put(pool,parent,6,form.value,2);
            for (const auto type:{0U,6U,7U,63U,170U,255U}) { // 직접 몸체가 읽지 않는 TYPE를 여러 유효 BYTE로 바꾼다.
                for (const auto state:{4U,255U}) { // 직접 getter/flags 조회는 free/dead/void 필터를 추가하지 않는다.
                    raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(state);raw[4]=raw[5]=0;
                    CHECK(!Throws([&] { CHECK(content.Robust(form));CHECK(content.Linked(form));raw[4]=0;raw[5]=0x80;CHECK(!content.Linked(form)); }));++inputs;
                }
            }
            raw[10]=static_cast<std::uint8_t>(formType);raw[11]=4;raw[4]=raw[5]=0;Put(pool,parent,6,0,2);pool.Release(form);
        }
        pool.Release(parent);
    }
    CHECK(inputs==96);
}
// WORLD 진단의 정상 복귀 뒤 현재 flags와 재확보된 풀을 읽고 실행 중 장치의 수명을 보존한다.
TEST_CASE(ContentFieldCurrent_RobustDiagnosticReadsCurrentFlagsAfterRepair) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 원문 파일명 대소문자와 두 진단 행을 구별한다.
        for (const bool keepWorld:{false,true}) { // 진단 복귀 뒤 WORLD를 다시 검사하거나 기존 flags 사본을 반환하지 않는다.
            SidPool pool(edition,kCapacity);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188:171);SquidFactory factory(pool,types);const auto form=factory.CreateContained(2);Put(pool,form,20,0x80,2);ContentFormState state;std::size_t diagnostics{};
            state.assertion=[&](auto expression,auto file,auto line) { const bool patch=edition==OriginalEdition::Patch1078;CHECK(expression=="!(getListFlags() & CFF_WORLD)" && file==(patch ? "\\Ns\\O\\Squid.cpp":"\\ns\\o\\Squid.cpp") && line==(patch ? 0xbb1U:0xb68U));++diagnostics;state.assertion={};pool.Reset(kCapacity+2);auto raw=pool.Slot(form);auto* changed=const_cast<std::uint8_t*>(raw.data());changed[10]=255;changed[11]=255;changed[20]=keepWorld ? 0xa0:0; }; // 원문 진단이 현재 바이트/장치와 물리 주소를 바꾸는 명시 입력이다.
            ContentFormHost content(pool,types,[](Sid,Sid,std::uint32_t) {},{},&state);bool result{};CHECK(!Throws([&] { result=content.Robust(form); }));CHECK(diagnostics==1 && result==keepWorld && pool.Capacity()==kCapacity+2);
        }
    }
}
// 원문 Regular+5c의 count0/음수는 this를 읽지 않고 전달 float를 그대로 반환한다.
TEST_CASE(ContentFieldCurrent_ZeroNegativeRegularSkipsObjectFields) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 같은 조기 반환을 판본별 저장소와 분리해 확인한다.
        SidPool pool(edition,kCapacity);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188:171);ContentFormHost content(pool,types,[](Sid,Sid,std::uint32_t) {});
        for (const auto number:{0U,65535U}) for (const auto count:{0,-1,(-2147483647-1)}) for (const auto bits:{0x80000000U,0xc0600000U}) { // 범위 밖 this라도 읽지 않는 조기 경로와 -0/음수 float 비트를 대조한다.
            const Sid form{static_cast<std::uint16_t>(number)};CHECK(!Throws([&] { CHECK(std::bit_cast<std::uint32_t>(content.Damage(form,count,std::bit_cast<float>(bits)))==bits); }));
        }
    }
}

// 분리 콜백의 VT/부모/flags 변경 뒤 실제 현재 재부착과 선택 실패의 완료 접두를 확인한다.
TEST_CASE(ContentFieldCurrent_ReattachUsesCurrentClassCachedParentAndWritePrefix) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // PATCH BYTE/CD WORD 쓰기를 같은 실제 Factory와 연결한다.
        for (const auto profile:{0U,1U,2U,3U}) { // Content 재부착/Gump 전환/실제 장치 실패/미연결 다른 VT를 구별한다.
            SidPool pool(edition,kCapacity);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188:171);SquidFactory factory(pool,types);std::uint32_t depth=17;factory.BindFormPopDepth(depth);const auto parent=factory.CreateContained(2),other=factory.CreateContained(2),form=factory.CreateContained(2);ContentFormHost content(pool,types,[](Sid,Sid,std::uint32_t) {});content.BindPop([&](Sid sid,auto flags) { factory.PopForm(sid,flags); });Put(pool,form,18,75,2);Put(pool,form,20,0x120,2);content.Attach(form,parent,0x10);pool.AllocatedBytes(form)[10]=7;SquidForms detached(pool,[](Sid,Sid) {});std::size_t reattached{};
            content.BindUnpop([&](Sid sid) { CHECK(sid==form);detached.Unpop(sid);Put(pool,form,16,other.value,2);Put(pool,form,20,0xab34,2);if (profile==1 || profile==3) Put(pool,form,0,SquidForms(pool).Vtable(3),4); }); // 원문이 먼저 저장한 부모를 콜백 뒤의 현재 필드로 대체하지 않는다.
            if (profile!=3) content.BindReattach([&](Sid sid,Sid into,auto flags) { CHECK(sid==form && into==parent && flags==0x10 && Read(pool,form,20,2)==0xaa34 && depth==17);++reattached;if (profile==2) throw std::runtime_error("명시 재부착 장치 실패");if (profile==1) factory.AttachForm(sid,into,flags);else content.Attach(sid,into,flags); }); // 선택한 현재 클래스의 실제 공통/정렬 부착을 사용한다.
            const bool failed=Throws([&] { CHECK(content.Damage(form,1,9.0f)==0); });CHECK(failed==(profile>=2) && reattached==(profile==3 ? 0U:1U) && depth==17 && Read(pool,form,20,2)==0xaa34);
            CHECK(Read(pool,form,16,2)==(profile>=2 ? other.value:parent.value));CHECK(Read(pool,parent,6,2)==(profile>=2 ? 0U:form.value));CHECK((pool.Slot(form)[11]&4U)==(profile>=2 ? 4U:0U));
        }
    }
}
// 양판본 네 Form의 +24 빈 통지는 TYPE/상태와 범위 밖 form 인자를 읽지 않는다.
TEST_CASE(ContentFieldCurrent_AllEightFormParentNoticesAreEmpty) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 여덟 실제 생성자 VT의 ret8을 같은 공개 분배기로 확인한다.
        SidPool pool(edition,kCapacity);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188:171);SquidFactory factory(pool,types);std::size_t derived{};RawContentParentNotice notice(pool,[&](Sid,Sid,std::uint32_t) { ++derived; });
        for (const auto kind:{1U,2U,3U,6U}) { // 사용중 및 명시 free/dead 바이트에서도 현재 VT가 빈 몸체를 결정한다.
            const auto parent=factory.Create(kind,2);auto raw=pool.AllocatedBytes(parent);
            for (const auto type:{0U,7U,170U,255U}) for (const auto state:{0U,255U}) { // 빈 몸체에서 사용하지 않는 인자와 필드를 변경한다.
                raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(state);CHECK(!Throws([&] { notice.Notice(parent,Sid{65535},0xffffffffU); }));
            }
            raw[10]=static_cast<std::uint8_t>(kind);raw[11]=4;pool.Release(parent);
        }
        CHECK(derived==0);
        const auto form=factory.CreateGumpForm(2);auto raw=pool.AllocatedBytes(form);Put(pool,form,16,0,2);auto zero=pool.Slot(Sid{});auto* physical=const_cast<std::uint8_t*>(zero.data());physical[6]=static_cast<std::uint8_t>(form.value);physical[7]=static_cast<std::uint8_t>(form.value>>8);ContentFormHost content(pool,types,[](Sid,Sid,std::uint32_t) {});CHECK(content.Linked(form));raw[4]=0;raw[5]=0x80;CHECK(!content.Linked(form)); // 예약 부모0도 실제 head를 읽고 자기 뒤 범위 실패를 확인한다.
    }
}
