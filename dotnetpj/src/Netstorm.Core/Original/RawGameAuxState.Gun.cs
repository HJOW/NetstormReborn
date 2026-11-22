// Gun 생성/회전/패킷 소비자가 공유하는 실제 현재 역할과 헤더다.
namespace Netstorm.Core.Original;

/// <summary>RawGameAuxState.h의 Gun 관련 장기 주소다. 일반 월드 준비로 초기화하지 않는다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>005410f8의 최초34 프로세스 TYPE다.</summary>
    public UIntBox GunProcessTypeBox { get; } = new() { Value = 34 };
    /// <summary>005409b4/b5의 최초3a/1 메시지/프로토콜 BYTE다.</summary>
    public byte[] GunProcessHeader { get; } = [0x3a, 1];
    /// <summary>00541288의 최초140 회전 분기 윈드 아처 TYPE다.</summary>
    public UIntBox WindArcherTypeBox { get; } = new() { Value = 140 };
    /// <summary>00541264의 최초131 회전 분기 썬더 캐논 TYPE다.</summary>
    public UIntBox ThunderCannonTypeBox { get; } = new() { Value = 131 };
    /// <summary>0054125c의 최초129 회전 분기 아이스 캐논 TYPE다.</summary>
    public UIntBox RainCannonTypeBox { get; } = new() { Value = 129 };
}
