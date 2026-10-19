// 독립 원본469590/46ae80의 주변 총 목록과 통지 순서를 같은 실제 finder/기하와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGunBlocker.h"
#include "o/SquidFactory.h"
#include <cstdio>
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint16_t,5> kGuns{15050,15051,15052,15053,15054}; // 독립 입력의 기존 총 번호다.
constexpr std::array<std::array<float,2>,5> kPositions{{{49,20},{49,80},{20,49},{80,49},{49,45}}}; // 네 캐논 ray와 가까운 발사기의 원점이다.
// 원본 입력의 같은 raw 슬롯·공간 해시·타입과 외부 기존 Gun 자료다.
struct GunBlockerScene {
    SidPool pool{OriginalEdition::Patch1078,24000}; // 실제 서버 SID를 Take로 확보한다.
    SquidHash hash; // 원본 입력 순서대로 등록하는 네 단계 버킷이다.
    std::vector<RiftTypeRecord> types{188}; // 독립 group/발자국/플래그 표다.
    std::vector<GunTargetType> targetTypes{188}; // 같은 TYPE 확장 높이다.
    std::vector<SquidDisplayShape> shapes{188}; // 기존 기하의 현재 SHP 번호 체계다.
    GunTargetState targetingState; // 실제 중심 getter의 기본 상태다.
    GunProcessState process; // 현재 boss/방향별 동맹/friendlyFire다.
    GunBlockerState state{70,20,78}; // 독립 입력이 공급한 원본 최대 사거리와 스톤 타입이다.
    std::vector<std::uint32_t> input; // 현재18개 원본 입력 열이다.
    std::vector<std::string> notices; // 기존 Gun.SetTarget의 실제 외부 통지 순서다.
    // 고정 서버 입력을 정상 Take로 준비하며 임의 SID를 client 번호로 바꾸지 않는다.
    GunBlockerScene() {
        SquidFactory factory(pool,types);
        // 총/현재 표적/새 타워의 원본 번호를 한 번 확보한다.
        for (const auto sid:{15050,15051,15052,15053,15054,15060,15061,15070}) CHECK(factory.Take(100,Sid{static_cast<std::uint16_t>(sid)}).value==sid);
    }
    // free/dead 독립 입력도 다음 행에서 재설정하도록 raw 전체를 검사 전용으로 읽는다.
    std::span<std::uint8_t> Slot(std::uint16_t sid) { const auto raw=pool.Slot(Sid{sid});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 원본 준비와 동일한 TYPE/슬롯/버킷 head 삽입이며 제품 Pop 전체를 대체하는 증거로 세지 않는다.
    void Object(std::uint16_t sid,std::uint8_t type,std::uint8_t flags,std::uint8_t extra,float x,float y,int width,int height,int extension) {
        auto raw=Slot(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;raw[11]=flags;raw[40]=extra;
        Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));types[type].footX=width;types[type].footY=height;targetTypes[type].heightExtension=extension;
        auto& head=hash.Cell(1,static_cast<int>(x)/2,static_cast<int>(y)/2);Put(raw,4,head,2);head=sid;
    }
    // 한 원본 입력을 준비하며 이전 행의 어떤 판단 결과도 빌리지 않는다.
    void Prepare(const std::string& encoded) {
        input.clear();notices.clear();hash.Reset();types.assign(188,{});targetTypes.assign(188,{});process={};
        // 쉼표 입력은 unsigned DWORD 원문 그대로 읽는다.
        for (const auto& value:Split(encoded,',')) input.push_back(Number(value));CHECK(input.size()==18);
        std::fill(Slot(0).begin(),Slot(0).end(),std::uint8_t{});
        // 원본 입력 순서의 역순도 같은 버킷에 머리 삽입한다.
        for (std::size_t order=0;order<kGuns.size();++order) {
            const auto index=input[11] ? kGuns.size()-1-order : order;const auto type=static_cast<std::uint8_t>(100+index);
            Object(kGuns[index],type,static_cast<std::uint8_t>(input[2]),static_cast<std::uint8_t>(input[3]),kPositions[index][0],kPositions[index][1],1,1,0);
            Slot(kGuns[index])[34]=2;types[type].group=input[1]<2 ? static_cast<std::int32_t>(input[1]) : index==4 ? 0 : 1;
        }
        Object(15060,90,0,0,49,150,2,2,static_cast<int>(input[16]));Object(15061,91,0,0,48,48,2,2,static_cast<int>(input[16]));
        Object(15070,78,0,0,std::bit_cast<float>(input[12]),std::bit_cast<float>(input[13]),static_cast<int>(input[14]),static_cast<int>(input[15]),0);Slot(15070)[34]=1;
        types[90].flags2=types[91].flags2=input[9] ? 0x100000U : 0U;
        process.boss=input[17]!=0;process.friendlyFire=input[5]!=0;process.allies[1][2]=input[4]!=0;
    }
    // 원본 실행기의 외부 기존 Gun/방향/포구/통지만 같은 독립 자료로 공급한다.
    GunBlockerHooks Hooks() {
        GunBlockerHooks hooks;
        hooks.currentTarget=[this](Sid sid)->std::optional<Sid> {
            const auto found=std::find(kGuns.begin(),kGuns.end(),sid.value);if (found==kGuns.end()) return std::nullopt;
            const auto index=static_cast<std::size_t>(found-kGuns.begin());if (input[7]&(1U<<index)) return std::nullopt;return Sid{static_cast<std::uint16_t>(input[8])};
        };
        hooks.mode=[this](Sid sid) {
            const auto found=std::find(kGuns.begin(),kGuns.end(),sid.value);if (found==kGuns.end()) return 0U;
            constexpr std::array<std::uint32_t,5> modes{4,0,2,6,0}; // 독립 입력의 현재 총 방향이다.
            return (modes[static_cast<std::size_t>(found-kGuns.begin())]+input[6])%8;
        };
        hooks.muzzle=[this](Sid sid) { const auto raw=pool.Slot(sid);return std::array<float,2>{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))}; };
        hooks.setTarget=[this](Sid gun,Sid blocker) { notices.push_back("N:"+std::to_string(gun.value)+':'+std::to_string(blocker.value)); };
        return hooks;
    }
    // 실제 제품 수집/통지를 실행하고 원본 결과 문자열과 같은 형식으로 반환한다.
    std::array<std::string,2> Run() {
        RawGunTarget target(pool,hash,types,targetTypes,shapes,targetingState);RawGunBlocker logic(pool,hash,types,targetTypes,target,process,state,Hooks());std::string list,events;
        if (input[0]) logic.Notify(Sid{15070});
        else {
            const auto collected=logic.Collect(1,{std::bit_cast<float>(input[12]),std::bit_cast<float>(input[13])},input[10]);
            // 같은 번호의 중복과 원본 목록 순서를 그대로 기록한다.
            for (const auto value:collected) { if (!list.empty()) list+=',';list+=std::to_string(value); }
        }
        // 기존 Gun으로 보낸 명령만 원래 순서대로 기록한다.
        for (const auto& value:notices) { if (!events.empty()) events+=';';events+=value; }
        return {list.empty() ? "-" : list,events.empty() ? "-" : events};
    }
};
}
// 실제 원본의 전체 목록/통지·네 방향·거리 비교·동맹/추상·빈 현재 표적·목록 포화를 검사한다.
TEST_CASE(GunBlocker_NativeCollectAndNotify) {
    const auto rows=LoadFixture(NETSTORM_GUN_BLOCKER_FIXTURE).rows;GunBlockerScene scene;std::size_t count=0,positive=0; // 독립 행과 비어 있지 않은 실제 결과 수다.
    // 기대값을 재계산하지 않고 원본 두 출력과 매 행을 직접 비교한다.
    for (const auto& row:rows) {
        CHECK(row.size()==3);scene.Prepare(row[0]);const auto actual=scene.Run();
        if (actual[0]!=row[1] || actual[1]!=row[2]) {
            std::fprintf(stderr,"GunBlocker input%s list%s/%s events%s/%s\n",row[0].c_str(),actual[0].c_str(),row[1].c_str(),actual[1].c_str(),row[2].c_str());CHECK(false);break;
        }
        ++count;positive+=actual[0]!="-" || actual[1]!="-";
    }
    CHECK(count==690 && positive>100);
}

// 최대 반경20에 정확히 닿는 후보는 포함하고 그보다 먼 후보는 제외하는 실제 원본 경계를 검사한다.
TEST_CASE(GunBlocker_NativeRadiusBoundary) {
    const auto path=std::filesystem::path(NETSTORM_GUN_BLOCKER_FIXTURE).parent_path()/"gun-blocker-boundary-x86.tsv"; // 기존690행을 덮지 않는 별도 원본 입력이다.
    const auto rows=LoadFixture(path.string().c_str()).rows;GunBlockerScene scene;std::size_t count=0; // 여섯 목록/통지 비교 행이다.
    // 아래/동일/위의 실제 원본 결과만 읽고 제품 구현으로 기대값을 만들지 않는다.
    for (const auto& row:rows) {
        CHECK(row.size()==3);scene.Prepare(row[0]);const auto actual=scene.Run();CHECK(actual[0]==row[1] && actual[1]==row[2]);++count;
    }
    CHECK(count==6);
}
