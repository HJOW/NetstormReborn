// 객체 부착 프로세스(ProcessForm)·Kernel 슬롯·Regular 이벤트를 저장된 독립 기계어 관찰과 비교한다.
#include "TestSupport.h"
#include "o/SquidProcess.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 풀 크기·합성 부모 가상 표 기록값·관찰 슬롯 수는 기계어 생성기와 같다.
constexpr std::uint32_t kCapacity=32768,kParentVtable=0x12010000,kWatchSlots=96,kNotFound=0xffffffffU;
// 원문 TSV/구분 항목을 순서대로 분리한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin=0;
    // 마지막 빈 항목도 보존한다.
    for (;;) {
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==std::string_view::npos ? end : end-begin));
        if (end==std::string_view::npos) return result;
        begin=end+1;
    }
}
// 기계어 출력만 읽으므로 일반 CTest에는 원본 PE/Python이 필요 없다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto rows=[] {
        std::ifstream input(NETSTORM_PROCESS_FIXTURE);
        if (!input) throw std::runtime_error("프로세스 fixture 없음");
        std::vector<std::vector<std::string>> result;std::string line;
        // 주석은 실행 입력이 아니다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (!line.empty() && line.front()!='#') result.push_back(Split(line,'\t'));
        }
        return result;
    }();return rows;
}
// Python zlib와 같은 전체 버퍼 Adler-32다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    // 표준 소수와 중간 넘침을 막는 누적 폭이다.
    constexpr std::uint64_t prime=65521;std::uint64_t a=1,b=0;std::size_t block=0;
    // 큰 풀도 4096바이트마다 나머지를 취한다.
    for (auto byte:bytes) { a+=byte;b+=a;if (++block==4096) { a%=prime;b%=prime;block=0; } }
    return static_cast<std::uint32_t>(((b%prime)<<16)|(a%prime));
}
// client/server 최근 삭제 기록의 모든 DWORD를 물리 순서대로 비교한다.
std::uint32_t LogAdler(const SidPool& pool) {
    std::vector<std::uint8_t> bytes;
    // 각 기록은 번호와 타입의 두 DWORD다.
    for (bool client:{true,false}) for (const auto& record:pool.Deletions(client))
        for (auto value:{record.sid,record.type}) for (int i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    return Adler(bytes);
}
// 합성 부모 타입의 genus(flags2)는 생성기와 같다.
std::vector<RiftTypeRecord> SceneTypes(bool patch) {
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);
    types[82].flags2=0;types[90].flags2=0x4000;types[122].flags2=0x10000010;types[100].flags2=0x400000;
    return types;
}
// 기계어 생성기의 연산을 같은 순서로 실행하고 같은 형식의 관찰 문자열을 만든다.
struct Scene {
    bool patch;
    SidPool pool;
    SquidHash hash;
    std::vector<RiftTypeRecord> types;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    SquidUnpop unpop;
    RawSquidDestroy destroy;
    Kernel kernel;
    SquidProcessState state;
    std::vector<std::string> events;
    std::vector<float> script;
    std::size_t cursor{};
    SquidProcessHost host;
    std::vector<Sid> parents;
    std::vector<RegularProcess*> created;      // 생성 순서의 포인터. 살아 있는 동안만 쓴다.
    std::map<ProcessId,std::uint32_t> slotIndex; // Kernel 슬롯 → 생성 순서 번호.
    Scene(bool patchEdition,bool server,bool boss,double now,std::uint32_t rotation)
        :patch(patchEdition),pool(patchEdition ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,server),
         types(SceneTypes(patchEdition)),unpop(pool,hash,spots),destroy(pool,unpop,types),
         host(pool,types,kernel,destroy,state,
              [this](Sid parent,std::uint32_t event,std::uint32_t count,float payload) {
                  // 부모 이벤트 처리기 대체: 호출을 기록하고 입력 목록의 다음 값을 돌려준다.
                  events.push_back("H:"+std::to_string(parent.value)+':'+std::to_string(event)+':'+std::to_string(count)+':'+
                      std::to_string(std::bit_cast<std::uint32_t>(payload)));
                  CHECK(cursor<script.size());return script.at(cursor++);
              },
              SquidDestroyHooks{[this](const SquidDestroyEvent& event) { Asset(event); },[] { return Sid{}; },{}}) {
        state.now=now;state.boss=boss;state.typeRotation=rotation;
    }
    // 부모 객체의 가상 pre/post와 삭제 전파 대체다. 깊이 감소는 실제 공통 훅과 같은 위치에서 한다.
    void Asset(const SquidDestroyEvent& event) {
        const auto tail=':'+std::to_string(event.sid.value)+':'+std::to_string(event.flags);
        switch (event.effect) {
        case SquidDestroyEffect::PreDestroy:events.push_back("P"+tail);destroy.CompletePreDestroy();break;
        case SquidDestroyEffect::PostDestroy:events.push_back("O"+tail);destroy.CompletePostDestroy();break;
        case SquidDestroyEffect::Transmit:events.push_back("X"+tail);break;
        default:throw std::logic_error("장면에 없는 자산 삭제 효과");
        }
    }
    // 살아 있는 프로세스의 생성 번호를 슬롯 순서로 모은다. 비어 있는 슬롯의 대응은 버린다.
    std::vector<std::pair<ProcessId,std::uint32_t>> Alive() {
        std::vector<std::pair<ProcessId,std::uint32_t>> result;
        // 생성기와 같은 범위의 슬롯만 직접 비교한다.
        for (ProcessId slot=1;slot<kWatchSlots;++slot) {
            if (kernel.Get(slot)) result.emplace_back(slot,slotIndex.at(slot));
            else slotIndex.erase(slot);
        }
        return result;
    }
    // 연산 하나를 실행하고 생성기의 반환값과 같은 수를 돌려준다.
    std::uint32_t Apply(const std::string& text) {
        const auto op=Split(text,':');const auto kind=op[0][0];
        auto number=[&](std::size_t index) { return static_cast<std::uint32_t>(std::stoull(op.at(index))); };
        events.clear();
        switch (kind) {
        case 'P': {
            const auto sid=pool.Allocate(number(1));auto raw=pool.AllocatedBytes(sid);
            // 가상 표 기록값은 호스트에서 역참조하지 않는다.
            for (std::size_t i=0;i<4;++i) raw[i]=static_cast<std::uint8_t>(kParentVtable>>(8*i));
            raw[10]=static_cast<std::uint8_t>(number(2));raw[11]=static_cast<std::uint8_t>(number(3));
            raw[patch ? 40 : 35]=static_cast<std::uint8_t>(number(4));
            parents.push_back(sid);return sid.value;
        }
        case 'A':case 'B': {
            auto* process=host.AddRegular(parents.at(number(1)),number(2),std::bit_cast<float>(number(3)),kind=='A' ? 0x50U : number(4));
            created.push_back(process);
            if (process) slotIndex[kernel.Find(*process)]=static_cast<std::uint32_t>(created.size()-1);
            return static_cast<std::uint32_t>(created.size()-1);
        }
        case 'T':state.now=std::bit_cast<double>(std::stoull(op.at(1)));return 0;
        case 'R': {
            script.clear();cursor=0;
            // 처리기 반환값 목록은 float 비트다.
            for (const auto& item:Split(op.at(1),',')) script.push_back(std::bit_cast<float>(static_cast<std::uint32_t>(std::stoull(item))));
            kernel.RunFrame();return static_cast<std::uint32_t>(cursor);
        }
        case 'K':host.Kill(*created.at(number(1)),number(2));return 0;
        case 'F':case 'M': {
            const auto parent=parents.at(number(1));
            auto* found=kind=='F' ? host.FindEvent(parent,number(2)) : host.FindEventMasked(parent,number(2),number(3));
            return found ? slotIndex.at(kernel.Find(*found)) : kNotFound;
        }
        case 'D':destroy.Destroy(parents.at(number(1)),number(2),host.Hooks());return 0;
        case 'S': {
            auto raw=pool.AllocatedBytes(parents.at(number(1)));
            raw[number(2)==0 ? 11 : (patch ? 40 : 35)]^=static_cast<std::uint8_t>(number(3));return 0;
        }
        default:throw std::logic_error("알 수 없는 연산");
        }
    }
    // 생성기의 observe와 같은 순서의 문자열이다. Pop 깊이는 form의 postPop이 바로 되돌리므로 항상 0이다.
    std::string Observe(std::uint32_t result,const std::vector<std::uint32_t>& before) {
        const auto alive=Alive();std::string text;
        const std::array<std::uint64_t,13> numbers{result,pool.FreeCount(),pool.PredictableCursor(),pool.FirstFree(true).value,
            pool.Tail(true).value,pool.FirstFree(false).value,pool.Tail(false).value,Adler(pool.Bytes()),LogAdler(pool),
            destroy.PreDepth(),destroy.PostDepth(),0,patch ? state.typeRotation : 0};
        // 숫자 묶음.
        for (std::size_t i=0;i<numbers.size();++i) { if (i) text+=',';text+=std::to_string(numbers[i]); }
        text+='|';std::string slots,fields;std::vector<std::uint32_t> now;
        // 슬롯과 프로세스 필드는 슬롯 순서다.
        for (const auto& [slot,index]:alive) {
            now.push_back(index);
            if (!slots.empty()) { slots+=',';fields+=';'; }
            slots+=std::to_string(slot)+':'+std::to_string(index);
            const auto& process=*created.at(index);
            fields+=std::to_string(index)+':'+std::to_string(process.Form().value)+':'+std::to_string(process.Parent().value)+':'+
                std::to_string(std::bit_cast<std::uint64_t>(process.Time()))+':'+std::to_string(process.Event())+':'+
                std::to_string(std::bit_cast<std::uint32_t>(process.Payload()))+':'+std::to_string(process.Count());
        }
        text+=(slots.empty() ? "-" : slots)+'|'+(fields.empty() ? "-" : fields)+'|';
        std::string joined;
        for (const auto& event:events) { if (!joined.empty()) joined+=';';joined+=event; }
        text+=(joined.empty() ? "-" : joined)+'|';
        // 이번 연산에서 사라진 프로세스가 해제된 프로세스다.
        std::string freed;
        for (auto index:before) if (std::find(now.begin(),now.end(),index)==now.end()) { if (!freed.empty()) freed+=',';freed+=std::to_string(index); }
        return text+(freed.empty() ? "-" : freed);
    }
    // 살아 있는 생성 번호만 오름차순으로 돌려준다.
    std::vector<std::uint32_t> AliveIndices() {
        std::vector<std::uint32_t> result;
        for (const auto& item:Alive()) result.push_back(item.second);
        std::sort(result.begin(),result.end());return result;
    }
};
// 판본 이름으로 고른 시나리오를 모두 재생한다.
void ReplayEdition(std::string_view name,std::size_t minimum) {
    std::size_t scenarios=0,operations=0;
    // 한 줄이 한 시나리오다.
    for (const auto& row:Fixture()) {
        if (row[0]!=name) continue;
        CHECK(row.size()==8);
        Scene scene(row[0]=="originals",row[2]=="1",row[3]=="1",std::bit_cast<double>(std::stoull(row[4])),
            static_cast<std::uint32_t>(std::stoul(row[5])));
        const auto ops=Split(row[6],' ');const auto expected=Split(row[7],' ');
        CHECK(expected.size()==ops.size()+1);
        CHECK(scene.Observe(0,{})==expected[0]);
        // 연산마다 직전의 살아 있는 프로세스와 비교해 해제 목록을 만든다.
        for (std::size_t i=0;i<ops.size();++i) {
            const auto before=scene.AliveIndices();
            const auto result=scene.Apply(ops[i]);
            const auto actual=scene.Observe(result,before);
            CHECK(actual==expected[i+1]);
            if (actual!=expected[i+1]) { std::printf("  %s 시나리오 %s 연산 %zu(%s)\n    기대 %s\n    실제 %s\n",row[0].c_str(),row[1].c_str(),i,ops[i].c_str(),expected[i+1].c_str(),actual.c_str());break; }
            ++operations;
        }
        ++scenarios;
    }
    CHECK(scenarios>=minimum);CHECK(operations>=minimum*10);
}
// 명시적인 C++ 보호 예외를 원본 assert 실행 없이 확인한다.
template<class F> bool Throws(F action) { try { action();return false; } catch (const std::exception&) { return true; } }
}

