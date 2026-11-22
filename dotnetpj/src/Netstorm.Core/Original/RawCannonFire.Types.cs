// 공통발사/반더즉시빔의현재원문전역과큰효과경계를보존한다.
namespace Netstorm.Core.Original;

/// <summary>같은공통Fire본문이소비하는실제파생종류다.</summary>
public enum CannonFireKind
{
    /// <summary>썬캐논의24프레임앞뒤변환이다.</summary>
    Sun,
    /// <summary>썬더캐논의문자구간발사다.</summary>
    Thunder,
    /// <summary>반더타워의동시발사/완료다.</summary>
    Vander,
}
/// <summary>현재시각과가상getter/효과음단계의mutable 원문전역이다.</summary>
public sealed class CannonFireState
{
    /// <summary>현재게임시각을소비위치마다읽는다.</summary>
    public double Now { get; set; }
    /// <summary>가상단계표를고르는실제파생종류다.</summary>
    public CannonFireKind Kind { get; set; } = CannonFireKind.Thunder;
    /// <summary>독립입력의최초131부모TYPE다.</summary>
    public uint Type { get; set; } = 131;
    /// <summary>실제Sun파생의현재00541258역할이다.</summary>
    public UIntBox? CurrentType { get; set; }
    /// <summary>현재thunderCannonMissile 표시132다.</summary>
    public uint DisplayType { get; set; } = 132;
    /// <summary>현재충돌효과TYPE다.</summary>
    public uint EffectType { get; set; }
    /// <summary>현재가상속도getter의float값이다.</summary>
    public float Speed { get; set; } = 20;
    /// <summary>현재원문float발사기한간격이다.</summary>
    public float Delay { get; set; } = .5f;
    /// <summary>발사bit/일치TYPE에서만곱하는float배율이다.</summary>
    public float LaunchScale { get; set; } = .25f;
    /// <summary>00541264의현재발사지연비교TYPE다.</summary>
    public uint ScaledType { get; set; } = 131;
    /// <summary>마지막프레임에서Growl소리를낼현재차이다.</summary>
    public int GrowlOffset { get; set; } = 5;
    /// <summary>마지막프레임에서Roar소리를낼현재차이다.</summary>
    public int RoarOffset { get; set; } = 1;
    /// <summary>공통Launch의현재반더148비교역할이다.</summary>
    public uint VanderType { get; set; } = 148;
    /// <summary>회피난수를선택하는현재Cloud Floater146역할이다.</summary>
    public uint FloaterType { get; set; } = 146;
    /// <summary>RandomFloat0..1000과비교하는부호있는원문DWORD다.</summary>
    public int AvoidanceCutoff { get; set; } = 800;
    /// <summary>생성/현재부모검증에서같은전체역할DWORD를읽는다.</summary>
    public uint ReadType() => CurrentType?.Value ?? Type;
}
/// <summary>일반Missile과구별한반더실제즉시빔/회피/피해장치다.</summary>
public sealed class CannonBeamHooks
{
    /// <summary>현재source/target의실제shortZap빔생성이다.</summary>
    public Action<ushort, ushort>? Create { get; set; }
    /// <summary>회피한경우최초source의시각갱신이다.</summary>
    public Action<ushort>? Refresh { get; set; }
    /// <summary>최초source에서현재target으로실제피해를준다.</summary>
    public Action<ushort, ushort>? Hit { get; set; }
    /// <summary>같은월드의단일게임난수상태다.</summary>
    public GameRandom? Random { get; set; }
    /// <summary>Source함수값과난수포인터를값복사한다.</summary>
    internal CannonBeamHooks Copy() => (CannonBeamHooks)MemberwiseClone();
}
