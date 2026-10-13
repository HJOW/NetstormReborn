// child몸체는재작성하지않고같은실제Pool/현재builder범위/Graph0에전달한다.
#include "client/RawFortWorldLoadBinding.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 누락효과나서로다른풀을부모실행전에거부하며모든선택경계의수명을호출자가보장한다.
o::FortWorldLoadHooks MakeFortWorldLoadBinding(RawSurfaceWorld& world,RawNormalTerrainWorldBinding& normal,
    const o::RawFortObjectLoad& objects,const o::RawFortMoneyLoad& money,const o::RawFortMetaLoad& meta,
    std::function<o::FortMoneyCursor&(const o::FortTemplate&,std::uint32_t)> moneyStream,
    std::function<void(const o::FortTemplate&,std::uint32_t,bool)> selectMeta,
    std::function<o::FortMetaStream&(const o::FortTemplate&,bool)> metaStream,o::FortWorldLoadHooks hooks) {
    const auto core=world.Core();if (&normal.Pool()!=&core.pool || !moneyStream)
        throw std::invalid_argument("Fort 부모의실제정상월드/현재Money선택누락");
    hooks=o::MakeFortWorldChildrenHooks(core.pool,objects,meta,std::move(selectMeta),std::move(metaStream),std::move(hooks));
    hooks.generate=[&normal](const o::RawIslandBuilderState& builder,std::int32_t region) {
        normal.Generate({builder.owner,builder.bounds,region,NormalTerrainBranch::Normal});
    };
    hooks.money=[&money,moneyStream=std::move(moneyStream)](const o::FortTemplate& fort,std::uint32_t owner) { money.Load(moneyStream(fort,owner)); };
    hooks.rebuildGraph=[&graph=core.graph](bool resetAll) { graph.Rebuild(resetAll); };return hooks;
}
}
