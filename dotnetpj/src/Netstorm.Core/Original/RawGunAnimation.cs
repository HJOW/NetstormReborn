using System.Buffers.Binary;

// 두 PE의 썬 캐논 회전/발사/원위치 순서와 실제 Missile 경계를 직역한다.
namespace Netstorm.Core.Original;

/// <summary>RawGunAnimation.cpp의 현재 시각/부모 프레임/효과 본체다.</summary>
public sealed partial class RawGunAnimation
{
    /// <summary>원위치 마지막8방향까지 포함하는 실제5행28프레임 표다.</summary>
    private static readonly uint[][] SunRotate =
    [
        [4,32768,12,19,5,6,7,8,9,10,11,1,13,14,15,16,20,16,17,18,21,8,8,22,32768,32768,32768,32768],
        [4,11,12,32768,5,6,7,8,22,8,9,10,13,14,15,16,17,18,19,3,16,20,23,16,32768,32768,32768,32768],
        [32768,11,12,19,0,4,5,6,7,8,9,10,13,14,15,16,20,16,17,18,21,8,8,22,32768,32768,32768,32768],
        [4,11,32768,19,5,6,7,8,22,8,9,10,2,12,13,14,15,16,17,18,16,20,23,16,32768,32768,32768,32768],
        [4,11,12,19,5,6,7,8,32768,8,9,10,13,14,15,16,32768,16,17,18,32768,20,23,16,32768,32768,32768,32768],
    ];
    /// <summary>원문 float 회전 지연이다.</summary>
    private const float RotateDelay = .2f;
    /// <summary>원문 float 발사 지연이다.</summary>
    private const float FireDelay = .07f;
    /// <summary>현재 실제 자산/프로세스의 같은 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>Source vector 값복사인 TYPE 표다.</summary>
    private readonly OriginalTypeRecord[] _types;
    /// <summary>상위가 갱신하는 현재 시각과 역할이다.</summary>
    private readonly GunAnimationState _state;
    /// <summary>최초 함수값을 복사한 큰 효과 경계다.</summary>
    private readonly GunAnimationHooks _hooks;
    /// <summary>같은 판본의 표를 복사하고 실제 풀/현재 전역을 빌린다.</summary>
    public RawGunAnimation(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, GunAnimationState state, GunAnimationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171)) throw new ArgumentException("총 애니메이션 TYPE 판본/크기 오류"); Pool = pool; _types = types.Select(t => t.Clone()).ToArray(); _state = state; _hooks = hooks.Copy();
    }
    /// <summary>외부 공장이 현재 같은 썬 캐논 역할을 확인한다.</summary>
    public uint SunCannonType() => _state.ReadSunCannonType();
    /// <summary>선택된 실제 효과만 요구한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("총 애니메이션 선택된 실제효과 미연결");
    /// <summary>dirty를 바꾸지 않고 double 필드만 기록한다.</summary>
    private static void PutTime(GunAnimationFields fields, double value) => BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(48), value);
    /// <summary>시각변경은 dirty8을 먼저 추가한다.</summary>
    private static void Time(GunAnimationFields fields, double value) { fields.Bytes[40] |= 8; PutTime(fields, value); }
    /// <summary>일반 생성자는 지정 필드/표적/시각만 쓰며 미작성 좌표와 패딩은 유지한다.</summary>
    public void Initialize(GunAnimationFields fields, GunAnimationKind kind, ushort gunForm, ushort target)
    {
        if (gunForm == 0 || gunForm >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(gunForm)); bool patch = Pool.Edition == OriginalEdition.Patch1078;
        uint outer = kind == GunAnimationKind.SunRotate ? (patch ? 0x50b260u : 0x505520u) : kind == GunAnimationKind.SunFire ? (patch ? 0x50b318u : 0x505480u) : (patch ? 0x50b2c8u : 0x505660u);
        uint inner = kind == GunAnimationKind.SunRotate ? (patch ? 0x50b258u : 0x505518u) : kind == GunAnimationKind.SunFire ? (patch ? 0x50b310u : 0x505478u) : (patch ? 0x50b2c4u : 0x505658u);
        Span<byte> raw = fields.Bytes; Put(raw, 0, outer); Put(raw, 4, 0); Put(raw, 8, 0); Put(raw, 32, inner); raw[40] = 0xff; BinaryPrimitives.WriteUInt16LittleEndian(raw[42..], gunForm); BinaryPrimitives.WriteUInt16LittleEndian(raw[44..], kind == GunAnimationKind.SunRest ? (ushort)0 : target); PutTime(fields, _state.Now); if (kind != GunAnimationKind.SunRotate) Put(raw, 56, 0);
    }
    /// <summary>현재 부모의 역할과 PATCH DWORD/CD BYTE 프레임을 확인한다.</summary>
    private int CurrentFrame(ushort parent)
    {
        if (parent == 0 || parent >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(parent)); ReadOnlySpan<byte> raw = Pool.Slot(parent); if (raw[10] != SunCannonType()) throw new InvalidOperationException("총 애니메이션 현재부모 파생 미연결"); return Pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) : raw[34];
    }
    /// <summary>두 공간효과를 먼저 요구한 뒤 Unpop→원문 폭 쓰기→Repop40을 실행한다.</summary>
    private void SetFrame(ushort parent, uint frame)
    {
        Action<ushort, uint> unpop = Need(_hooks.Unpop), repop = Need(_hooks.Repop); unpop(parent, 0); Span<byte> raw = Pool.AllocatedBytes(parent); if (Pool.Edition == OriginalEdition.Patch1078) Put(raw, 36, frame); else raw[34] = (byte)frame; repop(parent, 0x40);
    }
    /// <summary>살아 있는 표적의 현재 위치를 먼저 복사하고 실제 중심 효과 뒤 중심을 기록한다.</summary>
    private bool TrackTarget(GunAnimationFields fields)
    {
        ushort target = fields.Target(); if (target == 0 || target >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(fields)); ReadOnlySpan<byte> raw = Pool.Slot(target); if ((raw[11] & 3) != 0) return false; Func<ushort, float[]> center = Need(_hooks.Center);
        Put(fields.Bytes, 12, Read(raw, 14)); Put(fields.Bytes, 16, Read(raw, 18)); float[] point = center(target); Put(fields.Bytes, 20, BitConverter.SingleToUInt32Bits(point[0])); Put(fields.Bytes, 24, BitConverter.SingleToUInt32Bits(point[1])); return true;
    }
    /// <summary>유효 HP 표적에서만 실제 Missile 요청을 실행하며 접두중단을 성공으로 바꾸지 않는다.</summary>
    public bool Launch(GunAnimationFields fields)
    {
        ushort target = fields.Target(); if (target == 0 || target >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(fields)); ReadOnlySpan<byte> raw = Pool.Slot(target); if ((raw[11] & 3) != 0 || (_types[raw[10]].Flags1 & 0x10) == 0) return false;
        Func<ushort, ushort, float[]> point = Need(_hooks.TargetPoint); Action<GunProjectileRequest> create = Need(_hooks.CreateProjectile);
        var request = new GunProjectileRequest { Firing = fields.Parent(), Target = target, DisplayType = _state.DisplayType, EffectType = _state.EffectType, TargetPosition = [BinaryPrimitives.ReadSingleLittleEndian(fields.Bytes.AsSpan(12)), BinaryPrimitives.ReadSingleLittleEndian(fields.Bytes.AsSpan(16))] }; request.TargetCenter = point(request.Firing, target).ToArray(); create(request); return true;
    }
    /// <summary>두 효과를 먼저 요구하고 최초Gun/Form으로 통지한 뒤 삭제하며 이후 필드를 읽지 않는다.</summary>
    private void Finish(GunAnimationFields fields, uint info, uint value)
    {
        Action<ushort, uint, uint, uint> notify = Need(_hooks.Notify); Action<ushort, uint> kill = Need(_hooks.Kill); ushort gun = fields.GunForm(), form = fields.Form(); notify(gun, 1, info, value); kill(form, 0x10);
    }
    /// <summary>Y 절댓값 동률을 먼저 선택하는 원문 float 방향 판정이다.</summary>
    private static int Direction(float[] from, float[] to)
    {
        float dx = (float)((double)to[0] - from[0]), dy = (float)((double)to[1] - from[1]); if (MathF.Abs(dx) <= MathF.Abs(dy)) return dy > 0 ? 4 : 0; return dx <= 0 ? 6 : 2;
    }
    /// <summary>각 호출은 원문 물리프레임 하나와 현재시각 기준 다음기한만 진행한다.</summary>
    public void Frame(GunAnimationFields fields, GunAnimationKind kind)
    {
        ushort parent = fields.Parent(); int current = CurrentFrame(parent); if (current < 0 || current >= (kind == GunAnimationKind.SunFire ? 48 : 28)) throw new ArgumentOutOfRangeException(nameof(fields), "총 애니메이션 물리프레임 오류");
        if (kind == GunAnimationKind.SunFire)
        {
            bool alive = TrackTarget(fields); if (!(fields.Time() <= _state.Now)) return; uint code = current < 24 ? (uint)(current + 24) | 0x4000u : (uint)(current - 24) | 0x8000u; SetFrame(parent, code & 0x3fff);
            if ((code & 0x4000) != 0 && !Launch(fields) && alive) return; if ((code & 0x8000) != 0) { Finish(fields, 6, 7); return; } Time(fields, _state.Now + (double)FireDelay); return;
        }
        if (!(fields.Time() <= _state.Now)) return; int direction = fields.RestDirection() / 2;
        if (kind == GunAnimationKind.SunRotate) { if (fields.Target() == 0) throw new InvalidOperationException("총 회전 실제표적/중심 미연결"); Func<ushort, float[]> center = Need(_hooks.Center); float[] to = center(fields.Target()).ToArray(), from = center(parent).ToArray(); direction = Direction(from, to) / 2; }
        if (direction < 0 || direction > 4) throw new ArgumentOutOfRangeException(nameof(fields), "총 원위치 방향 오류"); uint next = SunRotate[direction][current];
        if ((next & 0x8000) != 0)
        {
            if (kind == GunAnimationKind.SunRotate) { Finish(fields, 3, 4); return; } if (fields.RestDirection() == 8) { Finish(fields, 9, 10); return; } Put(fields.Bytes, 56, unchecked((uint)(fields.RestDirection() + 2)));
        }
        else
        {
            if (kind == GunAnimationKind.SunRotate) { Func<int, int, int> random = Need(_hooks.RandomInteger); Action<ushort, string> sound = Need(_hooks.Sound); int number = random(1000, 10000) / 1000; sound(parent, "sunCannonRaiseLower" + number.ToString(System.Globalization.CultureInfo.InvariantCulture) + ".wav"); } SetFrame(parent, next & 0x3fff);
        }
        Time(fields, _state.Now + (double)RotateDelay);
    }
    /// <summary>현재 실제VT로 회전/발사 헤더를 고르는 독립 공통 writer다.</summary>
    public static ushort WritePacket(GunAnimationFields fields, Span<byte> packet, bool includeParent, bool force = false, byte protocol = 1) => WriteProcess(fields.Bytes, packet, includeParent, force, UsesFirePacket(fields.Bytes) ? (byte)0x3b : (byte)0x3c, protocol);
}
