// Path의 실제 저장 폭·생성자·시각 계산·한 꼭짓점씩 진행하는 Step을 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 경로에 저장되는 두 float 좌표다. 런타임의 double 좌표와 섞지 않는다.
struct RawPathPoint { float x{},y{}; };
// 원본 PathProcess의 배열/끝 필드 위치다. 판본마다 최대 길이와 마지막 필드 수가 다르다.
struct RawPathLayout {
    std::size_t capacity{},size{},times{},directions{},nextFrame{},current{},angle{},finished{},targetA{},targetB{},helper{},deadFlag{};
    std::uint32_t baseVtable{},teleportVtable{},priestVtable{};
};
// Step이 반환한 좌표·각도·끝 도착 여부다. 끝에 도달해도 현재 각도 필드를 새로 쓰지 않는 분기를 보존한다.
struct RawPathSample { RawPathPoint position; float angle{}; bool finished{}; };
// 실행 관찰 전역이다. 원본 Step은 활성화 비트와 관찰 시계를 쓴다.
struct RawPathObservation { std::uint32_t flags{}; std::uint64_t stamp{}; };
// 원본 메모리 이미지의 뜻 있는 필드를 보존한다. 호스트 포인터와 원본 vtable 주소를 혼동하지 않는다.
class RawPathRuntime {
public:
    // 실제 생성자처럼 시간 배열과 지정 필드만 초기화한다. heapFill은 독립 생성자 대조의 미초기화 입력이다.
    explicit RawPathRuntime(OriginalEdition edition,bool priest=false,std::uint8_t heapFill=0);
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
    // ProcessForm 부착 뒤의 실제 번호를 이미지에 기록한다.
    void SetAttachment(Sid form,Sid parent);
private:
    // 정렬되지 않은 little endian 필드의 읽기/쓰기를 한곳에서 처리한다.
    std::uint64_t Read(std::size_t offset,std::size_t width=4) const;
    void Write(std::size_t offset,std::uint64_t value,std::size_t width=4);
    // 꼭짓점 조회의 원본 assert 범위를 호스트 예외로 진단한다.
    void Check(std::int32_t index) const;
    OriginalEdition edition_;
    RawPathLayout layout_;
    std::vector<std::uint8_t> bytes_;
};
}
