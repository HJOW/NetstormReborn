// GunProcess의 실제 Cannon/Archer 표적 검색과 MayShoot를 같은 raw 풀·해시에 연결한다.
#pragma once
#include "o/RawGunProcess.h"
#include "o/RawSquidFinder.h"
#include "o/SquidDisplay.h"

namespace netstorm::o {
// 원본 Gunprocess.cpp가 값으로 복사하는 8DWORD 검색/발사 문맥이다.
struct GunTargetQuery {
    float x{},y{}; // 객체 Pop 위치이며 Cannon ray의 SHP 핫스폿 보정 시작점이다.
    float centerX{},centerY{}; // 발자국의 절반을 뺀 현재 중심이다.
    std::uint32_t type{}; // 발사자의 현재 타입 번호다.
    std::int32_t direction{}; // 공격 모드의 8방향 값이며 Wind 관측각과 Cannon 고정 방향에 쓴다.
    std::uint32_t owner{}; // 동맹 마스크를 읽는 현재 소유자 번호다.
    std::int32_t frame{}; // 썬 캐논 MayShoot가 검사하는 현재 그림 프레임이다.
};
// 원본 float 사각형이다. Cannon ray/현재 발자국/표적 경계점은 이 값을 공유한다.
struct GunTargetRect {
    float left{},top{},right{},bottom{}; // 각 경계는 포함되며 오른쪽/아래도 후보와 접촉할 수 있다.
};
// RiftTypeRecord 밖의 실제 타입 +44/+98/+15c..168/+1e4(CD+1c4) 검색 자료다.
struct GunTargetType {
    float range{}; // +44의 칸 단위 사거리이며 Archer는 엄격히 작은 거리만 허용한다.
    std::int32_t theme{}; // +98의 테마 번호이며 Rain1/Thunder2 Cannon은 지정한 방향으로만 쏜다.
    std::array<std::int32_t,4> directionFrames{}; // +15c의 L/M/N/O 첫 프레임이며 -1은0으로 변환한다.
    int heightExtension{}; // 확장된 발자국 높이 +1e4/CD+1c4다.
};
// 실제 전역/Player 표를 조회하는 상태다. 호출자는 같은 월드의 현재 값으로 갱신한다.
struct GunTargetState {
    bool friendlyFire{}; // shootMyFriends 전역: 동맹 후보의 배제를 해제한다.
    bool checking{}; // 패치의 미지원 group에 대한 flyer 원본 assert를 적용한다.
    bool showRanges{}; // 원본 진단 사각형 그리기를 켠다. 필요한 sink가 없으면 예외다.
    std::uint32_t sunCannonType{128},sunFenceType{136},windArcherType{140},thunderArcherType{148}; // 원본 전역 타입 번호다.
    float windHalfAngle{30}; // 005409c0/CD0051a198: 크로스보우 관측각의 절반이다.
    float scaleX{16},scaleY{11}; // Cannon SHP 핫스폿을 칸 좌표로 바꾸는 현재 픽셀 배율이다.
    std::array<std::uint32_t,kPlayerCount+1> allyMasks{}; // Player+54의 소유자별 동맹 비트 마스크다.
    std::array<std::uint8_t,kPlayerCount+1> playerFlags{}; // Player+50의 진단 ray 표시 억제 bit20이다.
    std::function<std::uint32_t(std::uint32_t)> allyMaskRead; // 실제 세션의 현재 Player+54를 해당 원본 읽기 위치에서 조회한다. 없으면 독립 검사 배열을 사용한다.
    std::function<std::uint32_t(std::uint32_t)> playerFlagsRead; // 실제 세션의 현재 Player+50을 진단 출력 조건에서 조회한다.
    // 현재 소유자 행의 마스크를 반환한다. 실제 세션은 동일 Player 저장소 조회기를 연결한다.
    std::uint32_t AllyMask(std::uint32_t owner) const;
    // 현재 참여 플래그를 반환한다. AI 부착 뒤에도 초기 사본을 읽지 않는다.
    std::uint32_t PlayerFlags(std::uint32_t owner) const;
};
// 게임 출력 부수효과를 명시적으로 연결한다. 검색·기하 계산은 훅으로 대체하지 않는다.
struct GunTargetHooks {
    std::function<void(GunTargetRect,bool)> drawRange; // ray는false, 최종 표적 발자국은true인 원본 진단 출력 경계다.
};
// 실제 후보 finder·현재 raw 필터·사거리/각도를 복원한 독립 런타임이다.
class RawGunTarget {
public:
    // 타입·프레임·SHP·상태의 수명은 호출자가 보장한다. 모든 조회는 현재 같은 풀·해시를 사용한다.
    RawGunTarget(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const GunTargetType> targetTypes,std::span<const SquidDisplayShape> shapes,
        GunTargetState& state,GunTargetHooks hooks={});
    // 0046b6f0/CD00435e10: 중심/프레임을 구성해 group0 Archer 또는 group1 Cannon finder로 검색한다.
    Sid FindTarget(float x,float y,std::uint32_t type,std::int32_t direction,std::uint32_t owner) const;
    // 0046af80/CD00434b20: 서버 번호·damageable·동맹·extra·사제 수량·동일 위치·MayShoot를 검사한다.
    bool MayTarget(const GunTargetQuery& query,Sid target) const;
    // 0046ac00/CD00435580: group0 사거리/윈드각, group1 Cannon ray, group3 직접 허용이다.
    bool MayShoot(const GunTargetQuery& query,Sid target) const;
    // 0046b070/CD00434c30: 썬 캐논의 현재 프레임 방향 검사 뒤 실제 MayTarget을 부른다. 투사체 생성이 아니다.
    bool MayFire(const GunTargetQuery& query,Sid target) const;
    // 00469190/CD00434d60: 지정 방향의 SHP 핫스폿·사거리 사각형이다. 실제4DWORD 반환을 보존한다.
    GunTargetRect CannonRay(const GunTargetQuery& query,std::int32_t direction) const;
    // 현재 객체의 발자국이다. 기본 extended=true는 공격 검색이며 false는 사정거리 표시의 원본 flags0이다.
    GunTargetRect Footprint(Sid target,bool extended=true) const;
    // 원본4d4c60/469120을 사정거리 표시와 공유한다. 방향은 원본의0/2/4/6을 받는다.
    std::int32_t DirectionFrame(std::uint32_t type,std::int32_t direction) const;
    bool CannonDirection(const GunTargetQuery& query,std::int32_t direction) const;
    // 확장 높이를 포함하지 않는 현재 타입 발자국의 실제 중심이다.
    std::array<float,2> Center(Sid target) const;
    // 같은 raw 월드에 대한 GunProcess 어댑터인지 확인한다.
    const SidPool& Pool() const;
private:
    // 입력 타입/소유자/유한 좌표와 물리 자료를 원본 정상 호출 계약으로 제한한다.
    void Validate(const GunTargetQuery& query) const;
    // 실제 Archer range 및 Wind 관측각의 포함 경계를 검사한다.
    bool ArcherShot(const GunTargetQuery& query,Sid target) const;
    // 실제 Cannon ray와 현재 확장 발자국의 접촉을 검사한다.
    bool CannonShot(const GunTargetQuery& query,Sid target) const;
    // 밴더 타워의 walker/flyer 필터와 추가 ArcherShot을 실행한다.
    bool ArcherTarget(const GunTargetQuery& query,Sid target) const;
    // 사각형의 발사자 방향 경계점을 원본 RectPoint/RectCoord로 선택한다.
    std::array<float,2> HitPoint(const GunTargetQuery& query,GunTargetRect rect) const;
    // 같은 거리의 후보는 첫 후보를 유지하며 확장된 일반 finder를 실제 순서로 순회한다.
    Sid FindArcher(const GunTargetQuery& query) const;
    //0/2/4/6 방향 순서의 사정선 finder에서 가장 작은 현재 점수를 고른다.
    Sid FindCannon(const GunTargetQuery& query) const;
    // pool_: 현재 후보 슬롯과 raw 필드를 조회하는 공유 SID 풀이다.
    const SidPool& pool_;
    // hash_: 일반 finder가 읽는 네 단계 공간 해시다.
    const SquidHash& hash_;
    // types_: 현재 후보의 플래그·발자국·group·타입 핫스폿이다.
    std::span<const RiftTypeRecord> types_;
    // targetTypes_: 사거리·패턴·방향별 프레임·확장 높이의 실제 타입 표다.
    std::span<const GunTargetType> targetTypes_;
    // shapes_: Cannon 발사 프레임의 물리 SHP 헤더다.
    std::span<const SquidDisplayShape> shapes_;
    // state_: 현재 동맹/전역/픽셀 배율이다.
    GunTargetState& state_;
    // hooks_: 활성화된 원본 진단 그리기를 전달하는 효과다.
    GunTargetHooks hooks_;
};
// 실제 표적 검색/MayFire만 같은 풀의 GunProcess에 연결한다. 나머지 효과 경계는 보존한다.
GunProcessHooks MakeGunTargetHooks(const SidPool& pool,const RawGunTarget& target,GunProcessHooks hooks);
}
