using System.Buffers.Binary;

// 실제 세 비행 VT의 전용 생성/그림자/음향/기본 Damageable 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawFlyerProcess.h의 현재 TYPE 보충 값이며 본체가 vector처럼 값 복사한다.</summary>
public readonly struct FlyerPostPopType
{
    /// <summary>TYPE+e0 실제 float speed의 IEEE 비트다.</summary>
    public uint Speed { get; init; }
    /// <summary>TYPE+114의 전체 물리 프레임 수다.</summary>
    public int Frames { get; init; }
    /// <summary>TYPE+74의 현재 impactSound 이름표 토큰이다.</summary>
    public uint ImpactSound { get; init; }
    /// <summary>TYPE+6c의 현재 activeSound 이름표 토큰이다.</summary>
    public uint ActiveSound { get; init; }
}

/// <summary>실제 할당 예약/가상 공간/소리/기본 후처리의 원문 경계다.</summary>
public sealed class FlyerPostPopHooks
{
    /// <summary>new70의 성공 뒤에만 현재 홈/speed를 읽는다.</summary>
    public Func<bool>? AllocateRainProcess { get; set; }
    /// <summary>확보한 같은 이미지의 기본 생성/flags8 부착 뒤 맨 오 워로 전문화한다.</summary>
    public Action<ushort, ushort, uint, float>? CreateRainProcess { get; set; }
    /// <summary>그림자의 같은 실제 TYPE/flags2 생성이다.</summary>
    public Func<uint, uint, ushort>? Create { get; set; }
    /// <summary>현재 그림자 또는 홈의 실제 가상90 Pop이다.</summary>
    public Action<ushort, float, float, uint>? Pop { get; set; }
    /// <summary>현재 그림자 또는 홈의 실제 가상48 Unpop이다.</summary>
    public Action<ushort, uint>? Unpop { get; set; }
    /// <summary>현재1/8초 압축 QA 시각과 실제 표면 통지다.</summary>
    public Action<ushort>? InitializeUnit { get; set; }
    /// <summary>현재 실제 가상c8의 두float 그림자 offset이다.</summary>
    public Func<ushort, (float X, float Y)>? ShadowOffset { get; set; }
    /// <summary>같은 실제0044bf80과 공통 장부를 한 번 처리한다.</summary>
    public Action<ushort, uint>? Damageable { get; set; }
    /// <summary>new88로 훨리기그의136BYTE를 먼저 확보한다.</summary>
    public Func<bool>? AllocateSunProcess { get; set; }
    /// <summary>같은136BYTE의 기본 부착 뒤 훨리기그 전용 쓰기다.</summary>
    public Action<ushort, ushort, uint, float>? CreateSunProcess { get; set; }
    /// <summary>최초 홈의frame10 뒤 실제 가상4c 재배치다.</summary>
    public Action<ushort, uint>? Repop { get; set; }
    /// <summary>서버/클라이언트 모두 첫Pop에서new28 소리를 확보한다.</summary>
    public Func<bool>? AllocateSound { get; set; }
    /// <summary>확보한 같은47객체에 부모/impact/active/flags1을 전달한다.</summary>
    public Action<ushort, uint, uint, uint>? CreateSound { get; set; }
    /// <summary>더스트 데빌 첫Pop의event0 정기 중복 조회다.</summary>
    public Func<ushort, uint, bool>? HasRegular { get; set; }
    /// <summary>실제new28 정기를 먼저 확보한다.</summary>
    public Func<bool>? AllocateWindRegular { get; set; }
    /// <summary>확보한 실제46객체를event0/0.25f/flags50으로 부착한다.</summary>
    public Action<ushort, uint, float>? CreateWindRegular { get; set; }
    /// <summary>외부 함수 슬롯의 값만 복사한다.</summary>
    internal FlyerPostPopHooks Copy() => (FlyerPostPopHooks)MemberwiseClone();
}

