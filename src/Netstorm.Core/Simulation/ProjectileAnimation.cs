using Netstorm.Assets;

namespace Netstorm.Core.Simulation;

/// <summary>원반·석궁 탄과 착탄 그림을 원본 클러스터 순서에서 고른다. 그림 선택은 피해 판정과 독립적이다.</summary>
public static class ProjectileAnimation
{
    /// <summary>팬게임에서 확인한 원반 회전 간격(초). 원본 영상의 정확한 간격은 후속 측정한다.</summary>
    public const double DiscFrameSeconds = 0.05;

    /// <summary>20방향 석궁 탄을 진행 방향에 맞추거나 원반의 B02·B03을 번갈아 고른다.</summary>
    public static int Frame(TypeDefinition type, CombatShot shot, double tick, int ticksPerSecond)
    {
        if (shot.AttackerType?.Equals("windArcher", StringComparison.OrdinalIgnoreCase) == true)
        {
            double angle = Math.Atan2(shot.EndX - shot.StartX, shot.StartY - shot.EndY);
            int orientation = ((int)Math.Round(angle * 20 / (2 * Math.PI)) % 20 + 20) % 20;
            return type.Frames.Find('A', TypeFrameTable.DefaultVariant, orientation);
        }
        int phase = (int)((tick - shot.FiredTick) / ticksPerSecond / DiscFrameSeconds) % 2;
        return type.Frames.Find('B', TypeFrameTable.DefaultVariant, 2 + phase);
    }

    /// <summary>폭발 그림은 클러스터 순서로 찾는다. D09·B09가 없으므로 이름 숫자를 순서로 간주하지 않는다.</summary>
    public static int ImpactFrame(TypeDefinition type, CombatImpact impact, double tick, int ticksPerSecond)
    {
        double interval = impact.Group is 'C' or 'D' ? BattleSession.ImpactFrameSeconds / 2 : BattleSession.ImpactFrameSeconds;
        int number = (int)((tick - impact.Tick) / ticksPerSecond / interval);
        // 같은 묶음의 실제 클러스터를 순서대로 세어 빠진 이름과 중복 이름을 건너뛴다.
        for (int frame = 0; frame < type.Clusters.Count; frame++)
        {
            if (type.Clusters[frame].Name[0] == impact.Group && number-- == 0) return frame;
        }
        return -1;
    }
}
