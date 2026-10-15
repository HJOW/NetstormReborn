// 두 PE의 실제 Gun 후보/거리/각도/finder/사정선/진단 출력 결과를 현재 raw 월드에서 재생한다.
#include "RawSceneSupport.h"
#include "o/RawGunTarget.h"
#include "o/SquidFactory.h"
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// fixture의 같은 source Pop 좌표와 현재 중심이다. 판단 결과를 만드는 상수는 아니다.
constexpr float kSourceX=50.75f,kSourceY=52.5f,kCenterX=50.25f,kCenterY=52.0f;
// 실제 공유 raw 풀·해시·타입/SHP와 진단 효과 사건을 묶은 장면이다.
struct GunTargetScene {
    bool patch; // 원본 슬롯 폭·서버 대역·수량 폭을 선택한다.
    SidPool pool; // 실제 candidate SID 및 current raw 데이터를 소유한다.
    SquidHash hash; // 네 단계 일반 finder의 공유 해시다.
    std::vector<RiftTypeRecord> types; // 후보의 현재 HP/genus·발자국·group·핫스폿이다.
    std::vector<GunTargetType> targetTypes; // source의 실제 range/theme/방향 프레임·확장 높이다.
    std::vector<SquidDisplayShape> shapes; // Cannon 사정선의 실제 SHP 핫스폿 입력이다.
    GunTargetState state; // 동맹·친구 발사·진단/배율의 현재 전역이다.
    std::vector<std::string> events; // 실제 drawRange 요청 순서/float 비트다.
    // 원본 client 할당/반납과 server/예측 Take로 고정 native 입력 번호를 확보한다.
    explicit GunTargetScene(bool chosen):patch(chosen),pool(chosen ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,24000),
        types(chosen ? 188 : 171),targetTypes(types.size()),shapes(types.size()) {
        // 마지막 free 슬롯은 원본의 예약 꼬리로 남기며 그 직전까지만 일반 할당한다.
        for (std::uint16_t sid=5;sid<pool.Layout().serverFirst-1;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        // 미사용6번을 반납해 새 예약 꼬리로 옮기면 이전 마지막 슬롯도 정상 Allocate가 가능하다.
        pool.Release(Sid{6});CHECK(pool.Allocate(2)==Sid{static_cast<std::uint16_t>(pool.Layout().serverFirst-1)});
        SquidFactory factory(pool,types);
        // 고정 native 번호의 서버/예측 수신은 실제 Take 계약을 사용한다. 후보를 client60으로 대체하지 않는다.
        for (const auto sid:{Sid{6000},Sid{14999},Sid{15000},Sid{15050},Sid{15060},Sid{15070}})
            if (sid.value>=pool.Layout().serverFirst) CHECK(factory.Take(86,sid)==sid);
    }
    // 독립 native 입력을 실제 chain/head에 같은 순서로 등록한다. Pop 전체를 증명하는 준비 함수가 아니다.
    void Object(Sid sid,std::uint8_t type,std::uint8_t extra,std::uint32_t xb,std::uint32_t yb,int width,int height,int extension,int level) {
        auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;raw[patch ? 40U : 35U]=extra;
        Put(raw,14,xb);Put(raw,18,yb);types[type].footX=width;types[type].footY=height;targetTypes[type].heightExtension=extension;
        const auto scale=SquidHash::kScales[static_cast<std::size_t>(level)];const auto x=static_cast<int>(std::bit_cast<float>(xb))/scale,y=static_cast<int>(std::bit_cast<float>(yb))/scale;
        auto& head=hash.Cell(level,x,y);Put(raw,4,head,2);head=sid.value;
    }
    // 입력 칸의 각 값만 옮기며 기대 판단은 native fixture에서 읽는다.
    void Prepare(std::span<const std::uint32_t> input) {
        CHECK(input.size()==18);events.clear();hash=SquidHash{};state={};types.assign(types.size(),{});targetTypes.assign(targetTypes.size(),{});shapes.assign(shapes.size(),{});
        // native는 매 입력의 전체 풀을0으로 준비하므로 비등록 후보도 이전 타입을 남기지 않는다.
        for (const auto sid:{Sid{60},Sid{5999},Sid{6000},Sid{14999},Sid{15000},Sid{15050},Sid{15060},Sid{15070}}) {
            auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        }
        const auto scene=input[15];const auto sid=Sid{static_cast<std::uint16_t>(input[17])};
        const int width=scene ? 3 : 1,height=scene ? 2 : 1,extension=scene==2 ? 3 : 0;
        // scene3만 후보 입력 순서를 뒤집어 동일 점수의 원본 첫 후보 보존을 검사한다.
        if (input[0]==0 && scene==3) {
            Object(Sid{15070},88,0,std::bit_cast<std::uint32_t>(50.75f),std::bit_cast<std::uint32_t>(56.5f),2,2,1,2);
            Object(Sid{15060},87,0,std::bit_cast<std::uint32_t>(55.75f),std::bit_cast<std::uint32_t>(52.5f),1,1,0,1);
            Object(sid,86,static_cast<std::uint8_t>(input[7]),input[5],input[6],width,height,extension,1);
        } else {
            Object(sid,86,static_cast<std::uint8_t>(input[7]),input[5],input[6],width,height,extension,1);
            if (input[0]==0) {
                Object(Sid{15060},87,0,std::bit_cast<std::uint32_t>(53.75f),std::bit_cast<std::uint32_t>(52.5f),1,1,0,1);
                Object(Sid{15070},88,0,std::bit_cast<std::uint32_t>(50.75f),std::bit_cast<std::uint32_t>(56.5f),2,2,1,2);
            }
        }
        // 모든 source 타입은 같은 독립 metadata를 받으며 특수 group만 다르다.
        for (const auto number:{71U,128U,129U,140U,148U,83U}) {
            auto& type=types[number];type.group=(number==128 || number==129) ? 1 : number==83 ? 3 : 0;type.footX=type.footY=1;
            type.hotspotX=scene ? 2 : 0;type.hotspotY=scene ? -3 : 0;
            targetTypes[number].range=std::bit_cast<float>(input[4]);targetTypes[number].theme=static_cast<std::int32_t>(input[3]);targetTypes[number].directionFrames={0,1,2,3};
            auto& shape=shapes[number];shape.loaded=true;shape.frameCount=4;shape.frames.resize(4);
            // 실제 sprite 헤더 입력의 signed short 값을 순서대로 옮긴다.
            for (std::size_t i=0;i<shape.frames.size();++i) {
                shape.frames[i].hotspotX=static_cast<std::int16_t>(scene ? 3*static_cast<int>(i)-4 : 0);
                shape.frames[i].hotspotY=static_cast<std::int16_t>(scene ? static_cast<int>(i)-2 : 0);
            }
        }
        // 수량/owner는 현재 raw 슬롯이고 후보 flags는 현재 타입 표에서 다시 읽는다.
        for (const auto target:{sid,Sid{15060},Sid{15070}}) {
            auto raw=pool.AllocatedBytes(target);raw[patch ? 34U : 32U]=static_cast<std::uint8_t>(input[11]);Put(raw,26,input[10],patch ? 4U : 2U);
            types[raw[10]].flags1=input[8];types[raw[10]].flags2=target==sid ? input[9] : target.value==15060 ? 0x100000U : 0U;
        }
        state.friendlyFire=input[13]!=0;state.showRanges=input[16]!=0;state.allyMasks[1]=(1U<<1)|(input[12] ? 1U<<input[11] : 0U);
    }
    // 원본 진단 rectangle 출력 경계만 기록하고 실제 후보/기하 결과를 대체하지 않는다.
    GunTargetHooks Hooks() {
        return {[this](GunTargetRect rect,bool target) {
            std::string text="D:"+std::to_string(target ? 1 : 0)+":";
            // 네 float 경계를 native 비트 순서로 기록한다.
            for (const auto value:{rect.left,rect.top,rect.right,rect.bottom}) { if (text.back()!=':') text+=',';text+=std::to_string(std::bit_cast<std::uint32_t>(value)); }
            events.push_back(std::move(text));
        }};
    }
    // 출력 목록의 원본 순서를 세미콜론 문자열로 만든다.
    std::string Events() const {
        std::string text;
        // 발생한 요청만 원래 순서대로 이어 붙인다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        return text.empty() ? "-" : text;
    }
};
// 사정선/발자국의 네 float 비트를 저장된 native 결과 형식으로 만든다.
std::string RectBits(GunTargetRect rect) {
    std::string result;
    // 왼쪽/위/오른쪽/아래 순서대로 비교한다.
    for (const auto value:{rect.left,rect.top,rect.right,rect.bottom}) { if (!result.empty()) result+=',';result+=std::to_string(std::bit_cast<std::uint32_t>(value)); }
    return result;
}
}

