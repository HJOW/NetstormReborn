using System.Buffers.Binary;

// 원문 압축 시각의 감김/절삭과 판본별 DPS 중간 저장을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>현재 총 TYPE의 실제 DPS와 비행 표적 대체 자료다.</summary>
public sealed class GunHitType
{
    /// <summary>실제TYPE+48의 float DPS다.</summary>
    public float DamagePerSecond { get; set; }
    /// <summary>PATCH+1b0의 별도 비행 표적 선택이다.</summary>
    public bool SpecialFlyerDamage { get; set; }
    /// <summary>PATCH+1b4의 대체 float DPS다.</summary>
    public float FlyerDamagePerSecond { get; set; }
}
/// <summary>같은 실제 월드의 피해/시간 통지와 전용 가상 입구다.</summary>
public sealed class GunHitHooks
{
    /// <summary>현재 표적98에 정수 피해/소유자/공격자를 전달한다.</summary>
    public Action<ushort, int, byte, ushort>? Damage { get; set; }
    /// <summary>시간 WORD 쓰기 직후 실제 공간 통지다.</summary>
    public Action<ushort>? NotifySurface { get; set; }
    /// <summary>generic target0/free/dead 관문 전에 선택하는 실제 source+c0 입구다.</summary>
    public Func<ushort, ushort, int, bool>? ApplyOverride { get; set; }
    /// <summary>함수값을 복사하고 실제 캡처 owner는 공유한다.</summary>
    internal GunHitHooks Copy() => (GunHitHooks)MemberwiseClone();
}
/// <summary>RawGunHit.cpp의 총 압축 시각/실제 직접 표적 피해 본문이다.</summary>
public sealed class RawGunHit
{
    /// <summary>WORD의 출력bit0을 뺀15비트 시간 마스크다.</summary>
    private const uint TimeMask = 0x7fff;
    /// <summary>별도 DPS를 선택하는 비행/벌룬 속성이다.</summary>
    private const uint FlyerOrBalloon = 0x120000;
    /// <summary>시간/소유자/표적 상태가 공유하는 실제 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>원문 span처럼 빌리는 현재 TYPE다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>원문 span처럼 빌리는 현재 DPS다.</summary>
    private readonly IReadOnlyList<GunHitType> _hitTypes;
    /// <summary>최초 값복사한 실제 외부 함수다.</summary>
    private readonly GunHitHooks _hooks;
    /// <summary>실제 판본 표 크기를 확인하고 현재 자료를 빌린다.</summary>
    public RawGunHit(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<GunHitType> hitTypes, GunHitHooks hooks)
    {
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || hitTypes.Count != types.Count) throw new ArgumentException("총 피해 TYPE/DPS 판본 오류"); Pool = pool; _types = types; _hitTypes = hitTypes; _hooks = hooks.Copy();
    }
    /// <summary>실제 선택된 경계만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("총 피해 선택된 실제 경계 미연결");
    /// <summary>원문 정상 CRT의 유한 int32 절삭 범위다.</summary>
    private static int Truncate(double value) { if (!double.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value)); return (int)value; }
    /// <summary>정수초×8과 소수8분의1초의 하위DWORD를 OR한다.</summary>
    private static uint Packed(double now) { int integral = Truncate(now), fraction = Truncate((now - integral) * 8); return unchecked((uint)integral * 8) | unchecked((uint)fraction); }
    /// <summary>시간 필드를 소유하는 group0/1 또는 flyer 실제 자산만 허용한다.</summary>
    private ReadOnlySpan<byte> Firing(ushort firing)
    {
        if (firing == 0 || firing >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(firing)); ReadOnlySpan<byte> raw = Pool.Slot(firing); if (raw[10] >= _types.Count) throw new ArgumentOutOfRangeException(nameof(firing)); OriginalTypeRecord type = _types[raw[10]]; if ((type.Flags2 & 0x100000) == 0 && type.Group is not (0 or 1)) throw new InvalidOperationException("총 피해 시간group 원문 진단"); return raw;
    }
    /// <summary>raw+12 WORD의 bit0을 제외한1/8초 시각이다.</summary>
    public double Time(ushort firing) { ReadOnlySpan<byte> raw = Firing(firing); uint packed = (uint)BinaryPrimitives.ReadUInt16LittleEndian(raw[12..]) >> 1; return (packed >> 3) + (packed & 7) * .125; }
    /// <summary>현재 WORD bit0을 보존해 압축 시각을 쓰고 공간 통지를 한다.</summary>
    public void SetTime(ushort firing, double now)
    {
        _ = Firing(firing); Action<ushort> notify = Need(_hooks.NotifySurface); Span<byte> raw = Pool.AllocatedBytes(firing); uint packed = Packed(now); BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], unchecked((ushort)((packed << 1) | (uint)(raw[12] & 1)))); notify(firing);
    }
    /// <summary>전용 source+c0 입구 뒤 generic live 표적의 현재 가상98에 피해를 전달한다.</summary>
    public void Apply(ushort firing, ushort target, int damage)
    {
        if (_hooks.ApplyOverride is not null && _hooks.ApplyOverride(firing, target, damage)) return; if (target == 0) return; if (target >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(target)); if ((Pool.Slot(target)[11] & 3) != 0) return; ReadOnlySpan<byte> source = Firing(firing); Need(_hooks.Damage)(target, damage, source[Pool.Edition == OriginalEdition.Patch1078 ? 34 : 32], firing);
    }
    /// <summary>양자화 시각차×DPS를 계산하고30초 초과는3초분으로 제한한 뒤 시간을 갱신한다.</summary>
    public void Hit(ushort firing, ushort target, double now)
    {
        ReadOnlySpan<byte> source = Firing(firing); bool patch = Pool.Edition == OriginalEdition.Patch1078; float dps = _hitTypes[source[10]].DamagePerSecond; if (!float.IsFinite(dps)) throw new ArgumentException("총 피해 비유한 DPS");
        if (patch && _hitTypes[source[10]].SpecialFlyerDamage && target != 0) { if (target >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(target)); ReadOnlySpan<byte> victim = Pool.Slot(target); if (victim[10] >= _types.Count) throw new ArgumentOutOfRangeException(nameof(target)); if ((_types[victim[10]].Flags2 & FlyerOrBalloon) != 0) dps = _hitTypes[source[10]].FlyerDamagePerSecond; }
        uint packed = Packed(now) & TimeMask; double elapsed = (packed >> 3) + (packed & 7) * .125 - Time(firing); float storedElapsed = (float)elapsed; double damage = elapsed * dps; if (damage >= 1) { double selected = storedElapsed < 0 || storedElapsed > 30 ? (double)dps * 3 : damage; Apply(firing, target, Truncate(patch ? selected : (float)selected)); } SetTime(firing, now);
    }
    /// <summary>미사일의 직접 피해/사거리 시각 갱신을 같은 현재 시계에 연결한다.</summary>
    public static MissileHooks MakeMissileHooks(RawGunHit hit, MissileState state, MissileHooks hooks) { MissileHooks current = hooks.Copy(); current.Hit = (firing, target) => hit.Hit(firing, target, state.Now); current.Refresh = firing => hit.SetTime(firing, state.Now); return current; }
}
