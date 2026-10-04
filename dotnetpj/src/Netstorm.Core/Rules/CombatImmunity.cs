using Netstorm.Assets;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Rules;

/// <summary>원본 방어 건물의 공격자 종류·입사 방위에 따른 피해 면역.</summary>
public static class CombatImmunity
{
    /// <summary>Bulwark는 비행 공격체에, Wind Tower는 지정한 면의 지상 공격에 면역이다. 폭발은 공격자 타입을 생략한다.</summary>
    public static bool Blocks(GameEntity target, TypeInfo? attacker, double attackerX, double attackerY)
    {
        if (attacker == null) return false;
        bool flyer = attacker.Definition.HasFlag("flyer");
        if (target.Type.Name.Equals("thunderBlocker", StringComparison.OrdinalIgnoreCase)) return flyer;
        if (!target.Type.Name.Equals("windBlocker", StringComparison.OrdinalIgnoreCase) || flyer
            || attacker.Definition.HasFlag("bomb") || attacker.Name.Equals("thunderFence", StringComparison.OrdinalIgnoreCase)) return false;
        double dx = attackerX - target.WorldX, dy = attackerY - target.WorldY;
        // 원본 FUN_0041ce90은 같은 크기의 대각선 벡터를 세로 방위로 분류한다.
        int incoming = Math.Abs(dx) > Math.Abs(dy) ? dx > 0 ? 1 : 3 : dy > 0 ? 2 : 0;
        return incoming == target.CannonDirection;
    }
}
