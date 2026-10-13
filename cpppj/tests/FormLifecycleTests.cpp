// 두 PE의 독립 Form 수명 관찰 및 공유 삭제/생산 생성 연결을 검사한다.
#include "TestSupport.h"
#include "o/DependFormHost.h"
#include "o/ContentFormHost.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 원본 관찰 도구와 같은 풀 크기이며 합성 부모의 가상 표 기록값은 역참조하지 않는다.
constexpr std::uint32_t kCapacity=32768;
// TSV/알림/슬롯 열은 끝의 빈 문자열까지 보존한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> fields;std::size_t begin=0;
    // 더 이상 구분자가 없으면 마지막 문자열을 추가하여 반환한다.
    for (;;) {
        const auto end=text.find(separator,begin);fields.emplace_back(text.substr(begin,end==std::string_view::npos ? end : end-begin));
        if (end==std::string_view::npos) return fields;begin=end+1;
    }
}
// 모든 슬롯 바이트를 소문자 두 자리 HEX로 표현한다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    // 원본 fixture와 같은 문자 표다.
    constexpr char digits[]="0123456789abcdef";std::string result;
    // 바이트를 생략하지 않고 저장하여 미작성 payload도 비교한다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result;
}
// 테스트의 합성 초기 슬롯은 free/server 번호도 포함한다. 예상 결과를 이 helper에서 만들지 않는다.
void Input(SidPool& pool,const std::string& slots) {
    if (slots.empty()) return;
    // fixture 입력의 전체 슬롯을 복원한다.
    for (const auto& item:Split(slots,';')) {
        const auto fields=Split(item,':');const Sid sid{static_cast<std::uint16_t>(std::stoul(fields[0]))};
        const auto raw=pool.Slot(sid);if (fields[1].size()!=raw.size()*2) throw std::runtime_error("Form 수명 입력 폭 오류");
        auto* bytes=const_cast<std::uint8_t*>(raw.data());
        // mutable 테스트 입력만 쓴다. C++ 구현이 만든 출력은 별도 비교한다.
        for (std::size_t index=0;index<raw.size();++index) bytes[index]=static_cast<std::uint8_t>(std::stoul(fields[1].substr(index*2,2),nullptr,16));
    }
}
// 슬롯의 WORD/DWORD를 원본 폭으로 읽는다.
std::uint32_t Read(const SidPool& pool,Sid sid,std::size_t offset,std::size_t width=2) {
    const auto raw=pool.Slot(sid);std::uint32_t value=0;
    // 주변 필드를 섞지 않고 little endian으로 조합한다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);return value;
}
// 직접 생성/콜백 재진입 검사에서만 명시 필드를 바꾼다.
void Put(SidPool& pool,Sid sid,std::size_t offset,std::uint32_t value,std::size_t width=2) {
    auto raw=pool.AllocatedBytes(sid);
    // store 폭 밖 payload를 유지한다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// watched 슬롯 밖의 변화를 Python zlib의 Adler-32와 대조한다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // 같은 소수·안전한 누적 블록 크기다.
    constexpr std::uint64_t prime=65521;std::uint64_t a=1,b=0;std::size_t block=0;
    // 전체 풀을 원래 바이트 순서대로 계산한다.
    for (const auto byte:bytes) { a+=byte;b+=a;if (++block==4096) { a%=prime;b%=prime;block=0; } }
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
// 알림 열에 콜백 당시 두 슬롯의 전체 상태를 기록한다.
void AddEvent(std::string& events,const std::string& event) { if (!events.empty()) events+=';';events+=event; }
// 잘못된 상태/미연결 경계의 사전 거부를 확인한다.
template<class Function> bool Throws(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
}

// 독립 PE의 전체 슬롯·알림 시점·중첩 삭제 카운터·풀 상태를 C++ 실행 결과와 직접 비교한다.
TEST_CASE(FormLifecycle_IndependentPeFixture) {
    std::ifstream input(NETSTORM_FORMLIFECYCLE_FIXTURE);if (!input) throw std::runtime_error("Form 수명 fixture 없음");
    std::string line;std::array<std::uint32_t,2> counts{};
    // 매 루트는 독립된 같은 크기의 풀에서 시작한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=12) throw std::runtime_error("Form 수명 fixture 열 수 오류");
        const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");++counts[patch ? 0 : 1];
        SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,row[5]=="1");
        Input(pool,row[7]);std::vector<RiftTypeRecord> types(patch ? 188 : 171);SquidHash hash;std::vector<std::uint8_t> spots(65536);
        SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);std::string events;
        // 부모 알림을 대체한 원본 경계와 같은 전체 바이트를 기록하고 같은 명시 mutation만 수행한다.
        ContentFormHost content(pool,types,[&](Sid parent,Sid form,std::uint32_t reason) {
            AddEvent(events,"content:"+std::to_string(parent.value)+':'+std::to_string(form.value)+':'+std::to_string(reason)+':'+Hex(pool.Slot(parent))+':'+Hex(pool.Slot(form)));
            if (reason==2 && row[6]!="0") Put(pool,form,20,static_cast<std::uint32_t>(std::stoul(row[6])));
        });
        // 실제 통지 콜백은 watched 부모/자기 Form을 읽고 결과만 기록한다.
        DependFormNotice notice=[&](Sid watcher,Sid watched,Sid form) {
            AddEvent(events,"watch:"+std::to_string(watcher.value)+':'+std::to_string(watched.value)+':'+std::to_string(form.value)+':'+Hex(pool.Slot(watcher))+':'+Hex(pool.Slot(form)));
        };
        // 서버 Pop과 삭제의 전송도 같은 현재 슬롯을 기록한다. 네트워크/GUI는 실행하지 않는다.
        FormPopTransmit transmit=[&](Sid form,std::uint32_t flags) { AddEvent(events,"transmit:"+std::to_string(form.value)+':'+std::to_string(flags)+':'+Hex(pool.Slot(form))); };
        SquidDestroyHooks fallback;fallback.emit=[&](const SquidDestroyEvent& event) {
            if (event.effect!=SquidDestroyEffect::Transmit) throw std::runtime_error("fixture 미선언 Form 효과");
            transmit(event.sid,event.flags);
        };
        DependFormHost depend(pool,destroy,notice,fallback,transmit);
        const Sid first{static_cast<std::uint16_t>(std::stoul(row[2]))},second{static_cast<std::uint16_t>(std::stoul(row[3]))};
        const auto flags=static_cast<std::uint32_t>(std::stoul(row[4]));std::uint32_t result=0;
        if (row[1]=="Pair") depend.Create(first,second);
        else if (row[1]=="Destroy") depend.Destroy(first,second.value);
        else if (row[1]=="Notify") depend.Notify(first,second);
        else if (row[1]=="Has") result=depend.Has(first,second) ? 1U : 0U;
        else if (row[1]=="Remove") depend.Remove(first,second,flags);
        else if (row[1]=="Attach") content.Attach(first,second,flags);
        else if (row[1]=="Robust") result=content.Robust(first) ? 1U : 0U;
        else if (row[1]=="Damage") CHECK(content.Damage(first,second.value,std::bit_cast<float>(flags))==0);
        else throw std::runtime_error("Form 수명 연산 오류");
        CHECK(result==std::stoul(row[9]));CHECK(events==row[11]);
        // watched 슬롯은 해시가 아닌 전체 바이트를 대조한다.
        for (const auto& item:Split(row[8],';')) { const auto fields=Split(item,':');CHECK(Hex(pool.Slot(Sid{static_cast<std::uint16_t>(std::stoul(fields[0]))}))==fields[1]); }
        const auto meta=Split(row[10],',');const std::array<std::uint32_t,11> actual{pool.Capacity(),pool.FreeCount(),pool.PredictableCursor(),
            pool.FirstFree(true).value,pool.Tail(true).value,pool.FirstFree(false).value,pool.Tail(false).value,Adler(pool.Bytes()),destroy.PreDepth(),destroy.PostDepth(),0};
        // 원본 공통 깊이 세 값은 정상 반환 후 모두 초기값이다. 메타데이터를 별도로 확인한다.
        for (std::size_t index=0;index<actual.size();++index) CHECK(actual[index]==std::stoul(meta.at(index)));
    }
    CHECK(counts[0]==NETSTORM_FORMLIFECYCLE_CASES);CHECK(counts[1]==NETSTORM_FORMLIFECYCLE_CASES);
}

