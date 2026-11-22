// 데빌메이커의 실제Host/상태writer가 같은현재 장기 역할을 공유한다.
namespace Netstorm.Core.Original;

/// <summary>RawGameAuxState.h의 현재59프로세스와74h헤더다. 138역할은 기존상자를 사용한다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>0054115c의 최초59이며 일반월드 준비가 초기화하지 않는다.</summary>
    public UIntBox WindAviaryProcessTypeBox { get; } = new() { Value = 59 };
    /// <summary>0053278a/b의 최초74/1인 메시지/프로토콜 두BYTE다.</summary>
    public byte[] WindAviaryHeader { get; } = [0x74, 1];
}
