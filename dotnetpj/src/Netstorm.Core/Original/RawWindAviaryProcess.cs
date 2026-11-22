using System.Buffers.Binary;

// 데빌메이커의 현재 전역/후처리/홈 통지/실제 임시 객체 삭제를 연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawWindAviaryProcess.h의 장기 owner 참조와 두DWORD 뷰다.</summary>
public sealed class WindAviaryGlobals
{
    /// <summary>현재fort 모드의 같은 owner다.</summary>
    public Func<bool> Fort { get; }
    /// <summary>현재server 모드의 같은 owner다.</summary>
    public Func<bool> Server { get; }
    /// <summary>server와 구별하는 현재 생성/삭제 boss 권한이다.</summary>
    public Func<bool> Boss { get; }
    /// <summary>현재Editor DWORD의 같은 owner다.</summary>
    public Func<uint> Editor { get; }
    /// <summary>현재 단일 게임 시각의 같은 owner다.</summary>
    public Func<double> Now { get; }
    /// <summary>현재59 프로세스 역할의 같은 주소다.</summary>
    public UIntBox ProcessType { get; }
    /// <summary>현재138 플라이어 역할의 같은 주소다.</summary>
    public UIntBox FlyerType { get; }
    /// <summary>Session이 소유한 실제건물139 역할이며 PE전역과 구별한다.</summary>
    public UIntBox AviaryType { get; }
    /// <summary>값 복사하는 현재 friendlyFire DWORD 뷰다.</summary>
    public RawCheckingFlag FriendlyFire { get; set; } = new();
    /// <summary>값 복사하는 현재 checking DWORD 뷰다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>실제 owner를 빌리며 현재 값 사본으로 바꾸지 않는다.</summary>
    public WindAviaryGlobals(Func<bool> fort, Func<bool> server, Func<bool> boss, Func<uint> editor, Func<double> now, UIntBox processType, UIntBox flyerType, UIntBox aviaryType)
    {
        ArgumentNullException.ThrowIfNull(fort); ArgumentNullException.ThrowIfNull(server); ArgumentNullException.ThrowIfNull(boss); ArgumentNullException.ThrowIfNull(editor); ArgumentNullException.ThrowIfNull(now); ArgumentNullException.ThrowIfNull(processType); ArgumentNullException.ThrowIfNull(flyerType); ArgumentNullException.ThrowIfNull(aviaryType);
        Fort = fort; Server = server; Boss = boss; Editor = editor; Now = now; ProcessType = processType; FlyerType = flyerType; AviaryType = aviaryType;
    }
    /// <summary>원문 참조는 공유하고 두DWORD 뷰만 값 복사한다.</summary>
    internal WindAviaryGlobals Copy() => new(Fort, Server, Boss, Editor, Now, ProcessType, FlyerType, AviaryType) { FriendlyFire = new(FriendlyFire), Checking = new(Checking) };
}

/// <summary>원문이 선택한 큰 효과만 요구하는 실제 Wind 장치다.</summary>
public sealed class WindAviaryHooks
{
    /// <summary>현재 첫 nondead 프로세스 Form 조회다.</summary>
    public Func<ushort, uint, ushort>? FindProcess { get; set; }
    /// <summary>실제80BYTE caller 이미지 또는 NULL 할당 결과다.</summary>
    public Func<WindAviaryProcessFields?>? Allocate { get; set; }
    /// <summary>같은 실제Host의 nullable 이미지/부모/현재TYPE flags8 부착이다.</summary>
    public Action<WindAviaryProcessFields?, ushort, uint>? Attach { get; set; }
    /// <summary>전용 후처리 뒤 직접 공통 Damageable이다.</summary>
    public Action<ushort, uint>? Damageable { get; set; }
    /// <summary>원문 Flyer.cpp의 해당 줄번호 진단이다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>현재 실제 슬롯의 프레임 가상 호출이다.</summary>
    public Action<ushort, int>? SetFrame { get; set; }
    /// <summary>checking이 선택한 현재 프레임 진단이다.</summary>
    public Action<ushort, int, uint>? CheckFrame { get; set; }
    /// <summary>현재 플라이어TYPE의 실제flags0 생성이다.</summary>
    public Func<uint, ushort>? Create { get; set; }
    /// <summary>새 더스트 데빌의 실제 가상 소유자 지정이다.</summary>
    public Action<ushort, uint>? SetOwner { get; set; }
    /// <summary>현재SHP 추가헤더의+14 높이 보정이다.</summary>
    public Func<ushort, int, float>? Height { get; set; }
    /// <summary>현재TYPE의 signed 발자국 높이다.</summary>
    public Func<ushort, float>? FootY { get; set; }
    /// <summary>새플라이어의 실제가상Pop flags0이다.</summary>
    public Action<ushort, float, float>? Pop { get; set; }
    /// <summary>같은 실제Flyer104BYTE 할당/생성/Host부착/전용표 전문화다.</summary>
    public Action<ushort, ushort>? CreateFlyerProcess { get; set; }
    /// <summary>현재공통FlyerProcess의 홈WORD 연결이다.</summary>
    public Action<ushort, ushort>? BindFlyer { get; set; }
    /// <summary>처음부모/실제새플라이어/현재원문으로 계산하는 특별 원주 배치다.</summary>
    public Func<ushort, ushort, WindAviaryProcessFields, (float X, float Y)>? CirclePosition { get; set; }
    /// <summary>상태13의 실제프로세스종료 flags0이다.</summary>
    public Action<WindAviaryProcessFields>? Finish { get; set; }
    /// <summary>현재임시플라이어의 실제가상삭제 flags0이다.</summary>
    public Action<ushort, uint>? Destroy { get; set; }
    /// <summary>원문 함수슬롯을 값 복사한다.</summary>
    internal WindAviaryHooks Copy() => (WindAviaryHooks)MemberwiseClone();
}