// 패치 10.78의 실제 기계어 관찰을 재생한다.
TEST_CASE(process_patch_x86_fixture) { ReplayEdition("originals",100); }
// CD 10.72 배포본의 실제 기계어 관찰을 재생한다.
TEST_CASE(process_cd_x86_fixture) { ReplayEdition("originalCD",100); }
// 추가 10.37 실행 파일은 CD와 같은 코드 배치지만 별도 PE의 출력을 재생한다.
TEST_CASE(process_1037_x86_fixture) { ReplayEdition("original1037",100); }

// 처리기가 없으면 원본 base처럼 payload가 주기가 된다. 첫 호출은 부착한 다음 프레임이다.
TEST_CASE(process_default_handler_repeats_with_payload_period) {
    Scene scene(true,true,true,100.0,0);
    SquidProcessHost plain(scene.pool,scene.types,scene.kernel,scene.destroy,scene.state);
    const auto parent=Sid{static_cast<std::uint16_t>(scene.Apply("P:2:82:0:0"))};
    auto* process=plain.AddRegular(parent,7,0.5f);
    CHECK(process && process->Count()==0 && process->Time()==100.0);
    scene.kernel.RunFrame();CHECK(process->Count()==1);CHECK(process->Time()==100.5);
    scene.kernel.RunFrame();CHECK(process->Count()==1);
    scene.state.now=100.5;scene.kernel.RunFrame();CHECK(process->Count()==2);CHECK(process->Time()==101.0);
}
// 다리의 지연 낙하 예약(이벤트 0x2692)은 같은 부모에서 조회되고, 처리기가 0을 돌려주면 프로세스와 form이 사라진다.
TEST_CASE(process_delayed_fall_event_registers_and_ends) {
    Scene scene(true,true,true,5.0,0);
    const auto parent=Sid{static_cast<std::uint16_t>(scene.Apply("P:0:82:0:0"))};
    const auto free=scene.pool.FreeCount();
    CHECK(!HasScheduledBridgeFall(scene.host,parent));
    auto* fall=ScheduleBridgeFall(scene.host,parent,20.75f,21.9f);
    scene.created.push_back(fall);scene.slotIndex[scene.kernel.Find(*fall)]=0;
    // payload는 (20 & 255) | (21 << 8) = 5396을 float로 바꾼 값이다.
    CHECK(fall->Event()==kBridgeFallEvent);CHECK(fall->Payload()==5396.0f);
    CHECK(HasScheduledBridgeFall(scene.host,parent));CHECK(scene.host.FindEvent(parent,1)==nullptr);
    CHECK(scene.host.FindEventMasked(parent,0xff00,0x2600)==fall);
    CHECK(scene.pool.FreeCount()==free-1);
    scene.Apply("R:0");
    CHECK(scene.kernel.Size()==0);CHECK(!HasScheduledBridgeFall(scene.host,parent));CHECK(scene.pool.FreeCount()==free);
    CHECK(scene.destroy.PreDepth()==0 && scene.destroy.PostDepth()==0);
}
// 처리기 안에서 자기 프로세스를 지워도 이후 필드를 건드리지 않는다.
TEST_CASE(process_handler_may_kill_its_own_process) {
    Scene scene(true,true,true,0.0,0);
    RegularProcess* target=nullptr;SquidProcessHost* self=nullptr;
    SquidProcessHost reentrant(scene.pool,scene.types,scene.kernel,scene.destroy,scene.state,
        [&](Sid,std::uint32_t,std::uint32_t,float) { self->Kill(*target,0);return 3.0f; });
    self=&reentrant;
    const auto parent=Sid{static_cast<std::uint16_t>(scene.Apply("P:2:82:0:0"))};
    target=reentrant.AddRegular(parent,1,1.0f);
    scene.kernel.RunFrame();
    CHECK(scene.kernel.Size()==0);CHECK(reentrant.FindEvent(parent,1)==nullptr);
}
// 가장 최근에 붙인 프로세스가 체인 머리이고, 중간 프로세스를 지워도 앞뒤 링크가 이어진다.
TEST_CASE(process_dependent_chain_order_and_unlink) {
    Scene scene(false,true,true,0.0,0);
    const auto parent=Sid{static_cast<std::uint16_t>(scene.Apply("P:2:82:0:0"))};
    // 같은 이벤트 세 개를 붙이면 마지막 것이 먼저 조회된다.
    for (int i=0;i<3;++i) scene.Apply("A:0:5:1065353216");
    CHECK(scene.host.FindEvent(parent,5)==scene.created[2]);
    scene.host.Kill(*scene.created[1],0);
    CHECK(scene.host.FindEvent(parent,5)==scene.created[2]);
    scene.host.Kill(*scene.created[2],0);
    CHECK(scene.host.FindEvent(parent,5)==scene.created[0]);
    scene.host.Kill(*scene.created[0],0);
    CHECK(scene.host.FindEvent(parent,5)==nullptr);CHECK(scene.kernel.Size()==0);
}
// 부모가 지워지는 중(dead)이면 form의 Unpop은 void만 켜고 종속 체인과 contained 비트를 그대로 둔다(004ae140/004ac6b0 ↔ CD 004af270/004ac160).
// 최종 풀 상태만 비교하는 기계어 관찰로는 부모가 정리된 뒤 이 차이가 지워지므로 Unpop 직후의 raw 필드를 직접 확인한다.
TEST_CASE(process_form_unpop_keeps_chain_while_parent_is_dying) {
    for (const bool patch:{true,false}) {
        // 체인 머리 쪽 form을 부모가 살아 있을 때와 죽는 중일 때 각각 Unpop한다. 두 판본 모두 같은 계약이다.
        for (const bool dying:{false,true}) {
            Scene scene(patch,true,true,0.0,0);
            const auto parent=Sid{static_cast<std::uint16_t>(scene.Apply("P:2:82:0:0"))};
            scene.Apply("A:0:1:1065353216");scene.Apply("A:0:2:1065353216");
            const auto older=scene.created[0]->Form(),head=scene.created[1]->Form();
            // 부모의 머리(+6)는 가장 최근 form, 그 form의 다음(+4)은 먼저 붙인 form이다.
            auto word=[&](Sid sid,std::size_t offset) { const auto raw=scene.pool.Slot(sid);return static_cast<std::uint16_t>(raw[offset]|(raw[offset+1]<<8)); };
            CHECK(word(parent,6)==head.value && word(head,4)==older.value);
            if (dying) scene.pool.AllocatedBytes(parent)[11]|=2;
            scene.host.Hooks().unpopForm(head);
            // 어느 쪽이든 form은 void가 된다.
            CHECK((scene.pool.Slot(head)[11]&4)!=0);
            if (dying) {
                CHECK(word(parent,6)==head.value);CHECK(word(head,4)==older.value);CHECK((scene.pool.Slot(head)[11]&8)!=0);
            } else {
                CHECK(word(parent,6)==older.value);CHECK((scene.pool.Slot(head)[11]&8)==0);
            }
        }
    }
}
// 잘못된 타입·전송이 필요한 flags·CD의 죽은 부모는 등록 전에 거부하고, 패치의 죽은 부모는 조용히 건너뛴다.
TEST_CASE(process_attach_guards) {
    Scene patch(true,true,true,0.0,7);
    const auto parent=Sid{static_cast<std::uint16_t>(patch.Apply("P:2:82:4:0"))};
    CHECK(Throws([&] { patch.host.Attach(std::make_unique<RegularProcess>(1,1.0f),9,parent,0x50); }));
    CHECK(Throws([&] { patch.host.Attach(std::make_unique<RegularProcess>(1,1.0f),63,parent,0x50); }));
    CHECK(Throws([&] { patch.host.Attach(std::make_unique<RegularProcess>(1,1.0f),46,parent,0); }));
    CHECK(patch.kernel.Size()==0);CHECK(patch.state.typeRotation==7);
    patch.Apply("D:0:0");
    CHECK(patch.host.AddRegular(parent,1,1.0f)==nullptr);CHECK(patch.kernel.Size()==0);
    Scene cd(false,true,true,0.0,0);
    const auto cdParent=Sid{static_cast<std::uint16_t>(cd.Apply("P:2:82:4:0"))};
    cd.Apply("D:0:0");
    CHECK(Throws([&] { cd.host.AddRegular(cdParent,1,1.0f); }));CHECK(cd.kernel.Size()==0);
}
// form 루트는 전용 Unpop 훅 없이는 공통 삭제에 넣을 수 없다.
TEST_CASE(process_form_root_requires_unpop_hook) {
    Scene scene(true,true,true,0.0,0);
    scene.Apply("P:2:82:0:0");scene.Apply("A:0:1:1065353216");
    const auto form=scene.created[0]->Form();
    SquidDestroyHooks bare{[](const SquidDestroyEvent&) {},[] { return Sid{}; },{}};
    CHECK(Throws([&] { scene.destroy.Destroy(form,0,bare); }));
    CHECK(scene.kernel.Size()==1);CHECK(!(scene.pool.Slot(form)[11]&2));
}
