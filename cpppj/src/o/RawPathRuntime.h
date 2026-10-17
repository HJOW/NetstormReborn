// Path의 실제 저장 폭·생성자·시각 계산·한 꼭짓점씩 진행하는 Step을 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 경로에 저장되는 두 float 좌표다. 런타임의 double 좌표와 섞지 않는다.
struct RawPathPoint { /* x: X 좌표. */ /* y: Y 좌표. */ float x{},y{}; };
// 원본 PathProcess의 배열/끝 필드 위치다. 판본마다 최대 길이와 마지막 필드 수가 다르다.
struct RawPathLayout {
    // capacity: 판본에서 허용하는 경로 꼭짓점의 최대 개수.
    // size: 원본 경로 프로세스의 전체 이미지 바이트 크기.
    // times: 꼭짓점 도착 시각 배열이 시작되는 바이트 오프셋.
    // directions: 꼭짓점 방향 배열이 시작되는 바이트 오프셋.
    // nextFrame: 다음 프레임 예약 필드의 바이트 오프셋.
    // current: 현재 경로 단계 필드의 바이트 오프셋.
    // angle: 현재 회전 각도 필드의 바이트 오프셋.
    // finished: 경로 완료 플래그 필드의 바이트 오프셋.
    // targetA: 도착 가상 함수에 넘길 첫 번째 대상 필드의 바이트 오프셋.
    // targetB: 도착 가상 함수에 넘길 두 번째 대상 필드의 바이트 오프셋.
    // helper: 보조 객체 필드의 바이트 오프셋.
    // deadFlag: 프로세스 사망 플래그 필드의 바이트 오프셋.
    std::size_t capacity{},size{},times{},directions{},nextFrame{},current{},angle{},finished{},targetA{},targetB{},helper{},deadFlag{};
    // baseVtable: 기본 경로 프로세스의 원본 가상 함수 표 주소.
    // teleportVtable: 텔레포트 경로 프로세스의 원본 가상 함수 표 주소.
    // priestVtable: 사제 경로 프로세스의 원본 가상 함수 표 주소.
    std::uint32_t baseVtable{},teleportVtable{},priestVtable{};
};
// Step이 반환한 좌표·각도·끝 도착 여부다. 끝에 도달해도 현재 각도 필드를 새로 쓰지 않는 분기를 보존한다.
struct RawPathSample { /* position: 현재 좌표 또는 저장소의 읽기 위치. */ RawPathPoint position; /* angle: 방향 또는 회전에 사용하는 각도. */ float angle{}; /* finished: 현재 경로 또는 동작이 끝에 도달했는지. */ bool finished{}; };
// 실행 관찰 전역이다. 원본 Step은 활성화 비트와 관찰 시계를 쓴다.
struct RawPathObservation { /* flags: 동작을 선택하는 플래그 비트. */ std::uint32_t flags{}; /* stamp: 관찰 시점에 저장한 원본 시계 비트 값. */ std::uint64_t stamp{}; };
// 원본 메모리 이미지의 뜻 있는 필드를 보존한다. 호스트 포인터와 원본 vtable 주소를 혼동하지 않는다.
class RawPathRuntime {
public:
    // 실제 생성자처럼 시간 배열과 지정 필드만 초기화한다. heapFill은 독립 생성자 대조의 미초기화 입력이다.
    explicit RawPathRuntime(OriginalEdition edition,bool priest=false,std::uint8_t heapFill=0,bool flying=false);
    // PATCH48af00/488d00: 표면/BFS를 조회하지 않고 현재 부모와 목적지를 실제 방향/시각0으로 append한다.
    void InitializeFlight(RawPathPoint parent,RawPathPoint destination);
    double NextFrame() const; // fly39의186b8 현재 예약 시각이다.
    void SetNextFrame(double time); // 실제 프레임이 now+0.02를 저장하는 같은 QWORD다.
    // 패치 0048b2c0/CD0047d4d0의 배열 길이와 끝 필드를 반환한다.
    static RawPathLayout Layout(OriginalEdition edition);
    // 독립 원본 대조에 raw 전체 이미지를 공개한다. 포인터를 호스트에서 실행하지 않는다.
    std::span<const std::uint8_t> Bytes() const;
    // 경로 탐색/네트워크 복원부가 실제 원본 순서로 만든 꼭짓점·방향·시각을 받는다. 직선 경로를 만들어 주지 않는다.
    void SetPath(std::span<const RawPathPoint> points,std::span<const std::int32_t> directions,std::span<const double> times={});
    // 패치 00488e20/CD00402590. 늦은 startTime 선택·판본별 float 중간 저장·회전/거리 시간을 계산한다.
    void Prepare(double now,double startTime,float speed,float turningSpeed,std::int32_t firstDirection);
    // 0048a330/CD0047e820. 한 호출에 최대 한 꼭짓점만 넘으며 짝수 전환에서 현재 부모 위치의 stepSound를 요청한다.
    RawPathSample Step(double now,RawPathPoint parent,RawPathObservation& observation,
        const std::function<void()>& stepSound,std::uint64_t cdObservationStamp=0);
    // 0041cd60/CD0043f460. 4방향 차이에서는 양의 180도를 고른다.
    static double DirectionDelta(std::int32_t previous,std::int32_t next);
    // 경로 개수·현재 단계와 각도/종료를 외부 실제 Process에 제공한다.
    std::int32_t Count() const;
    std::int32_t Current() const;
    float Angle() const;
    bool Finished() const;
    // Init/도착의 실제 필드 쓰기다. ResetCurrent는 finished/angle을 초기화하지 않는다.
    void ResetCurrent();
    void SetFinished(bool value);
    // 원본 raw 상태를 독립 입력/수신 어댑터에서 이관한다. 이동 계산이나 경로 탐색 결과를 새로 만들지 않는다.
    void RestoreStep(std::int32_t current,float angle,bool finished=false);
    // 경로의 현재 방향·시각·좌표를 실제 애니메이션/건설 시각 조정에 제공한다.
    std::int32_t Direction(std::int32_t index) const;
    double Time(std::int32_t index) const;
    RawPathPoint Point(std::int32_t index) const;
    // 부모 가상 도착에 넘기는 두 raw 대상 번호다. 종류/소유자를 새로 해석하지 않는다.
    std::uint32_t TargetA() const;
    std::uint32_t TargetB() const;
    void SetTargets(std::uint32_t first,std::uint32_t second);
    // 시작 상위 몸체의 명령 DWORD 세 개와 패치 전용 목적지 두 float를 실제 끝 필드에 쓴다.
    void SetCommandFields(std::uint32_t helper,std::uint32_t tailWord,std::uint32_t tailByte,RawPathPoint destination);
    // 시작의 연속 꼭짓점 좌표만 덮어쓴다. 시각·방향·개수는 그대로 보존한다.
    void ReplacePoint(std::int32_t index,RawPathPoint point);
    // cleanup이 읽는 helper DWORD다. 도착 명령 대상 두 개와 혼동하지 않는다.
    std::uint32_t Helper() const;
    // SharedRegular 예약 시 helper 필드 하나만0으로 쓴다. 명령 꼬리·목적지·경로는 보존한다.
    void ClearHelper();
    // Dais 다음 목표 이동이 읽는 signedDWORD 깊이다. helper+4(패치186d8/CD2f18)이며 명령 꼬리와 다르다.
    std::int32_t MoveAwayDepth() const;
    // 같은 필드 DWORD만 쓴다. 원본 BridgeCommand가 기존 깊이+1을 보관하며 다른 필드는 그대로다.
    void SetMoveAwayDepth(std::int32_t depth);
    // ProcessForm 부착 뒤의 실제 번호를 이미지에 기록한다.
    void SetAttachment(Sid form,Sid parent);
private:
    // 정렬되지 않은 little endian 필드의 읽기/쓰기를 한곳에서 처리한다.
    std::uint64_t Read(std::size_t offset,std::size_t width=4) const;
    void Write(std::size_t offset,std::uint64_t value,std::size_t width=4);
    // 꼭짓점 조회의 원본 assert 범위를 호스트 예외로 진단한다.
    void Check(std::int32_t index) const;
    // edition_: 판본별 필드 배치와 동작을 선택하는 원본 버전.
    OriginalEdition edition_;
    // layout_: 좌표 배치 계산 또는 판본별 필드 위치 정보.
    RawPathLayout layout_;
    // bytes_: 필드 접근이나 파일 읽기에 사용하는 원시 바이트.
    std::vector<std::uint8_t> bytes_;
};
}