// 실제 쌍을 생성한 뒤 원본 방향의 조회/삭제와 watcher/자산이 같은 풀에서 섞이는 경우를 검사한다.
TEST_CASE(FormLifecycle_DependencyPairAndReentrantNotice) {
    // 두 판본의 일반/client·서버 풀 경계를 각각 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity);SquidForms forms(pool);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
        const auto watched=factory.Create(90,2),watcher=factory.Create(91,2);DependFormHost depend(pool,destroy);
        const auto pair=depend.Create(watched,watcher);CHECK(depend.Has(watched,watcher));CHECK(!depend.Has(watcher,watched));
        CHECK(forms.Parent(pair.watchedForm)==watched);CHECK(forms.Parent(pair.watcherForm)==watcher);
        depend.Remove(watcher,watched);CHECK(!depend.Has(watched,watcher));CHECK(Read(pool,watched,6)==0);CHECK(Read(pool,watcher,6)==0);
        CHECK((pool.Slot(pair.watchedForm)[11]&3)==3);CHECK((pool.Slot(pair.watcherForm)[11]&3)==3);
        CHECK(pool.Deletions(true)[0].sid==pair.watchedForm.value);CHECK(pool.Deletions(true)[1].sid==pair.watcherForm.value);
        // 조회 알림이 같은 쌍을 삭제해도 outer caller가 Form을 재삭제할 때 dead에서 자연 반환한다.
        std::uint32_t calls=0;DependFormHost* bound=nullptr;
        DependFormHost reentrant(pool,destroy,[&](Sid notifiedWatcher,Sid notifiedWatched,Sid form) {
            ++calls;CHECK(notifiedWatcher==watcher);CHECK(notifiedWatched==watched);bound->Destroy(form,0x10);
        });bound=&reentrant;const auto second=reentrant.Create(watched,watcher);
        reentrant.Notify(second.watchedForm,watched);CHECK(calls==1);CHECK(!reentrant.Has(watched,watcher));
        CHECK(destroy.PreDepth()==0);CHECK(destroy.PostDepth()==0);
    }
}

