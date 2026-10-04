using Netstorm.Assets;

namespace Netstorm.Core.Bridges;

/// <summary>다리 칸의 상태 (bridge.type 클러스터 번호 규칙)</summary>
public enum BridgeCondition
{
    /// <summary>보통 (변형 번호 그대로, 예: J01)</summary>
    Normal,
    /// <summary>금 간 상태 (변형 + 10, 예: J11. bridge.type 주석: "cracked 프레임은 변형 번호 + 10")</summary>
    Cracked,
    /// <summary>단단해진 상태 (번호 20, 예: J20)</summary>
    Hard,
}

/// <summary>
/// 다리 칸 → bridge.type 프레임 번호. 원본 Canondecoder.cpp FUN_00425860 은
/// FUN_0049a940(회전된 글자, 'P', 변형)으로 찾는다 (한 글자 클러스터의 둘째 글자는 'P').
/// </summary>
public static class BridgeFrames
{
    /// <summary>금 간 프레임의 변형 번호 차이</summary>
    public const int CrackedOffset = 10;

    /// <summary>단단해진 프레임 번호</summary>
    public const int HardNumber = 20;

    /// <summary>칸 모양의 프레임 번호 (없으면 -1)</summary>
    /// <param name="table">bridge 타입의 프레임 표</param>
    /// <param name="cell">방향 글자·변형</param>
    /// <param name="condition">칸 상태</param>
    public static int Find(TypeFrameTable table, BridgeCell cell, BridgeCondition condition = BridgeCondition.Normal)
    {
        int number = condition switch
        {
            BridgeCondition.Cracked => cell.Variation + CrackedOffset,
            BridgeCondition.Hard => HardNumber,
            _ => cell.Variation,
        };
        return table.Find(cell.Letter, TypeFrameTable.DefaultVariant, number);
    }
}
