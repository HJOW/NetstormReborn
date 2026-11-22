using System.Buffers.Binary;

// 맨 오 워 풀의 현재 전역과 같은 원문 상태/생성/홈 통지 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>원문 fort/server/Editor/시각/역할 참조와 두DWORD 뷰다.</summary>
public sealed class RainAviaryProcessGlobals
{
    /// <summary>현재 편집 fort 모드의 같은 owner다.</summary>
    public Func<bool> Fort { get; }
    /// <summary>boss와 다른 현재 서버 owner다.</summary>
    public Func<bool> Server { get; }
    /// <summary>현재 Editor DWORD의 같은 owner다.</summary>
    public Func<uint> Editor { get; }
    /// <summary>현재 단일 게임 시각의 같은 owner다.</summary>
    public Func<double> Now { get; }
    /// <summary>현재60 프로세스 역할의 같은 주소다.</summary>
    public UIntBox ProcessType { get; }
    /// <summary>현재144 플라이어 역할의 같은 주소다.</summary>
    public UIntBox FlyerType { get; }
    /// <summary>원문 구조체처럼 값 복사하는 friendlyFire 뷰다.</summary>
    public RawCheckingFlag FriendlyFire { get; set; } = new();
    /// <summary>원문 구조체처럼 값 복사하는 checking 뷰다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>실제 owner를 빌리며 현재 값 사본으로 대체하지 않는다.</summary>
    public RainAviaryProcessGlobals(Func<bool> fort, Func<bool> server, Func<uint> editor, Func<double> now, UIntBox processType, UIntBox flyerType)
    {
        ArgumentNullException.ThrowIfNull(fort); ArgumentNullException.ThrowIfNull(server); ArgumentNullException.ThrowIfNull(editor); ArgumentNullException.ThrowIfNull(now); ArgumentNullException.ThrowIfNull(processType); ArgumentNullException.ThrowIfNull(flyerType);
        Fort = fort; Server = server; Editor = editor; Now = now; ProcessType = processType; FlyerType = flyerType;
    }
    /// <summary>현재 owner는 공유하고 두DWORD 뷰만 원문 값 구조체처럼 복사한다.</summary>
    internal RainAviaryProcessGlobals Copy() => new(Fort, Server, Editor, Now, ProcessType, FlyerType) { FriendlyFire = new(FriendlyFire), Checking = new(Checking) };
}

/// <summary>원문이 선택한 큰 장치만 요구하는 실제 맨 오 워 풀 효과다.</summary>
public sealed class RainAviaryProcessHooks
{
    /// <summary>현재 첫 nondead 프로세스 Form 조회다.</summary>
    public Func<ushort, uint, ushort>? FindProcess { get; set; }
    /// <summary>실제48BYTE caller 이미지 또는 NULL 할당 결과다.</summary>
    public Func<int, RainAviaryProcessFields?>? Allocate { get; set; }
    /// <summary>같은 실제Host에 전달하는 nullable 이미지/현재TYPE/flags8이다.</summary>
    public Action<RainAviaryProcessFields?, ushort, uint, uint>? Attach { get; set; }
    /// <summary>직접 공통 Damageable 후처리다.</summary>
    public Action<ushort, uint>? Damageable { get; set; }
    /// <summary>중복/부모머리/잘못된 단계의 원문 줄번호 진단이다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>같은 부모의 실제 프레임 가상 호출이다.</summary>
    public Action<ushort, int>? SetFrame { get; set; }
    /// <summary>첫 부모의 현재 좌표와 manOWarBuild.wav 요청이다.</summary>
    public Action<float, float, string>? Sound { get; set; }
    /// <summary>checking이 선택한 실제 프레임 진단이다.</summary>
    public Action<ushort, int, uint>? CheckFrame { get; set; }
    /// <summary>현재 플라이어TYPE의 실제flags0 생성이다.</summary>
    public Func<uint, uint, ushort>? Create { get; set; }
    /// <summary>새 플라이어의 실제 소유자 설정이다.</summary>
    public Action<ushort, uint>? SetOwner { get; set; }
    /// <summary>새 플라이어의 실제 공간 배치다.</summary>
    public Action<ushort, float, float, uint>? Pop { get; set; }
    /// <summary>현재 공통 FlyerProcess의 같은 홈 연결이다.</summary>
    public Action<ushort, ushort>? BindFlyer { get; set; }
    /// <summary>원문 함수 슬롯의 값 사본이다.</summary>
    internal RainAviaryProcessHooks Copy() => (RainAviaryProcessHooks)MemberwiseClone();
}

