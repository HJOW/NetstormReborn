// 외부 계산 대체 없이 얻은 실제 PATCH fencemark 결과와 같은 raw 저장소의 C++ 계산을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFenceTarget.h"
using namespace netstorm::test::rawscene;
namespace {
// 원본 관찰과 같은 실제 server SID의 역할 순서다.
constexpr std::array<std::uint16_t,8> kIds{15000,15001,15002,15003,15004,15005,15006,15007};
// 고정 주소의 현재 raw/TYPE/배율/동맹을 같은 실제 분배기에 공급하는 검사 입력이다.
struct FenceFixture {
    SidPool pool{OriginalEdition::Patch1078,24000}; // 실제 server SID를 수용하는 같은 용량이다.
    SquidHash hash; // 원본과 같은 네 단계/현재 선택 단계다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 원본 폭에 대응하는 TYPE 입력이다.
    std::vector<ProjectileGeometryType> geometry=std::vector<ProjectileGeometryType>(188); // 실제 signed SHP 포구 WORD 입력이다.
    std::vector<GunTargetType> supplemental=std::vector<GunTargetType>(188); // 중심 getter와 공유할 현재 보충 표다.
    std::vector<SquidDisplayShape> shapes=std::vector<SquidDisplayShape>(188); // 이 검사에서 그림을 그리지 않는 실제 shape 계약 표다.
    GunTargetState targeting; // 현재 변경되는 실제 픽셀 배율이다.
    GunProcessState process; // 현재 비대칭9×9 동맹 표다.
    RawGunTarget target{pool,hash,types,supplemental,shapes,targeting}; // 기존 실제 발자국 중심 getter다.
    RawFenceTarget fence{pool,hash,types,geometry,target,targeting,process}; // 같은 입력을 빌리는 실제 구현이다.
    // 각 관찰 행의 슬롯/타입/SHP/체인 입력만 복원하며 결과를 계산하지 않는다.
    void Setup(const std::vector<std::string>& row) {
        pool.Reset();hash.Reset();hash.Select(static_cast<int>(Number(row[23])));process={};targeting={};
        targeting.scaleX=std::bit_cast<float>(Number(row[16]));targeting.scaleY=std::bit_cast<float>(Number(row[17]));
        const auto owner=Number(row[14]);process.allies[1][owner]=Number(row[15])!=0;
        types[71].footX=static_cast<int>(Number(row[9]));types[71].footY=static_cast<int>(Number(row[10]));types[71].group=static_cast<int>(Number(row[13]));
        types[158].footX=static_cast<int>(Number(row[11]));types[158].footY=static_cast<int>(Number(row[12]));
        types[136].footX=1;types[136].footY=1;types[90].footX=1;types[90].footY=1;
        types[71].hotspotX=static_cast<std::int32_t>(std::stol(row[18]));types[71].hotspotY=static_cast<std::int32_t>(std::stol(row[19]));
        const auto ox=std::stoi(row[20]),oy=std::stoi(row[21]);geometry[71].shotOffsets.clear();
        // 원본 SHP 헤더에 넣은 네 signed WORD 쌍을 그대로 입력한다.
        for (int frame=0;frame<4;++frame) geometry[71].shotOffsets.push_back({static_cast<std::int16_t>(ox+frame*3),static_cast<std::int16_t>(oy-frame)});
        const std::array<float,2> a{std::bit_cast<float>(Number(row[5])),std::bit_cast<float>(Number(row[6]))},
            b{std::bit_cast<float>(Number(row[7])),std::bit_cast<float>(Number(row[8]))};
        const std::array<std::array<float,2>,8> positions{{
            {std::bit_cast<float>(Number(row[1])),std::bit_cast<float>(Number(row[2]))},
            {std::bit_cast<float>(Number(row[3])),std::bit_cast<float>(Number(row[4]))},a,b,
            {(a[0]+b[0])*0.5f,(a[1]+b[1])*0.5f},{(a[0]+b[0])*0.5f+4,(a[1]+b[1])*0.5f},
            {a[0]+4,a[1]},{b[0]+4,b[1]}}};
        constexpr std::array<std::uint8_t,8> kTypes{71,158,136,136,90,90,136,136}; // 실제 관찰의 같은 TYPE 역할 순서다.
        // 같은 server 순서로 할당한 뒤 원본과 같은 raw 필드를 쓴다.
        for (std::size_t index=0;index<kIds.size();++index) {
            const auto sid=pool.Allocate();CHECK(sid.value==kIds[index]);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            raw[10]=kTypes[index];raw[34]=static_cast<std::uint8_t>(index==0 ? 1 : owner);
            Put(raw,14,std::bit_cast<std::uint32_t>(positions[index][0]));Put(raw,18,std::bit_cast<std::uint32_t>(positions[index][1]));
            Put(raw,36,index==0 ? Number(row[22]) : 0);
            if (index==4 || index==5) {
                const auto first=kIds[index==4 ? 2U : 6U],second=kIds[index==4 ? 3U : 7U];
                Put(raw,12,first,2);Put(raw,26,second>>8);raw[30]=static_cast<std::uint8_t>(second&255);
            }
        }
        const int marks=std::stoi(row[24]);std::vector<std::size_t> registered{0,1,2,3};
        if (std::abs(marks)>=1) registered.push_back(4);if (std::abs(marks)==2) registered.push_back(5);
        if (marks<0) std::reverse(registered.begin(),registered.end());
        // 입력으로 정한 현재 버킷에 머리 삽입하며 finder 결과는 계산하지 않는다.
        for (const auto index:registered) {
            auto& head=hash.Cell(1,static_cast<int>(positions[index][0])/2,static_cast<int>(positions[index][1])/2);
            Put(pool.AllocatedBytes(Sid{kIds[index]}),4,head,2);head=kIds[index];
        }
    }
};
// 선택한 실제 루트 종류의 모든 DWORD를 비교한다. 실패할 때 행 번호와 실제/기대 비트를 남긴다.
void CheckFenceRows(char kind,std::size_t expectedCount) {
    FenceFixture fixture;std::size_t count=0,index=0;
    // 한 행은 Native 고유 루트 한 개이며 반복 재실행을 새 관찰로 세지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_FENCE_TARGET_FIXTURE).rows) {
        ++index;if (row[0][0]!=kind) continue;CHECK(row.size()==29);fixture.Setup(row);
        const auto point=[&](std::size_t x,std::size_t y) { return std::array<float,2>{std::bit_cast<float>(Number(row[x])),std::bit_cast<float>(Number(row[y]))}; }; // 명시 입력 좌표를 읽는다.
        std::array<float,4> output{};
        if (kind=='R') { const auto rect=fixture.fence.SightBounds(Sid{kIds[0]});output={rect.left,rect.top,rect.right,rect.bottom}; }
        else {
            const auto result=kind=='A' ? fixture.fence.Across(Sid{kIds[4]},Sid{kIds[0]},Sid{kIds[1]}) :
                kind=='P' ? fixture.fence.Point(Sid{kIds[0]},Sid{kIds[1]}) : kind=='M' ? fixture.fence.Muzzle(Sid{kIds[0]}) :
                RawFenceTarget::Intersection(point(5,6),point(7,8),point(1,2),point(3,4));
            output[0]=result[0];output[1]=result[1];
        }
        bool same=true;
        // 원본 출력16바이트를 정렬/소수 문자열 반올림 없이 대조한다.
        for (std::size_t column=0;column<4;++column) {
            const auto actual=std::bit_cast<std::uint32_t>(output[column]),expected=Number(row[25+column]);
            if (actual!=expected) { same=false;std::printf("Fence fixture row%zu column%zu actual%08x expected%08x\n",index,column,actual,expected); }
        }
        CHECK(same);++count;
    }
    CHECK(count==expectedCount);
}
}
// 한 배리어의 방향·끝점·발자국·동맹·Cannon/대각/픽셀 배율 보정 원본626개다.
TEST_CASE(FenceTarget_NativeAcross) { CheckFenceRows('A',626); }
// 전체 finder와 현재 해시 단계·후보 유무/가까운 후보 선택 원본48개다.
TEST_CASE(FenceTarget_NativePointFinder) { CheckFenceRows('P',48); }
// 일반 포구/시야 사각형은 각24개 원본과 대조한다.
TEST_CASE(FenceTarget_NativeMuzzleAndSight) { CheckFenceRows('M',24);CheckFenceRows('R',24); }
// 실제 선분의 평행/수직/사선/끝점/소수 교차 원본134개다.
TEST_CASE(FenceTarget_NativeSegmentIntersection) { CheckFenceRows('I',134); }
