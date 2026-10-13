// 실제 드롭 두 갈래의 owner/좌표/float지연과 CD의 실패생성 task0 호출을 보존한다.
#include "o/RawDudeDrop.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 실제 PE의 두지연상수다. 00501784의3333333f 바이트는ASCII333이 아닌0.7f다.
constexpr float kRandomDelay=0.1f,kPointDelay=0.7f;
// 원본 게임 float난수 분모와 드롭 작업번호다.
constexpr double kRandomDivisor=65536;
constexpr std::uint32_t kRandomTask=0x5103,kPointTask=0x5104;
// 원본 비정렬 현재좌표 DWORD를 IEEE float로 읽는다.
float DropCoordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 원본 low byte부터 네 바이트를 합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return std::bit_cast<float>(value);
}
}
// 같은 풀을 사용한 실제 하위몸체만 연결하며 각생성/Pop의미완료를상위완료로숨기지않는다.
RawDudeDrop::RawDudeDrop(const SidPool& pool,std::span<const RiftTypeRecord> types,RawDudeSpawn& spawn,
    const RawDudeTask& task,const RawDudeNotice& notice,GameRandom& random)
    :pool_(pool),types_(types),spawn_(spawn),task_(task),notice_(notice),random_(random) {
    if (&spawn.Pool()!=&pool_ || &task.Pool()!=&pool_ || &notice.Pool()!=&pool_) throw std::invalid_argument("Dude drop/Spawn/Task/Point SID 풀 불일치");
}
// float max/min의차이는53비트로계산하며최종좌표저장전까지float로절삭하지않는다.
double RawDudeDrop::RandomFloat(float minimum,float maximum) const {
    const double factor=static_cast<double>(random_.Next(65536))/kRandomDivisor;
    return factor*std::abs(static_cast<double>(maximum)-minimum)+minimum;
}
// 원본은source free만검사하므로죽은부모의정상PostDestroy드롭도이호출을사용한다.
void RawDudeDrop::Drop(Sid source,std::int32_t index,bool random) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (!source.value) { if (patch) throw std::logic_error("Dude drop sourceSID 원본 assert");return; }
    const auto raw=pool_.Slot(source);if (raw[11]&1) return;
    const auto type=raw[10];if (type>=types_.size()) throw std::out_of_range("Dude drop 현재 타입 오류");
    std::array<float,2> point;
    if (random) {
        const auto& record=types_[type];
        if (record.footX<0 || record.footY<0 || record.footX>256 || record.footY>256) throw std::out_of_range("Dude drop 원본 발자국 범위 오류");
        const auto x=DropCoordinate(raw,14),y=DropCoordinate(raw,18);
        if (!std::isfinite(x) || !std::isfinite(y)) throw std::invalid_argument("Dude drop 유한 source 좌표 오류");
        // x/y난수소비와각최종float저장순서를원본대로유지한다.
        point[0]=static_cast<float>(static_cast<double>(x)-RandomFloat(kRandomDelay,static_cast<float>(record.footX)-kRandomDelay));
        point[1]=static_cast<float>(static_cast<double>(y)-RandomFloat(kRandomDelay,static_cast<float>(record.footY)-kRandomDelay));
    } else point=notice_.ResidencePoint(source);
    const auto born=spawn_.Spawn(point[0],point[1],pool_.Slot(source)[patch ? 34U : 32U]);
    if (patch && (!born.value || (pool_.Slot(born)[11]&3))) return;
    // CD는SID0/죽은새객체를일반setter에넘기고그실제자연반환을사용한다.
    const auto delay=static_cast<float>(static_cast<double>(index)*(random ? kRandomDelay : kPointDelay));
    task_.Set(born,random ? kRandomTask : kPointTask,delay);
}
// 현재 DamageablePostDestroy가 다른 세계의 생성기를 연결하지 않도록 확인한다.
const SidPool& RawDudeDrop::Pool() const { return pool_; }
}
