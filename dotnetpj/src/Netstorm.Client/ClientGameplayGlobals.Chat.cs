using Netstorm.Core.Original;

// Chat 명령과 후속 Mana 공급 모듈이 같은 장기 치트 DWORD를 빌린다.
namespace Netstorm.Client;

/// <summary>전체 EnergySupplyState 이식에 앞서 현재005cab30의 실제 주소를 유지한다.</summary>
public sealed partial class ClientGameplayGlobals
{
    /// <summary>005cab30의 BSS0이다. 명령/Session/모드 교체로 초기화하지 않으며 공급 판정도 이 상자를 사용해야 한다.</summary>
    public UIntBox UserCheat { get; } = new();
    /// <summary>0055a484의 별도 BSS DWORD다. 옵션의 특수 슬롯 초기화가 Player.Special과 함께0을 쓴다.</summary>
    public UIntBox SpecialAuxiliary { get; } = new();
    /// <summary>005953fc의 현재 로컬 주소 BSS0이며 Identity/객체 메뉴가 같은 값을 빌린다. 실제 주소 writer는 후순위 LAN이다.</summary>
    public UIntBox LocalAddress { get; } = new();
}