// 기존 contained 생성기의 실제 타입6 생성→필드 저장→정렬 부착을 한 저장소에 연결한다.
TEST_CASE(FormLifecycle_ContentCreateAndCurrentFillState) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity);std::vector<RiftTypeRecord> types(188);types[90].flags2=0x10000;types[80].flags2=0x200000;
    SquidFactory factory(pool,types);const auto parent=factory.Create(90,2);ContentFormState state{true,true};std::vector<std::uint32_t> reasons;std::uint32_t filled=0;
    ContentFormHost content(pool,types,[&](Sid notifiedParent,Sid form,std::uint32_t reason) {
        CHECK(notifiedParent==parent);reasons.push_back(reason);if (reason==1) Put(pool,form,12,0xfe);
    },{},&state,[&](Sid filledParent,std::uint32_t pattern) { CHECK(filledParent==parent);CHECK(pattern==0x7e);++filled; });
    ContainedCreateHooks hooks;hooks.notifySurface=[](Sid) {};
    ContainedCreateState createState;RawContainedCreate create(pool,createState,content.BindCreate(factory,hooks));
    const auto form=create.Create(parent,80,7,0x20,0x10,3);CHECK(content.Robust(form));CHECK(filled==1);CHECK(reasons==std::vector<std::uint32_t>{1});
    CHECK(Read(pool,form,18)==80);CHECK(Read(pool,parent,6)==form.value);CHECK(pool.Slot(form)[30]==3);
    CHECK(content.Damage(form,0,-3.5f)==-3.5f);CHECK(content.Damage(form,1,9)==0);CHECK(filled==2);
    CHECK(reasons==std::vector<std::uint32_t>({1,2,1}));CHECK((Read(pool,form,20)&0x100)==0);
    content.Unpop(form);CHECK(reasons.back()==2);CHECK(Read(pool,parent,6)==0);
}

// 원본의 금지 상태/필수 전송 경계가 누락될 때 변경 전에 실패하는지 확인한다.
TEST_CASE(FormLifecycle_ContractsAndSameParentPair) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity);SquidForms forms(pool);std::vector<RiftTypeRecord> types(188);SquidFactory factory(pool,types);
    SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
    const auto parent=factory.Create(90,2),form=forms.Create(1,0);const auto before=Hex(pool.Slot(form));
    CHECK(Throws([&] { forms.Attach(form,parent); }));CHECK(Hex(pool.Slot(form))==before);
    DependFormHost depend(pool,destroy);const auto pair=depend.Create(parent,parent);CHECK(depend.Has(parent,parent));
    depend.Destroy(pair.watchedForm,0x10);CHECK(Read(pool,parent,6)==0);CHECK(destroy.PreDepth()==0);
    ContentFormHost content(pool,types,[](Sid,Sid,std::uint32_t) {});const auto value=factory.CreateContained(2);Put(pool,value,20,0x80);
    CHECK(Throws([&] { content.Robust(value); }));Put(pool,value,20,0x20);pool.AllocatedBytes(parent)[11]|=2;
    CHECK(Throws([&] { content.Attach(value,parent,0x10); }));CHECK((pool.Slot(value)[11]&4)!=0);
}
