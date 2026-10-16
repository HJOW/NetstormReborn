// 삭제후 packed count를 같은 실제 Factory/Pool/Kernel의 새 Regular에 직접 쓴다.
#include "RawSceneSupport.h"
#include "o/RawDamageablePostDestroy.h"
#include "o/SquidFactory.h"
#include "o/SquidProcess.h"
#include <tuple>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 선택 장면에서 실행되지 않은 하위 생성/섬/다리 경계는 기본 성공값으로 대체하지 않는다.
[[noreturn]] void MissingPostEffect() { throw std::logic_error("선택 Post Host 검사의 미복원 하위효과 요청"); }
}

// packed count/event/payload/time/Form을 실제 Host가 보존하고 첫 Kernel 호출에 그대로 전달한다.
TEST_CASE(DamageablePostDestroy_ActualHostInitialCountAndFirstKernelCall) {
    // 두 판본의 단일 풀에서 정상 피해와 DWORD carry를 별도로 확인한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        // 각 피해량은 다른 실제 장면이며 이전 Form 반환의 free 상태를 재사용하지 않는다.
        for (const auto maximum:{100,131073}) {
            SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);
            std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188U : 171U);
            std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));auto& residence=types[89];
            residence.constructorAddress=TypeConstructorAddress(edition,89);residence.flags1=0x10;residence.maxHitPoints=maximum;residence.footX=residence.footY=3;residence.group=0; // native와같은합성group0이주변피해Regular분기를선택한다.
            SquidFactory factory(pool,types);
            // 실제 짧은 거주 구역 생성자와 공통 HP postCreate로 관찰번호5..60을 확보한다.
            for (std::uint16_t id=5;id<=60;++id) CHECK(factory.Create(89,2)==Sid{id});
            const bool patch=edition==OriginalEdition::Patch1078;const Sid source{50},victim{60};
            auto parent=pool.AllocatedBytes(source);parent[11]=2;Put(parent,14,std::bit_cast<std::uint32_t>(20.5F));Put(parent,18,std::bit_cast<std::uint32_t>(20.5F));
            auto target=pool.AllocatedBytes(victim);target[11]=0;target[patch ? 40U : 35U]=0;Put(target,14,std::bit_cast<std::uint32_t>(21.0F));Put(target,18,std::bit_cast<std::uint32_t>(21.0F));
            Put(target,4,0,2);hash.Bucket(1,21,21)=victim.value;
            SquidPostPopState books;SquidRewardState hpState;GameRandom random{12345};ScrambledSpStore sp(random,[] { return 1U; });
            SquidReward hp(pool,types,books,hpState,patch ? &sp : nullptr);RawSquidNeighbors footprint(pool,hash,spots,types,frames);
            SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel(patch ? kProcessCapacity : kCdProcessCapacity);
            SquidProcessState clock;clock.now=17.25;std::vector<std::tuple<std::uint16_t,std::uint32_t,std::uint32_t,std::uint32_t>> calls;
            SquidProcessHost host(pool,types,kernel,destroy,clock,[&](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
                calls.emplace_back(sid.value,event,count,std::bit_cast<std::uint32_t>(payload));return 0.0F; // 명시 handler 관찰 경계이며 가상 HP/지면 피해 완료가 아니다.
            });
            DamageablePostDestroyState state;state.battle=true;std::vector<Sid> regularForms;std::size_t bases=0;DamageablePostDestroyHooks hooks;
            hooks.dropDude=[](Sid,std::int32_t,bool) { MissingPostEffect(); };hooks.dropIsland=[](float,float,std::uint32_t,std::uint32_t) { MissingPostEffect(); };
            hooks.regular=[&](Sid sid,std::uint32_t event,float payload,std::uint32_t count) {
                auto* process=host.AddRegular(sid,event,payload);CHECK(process!=nullptr);if (!process) throw std::logic_error("실제 Post Regular 생성 실패");
                process->SetInitialCount(count);CHECK(process->Event()==6699 && process->Payload()==0 && process->Time()==17.25 && process->Count()==count);
                CHECK(process->Parent()==victim && host.FromForm(process->Form())==process);regularForms.push_back(process->Form());
            };
            hooks.hasBridgeFall=[&](Sid sid) { return HasScheduledBridgeFall(host,sid); };
            hooks.notifySurface=hooks.displayUpdate=[](Sid) { MissingPostEffect(); };hooks.destroy=[](Sid,std::uint32_t) { MissingPostEffect(); };
            hooks.base=[&](Sid sid,std::uint32_t flags) { CHECK(sid==source && flags==0x10000);++bases; }; // 마지막 전체 공통삭제는 Path 소비자에서 별도 연결한다.
            RawDamageablePostDestroy post(pool,hash,types,frames,hp,footprint,state,hooks);post.PostDestroy(source,0x10000);
            CHECK(bases==1 && regularForms.size()==1 && kernel.Size()==1 && &host.Pool()==&pool && &post.Pool()==&pool);
            auto* process=host.FindEvent(victim,6699);CHECK(process!=nullptr);if (!process) return;
            const auto expected=maximum==100 ? 65587U : 131073U;CHECK(process->Count()==expected && process->Payload()==0 && process->Time()==17.25);
            clock.now=17.24;kernel.RunFrame();CHECK(calls.empty() && kernel.Size()==1);
            clock.now=17.25;kernel.RunFrame();CHECK(calls.size()==1);
            CHECK((calls.front()==std::tuple<std::uint16_t,std::uint32_t,std::uint32_t,std::uint32_t>(std::uint16_t{60},6699U,expected,0U)));
            CHECK(kernel.Size()==0 && (pool.Slot(regularForms.front())[11]&1) && Get(pool.Slot(victim),6,2)==0);
            CHECK(hash.Bucket(1,21,21)==victim.value && random.State()==12345 && !sp.Initialized());
            CHECK(destroy.PreDepth()==0 && destroy.PostDepth()==0); // 실제 ProcessForm 삭제깊이도 복구된다.
        }
    }
}
