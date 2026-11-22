// 맨 오 워 풀의 실제 생성/상태 writer가 같은 장기 역할과 헤더를 빌린다.
namespace Netstorm.Core.Original;

/// <summary>RawGameAuxState.h의 현재60 프로세스와73h 헤더다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>00541160의 최초60이며 일반 월드 준비가 초기화하지 않는다.</summary>
    public UIntBox RainAviaryProcessTypeBox { get; } = new() { Value = 60 };
    /// <summary>0053278e/f의 최초73/1인 메시지/프로토콜 두BYTE다.</summary>
    public byte[] RainAviaryHeader { get; } = [0x73, 1];
}
