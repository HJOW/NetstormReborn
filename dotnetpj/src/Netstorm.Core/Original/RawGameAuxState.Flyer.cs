// 공통 비행 생성/홈 조회/상태 writer가 같은 현재 주소를 빌린다.
namespace Netstorm.Core.Original;

/// <summary>RawGameAuxState.h의 장기 공통 비행 TYPE와 상태 헤더다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>005410ec의 최초31이며 일반 월드 준비가 초기화하지 않는다.</summary>
    public UIntBox FlyerProcessTypeBox { get; } = new() { Value = 31 };
    /// <summary>00532780/1의 최초39/1이며 같은 메시지/프로토콜 BYTE다.</summary>
    public byte[] FlyerProcessHeader { get; } = [0x39, 1];
}
