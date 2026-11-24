// 메타에서 일반 전투로 같은 실제 OS 노드/등록표를 인계하는 소유권 묶음이다.
// cpppj/src/client/MissionNetwork.h 의 MissionNetwork 구조체를 이식한다.
// 0.967에서 R4-6 전송·수신(수송 생성·재결합) 묶음용으로 추가됐다.
namespace Netstorm.Net;

/// <summary>빈 묶음은 기존 새 장치 생성이며 값이 있으면 같은 두 owner를 대여한다.</summary>
public sealed class MissionNetwork
{
    /// <summary>첫 메타 진입은 비어 있고 재진입은 기존 내부 끝까지 같은 owner를 인계한다.</summary>
    public CompositeMissionTransport? Composite { get; init; }

    /// <summary>현재9/10/11과 pending/listener의 실제 TCP owner다. DN 수송은 내부 전용이라 비어 있다.</summary>
    public LocalTcpTransport? Tcp { get; init; }

    /// <summary>열려 있다면 현재12의 같은 데이터그램 owner다. DN 수송은 내부 전용이라 비어 있다.</summary>
    public LocalUdpTransport? Udp { get; init; }

    /// <summary>등록된 기존 메타 callback의 실제 전체 owner를 함께 pin한다.</summary>
    public object? Lifetime { get; init; }

    // Editor 전송은 DN에 대응 장치가 없어 묶음에 두지 않는다. Editor 역할·LAN backend 인계는 R11에서 잇는다.
}