// 같은 원본 입력을 값형 준비 또는 현재 Player 조회기로 재생한다. 기대값은 native fixture를 유지한다.
void ReplayGunTarget(bool currentPlayers) {
    const auto fixture=LoadFixture(NETSTORM_GUNTARGET_FIXTURE);GunTargetScene patch(true),cd(false);std::size_t compared=0;
    std::size_t maskReads=0,flagReads=0; // 현재 저장소가 실제 분기에서 조회되는지 확인하는 관찰 수다.
    // 각 고유 원본 입력은 두 정밀도에서 같은 결과임이 별도 native 보고서에 기록되어 있다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==5);auto& scene=row[0]=="originals" ? patch : cd;std::vector<std::uint32_t> input;
        // 기대값 계산 없이18개 입력 칸을 같은 순서로 옮긴다.
        for (const auto& value:Split(row[1],',')) input.push_back(Number(value));scene.Prepare(input);
        auto masks=scene.state.allyMasks;auto flags=scene.state.playerFlags; // getter 수명보다 오래 사는 별도 현재 Player 입력이다.
        if (currentPlayers) {
            scene.state.allyMaskRead=[&](std::uint32_t owner) { ++maskReads;return masks.at(owner); };
            scene.state.playerFlagsRead=[&](std::uint32_t owner) { ++flagReads;return flags.at(owner); };
            // fallback이 잘못 사용되면 기존 native 출력과 달라지는 반대 입력을 남긴다.
            scene.state.allyMasks.fill(0xffffffffU);scene.state.playerFlags.fill(0x20);
        }
        RawGunTarget logic(scene.pool,scene.hash,scene.types,scene.targetTypes,scene.shapes,scene.state,scene.Hooks());
        GunTargetQuery query{kSourceX,kSourceY,kCenterX,kCenterY,input[1],std::bit_cast<std::int32_t>(input[2]),1,std::bit_cast<std::int32_t>(input[14])};
        const auto target=Sid{static_cast<std::uint16_t>(input[17])};std::uint32_t result=0;GunTargetRect rect{};
        switch (input[0]) {
        case 0:result=logic.FindTarget(query.x,query.y,query.type,query.direction,query.owner).value;break;
        case 1:result=logic.MayTarget(query,target);break;
        case 2:result=logic.MayShoot(query,target);break;
        case 3:result=logic.MayFire(query,target);break;
        case 4:rect=logic.CannonRay(query,query.direction);break;
        case 5:rect=logic.Footprint(target);break;
        default:throw std::runtime_error("Gun fixture kind 오류");
        }
        if (result!=Number(row[2]) || RectBits(rect)!=row[3] || scene.Events()!=row[4]) {
            std::fprintf(stderr,"GunTarget %s input%s result%u/%s rect%s/%s events%s/%s\n",row[0].c_str(),row[1].c_str(),result,row[2].c_str(),RectBits(rect).c_str(),row[3].c_str(),scene.Events().c_str(),row[4].c_str());
            CHECK(false);
        }
        ++compared;
    }
    CHECK(compared>1000);
    if (currentPlayers) CHECK(maskReads>0 && flagReads>0);
}
// 두 판본의 기존 값형 입력 대조를 보존한다.
TEST_CASE(rawGunTarget_native_all) { ReplayGunTarget(false); }
// 같은 native 입력으로 현재 Player 읽기를 검증하며 신규 원본 관찰 수로 합산하지 않는다.
TEST_CASE(rawGunTarget_current_player_reads) { ReplayGunTarget(true); }

