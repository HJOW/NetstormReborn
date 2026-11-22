// 실제 아비어리 생성/상태 writer가 같은 장기 현재 역할을 공유한다.
namespace Netstorm.Core.Original;

/// <summary>RawGameAuxState.h의 훨리베이스 프로세스/플라이어와 현재 헤더다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>005410f0의 최초32이며 일반 월드 준비가 초기화하지 않는다.</summary>
    public UIntBox SunAviaryProcessTypeBox { get; } = new() { Value = 32 };
    /// <summary>0054117c의 최초73이며 실제 생성/수신이 공유할 장기 역할이다.</summary>
    public UIntBox SunFlyerTypeBox { get; } = new() { Value = 73 };
    /// <summary>00532786/7의 최초38/1인 메시지/프로토콜 두BYTE다.</summary>
    public byte[] SunAviaryHeader { get; } = [0x38, 1];
}