/// <summary>RawWindAviaryProcess.cpp의 현재 전역/후처리/홈 통지/가상삭제다.</summary>
public sealed partial class RawWindAviaryProcess
{
    /// <summary>현재부모/플라이어와 공유하는 같은 실제Pool이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>현재owner와 값복사한 두DWORD 뷰다.</summary>
    private readonly WindAviaryGlobals _globals;
    /// <summary>선택된 실제효과의 함수값 사본이다.</summary>
    private readonly WindAviaryHooks _hooks;
    /// <summary>같은PATCH Pool과현재owner를 빌린다.</summary>
    public RawWindAviaryProcess(RawSidPool pool, WindAviaryGlobals globals, WindAviaryHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("데빌메이커는 PATCH만 복원했습니다");
        Pool = pool; _globals = globals.Copy(); _hooks = hooks.Copy();
    }
    /// <summary>선택된 누락만 앞선효과를 보존하며 거부한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("데빌메이커 실제 하위 효과 미연결");
    /// <summary>비정렬 원문DWORD를 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    /// <summary>지정한 원문DWORD만 기록한다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>원문double을 계산 없이 읽는다.</summary>
    private static double Time(WindAviaryProcessFields fields, int offset) => BinaryPrimitives.ReadDoubleLittleEndian(fields.Bytes.AsSpan(offset));
    /// <summary>dirty를 추가하지 않고 원문double8BYTE만 쓴다.</summary>
    private static void Time(WindAviaryProcessFields fields, int offset, double value) => BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(offset), value);
    /// <summary>상태DWORD와 원문dirty4를 함께 쓴다.</summary>
    private static void State(WindAviaryProcessFields fields, uint value) { fields.Bytes[8] |= 4; Put(fields.Bytes, 24, value); }
    /// <summary>임시플라이어DWORD와 원문dirty20을 함께 쓴다.</summary>
    private static void Flyer(WindAviaryProcessFields fields, uint value) { fields.Bytes[8] |= 0x20; Put(fields.Bytes, 48, value); }
    /// <summary>현재inner 부모DWORD의 하위WORD를 읽는다.</summary>
    private static ushort Parent(WindAviaryProcessFields fields) => unchecked((ushort)Read(fields.Bytes, 64));
    /// <summary>현재임시플라이어DWORD의 하위WORD를 읽는다.</summary>
    private static ushort Flyer(WindAviaryProcessFields fields) => unchecked((ushort)Read(fields.Bytes, 48));
    /// <summary>flags/fort/friendly/server 관문과 nullable할당/현재생성/flags8부착/공통순서다.</summary>
    public void PostPop(ushort parent, uint flags)
    {
        if ((flags & 1) != 0 && !(_globals.Fort() && !_globals.FriendlyFire.IsEnabled) && _globals.Server())
        {
            if (Need(_hooks.FindProcess)(parent, _globals.ProcessType.Value) != 0) Need(_hooks.Assertion)(0x275);
            WindAviaryProcessFields? allocated = Need(_hooks.Allocate)(); WindAviaryProcessFields? fields = allocated is null ? null : new(allocated);
            if (fields is not null) RawWindAviaryState.Initialize(fields, _globals.Server(), _globals.Now()); Need(_hooks.Attach)(fields, parent, _globals.ProcessType.Value);
        }
        Need(_hooks.Damageable)(parent, flags);
    }
    /// <summary>현재서버에서만 실제동일이미지의 홈 통지를 실행한다.</summary>
    public void FlyerDied(WindAviaryProcessFields fields, Action<WindAviaryProcessFields>? refresh)
    {
        if (!_globals.Server()) return; FlyerDiedIn(fields, refresh);
    }
    /// <summary>난수/시각변경 없이 AP2/dirty4를 먼저쓰고 실제갱신을 요구한다.</summary>
    public static void FlyerDiedIn(WindAviaryProcessFields fields, Action<WindAviaryProcessFields>? refresh)
    {
        State(fields, 2); Action<WindAviaryProcessFields> call = Need(refresh); call(fields);
    }
    /// <summary>전체DWORD 주소감김/범위/dead2관문 뒤 최초SID의 현재실제가상삭제를 수행한다.</summary>
    public void FormDestroy(WindAviaryProcessFields fields)
    {
        if (!_globals.Boss()) return; uint stored = Read(fields.Bytes, 48); if (stored == 0) return;
        uint offset = unchecked(stored * 50u); if (offset < 50 || offset >= (uint)Pool.Capacity * 50u) return;
        if (offset % 50 != 0) throw new InvalidOperationException("데빌메이커 임시 포인터가 슬롯 경계 밖입니다");
        ushort sid = unchecked((ushort)(offset / 50)); ReadOnlySpan<byte> raw = Pool.Slot(sid);
        if ((raw[11] & 2) != 0) return; if (raw[10] != _globals.FlyerType.Value) Need(_hooks.Assertion)(0x8e4); Need(_hooks.Destroy)(sid, 0);
    }
}
