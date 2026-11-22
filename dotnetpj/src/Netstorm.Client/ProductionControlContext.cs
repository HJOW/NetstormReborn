using Netstorm.Client.Display;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

namespace Netstorm.Client; // 생산 UI가 실제 장기 owner와 현재 설정을 값 사본 없이 빌린다.

/// <summary>Source ProductionControlContext의 실제 물리/전역/Player와 최초 옵션 참조다.</summary>
public sealed class ProductionControlContext
{
    /// <summary>실제 Client Pool/TYPE/공간의 고정 물리다.</summary>
    public required RawSurfaceStorage Storage { get; init; }
    /// <summary>Session 없는 생성도 읽는 실제 현재 Player/커서/편집 전역이다.</summary>
    public required ClientGameplayGlobals Gameplay { get; init; }
    /// <summary>현재 PlayerKnown일 때 실제 기록을 조회하는 장기 owner다.</summary>
    public required ClientPlayerRecords Players { get; init; }
    /// <summary>생산 부모보다 오래 사는 실제87BYTE 옵션 배열이다.</summary>
    public required byte[] Options { get; init; }
    /// <summary>실제 Client는 같은 분산 SP 저장소를 제공하며 독립 Session만 생략한다.</summary>
    public ClientSpGlobals? Sp { get; init; }
    /// <summary>최초 HUD도 같은 현재 TYPE별 색표 저장소를 빌린다.</summary>
    public OwnerColorTableState? OwnerColorTables { get; init; }
}