/// <summary>RawRainAviaryProcess.cpp의 실제 생성/후처리/상태기계/난수 홈 통지다.</summary>
public sealed partial class RawRainAviaryProcess
{
    /// <summary>현재 플라이어와 같은 실제 PATCH Pool이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>현재 owner와 값 복사한 두 진단 뷰다.</summary>
    private readonly RainAviaryProcessGlobals _globals;
    /// <summary>선택된 실제 함수 슬롯의 값 사본이다.</summary>
    private readonly RainAviaryProcessHooks _hooks;
    /// <summary>PE005066e0의 실제 double 프레임 지연이다.</summary>
    private const double FrameDelay = 0.20000000298023224;
    /// <summary>PE00501448의 실제 발사 지연이다.</summary>
    private const double LaunchDelay = 3.0;
    /// <summary>같은 PATCH Pool과 현재 전역만 빌린다.</summary>
    public RawRainAviaryProcess(RawSidPool pool, RainAviaryProcessGlobals globals, RainAviaryProcessHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("맨 오 워 풀 프로세스는 PATCH만 복원했습니다");
        Pool = pool; _globals = globals.Copy(); _hooks = hooks.Copy();
    }
    /// <summary>선택된 누락에서만 실패하며 앞선 효과는 유지한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("맨 오 워 풀 실제 하위 소비자 미연결");
    /// <summary>비서버 stage와44DWORD를 포함한 caller padding을 쓰지 않는 실제 생성자다.</summary>
    public void Initialize(RainAviaryProcessFields fields)
    {
        StoreTime(fields, 0); fields.Bytes[8] = 0xff; Put(fields.Bytes, 32, 0x5019ec); Put(fields.Bytes, 40, 0); Put(fields.Bytes, 36, 0);
        Put(fields.Bytes, 0, 0x509e20); Put(fields.Bytes, 32, 0x509e00);
        if (_globals.Server()) { StoreTime(fields, _globals.Now()); fields.Bytes[8] |= 4; Put(fields.Bytes, 24, 1); }
    }
    /// <summary>현재 fort/friendly/server 관문 뒤 조회/nullable 할당/flags8 부착/공통 후처리다.</summary>
    public void PostPop(ushort parent, uint flags)
    {
        if (_globals.Server() && !(_globals.Fort() && !_globals.FriendlyFire.IsEnabled) && (flags & 1) != 0)
        {
            if (Need(_hooks.FindProcess)(parent, _globals.ProcessType.Value) != 0) Need(_hooks.Assertion)(0x257);
            RainAviaryProcessFields? allocated = Need(_hooks.Allocate)(48); RainAviaryProcessFields? fields = allocated is null ? null : new(allocated);
            if (fields is not null) Initialize(fields); Need(_hooks.Attach)(fields, parent, _globals.ProcessType.Value, 8);
        }
        Need(_hooks.Damageable)(parent, flags);
    }
    /// <summary>서버에서만 같은 현재 시각/게임 난수의 실제 홈 통지 본체를 호출한다.</summary>
    public void FlyerDied(RainAviaryProcessFields fields, GameRandom random, Action<RainAviaryProcessFields>? refresh)
    {
        if (!_globals.Server()) return; FlyerDiedIn(fields, _globals.Now, () => random, refresh);
    }
    /// <summary>첫dirty를 유지하고stage10/첫시각 뒤 난수와 두 번째현재시각/정수10..14초/갱신을 소비한다.</summary>
    public static void FlyerDiedIn(RainAviaryProcessFields fields, Func<double>? now, Func<GameRandom>? random, Action<RainAviaryProcessFields>? refresh)
    {
        byte dirty = fields.Bytes[8]; fields.Bytes[8] = (byte)(dirty | 4); Put(fields.Bytes, 24, 10);
        double firstTime = Need(now)(); fields.Bytes[8] = (byte)(dirty | 6); StoreTime(fields, firstTime);
        Func<GameRandom> randomCall = Need(random); uint milliseconds = randomCall().Next(5000) + 10000;
        StoreTime(fields, Need(now)() + milliseconds / 1000u); Action<RainAviaryProcessFields> refreshCall = Need(refresh); refreshCall(fields);
    }
    /// <summary>처음부모/frame과 큰효과 뒤 현재부모/시각/단계를 구별하는 실제 상태기계다.</summary>
    public void Frame(RainAviaryProcessFields fields)
    {
        ushort parent = fields.Parent(); ReadOnlySpan<byte> raw = Pool.Slot(parent); if ((raw[40] & 0x20) != 0 || _globals.Editor() != 0) return;
        uint stage = fields.Stage(); int frame = BinaryPrimitives.ReadInt32LittleEndian(raw[36..]);
        switch (stage)
        {
            case 1:
                Need(_hooks.SetFrame)(parent, 0); Stage(fields, 2); Need(_hooks.Sound)(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), BinaryPrimitives.ReadSingleLittleEndian(raw[18..]), "manOWarBuild.wav"); return;
            case 2:
                if (frame > 11) { Stage(fields, 6); return; } if (!(_globals.Now() >= fields.Time())) return;
                if (_globals.Checking.IsEnabled) Need(_hooks.CheckFrame)(parent, unchecked(frame + 1), 0x7fa);
                Need(_hooks.SetFrame)(parent, unchecked(frame + 1)); SetTime(fields, FrameDelay); return;
            case 6:
                if (_globals.Server())
                {
                    ushort flyer = Need(_hooks.Create)(_globals.FlyerType.Value, 0); Put(Pool.AllocatedBytes(flyer), 36, 0);
                    Need(_hooks.SetOwner)(flyer, raw[34]); ushort current = fields.Parent();
                    if (BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(current)[6..]) == 0) Need(_hooks.Assertion)(0xa4);
                    BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(current)[12..], flyer);
                    float x = (float)((double)BinaryPrimitives.ReadSingleLittleEndian(raw[14..]) - 0.5), y = (float)((double)BinaryPrimitives.ReadSingleLittleEndian(raw[18..]) - 2.0);
                    BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(flyer)[12..], fields.Parent()); Need(_hooks.Pop)(flyer, x, y, 0); Need(_hooks.BindFlyer)(flyer, fields.Parent());
                }
                Stage(fields, 5); Need(_hooks.SetFrame)(parent, 0); SetTime(fields, LaunchDelay); return;
            case 10: if (_globals.Now() >= fields.Time()) Stage(fields, 2); return;
            default: if (_globals.Checking.IsEnabled && (stage < 1 || stage > 10)) Need(_hooks.Assertion)(0x820); return;
        }
    }
}
