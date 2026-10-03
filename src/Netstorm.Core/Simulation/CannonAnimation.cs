using Netstorm.Assets;

namespace Netstorm.Core.Simulation;

/// <summary>캐논의 방위·사격 프레임과 원본 Gunanim.cpp에서 확인한 주기를 공유한다.</summary>
public static class CannonAnimation
{
    /// <summary>썬더 캐논의 프레임 간격(원본 FUN_00468500).</summary>
    public const double ThunderFrameSeconds = 0.5;
    /// <summary>썬더 캐논은 첫 충전 그림에서 일곱 간격 뒤 마지막 그림으로 발사한다.</summary>
    public const double ThunderChargeSeconds = 7 * ThunderFrameSeconds;
    /// <summary>마지막 썬더 발사 그림의 간격은 보통 간격의 1/4이다(원본 FUN_00467330).</summary>
    public const double ThunderRecoverySeconds = ThunderFrameSeconds / 4;
    /// <summary>아이스 캐논의 프레임 간격(원본 FUN_00468630).</summary>
    public const double IceFrameSeconds = 0.08;
    /// <summary>아이스 캐논의 세 프레임으로 이루어진 연속 사격 주기.</summary>
    public const double IceShotSeconds = 3 * IceFrameSeconds;
    /// <summary>아이스 캐논이 연속 사격하는 시간(원본 FUN_00468660, 0x506588).</summary>
    public const double IceBurstSeconds = 4;
    /// <summary>아이스 캐논이 연속 사격 사이에 쉬는 시간(원본 0x50b440).</summary>
    public const double IceRestSeconds = 6;
    /// <summary>태양 캐논의 발사 그림 표시 시간(원본 FUN_00468490).</summary>
    public const double SunFireSeconds = 0.07;

    /// <summary>타입이 이번 분석의 네 방향 직선 캐논인지.</summary>
    public static bool IsCannon(TypeInfo type) => type.Name.Equals("sunCannon", StringComparison.OrdinalIgnoreCase) || IsFixed(type);

    /// <summary>아이스·썬더 캐논은 배치 방위에서만 발사한다.</summary>
    public static bool IsFixed(TypeInfo type) => type.Name.Equals("rainCannon", StringComparison.OrdinalIgnoreCase)
        || type.Name.Equals("thunderCannon", StringComparison.OrdinalIgnoreCase);

    /// <summary>북/동/남/서 방위를 원본 캐논 클러스터의 L/M/N/O 측면으로 바꾼다.</summary>
    public static char Side(int direction) => (char)('L' + ((direction % 4 + 4) % 4));

    /// <summary>실제 세션 틱으로 준비·발사 그림을 고른다. 일시정지하면 같은 그림을 유지한다.</summary>
    public static int Frame(GameEntity entity, long tick, int ticksPerSecond)
    {
        TypeDefinition type = entity.Type.Definition;
        char side = Side(entity.CannonDirection);
        int frame;
        if (entity.Type.Name.Equals("sunCannon", StringComparison.OrdinalIgnoreCase))
        {
            if (entity.LastShotTick >= 0 && (tick - entity.LastShotTick) / (double)ticksPerSecond < SunFireSeconds)
                return 24 + (entity.CannonDirection switch { 0 => 1, 1 => 3, 2 => 0, _ => 2 });
            if (entity.AttackTargetId == 0 && entity.LastShotTick < 0) return entity.Source?.Object.Frame ?? type.Frames.DefaultFrame;
            // 태양 캐논의 펼쳐진 자세는 L01/M03/N00/O02로 저장되어 있다.
            frame = entity.CannonDirection switch { 0 => 1, 1 => 3, 2 => 0, _ => 2 };
        }
        else if (entity.Type.Name.Equals("thunderCannon", StringComparison.OrdinalIgnoreCase))
        {
            int number = entity.AttackStartedTick >= 0
                ? Math.Clamp(1 + (int)((tick - entity.AttackStartedTick) / (double)ticksPerSecond / ThunderFrameSeconds), 1, 7)
                : entity.LastShotTick >= 0 && (tick - entity.LastShotTick) / (double)ticksPerSecond < ThunderRecoverySeconds ? 8 : 0;
            frame = type.Frames.Find(side, TypeFrameTable.DefaultVariant, number);
        }
        else
        {
            double elapsed = entity.AttackStartedTick < 0 ? 0 : (tick - entity.AttackStartedTick) / (double)ticksPerSecond;
            bool active = entity.AttackStartedTick >= 0 && elapsed % (IceBurstSeconds + IceRestSeconds) < IceBurstSeconds;
            // 틱 올림으로 실제 발사는 0.25초마다 일어날 수 있다. 매 발사 시점에 맞춰 발사 그림을 다시 시작한다.
            int number = !active ? 0 : entity.LastShotTick < entity.AttackStartedTick ? 1
                : (2 + (int)((tick - entity.LastShotTick) / (double)ticksPerSecond / IceFrameSeconds)) % 3;
            frame = type.Frames.Find(side, TypeFrameTable.DefaultVariant, number);
        }
        return frame >= 0 && frame < type.Clusters.Count ? frame : type.Frames.DefaultFrame;
    }
}
