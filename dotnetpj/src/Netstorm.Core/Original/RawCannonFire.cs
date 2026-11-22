using System.Buffers.Binary;

// 실제공통Fire의snapshot/효과뒤현재재조회와반더빔분기를보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawCannonFire.cpp의썬/썬더/반더공통발사본문이다.</summary>
public sealed partial class RawCannonFire
{
    /// <summary>실제자산과프로세스의같은풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>원문span처럼빌린현재HP/TYPE표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>원문span처럼빌린현재문자코드/구간표다.</summary>
    private readonly IReadOnlyList<PriestPlainCanonType> _frames;
    /// <summary>소비위치마다다시읽는현재원문전역이다.</summary>
    private readonly CannonFireState _state;
    /// <summary>최초함수값을복사한월드/통지경계다.</summary>
    private readonly GunAnimationHooks _hooks;
    /// <summary>현재빔/피해함수값과같은난수포인터다.</summary>
    private readonly CannonBeamHooks _beam;
    /// <summary>PATCH 실제판본/표/현재역할을확인하고span/현재전역을빌린다.</summary>
    public RawCannonFire(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<PriestPlainCanonType> frames, CannonFireState state, GunAnimationHooks hooks, CannonBeamHooks? beam = null)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || frames.Count != types.Count || state.ReadType() >= types.Count) throw new ArgumentException("캐논 발사 실제판본/TYPE자료 오류"); Pool = pool; _types = types; _frames = frames; _state = state; _hooks = hooks.Copy(); _beam = beam?.Copy() ?? new();
    }
    /// <summary>실제공장이확인하는현재부모역할이다.</summary>
    public uint Type() => _state.ReadType();
    /// <summary>선택된실제효과만요구한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("캐논 발사 선택된 실제효과 미연결");
    /// <summary>비정렬DWORD를읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int at) => BinaryPrimitives.ReadUInt32LittleEndian(raw[at..]);
    /// <summary>주변패딩을보존하고DWORD만쓴다.</summary>
    private static void Put(Span<byte> raw, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    /// <summary>dirty를추가하지않고현재double만쓴다.</summary>
    private static void Time(GunAnimationFields fields, double value) => BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(48), value);
    /// <summary>현재float좌표쌍을값으로읽는다.</summary>
    private static float[] Point(GunAnimationFields fields, int at) => [BinaryPrimitives.ReadSingleLittleEndian(fields.Bytes.AsSpan(at)), BinaryPrimitives.ReadSingleLittleEndian(fields.Bytes.AsSpan(at + 4))];
    /// <summary>공통64BYTE의실제파생VT/식별자/두WORD/현재시각만초기화한다.</summary>
    public void Initialize(GunAnimationFields fields, ushort gunForm, ushort target)
    {
        if (gunForm == 0 || gunForm >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(gunForm)); bool sun = _state.Kind == CannonFireKind.Sun, vander = _state.Kind == CannonFireKind.Vander; Put(fields.Bytes, 0, vander ? 0x50b170u : sun ? 0x50b318u : 0x50b370u); Put(fields.Bytes, 4, 0); Put(fields.Bytes, 8, 0); Put(fields.Bytes, 32, vander ? 0x50b16cu : sun ? 0x50b310u : 0x50b368u); fields.Bytes[40] = 0xff; BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(42), gunForm); BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(44), target); Time(fields, _state.Now); Put(fields.Bytes, 56, 0);
    }
    /// <summary>현재부모SID와실제역할을검사한다.</summary>
    private ushort Parent(GunAnimationFields fields) { ushort parent = fields.Parent(); if (parent == 0 || parent >= Pool.Capacity || Pool.Slot(parent)[10] != Type()) throw new InvalidOperationException("캐논 발사 현재부모TYPE/번호 오류"); return parent; }
    /// <summary>최초부모를잡고Unpop뒤에도그SID에프레임을쓰고재등록한다.</summary>
    private void SetFrame(GunAnimationFields fields, uint frame)
    {
        Action<ushort, uint> unpop = Need(_hooks.Unpop), repop = Need(_hooks.Repop); ushort parent = Parent(fields); unpop(parent, 0); Put(Pool.AllocatedBytes(parent), 36, frame); repop(parent, 0x40);
    }
    /// <summary>GunForm은통지전·삭제Form은통지뒤현재필드를읽는다.</summary>
    private void Finish(GunAnimationFields fields) { Action<ushort, uint, uint, uint> notify = Need(_hooks.Notify); Action<ushort, uint> kill = Need(_hooks.Kill); ushort gun = fields.GunForm(); notify(gun, 1, 6, 7); kill(fields.Form(), 0x10); }
    /// <summary>원문파생단계와현재문자구간/음향전역의재조회를수행한다.</summary>
    public uint Step(GunAnimationFields fields, int frame)
    {
        if (_state.Kind == CannonFireKind.Vander) return unchecked((uint)frame) | 0xc000; if (_state.Kind == CannonFireKind.Sun) return frame < 24 ? unchecked((uint)(frame + 24)) | 0x4000 : unchecked((uint)(frame - 24)) | 0x8000;
        ushort parent = Parent(fields); ReadOnlySpan<byte> raw = Pool.Slot(parent); uint current = Read(raw, 36); RiftTypeFrames table = _frames[raw[10]].Frames; if (current >= table.Codes.Count) throw new ArgumentOutOfRangeException(nameof(fields), "썬더 현재프레임코드 범위"); RiftTypeFrames.LetterRun run = table.Run(table.Codes[(int)current].Side); int last = unchecked(run.First + run.Count - 1), next = unchecked(frame + 1); if (next > last) return unchecked((uint)run.First) | 0x8000; if (next == last) return unchecked((uint)next) | 0x4000;
        if (next == unchecked(last - _state.GrowlOffset)) Need(_hooks.Sound)(Parent(fields), "thunderCannonGrowl.wav"); if (next == unchecked(last - _state.RoarOffset)) Need(_hooks.Sound)(Parent(fields), "thunderCannonRoar.wav"); return unchecked((uint)next);
    }
    /// <summary>반더즉시빔의현재실제세효과/같은난수가모두있는지검사한다.</summary>
    private void RequireBeam() { _ = Need(_beam.Create); _ = Need(_beam.Refresh); _ = Need(_beam.Hit); if (_beam.Random is null) throw new InvalidOperationException("반더 실제같은난수 미연결"); }
    /// <summary>최초raw주소와효과뒤현재SID/역할을구별하여일반Missile또는즉시빔을실행한다.</summary>
    public bool Launch(GunAnimationFields fields)
    {
        ushort source = Parent(fields), target = fields.Target(); ReadOnlySpan<byte> initialSource = Pool.Slot(source); if (target == 0 || target >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(fields)); ReadOnlySpan<byte> raw = Pool.Slot(target); if ((raw[11] & 3) != 0) return false; if (raw[10] >= _types.Count) throw new ArgumentOutOfRangeException(nameof(fields)); if ((_types[raw[10]].Flags1 & 0x10) == 0) return false;
        Func<ushort, ushort, float[]> point = Need(_hooks.TargetPoint); if (initialSource[10] == _state.VanderType) RequireBeam(); else _ = Need(_hooks.CreateProjectile); float[] corner = point(fields.Parent(), fields.Target()).ToArray();
        if (initialSource[10] == _state.VanderType)
        {
            RequireBeam(); _beam.Create!(fields.Parent(), fields.Target()); bool avoided = (raw[11] & 3) != 0; if (!avoided && raw[10] == _state.FloaterType) avoided = (double)_beam.Random!.Next(65536) * (1.0 / 65536.0) * 1000 < _state.AvoidanceCutoff; if (avoided) _beam.Refresh!(source); else _beam.Hit!(source, fields.Target()); return (raw[11] & 3) == 0;
        }
        Action<GunProjectileRequest> create = Need(_hooks.CreateProjectile); var request = new GunProjectileRequest { Firing = fields.Parent(), Target = fields.Target(), Speed = _state.Speed, DisplayType = _state.DisplayType, EffectType = _state.EffectType, TargetPosition = Point(fields, 12), TargetCenter = corner, FiringPosition = [-10000,-10000] }; create(request); return true;
    }
    /// <summary>공통부모는같은이미지의현재실제가상Step만선택한다.</summary>
    public void Frame(GunAnimationFields fields) => FrameWithStep(fields, current => Step(fields, current));
    /// <summary>최초dead/raw주소와가상Step뒤현재부모/표적/시각을각소비위치에서읽는다.</summary>
    public void FrameWithStep(GunAnimationFields fields, Func<int, uint>? step)
    {
        ushort target = fields.Target(); if (target == 0 || target >= Pool.Capacity) throw new InvalidOperationException("캐논 발사 표적0 원문관문"); ReadOnlySpan<byte> raw = Pool.Slot(target); bool dead = (raw[11] & 3) != 0;
        if (!dead) { Func<ushort, float[]> center = Need(_hooks.Center); Put(fields.Bytes, 12, Read(raw, 14)); Put(fields.Bytes, 16, Read(raw, 18)); float[] current = center(target).ToArray(); Put(fields.Bytes, 20, BitConverter.SingleToUInt32Bits(current[0])); Put(fields.Bytes, 24, BitConverter.SingleToUInt32Bits(current[1])); }
        ReadOnlySpan<byte> source = Pool.Slot(Parent(fields)); if (!(fields.Time() <= _state.Now)) return; int frame = unchecked((int)Read(source, 36)); uint code = Need(step)(frame); SetFrame(fields, code & 0x3fff); if ((code & 0x4000) != 0 && !Launch(fields) && !dead) return; if ((code & 0x8000) != 0) { Finish(fields); return; }
        fields.Bytes[40] |= 8; Time(fields, _state.Now); double delay = (double)_state.Delay * ((code & 0x4000) != 0 && source[10] == _state.ScaledType ? (double)_state.LaunchScale : 1); Time(fields, _state.Now + delay);
    }
}
