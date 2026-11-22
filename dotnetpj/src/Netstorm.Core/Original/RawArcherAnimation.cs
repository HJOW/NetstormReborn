using System.Buffers.Binary;

// 현재원문아처의회전/각기하와같은월드상태를연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawArcherAnimation.cpp의일반생성/현재조회/회전과각기하helper다.</summary>
public sealed partial class RawArcherAnimation
{
    /// <summary>원문20칸의비균등회전각이다.</summary>
    private static readonly float[] Angles = [0,21,35,52,71,86,97,115,121,148,180,205,237,256,272,282,291,314,328,345];
    /// <summary>원문근사pi로계산된라디안→도계수다.</summary>
    private const double DegreesPerRadian = 57.295827908797776;
    /// <summary>원문float회전지연이다.</summary>
    private const float RotateDelay = .1f;
    /// <summary>표적0 aim BYTE의원문float배율이다.</summary>
    private const float AimAngleScale = 90;
    /// <summary>같은Host와자산의현재풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>Source span처럼빌린현재TYPE표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>Source span처럼빌린현재방향자료다.</summary>
    private readonly IReadOnlyList<ArcherTypeData> _data;
    /// <summary>매프레임현재시각/경과/역할/각상수를읽는다.</summary>
    private readonly ArcherAnimationState _state;
    /// <summary>최초함수값을복사한큰효과장치다.</summary>
    private readonly ArcherAnimationHooks _hooks;
    /// <summary>동일판본의현재TYPE/방향자료와현재역할을확인한다.</summary>
    public RawArcherAnimation(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<ArcherTypeData> data, ArcherAnimationState state, ArcherAnimationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(data); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || data.Count != types.Count || state.ReadType() >= types.Count) throw new ArgumentException("아처 현재TYPE/방향자료 계약오류"); Pool = pool; _types = types; _data = data; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>실제공장이소비하는현재부모역할이다.</summary>
    public uint Type() => _state.ReadType();
    /// <summary>선택한실제효과만요구한다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("아처 선택된 실제효과 미연결");
    /// <summary>비정렬DWORD의불투명비트를읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int at) => BinaryPrimitives.ReadUInt32LittleEndian(raw[at..]);
    /// <summary>원문DWORD폭만쓴다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>원문float저장값을읽는다.</summary>
    private static float F(ReadOnlySpan<byte> raw, int at) => BinaryPrimitives.ReadSingleLittleEndian(raw[at..]);
    /// <summary>중간연산을원문float로저장한다.</summary>
    private static void F(Span<byte> raw, int at, float value) => BinaryPrimitives.WriteSingleLittleEndian(raw[at..], value);
    /// <summary>현재좌표쌍을std::array와같은값으로반환한다.</summary>
    private static float[] Point(ReadOnlySpan<byte> raw, int at) => [F(raw, at), F(raw, at + 4)];
    /// <summary>기한쓰기는dirty8을먼저켠다.</summary>
    private static void Time(ArcherAnimationFields fields, double value) { fields.Bytes[40] |= 8; BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(48), value); }
    /// <summary>공통생성접두만쓰고부착뒤실제Fire초기화는분리한다.</summary>
    public void Initialize(ArcherAnimationFields fields, bool fire, ushort gunForm, ushort target)
    {
        if (gunForm == 0 || gunForm >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(gunForm)); bool patch = Pool.Edition == OriginalEdition.Patch1078; Put(fields.Bytes, 0, fire ? (patch ? 0x50b1c0u : 0x5054d0u) : (patch ? 0x50b450u : 0x505570u)); Put(fields.Bytes, 4, 0); Put(fields.Bytes, 8, 0); Put(fields.Bytes, 32, fire ? (patch ? 0x50b1bcu : 0x5054ccu) : (patch ? 0x50b448u : 0x50556cu)); fields.Bytes[40] = 0xff; BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(42), gunForm); BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(44), target); Time(fields, _state.Now); if (fire) Put(fields.Bytes, 56, 0);
    }
    /// <summary>현재부모SID/현재TYPE를검사하며같은슬롯을빌린다.</summary>
    private ReadOnlySpan<byte> Parent(ArcherAnimationFields fields)
    {
        ushort sid = fields.Parent(); if (sid == 0 || sid >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(fields)); ReadOnlySpan<byte> raw = Pool.Slot(sid); if (raw[10] != Type()) throw new InvalidOperationException("아처 현재부모TYPE 미연결"); return raw;
    }
    /// <summary>PATCH DWORD/CD BYTE의현재프레임이다.</summary>
    private int Frame(ushort parent) => Pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(Pool.Slot(parent)[36..]) : Pool.Slot(parent)[34];
    /// <summary>두공간효과를먼저요구하고Unpop→폭보존쓰기→Repop40을실행한다.</summary>
    private void SetFrame(ushort parent, int frame)
    {
        Action<ushort, uint> unpop = Need(_hooks.Common.Unpop), repop = Need(_hooks.Common.Repop); unpop(parent, 0); Span<byte> raw = Pool.AllocatedBytes(parent); if (Pool.Edition == OriginalEdition.Patch1078) Put(raw, 36, unchecked((uint)frame)); else raw[34] = unchecked((byte)frame); repop(parent, 0x40);
    }
    /// <summary>중심효과뒤현재raw좌표를재조회하며free/dead는기존좌표를유지한다.</summary>
    private bool Track(ArcherAnimationFields fields)
    {
        ushort sid = fields.Target(); if (sid == 0 || sid >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(fields)); ReadOnlySpan<byte> raw = Pool.Slot(sid); if ((raw[11] & 3) != 0) return false; float[] center = Need(_hooks.Common.Center)(sid).ToArray(); Put(fields.Bytes, 12, Read(raw, 14)); Put(fields.Bytes, 16, Read(raw, 18)); F(fields.Bytes, 20, center[0]); F(fields.Bytes, 24, center[1]); return true;
    }
    /// <summary>두완료효과를먼저요구하고최초Gun/Form통지뒤삭제한다.</summary>
    private void Finish(ArcherAnimationFields fields, uint info, uint value)
    {
        Action<ushort, uint, uint, uint> notify = Need(_hooks.Common.Notify); Action<ushort, uint> kill = Need(_hooks.Common.Kill); ushort gun = fields.GunForm(), form = fields.Form(); notify(gun, 1, info, value); kill(form, 0x10);
    }
    /// <summary>가장가까운원문비균등각을고르며동률의앞선칸을보존한다.</summary>
    public static int NearestDirection(float angle, OriginalEdition edition)
    {
        if (!float.IsFinite(angle)) throw new ArgumentException("아처 회전 비유한각"); double best = -1000; int result = 0;
        // 현재최선차이가양수가되면원문처럼후속칸을읽지않는다.
        for (int i = 0; i < Angles.Length && best < 0; i++) { double difference = (double)Angles[i] - angle; if (Math.Abs(difference) < Math.Abs(best)) { result = i; best = edition == OriginalEdition.Patch1078 ? difference : (float)difference; } }
        return result == 19 && Math.Abs(360.0 - angle) < Math.Abs(best) ? 0 : result;
    }
    /// <summary>원문초기0..32프레임을22.5도각으로변환한다.</summary>
    public static float InitialAngle(int frame) { if (frame > 15) frame -= 16; double angle = frame * -22.5 + 360; if (!(angle >= 0 && angle <= 360)) throw new ArgumentOutOfRangeException(nameof(frame)); return (float)angle; }
    /// <summary>원문fmod→float저장→0.5f/절삭과sprung 프레임오프셋을유지한다.</summary>
    public static int FireFrame(float angle, bool sprung)
    {
        if (!float.IsFinite(angle)) throw new ArgumentException("아처 발사 비유한각"); double inverse = 360 - ((double)angle % 360); if (!(inverse > 0 && inverse <= 360)) throw new ArgumentOutOfRangeException(nameof(angle)); if (inverse >= 360) inverse = 359; float value = (float)(inverse * .044444444444444446); int frame = (int)(value + .5f); return (frame + (sprung ? 2 : 0)) % 16 + (sprung ? 16 : 0);
    }
    /// <summary>동일좌표는0이며근사pi계수의원문atan2 각을계산한다.</summary>
    public static double Degrees(float[] from, float[] to) => from[0] == to[0] && from[1] == to[1] ? 0 : Math.Atan2(-((double)to[1] - from[1]), (double)to[0] - from[0]) * DegreesPerRadian;
    /// <summary>450-atanDegrees를float로저장하고기존현재진단부모로정규화한다.</summary>
    public static float DirectionAngle(float[] from, float[] to, OriginalEdition edition, BoardAngleDiagnostics? diagnostics = null) => from[0] == to[0] && from[1] == to[1] ? 0 : (float)RawBoardAngle.Normalize((float)(450 - Degrees(from, to)), edition, diagnostics);
    /// <summary>표적을기한앞에서갱신하고한틱에방향한칸만진행한다.</summary>
    public void Rotate(ArcherAnimationFields fields)
    {
        ReadOnlySpan<byte> raw = Parent(fields); int type = raw[10]; if (_data[type].Directions <= 0 || _data[type].FrameCount / _data[type].Directions <= 0) throw new InvalidOperationException("아처 현재방향/프레임자료 미공급"); int per = _data[type].FrameCount / _data[type].Directions, current = Frame(fields.Parent()) / per; float angle;
        if (fields.Target() == 0) angle = Need(_hooks.AimDirection)(fields.Parent()) * AimAngleScale; else { Track(fields); float[] from = Need(_hooks.Common.Center)(fields.Parent()).ToArray(); angle = DirectionAngle(from, Point(fields.Bytes, 20), Pool.Edition, _state.Angles); }
        int wanted = NearestDirection(angle, Pool.Edition); if (!(fields.Time() <= _state.Now)) return; if (current == wanted) { Finish(fields, 3, 4); return; }
        int directions = _data[type].Directions, backward = current < wanted ? current - wanted + directions : current - wanted, forward = wanted < current ? wanted - current + directions : wanted - current, step = forward < backward ? 1 : -1; int next = current == 0 && step == -1 ? directions - 1 : (current + step) % directions; SetFrame(fields.Parent(), unchecked(next * per)); Time(fields, _state.Now + (double)RotateDelay);
        Func<int, int, int> random = Need(_hooks.Common.RandomInteger); Action<ushort, string> sound = Need(_hooks.Common.Sound); int number = random(1000, 10000) / 1000; sound(fields.Parent(), "sunDiscThrowerRatchet0" + number.ToString(System.Globalization.CultureInfo.InvariantCulture) + ".wav");
    }
}
