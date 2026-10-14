// 실제 설치된 캠페인 총/사제와 동일 Kernel의 감시 Form 교체·제거를 확인한다.
#include "CampaignInstallSupport.h"
#include "o/RawGunProcess.h"
#include "RawSceneSupport.h"

namespace {
// 현재 실제 GunProcess Form에서 같은 Host 객체를 찾는다.
GunProcess& CampaignGun(InstallScene& scene) {
    GunProcess* found=nullptr;
    // 현재 실제 Form/PID를 조회하고 첫 총만 선택한다. 별도 프로세스를 만들지 않는다.
    for (std::uint32_t id=5;id<scene.core.pool.Capacity();++id) {
        const auto sid=Sid{static_cast<std::uint16_t>(id)};const auto raw=scene.core.pool.Slot(sid);
        if ((raw[11]&1) || raw[10]!=34) continue;
        found=dynamic_cast<GunProcess*>(scene.core.processes.FromForm(sid));if (found) break;
    }
    if (!found) throw std::logic_error("보호 캠페인의 실제 GunProcess 없음");return *found;
}
// 실제 Host의 virtual notify에서 받은 PID/인자를 관찰하는 명시 파생 경계다.
class CampaignNotifyObserver final : public SquidProcess {
public:
    // 같은 Kernel의 실제 등록 번호와 원본 요청 관찰 저장소를 받는다.
    CampaignNotifyObserver(std::uint32_t pid,std::vector<std::string>& events):pid_(pid),events_(events) {}
    // 독립 notify 입력은 프레임을 실행하지 않는다.
    void RunFrame() override { throw std::logic_error("독립 notify 검사에서 프레임 실행 예상 밖"); }
    // 실제 dispatch의 event/info/value를 계산 없이 기록한다.
    void Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) override {
        events_.push_back("N:"+std::to_string(pid_)+','+std::to_string(event)+','+std::to_string(info)+','+std::to_string(value));
    }
private:
    std::uint32_t pid_; // 실제 Kernel 슬롯과 같은 관찰 프로세스의 입력 번호다.
    std::vector<std::string>& events_; // callback 시점의 실제 요청 순서다.
};
// 한 판본의 실제 ProcessForm/PID/서버 조건 통지를 독립 원본 fixture와 비교한다.
void CheckCampaignNotifyNative(OriginalEdition edition) {
    std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
    std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());
    RawSurfaceWorld world(edition,types,frames,shapes,CampaignLinks(),97);auto core=world.Core();
    const auto parent=core.pool.Allocate(2);core.pool.AllocatedBytes(parent)[10]=74;
    std::array<Sid,256> forms{};std::vector<std::string> events;
    // 원본과 같은1..255의 현재 Kernel 슬롯을 실제 Host에 확보한다.
    for (std::uint32_t pid=1;pid<=255;++pid) {
        auto* process=core.processes.Attach(std::make_unique<CampaignNotifyObserver>(pid,events),34,parent,0x50);
        CHECK(process!=nullptr);forms[pid]=process->Form();
    }
    const auto rows=netstorm::test::rawscene::LoadFixture(NETSTORM_GUN_DEPEND_FIXTURE).rows;std::size_t count=0;
    // 기존 원본의 가상34 호출 결과만 비교하며 기대 인자 순서를 재계산하지 않는다.
    for (const auto& row:rows) {
        if (row[0]!=(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD") || row[1]!="N") continue;
        events.clear();core.processState.server=netstorm::test::rawscene::Number(row[2])!=0;
        const auto pid=netstorm::test::rawscene::Number(row[3]);
        core.processes.NotifyDependency(pid==0xffffffffU ? Sid{} : forms[pid],
            Sid{static_cast<std::uint16_t>(netstorm::test::rawscene::Number(row[4]))},
            Sid{static_cast<std::uint16_t>(netstorm::test::rawscene::Number(row[5]))});
        CHECK((events.empty() ? "-" : events.front())==row[6] && events.size()<=1);++count;
    }
    CHECK(count==60);world.ClearProcesses();
}
}
// 실제 저장 사제 두 부모 사이의 표적 교체가 각 방향 Form 쌍을 현재 풀에서 제거/생성한다.
TEST_CASE(CampaignTarget_ActualWarBeginsDependencySwap) {
    InstallScene scene;scene.runtime->Load();auto& gun=CampaignGun(scene);const auto form=gun.Form();
    const auto first=Sid{static_cast<std::uint16_t>(scene.priestState.priests.priests.Items()[0])};
    const auto second=Sid{static_cast<std::uint16_t>(scene.priestState.priests.priests.Items()[1])};
    const auto before=scene.core.pool.FreeCount();
    scene.assetBindings->Gun().SetTarget(gun.Fields(),first);
    CHECK(gun.Fields().Target()==first && scene.core.dependencies.Has(first,form));
    CHECK(scene.core.pool.FreeCount()==before-2 && scene.core.bookkeeping.depth==0);
    scene.assetBindings->Gun().SetTarget(gun.Fields(),second);
    CHECK(!scene.core.dependencies.Has(first,form) && scene.core.dependencies.Has(second,form));
    CHECK(gun.Fields().Target()==second && scene.core.pool.FreeCount()==before-2 && scene.core.bookkeeping.depth==0);
    scene.assetBindings->Gun().ClearTarget(gun.Fields(),true);
    CHECK(gun.Fields().Target()==Sid{} && gun.Fields().Stage()==1 && !scene.core.dependencies.Has(second,form));
    CHECK(scene.core.pool.FreeCount()==before && scene.core.bookkeeping.depth==0);
}
// 패치판의 정상/서버0·잘못된PID를 건너뛰는60개 가상34 입력을 비교한다.
TEST_CASE(CampaignTarget_PatchNativeNotify) { CheckCampaignNotifyNative(OriginalEdition::Patch1078); }
// CD판의 실제 현재PID getter/notify 인자 순서60개를 비교한다.
TEST_CASE(CampaignTarget_CdNativeNotify) { CheckCampaignNotifyNative(OriginalEdition::Cd1072); }
// 실제 저장 표적이 dead인 원본 통지 입력에서 동일 Gun의 표적0/재시작/현재Form 전송을 확인한다.
TEST_CASE(CampaignTarget_ActualWarBeginsDeletedTargetNotice) {
    InstallScene scene;scene.runtime->Load();auto& gun=CampaignGun(scene);
    const auto target=Sid{static_cast<std::uint16_t>(scene.priestState.priests.priests.Items()[0])};
    scene.assetBindings->Gun().SetTarget(gun.Fields(),target);
    const auto watchedForm=Sid{static_cast<std::uint16_t>(netstorm::test::rawscene::Get(scene.core.pool.Slot(target),6,2))};
    CHECK(scene.core.pool.Slot(watchedForm)[10]==1);const auto before=scene.external.size();
    // 삭제의 ReleaseDependent가 호출되는 dead 조건만 명시 입력한다. 큰 사제 사망 사슬은 이 검사 범위 밖이다.
    scene.core.pool.AllocatedBytes(target)[11]|=2;
    scene.core.dependencies.Notify(watchedForm,target);
    CHECK(gun.Fields().Target()==Sid{} && gun.Fields().Stage()==1 && scene.external.size()>before);
    scene.core.dependencies.Destroy(watchedForm,0);CHECK(scene.core.bookkeeping.depth==0);
    scene.core.pool.AllocatedBytes(target)[11]&=static_cast<std::uint8_t>(~2U);
}