TEST_CASE(rawGunTarget_bindings_and_missing_effect) {
    GunTargetScene scene(true);const std::array<std::uint32_t,18> input{4,128,0,0,std::bit_cast<std::uint32_t>(10.5f),std::bit_cast<std::uint32_t>(55.75f),std::bit_cast<std::uint32_t>(52.5f),0,0x10,0,2,2,0,0,1,0,1,15050};scene.Prepare(input);
    RawGunTarget missing(scene.pool,scene.hash,scene.types,scene.targetTypes,scene.shapes,scene.state);
    const GunTargetQuery query{kSourceX,kSourceY,kCenterX,kCenterY,128,0,1,1};CHECK(Throws([&] { missing.CannonRay(query,0); }));
    RawGunTarget logic(scene.pool,scene.hash,scene.types,scene.targetTypes,scene.shapes,scene.state,scene.Hooks());
    GunProcessHooks original;bool preserved=false;original.refreshRange=[&](Sid) { preserved=true; };
    const auto hooks=MakeGunTargetHooks(scene.pool,logic,original);CHECK(hooks.findTarget && hooks.shoot);hooks.refreshRange(Sid{15050});CHECK(preserved);
    SidPool other(OriginalEdition::Patch1078,24000);CHECK(Throws([&] { MakeGunTargetHooks(other,logic,{}); }));
    // 플레이어 bit20이면 ray 진단 출력만 생략하며 검색/판정은 그대로 실행된다.
    scene.state.playerFlags[1]=0x20;scene.events.clear();logic.CannonRay(query,0);CHECK(scene.events.empty());
}