/// <summary>cpppj/src/o/RawFlyerProcess.cpp의 RawFlyerPostPop 전체 본체다.</summary>
public sealed class RawFlyerPostPop
{
    /// <summary>공통 후처리/Host/그림자가 공유하는 같은 실제 Pool이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>원문 vector가 값 복사한 불변 flags1 표다.</summary>
    private readonly OriginalTypeRecord[] _types;
    /// <summary>같은 TYPE 번호의 speed/프레임/소리 보충 값 사본이다.</summary>
    private readonly FlyerPostPopType[] _details;
    /// <summary>처리 중 효과 뒤에도 현재값을 읽는 실제 서버 owner다.</summary>
    private readonly Func<bool> _server;
    /// <summary>외부 함수 값의 사본이다.</summary>
    private readonly FlyerPostPopHooks _hooks;
    /// <summary>같은 PATCH188 계약을 확인하고 TYPE/보충 표와 함수 슬롯을 값 복사한다.</summary>
    public RawFlyerPostPop(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<FlyerPostPopType> details, Func<bool> server, FlyerPostPopHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(details); ArgumentNullException.ThrowIfNull(server); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || details.Count != 188) throw new ArgumentException("비행 후처리 PATCH188 자료 계약");
        Pool = pool; _types = types.Select(type => type.Clone()).ToArray(); _details = details.ToArray(); _server = server; _hooks = hooks.Copy();
    }
    /// <summary>선택된 큰 효과의 누락을 그 소비 위치에서만 거부한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("비행 후처리의 실제 선택 효과 미연결");
    /// <summary>현재 실제 DWORD를 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw[offset..]);
    /// <summary>같은 DWORD만 써 주변 바이트를 보존한다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>현재 raw WORD를 읽는다.</summary>
    private static ushort Word(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt16LittleEndian(raw[offset..]);
    /// <summary>현재 raw float의 비트만 해석한다.</summary>
    private static float Position(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(Read(raw, offset));
    /// <summary>signed 프레임 수와 현재 frame의 낮은DWORD 감김이다.</summary>
    private static uint FrameSum(int frames, ReadOnlySpan<byte> raw) => unchecked((uint)frames + Read(raw, 36));
    /// <summary>현재 실제 세 전용 가상 표만 지원한다.</summary>
    public bool Handles(int sid) => Read(Pool.Slot(sid), 0) is 0x50a388 or 0x50a1e8 or 0x50a050;
    /// <summary>현재 TYPE를 실제 불변 표 범위에서 선택한다.</summary>
    private byte TypeOf(ushort sid)
    {
        byte type = Pool.Slot(sid)[10]; if (type >= _types.Length) throw new ArgumentOutOfRangeException(nameof(sid), "비행 후처리 TYPE 범위 오류"); return type;
    }
    /// <summary>전용 최초 생성/현재 그림자/기본 Damageable 순서이며 효과 뒤 current 조회와 최초 detail을 구분한다.</summary>
    public void PostPop(ushort sid, uint flags)
    {
        if (!Handles(sid)) throw new ArgumentException("다른 비행 파생 PostPop 미복원");
        bool sun = Read(Pool.Slot(sid), 0) == 0x50a1e8, wind = Read(Pool.Slot(sid), 0) == 0x50a050;
        if (wind && (flags & 1) != 0 && !Need(_hooks.HasRegular)(sid, 0) && Need(_hooks.AllocateWindRegular)()) Need(_hooks.CreateWindRegular)(sid, 0, 0.25f);
        if (sun && (flags & 1) != 0)
        {
            FlyerPostPopType original = _details[TypeOf(sid)];
            if (_server())
            {
                ushort home = Word(Pool.Slot(sid), 12); Need(_hooks.Unpop)(home, 0); Put(Pool.AllocatedBytes(home), 36, 10); Need(_hooks.Repop)(home, 0);
                if (Need(_hooks.AllocateSunProcess)()) Need(_hooks.CreateSunProcess)(sid, Word(Pool.Slot(sid), 12), original.Speed, 0.6f);
            }
            if (Need(_hooks.AllocateSound)()) Need(_hooks.CreateSound)(sid, original.ImpactSound, original.ActiveSound, 1);
        }
        if (!sun && !wind && (flags & 1) != 0 && _server() && Need(_hooks.AllocateRainProcess)())
        {
            ushort home = Word(Pool.Slot(sid), 12); uint speed = _details[TypeOf(sid)].Speed; Need(_hooks.CreateRainProcess)(sid, home, speed, 0.5f);
        }
        byte type = TypeOf(sid); FlyerPostPopType details = _details[type];
        if ((flags & 1) != 0)
        {
            if (unchecked((int)Read(Pool.Slot(sid), 36)) < details.Frames && (_types[type].Flags1 & 0x400000) != 0)
            {
                ushort created = Need(_hooks.Create)(Pool.Slot(sid)[10], 2); Span<byte> raw = Pool.AllocatedBytes(created); Put(raw, 36, FrameSum(details.Frames, Pool.Slot(sid))); raw[40] |= 8;
                BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(sid)[8..], created); ReadOnlySpan<byte> parent = Pool.Slot(sid); Need(_hooks.Pop)(created, Position(parent, 14), Position(parent, 18), 0);
            }
            Need(_hooks.InitializeUnit)(sid);
        }
        ushort shadow = Word(Pool.Slot(sid), 8);
        if (shadow != 0)
        {
            Put(Pool.AllocatedBytes(shadow), 36, FrameSum(details.Frames, Pool.Slot(sid))); Need(_hooks.Unpop)(shadow, 0);
            float x = Position(Pool.Slot(sid), 14), y = Position(Pool.Slot(sid), 18); var offset = Need(_hooks.ShadowOffset)(sid);
            Need(_hooks.Pop)(shadow, (float)((double)x + offset.X), (float)((double)y + offset.Y), 0);
        }
        Need(_hooks.Damageable)(sid, flags);
    }
}
