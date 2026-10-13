// 실제 저장 Fort 부모의 현재Template 재조회·Territory120BYTE·생성/후처리 순서를 공유한다.
#pragma once
#include "o/RawIslandBuilder.h"
#include "o/RawFortObjectLoad.h"
#include "o/RawFortMoneyLoad.h"
#include "o/RawFortMetaLoad.h"

namespace netstorm::o {
// Territory getter가 현재 pos에서 직접 읽는 원본 MemStream이다. missing 데이터의 가짜0을 제공하지 않는다.
struct FortTerritoryStream {
    std::span<const std::uint8_t> bytes; // 현재 원본 섹션의 실제 물리바이트다.
    std::uint32_t position{},end{}; // getter는position에120을더하고end는보존한다.
};
// 실제 보고 뒤 계속하는 Template/stream 진단이다.
enum class FortWorldLoadAssertion {
    MissingTerritoryTemplate, // Territory를 읽기전 현재Template의첫DWORD가0이다.
    TerritoryPastEnd // 120BYTE 읽은뒤 signed position>end다.
};
// 모든 child는 실제 완료몸체 또는 명시 미복원경계이며같은raw자료를공급받는다.
struct FortWorldLoadHooks {
    std::function<bool(std::uint32_t)> templatePresent; // 전달된현재Template칸의첫DWORD를조회한다.
    std::function<FortTerritoryStream&(std::uint32_t)> territory; // Territory lookup(resetpos0)의현재stream을돌려준다.
    std::function<const FortTemplate&(std::uint32_t)> currentTemplate; // 각후처리직전현재localOwner의저장Template를다시조회한다.
    std::function<void(FortWorldLoadAssertion)> assertion; // 원본보고뒤현재상태로계속하는경계다.
    std::function<void(std::uint32_t)> loadingDepthChanged; // CD Territory의진입즉시증감값을같은소비자에전달한다.
    std::function<void(RawIslandBuilderAssertion)> builderAssertion; // 기존실제Place/PlaceAll의필수진단이다.
    std::function<void(const RawIslandBuilderState&,std::int32_t)> generate; // 같은builder owner/bounds와현재noRegion으로기존정상지형을생성한다.
    std::function<void(const FortTemplate&,std::uint32_t)> objects,money,badges,technology; // 현재Template를각기존전체child에같은풀/깊이로전달한다.
    std::function<void(const RawIslandBuilderState&,std::int32_t)> populateIsland; // param nonzero와현재island0 Exists일때만0046e630/CD004906c0를요청한다.
    std::function<void(bool)> rebuildGraph; // 마지막00463110/CD0045bd60의인자0(false)를그대로전달한다.
};
// 새부모는기존Builder/Runtime를재사용하고미복원전체child를기본성공값으로대체하지않는다.
class RawFortWorldLoad {
public:
    // world/Runtime/frames/현재원본전역은부모보다오래살아야한다. builder의world주소동일성을보존한다.
    RawFortWorldLoad(const SidPool& pool,ChunkMap& world,RawIslandRuntime& islands,std::span<const RiftTypeFrames> frames,
        std::uint32_t& puzzlePiece,std::uint32_t& localOwner,std::int32_t& noRegion,std::uint32_t& loadingDepth,FortWorldLoadHooks hooks);
    // 004b5f70/CD004330c0: ctor/attach→zero120→Territory→PlaceAll→Generate→모드별후처리→Graph0다.
    void Load(std::int32_t alternate) const;
    // 004bd660/CD00427c00:현재pos의120BYTE를복사하고현재pos에120을더한다. 새전체parse로세지않는다.
    void ReadTerritory(std::uint32_t templateSlot,std::span<std::uint8_t,120> output) const;
    // 실제child와현재부모가같은저장소인지확인한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_; // 같은판본/Factory/섬목록의raw풀이다.
    ChunkMap& world_; // 최초builder attach가가리키는실제지도다.
    RawIslandRuntime& islands_; // 현재3204BYTE count/Exists와Place의Add/Bounds를공유한다.
    std::span<const RiftTypeFrames> frames_; // 기존실제CanonDecoder의불변타입별코드다.
    std::uint32_t& puzzlePiece_; // 실제builder가현재읽는puzzlePiece번호다.
    std::uint32_t& localOwner_; // child직전마다다시읽는원본현재플레이어DWORD다.
    std::int32_t& noRegion_; // Generate직전의현재지역없음전역이다.
    std::uint32_t& loadingDepth_; // Territory/CD 및Money/Meta가공유하는현재DWORD다.
    FortWorldLoadHooks hooks_; // 미복원child/출력/진단의필수명시경계다.
};
// 기존ObjectLoad와Meta의완료몸체만연결하며Money와섹션선택/TypeNames/UI는호출자경계를유지한다.
// selectMeta는매child의현재Template칸/포인터/owner를기존MetaState에동기화해야한다.
// metaStream은각호출이실제Badges/Technology섹션을선택해현재cursor를반환해야한다.
FortWorldLoadHooks MakeFortWorldChildrenHooks(const SidPool& pool,const RawFortObjectLoad& objects,
    const RawFortMetaLoad& meta,std::function<void(const FortTemplate&,std::uint32_t,bool)> selectMeta,
    std::function<FortMetaStream&(const FortTemplate&,bool)> metaStream,FortWorldLoadHooks hooks);
}
