// 단위377: 두 실제Terr와Chaff의현재객체변경을전체Objects저장·작업파일·실제Session재로드로확인한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/CampaignTemplateStorage.h"
#include "client/EditorFiles.h"
#include "o/Islandbuilder.h"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <set>
#include <stdexcept>
using namespace netstorm::client;
namespace {
// ThemeSessionServices와같은외부장치만제공하고실제Factory/섬/저장/로더는Session에맡긴다.
struct ObjectsSnapshotServices {
    SoundList names{OriginalEdition::Patch1078}; // 세션보다오래사는실제소리이름표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 설치의명시영어번역바이트다.
    // 원본계산을대체하지않는소리/번역/표시경계다.게임프레임이나GUI는실행하지않는다.
    RawCampaignSessionServices Make() {
        names.Initialize();RawCampaignSessionServices result;result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.sound.wallSeconds=[] { return 100.0; };result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };
        result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.playerContent.gump=[](std::uint32_t) { return 0U; };result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("Objects검사에서없는UI대상처리"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("Objects검사의미선택삭제효과"); };result.player.log=[](std::uint32_t,std::string_view) {};return result;
    }
};
// 부분snapshot검사가Chaff분기를명시선택한다.Editor설치를가장하지않고실행뒤원래모드를복구한다.
struct ObjectsSnapshotFortMode {
    bool& current; // 같은실제core.ownerMode.fort참조다.
    bool previous; // 검사입력의원래분기값이다.
    // 원본부모의00594fb8비영가지를선택하는명시입력이다.
    explicit ObjectsSnapshotFortMode(bool& flag):current(flag),previous(flag) { current=true; }
    // 오류로중단돼도현재모드를원래값으로돌려준다.
    ~ObjectsSnapshotFortMode() { current=previous; }
};
// 실제Create/Pop로생성한한대상의좌표와기대저장영역이다.
struct ObjectsSnapshotTarget {
    Sid sid; // 같은실제Pool에생성된대상이다.
    int x{},y{}; // 저장/재로드전후비교할실제정수칸이다.
    int territory{}; //0/1은원본두Terr이며-1은실제섬이없는Chaff다.
};
// 보호Territory의패턴배치로예상청크순서만읽는다.실제월드/저장판정은대체하지않는다.
struct ObjectsSnapshotLayout {
    ChunkMap map; // 원본입력의영역→청크대응만확인하는검사용로컬뷰다.
    IslandList islands; // 같은입력배치의번호/영역관계다.실제Session장부로전달하지않는다.
    std::array<std::vector<ChunkCoordinate>,2> chunks; // 저장파일두Terr의원본y/x청크순서다.
    // 실제보호패턴/107프레임으로동일입력의청크대응을만든다.
    ObjectsSnapshotLayout(const FortTemplate& fort,const RiftTypeFrames& frames) {
        IslandBuilder builder(map,islands,frames,1);builder.PlaceAll(fort.territory);
        // 원본TheWarBegins에서활성인두영역을각각별도확인한다.
        for (int region=0;region<2;++region) { chunks[region]=TerritoryChunks(map,islands,1,region);if (chunks[region].empty()) throw std::logic_error("원본두Terr의청크입력을발견하지못함"); }
    }
};
// 실제spot/표면의3×3이입력영역또는빈공중인지확인하고발견실패를숨기지않는다.
std::array<int,2> ObjectsSnapshotPoint(const RawWorldCore& core,const ObjectsSnapshotLayout& layout,int region) {
    // 청크경계와지도테두리안쪽을실제입력순서로검색한다.
    for (int y=4;y<248;++y) {
        // 한행의모든후보를확인한다.
        for (int x=4;x<248;++x) {
            const auto inRegion=[&](int px,int py) { const auto& chunk=layout.map.At(px/16,py/16);return region<0 ? !layout.map.HasIsland(px/16,py/16) : layout.islands.Matches(chunk.island,1,region); }; // 보호배치의어느영역을검사하는지만판정한다.
            if (!inRegion(x,y) || x%16>13 || y%16>13) continue;bool free=true;
            // 실제3×3전체의현재표면/점유를읽고한칸이라도다르면다음후보를선택한다.
            for (int dy=0;dy<3 && free;++dy) {
                // x방향의실제세칸을직접검사한다.
                for (int dx=0;dx<3;++dx) {
                    const auto px=x+dx,py=y+dy;const auto spot=core.spots[static_cast<std::size_t>(py)*256+px];
                    if (!inRegion(px,py) || core.building.HasIsland(static_cast<float>(px),static_cast<float>(py))!=(region>=0) || (spot&0x38U) || (region>=0 ? !(spot&6U) : core.building.Surface(px,py).value!=0)) { free=false;break; }
                }
            }
            if (free) return {x,y};
        }
    }
    throw std::logic_error("Objects검사의실제영역/공중3x3빈지점을발견하지못함");
}
// 저장된고정좌표가해당Terr의몇번째청크인지입력청크순서로확인한다.
std::size_t ObjectsSnapshotChunk(const ObjectsSnapshotTarget& target,const ObjectsSnapshotLayout& layout) {
    if (target.territory<0) return static_cast<std::size_t>(target.y/16)*16+target.x/16;
    const auto& chunks=layout.chunks.at(static_cast<std::size_t>(target.territory));
    // 원본은y바깥/x안쪽으로조건에맞는청크만파일에기록한다.
    for (std::size_t index=0;index<chunks.size();++index) if (chunks[index].x==target.x/16 && chunks[index].y==target.y/16) return index;
    throw std::logic_error("실제대상좌표의예상Terr청크가없음");
}
// 파일의지정청크에서세새대상의현재owner가각각정확히한번기록됐는지확인한다.
void CheckObjectsSnapshotFile(const FortTemplate& fort,const std::array<ObjectsSnapshotTarget,3>& targets,const ObjectsSnapshotLayout& layout,int type,const std::array<std::uint8_t,3>& owners) {
    // 서로다른Terr00/Terr01/Chaff를독립적으로확인한다.
    for (std::size_t index=0;index<targets.size();++index) {
        const auto& target=targets[index];const auto& section=target.territory<0 ? fort.chaff : fort.territories.at(static_cast<std::size_t>(target.territory));CHECK(section.present && section.version==2);
        const auto expectedChunks=target.territory<0 ? std::size_t{256} : layout.chunks.at(static_cast<std::size_t>(target.territory)).size();CHECK(section.chunks.size()==expectedChunks);
        const auto ordinal=ObjectsSnapshotChunk(target,layout);if (ordinal>=section.chunks.size()) throw std::logic_error("저장된전체Terr/Chaff청크누락");std::size_t matches{};
        const auto position=static_cast<std::uint8_t>(((target.x%16)<<4)|(target.y%16)); // 원본위치BYTE는x상위/y하위다.
        // 해당청크내같은현재타입/위치/owner기록을직접읽는다.
        for (const auto& object:section.chunks[ordinal].objects) if (object.type==type && object.position==position && object.owner==owners[index]) ++matches;
        CHECK(matches==1);
    }
}
// 실제재로드한Pool의동일좌표/타입/owner를읽으며성공결과를입력으로주입하지않는다.
void CheckObjectsSnapshotReload(RawCampaignSession& session,const std::array<ObjectsSnapshotTarget,3>& targets,int type,const std::array<std::uint8_t,3>& owners) {
    const auto core=session.Core();std::array<std::size_t,3> matches{};
    // 실제Pool전체에서살아있는저장대상타입만읽는다.
    for (std::uint32_t id=5;id<core.pool.Capacity();++id) {
        const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(id)});if (raw[10]!=type || (raw[11]&7U)) continue;
        // 세대상의서로다른정수좌표와현재owner를각각확인한다.
        for (std::size_t index=0;index<targets.size();++index) if (std::bit_cast<float>(Get(raw,14))==static_cast<float>(targets[index].x) && std::bit_cast<float>(Get(raw,18))==static_cast<float>(targets[index].y) && raw[34]==owners[index]) ++matches[index];
    }
    // 새SID번호는동일조건으로요구하지않고재로드된실제게임자료를확인한다.
    for (const auto count:matches) CHECK(count==1);
}
}
// 전체Objects의실제두Terr/Chaff현재변경을두차례파일저장·재파싱·별도Session재로드로확인한다.
TEST_CASE(CampaignFortObjectsSnapshot_TwoTerritoriesAndChaffRealRoundtrip) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto original=assets.files.Read("d/thewarbegins.fort");const auto source=FortTemplate::Parse(original,assets.game.TypeTable());ObjectsSnapshotLayout layout(source,assets.frames.at(107));
    ObjectsSnapshotServices external;auto services=external.Make();const auto storage=std::make_shared<CampaignTemplateStorage>();services.templateStorage=storage;
    RawCampaignSession session(assets.game,assets.files,source,"thewarbegins",std::move(services));session.Load();const auto core=session.Core();CHECK(session.LocalOwner()==1 && session.TemplateOwner()==1);CHECK(session.FortObjectSettings().chaffMode && !core.ownerMode.fort);
    CHECK(&session.FortObjectSettings().version==&storage->SerializationVersion()); // 실제저장/로더와manager가복제값없이같은DWORD를빌려야한다.
    // 원본두섬의현재신전Owner는각각1/2다.chaffMode의영역선택을두섬모두owner1인상태로가장하지않는다.
    for (std::uint32_t region=0;region<2;++region) {
        const auto owner=region+1;const auto temple=Get(session.PlayerRecords(),owner*0xb4+0x7c);if (!temple || temple>=core.pool.Capacity()) throw std::logic_error("원본두섬의실제현재신전부재");const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(temple)});CHECK(raw[34]==owner);
        const auto x=static_cast<int>(std::bit_cast<float>(Get(raw,14))),y=static_cast<int>(std::bit_cast<float>(Get(raw,18)));CHECK(layout.islands.Matches(layout.map.At(x/16,y/16).island,1,static_cast<int>(region)));
    }
    const auto type=assets.game.TypeTable().Find("windBattery");if (type!=75) throw std::logic_error("실제windBattery타입75입력불일치");CHECK((core.types[static_cast<std::size_t>(type)].flags2&session.FortObjectSettings().ownerSavedMask)!=0);
    std::array<ObjectsSnapshotTarget,3> targets{};
    // 두Terr의실제지면과별도공중에실제Factory/Create/Owner/Pop로새제네레이터를만든다.
    for (std::size_t index=0;index<targets.size();++index) {
        const auto region=index<2 ? static_cast<int>(index) : -1;const auto point=ObjectsSnapshotPoint(core,layout,region);const auto sid=session.World().Create(static_cast<std::uint32_t>(type));core.owner.Set(sid,1);session.World().Pop(sid,static_cast<float>(point[0]),static_cast<float>(point[1]),0);
        targets[index]={sid,point[0],point[1],region};const auto raw=core.pool.Slot(sid);CHECK(!(raw[11]&7U) && !(raw[40]&9U));CHECK(core.building.HasIsland(static_cast<float>(point[0]),static_cast<float>(point[1]))==(region>=0));
    }
    const auto root=assets.root.parent_path();const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();EditorFiles files(root/"cpppj/build/test-data"/("snapshot-full-objects-"+std::to_string(stamp)),{assets.root,root/"originalCD"});const auto path=files.Prepare("FullObjectsSnapshot",original);
    storage->InstallFile(1,FortTemplate::Parse(original,assets.game.TypeTable()),original,path.string());std::vector<std::uint8_t> previous;
    const std::array<std::array<std::uint8_t,3>,2> owners{{{6,7,8},{3,4,5}}}; // 두pass의명시현재owner입력이며저장결과를쓰지않는다.
    // 같은세실제객체의다른현재owner가두차례저장·재로드에반영되어야한다.
    for (std::size_t pass=0;pass<owners.size();++pass) {
        // 기존한청크검사와같이현재rawownerBYTE만수정한다.파일섹션/직렬화결과는수정하지않는다.
        for (std::size_t index=0;index<targets.size();++index) core.pool.AllocatedBytes(targets[index].sid)[34]=owners[pass][index];
        session.SnapshotFortTypeNames(1);
        { ObjectsSnapshotFortMode mode(core.ownerMode.fort);session.SnapshotFortObjects(1); }
        CHECK(session.FortObjectSettings().version==2 && session.FortObjectSettings().scope==2);
        std::uint32_t readOnly{},owner=session.LocalOwner();std::array<std::uint8_t,9> hidden{};FortImageHooks image;image.typeCount=[&] { return static_cast<std::uint32_t>(core.types.size()); };image.assertion=[](auto) { throw std::logic_error("전체Objects작업파일의예상밖이미지진단"); };
        storage->SaveFile(1,{readOnly,owner,hidden},image,files.Writer());const auto saved=ReadFileBytes(path);const auto decoded=FortTemplate::Parse(saved,assets.game.TypeTable());CHECK(saved!=original && saved!=previous);CheckObjectsSnapshotFile(decoded,targets,layout,type,owners[pass]);
        // 이검사는TypeNames/전체Objects만저장한다.다른live snapshot/전체Editor완료로범위를확대하지않는다.
        for (const auto name:{"Territory","Money","Mission","Badges","Deck","State"}) { const auto actual=decoded.Section(name),before=source.Section(name);CHECK(actual.size()==before.size() && std::equal(actual.begin(),actual.end(),before.begin())); }
        if (pass) { const auto before=FortTemplate::Parse(previous,assets.game.TypeTable());
            // 두Terr와Chaff각각의원시저장자료가이전pass와달라야한다.
            for (const auto name:{"Terr00","Terr01","Chaff"}) { const auto actual=decoded.Section(name),prior=before.Section(name);CHECK(actual.size()!=prior.size() || !std::equal(actual.begin(),actual.end(),prior.begin())); }
        }
        ObjectsSnapshotServices reloadExternal;auto reloadServices=reloadExternal.Make();reloadServices.missionFortImage=std::make_shared<const std::vector<std::uint8_t>>(saved);
        RawCampaignSession reloaded(assets.game,assets.files,FortTemplate::Parse(saved,assets.game.TypeTable()),"FullObjectsSnapshotReload",std::move(reloadServices));reloaded.Load();CheckObjectsSnapshotReload(reloaded,targets,type,owners[pass]);previous=saved;
        CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
    }
    CHECK(assets.files.Read("d/thewarbegins.fort")==original);
}
