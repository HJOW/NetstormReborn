// 두 owner를 합치지 않고 원본 getter가 사용하는 현재 전역을 공유한다.
namespace Netstorm.Core.Original;

// Player 월드 상태를 담는 네임스페이스다. 2026-11-07 F2d Player 이식에서 추가됐다.

// 두 owner를 합치지 않고 원본 getter·첫 예약이 사용하는 현재 전역이다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerWorldState
{
    /// <summary>현재 localPlayer 번호가 알려졌는가다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint PlayerKnown { get; set; }

    /// <summary>현재 owner다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint BattleOwner { get; set; } = 1u;

    /// <summary>저장 Template 선택용 별도 owner다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint TemplateOwner { get; set; } = 1u;

    /// <summary>생성·조회가 매 호출 읽는 실제 playerType 전역이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint PlayerType { get; set; } = 152u;

    /// <summary>local 최초 Regular1을 생략하는 조건이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public bool FortMode { get; set; }

    /// <summary>원시 DWORD다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint Special { get; set; }
}