TEST_CASE(rawGunTarget_actual_gun_frame_fire) {
    GunTargetScene scene(true);const std::array<std::uint32_t,18> input{3,128,0,0,std::bit_cast<std::uint32_t>(10.5f),std::bit_cast<std::uint32_t>(55.75f),std::bit_cast<std::uint32_t>(52.5f),0,0x10,0,2,2,0,0,3,0,0,15050};scene.Prepare(input);
    const Sid parent{15000};auto raw=scene.pool.AllocatedBytes(parent);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=128;raw[34]=1;
    Put(raw,14,std::bit_cast<std::uint32_t>(kSourceX));Put(raw,18,std::bit_cast<std::uint32_t>(kSourceY));Put(raw,36,3);
    RawGunTarget target(scene.pool,scene.hash,scene.types,scene.targetTypes,scene.shapes,scene.state,scene.Hooks());
    GunProcessState state;state.now=10;std::vector<std::string> events;GunProcessHooks boundaries;
    // 실제 parentBlocked raw bit20과 현재8DWORD shot 문맥을 같은 풀에서 직접 공급한다.
    boundaries.parentBlocked=[&](Sid sid) { return (scene.pool.Slot(sid)[40]&0x20)!=0; };
    boundaries.shotContext=[&](Sid sid) {
        const auto image=scene.pool.Slot(sid);const auto center=target.Center(sid);
        return std::array<std::uint32_t,8>{Get(image,14),Get(image,18),std::bit_cast<std::uint32_t>(center[0]),std::bit_cast<std::uint32_t>(center[1]),image[10],0,image[34],Get(image,36)};
    };
    // 타입 발사 간격 로더/감시 제거/전송은 이 검사에서 아직 명시 효과 경계다.
    boundaries.firingInterval=[](Sid) { return 2.25; };
    boundaries.removeDependency=[&](Sid form,Sid sid) { events.push_back("D:"+std::to_string(form.value)+':'+std::to_string(sid.value)); };
    boundaries.repopForm=[&](GunProcessFields&) { events.push_back("P"); };
    RawGunProcess gun(scene.pool,scene.types,state,MakeGunTargetHooks(scene.pool,target,std::move(boundaries)));GunProcessFields fields;gun.Initialize(fields,parent);
    Put(fields.bytes,44,5);Put(fields.bytes,48,parent.value);Put(fields.bytes,24,15050,2);Put(fields.bytes,28,7);
    gun.Frame(fields);CHECK(fields.Stage()==2);CHECK(fields.Target()==Sid{15050});CHECK(fields.Time()==12.25);CHECK(events.empty());
    // 방향에 맞지 않는 현재 프레임에서는 실제 MayFire가 거부해 ClearTarget/상태1/전송을 실행한다.
    Put(raw,36,1);Put(fields.bytes,28,7);Put(fields.bytes,16,static_cast<std::uint32_t>(std::bit_cast<std::uint64_t>(state.now)));Put(fields.bytes,20,static_cast<std::uint32_t>(std::bit_cast<std::uint64_t>(state.now)>>32));
    gun.Frame(fields);CHECK(fields.Stage()==1);CHECK(fields.Target()==Sid{});CHECK(fields.Time()==12.25);CHECK(events==std::vector<std::string>({"D:5:15050","P"}));
}
