using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>한 플레이어의 게임 상태: Storm Power, 생산 창(덱), 다리 조각 칸, 미션이 정한 기술 허용.</summary>
public sealed class PlayerState
{
    /// <summary>플레이어 번호 (1부터)</summary>
    public int Number { get; }

    /// <summary>현재 Storm Power</summary>
    public int StormPower { get; internal set; }

    /// <summary>생산 창(덱): 템플이 공급하는 다리·골렘과 워크샵 등록 유닛, 획득한 지식</summary>
    public ProductionDeck Deck { get; } = new();

    /// <summary>다리 조각 칸 (템플이 있는 동안 1초마다 채워진다)</summary>
    public BridgeTray Tray { get; }

    /// <summary>커서로 집고 있는 다리 조각 (없으면 null). 회전은 화면이 관리하므로 모양만 의미가 있다.</summary>
    public BridgePiece? HeldPiece { get; internal set; }

    /// <summary>미션 techAllowed 가 정한 만들 수 있는 기술 (제한이 없으면 모두 허용)</summary>
    public TechPermissions Tech { get; }

    /// <summary>유닛 타입 이름 → 그 유닛을 다시 놓을 수 있게 되는 틱 (재충전, 대소문자 무시)</summary>
    internal Dictionary<string, long> UnitReadyTick { get; } = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>템플이 있는지 (다리 조각·골렘 공급 조건)</summary>
    public bool HasTemple => Deck.TempleId != null;

    /// <summary>플레이어 상태를 만든다</summary>
    /// <param name="number">플레이어 번호</param>
    /// <param name="stormPower">시작 Storm Power</param>
    /// <param name="tray">다리 조각 칸</param>
    /// <param name="tech">기술 허용</param>
    internal PlayerState(int number, int stormPower, BridgeTray tray, TechPermissions tech)
    {
        Number = number;
        StormPower = stormPower;
        Tray = tray;
        Tech = tech;
    }
}
