using System.Buffers.Binary;

// 현재 서버/시각과 원문 훨리베이스의 동일 객체 효과 순서를 연결한다.
namespace Netstorm.Core.Original;

/// <summary>원문 전역 참조와 checking 값 복사를 구별하는 장기 입력이다.</summary>
public sealed class SunAviaryProcessGlobals
{
    /// <summary>boss와 별개인 현재 서버DWORD의 뷰다.</summary>
    public Func<bool> Server { get; }
    /// <summary>현재 Editor 전역의 뷰다.</summary>
    public Func<uint> Editor { get; }
    /// <summary>현재 단일 게임 시각의 뷰다.</summary>
    public Func<double> Now { get; }
    /// <summary>현재32 프로세스 역할의 같은 주소다.</summary>
    public UIntBox ProcessType { get; }
    /// <summary>현재73 플라이어 역할의 같은 주소다.</summary>
    public UIntBox FlyerType { get; }
    /// <summary>원문 구조체처럼 값 복사하는 개발자 진단 뷰다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>실제 owner를 빌리며 현재 값으로 대체하지 않는다.</summary>
    public SunAviaryProcessGlobals(Func<bool> server, Func<uint> editor, Func<double> now, UIntBox processType, UIntBox flyerType)
    {
        ArgumentNullException.ThrowIfNull(server); ArgumentNullException.ThrowIfNull(editor); ArgumentNullException.ThrowIfNull(now); ArgumentNullException.ThrowIfNull(processType); ArgumentNullException.ThrowIfNull(flyerType);
        Server = server; Editor = editor; Now = now; ProcessType = processType; FlyerType = flyerType;
    }
    /// <summary>전역 참조는 공유하고 checking 값만 원문처럼 복사한다.</summary>
    internal SunAviaryProcessGlobals Copy() => new(Server, Editor, Now, ProcessType, FlyerType) { Checking = new(Checking) };
}

/// <summary>원문이 선택한 큰 효과만 요구하는 실제 장치 경계다.</summary>
public sealed class SunAviaryProcessHooks
{
    /// <summary>현재 부모 체인의 첫 nondead Form 조회다.</summary>
    public Func<ushort, uint, ushort>? FindProcess { get; set; }
    /// <summary>48BYTE caller 이미지 또는 실제 NULL 할당 결과다.</summary>
    public Func<int, SunAviaryProcessFields?>? Allocate { get; set; }
    /// <summary>NULL도 같은 flags8 부착 경계에 전달한다.</summary>
    public Action<SunAviaryProcessFields?, ushort, uint, uint>? Attach { get; set; }
    /// <summary>원문 공통 Damageable 후처리다.</summary>
    public Action<ushort, uint>? Damageable { get; set; }
    /// <summary>중복/현재 머리/잘못된 상태의 원문 줄 번호 진단이다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>같은 부모의 실제 프레임 가상 호출이다.</summary>
    public Action<ushort, int>? SetFrame { get; set; }
    /// <summary>첫 부모의 현재 좌표와 apStart.wav 요청이다.</summary>
    public Action<float, float, string>? Sound { get; set; }
    /// <summary>checking이 선택한 실제 프레임 진단이다.</summary>
    public Action<ushort, int, uint>? CheckFrame { get; set; }
    /// <summary>현재 플라이어TYPE의 실제 flags0 생성이다.</summary>
    public Func<uint, uint, ushort>? Create { get; set; }
    /// <summary>새 플라이어의 실제 소유자 설정이다.</summary>
    public Action<ushort, uint>? SetOwner { get; set; }
    /// <summary>새 플라이어의 실제 공간 배치다.</summary>
    public Action<ushort, float, float, uint>? Pop { get; set; }
    /// <summary>같은 공통 FlyerProcess와 현재 홈 연결이다.</summary>
    public Action<ushort, ushort>? BindFlyer { get; set; }
    /// <summary>현재 같은 Host 이미지의 재부착/상태 전송이다.</summary>
    public Action<SunAviaryProcessFields>? RefreshForm { get; set; }
    /// <summary>현재 Form/PID가 소유한 같은 실제 이미지 또는 NULL이다.</summary>
    public Func<ushort, SunAviaryProcessFields?>? ProcessFields { get; set; }
    /// <summary>홈 프레임 직접 쓰기 앞의 실제 공간 해제다.</summary>
    public Action<ushort, uint>? Unpop { get; set; }
    /// <summary>홈 프레임27 직접 쓰기 뒤의 실제 공간 복귀다.</summary>
    public Action<ushort, uint>? Repop { get; set; }
    /// <summary>원문 함수 슬롯을 값 복사한다.</summary>
    internal SunAviaryProcessHooks Copy() => (SunAviaryProcessHooks)MemberwiseClone();
}

/// <summary>RawSunAviaryProcess.cpp의 생성/후처리/전체 상태 기계/홈 통지다.</summary>
public sealed partial class RawSunAviaryProcess
{
    /// <summary>현재 게임 객체와 공유하는 실제 PATCH Pool이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>현재 원문 owner와 copied checking 뷰다.</summary>
    private readonly SunAviaryProcessGlobals _globals;
    /// <summary>선택된 실제 효과의 함수 값 사본이다.</summary>
    private readonly SunAviaryProcessHooks _hooks;
    /// <summary>PE005066e0의 실제 double 프레임 지연이다.</summary>
    private const double FrameDelay = 0.20000000298023224;
    /// <summary>PE00501448의 실제 발사 지연이다.</summary>
    private const double LaunchDelay = 3.0;
    /// <summary>같은 PATCH Pool과 장기 입력만 빌린다.</summary>
    public RawSunAviaryProcess(RawSidPool pool, SunAviaryProcessGlobals globals, SunAviaryProcessHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("훨리베이스 프로세스는 PATCH만 복원했습니다");
        Pool = pool; _globals = globals.Copy(); _hooks = hooks.Copy();
    }
    /// <summary>선택되지 않은 효과는 요구하지 않고 선택된 누락만 거부한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("훨리베이스 실제 하위 소비자 미연결");
    /// <summary>비서버 stage와 caller padding을 보존하는 원문 생성자다.</summary>
    public void Initialize(SunAviaryProcessFields fields)
    {
        StoreTime(fields, 0); fields.Bytes[8] = 0xff; Put(fields.Bytes, 32, 0x5019ec); Put(fields.Bytes, 40, 0); Put(fields.Bytes, 36, 0);
        Put(fields.Bytes, 0, 0x509dd8); Put(fields.Bytes, 32, 0x509db8); Put(fields.Bytes, 44, 0);
        if (_globals.Server()) { StoreTime(fields, _globals.Now()); fields.Bytes[8] |= 4; Put(fields.Bytes, 24, 1); }
    }
    /// <summary>현재 조회/할당/초기화/flags8 부착 뒤 공통 후처리를 호출한다.</summary>
    public void PostPop(ushort parent, uint flags)
    {
        if (_globals.Server() && (flags & 1) != 0)
        {
            if (Need(_hooks.FindProcess)(parent, _globals.ProcessType.Value) != 0) Need(_hooks.Assertion)(0x1e2);
            SunAviaryProcessFields? allocated = Need(_hooks.Allocate)(48); SunAviaryProcessFields? fields = allocated is null ? null : new(allocated);
            if (fields is not null) Initialize(fields); Need(_hooks.Attach)(fields, parent, _globals.ProcessType.Value, 8);
        }
        Need(_hooks.Damageable)(parent, flags);
    }
    /// <summary>서버 관문과 첫 Form/이미지를 잡은 뒤 원문 공간 효과를 실행한다.</summary>
    public void FlyerDied(ushort home)
    {
        if (!_globals.Server()) return;
        ushort form = Need(_hooks.FindProcess)(home, _globals.ProcessType.Value); SunAviaryProcessFields? fields = form != 0 ? Need(_hooks.ProcessFields)(form) : null;
        FlyerDiedIn(Pool, fields, home, _hooks.Unpop, _hooks.Repop, _hooks.RefreshForm);
    }
    /// <summary>NULL 이미지도 Unpop/직접27/Repop 뒤에 거부하며 Native는 같은 물리 슬롯을 쓴다.</summary>
    public static void FlyerDiedIn(RawSidPool pool, SunAviaryProcessFields? fields, ushort home, Action<ushort, uint>? unpop, Action<ushort, uint>? repop, Action<SunAviaryProcessFields>? refresh, bool nativePhysical = false)
    {
        Action<ushort, uint> unpopCall = Need(unpop); unpopCall(home, 0);
        Span<byte> raw = nativePhysical ? pool.RawSlot(home) : pool.AllocatedBytes(home); Put(raw, 36, 27);
        Action<ushort, uint> repopCall = Need(repop); repopCall(home, 0);
        if (fields is null) throw new InvalidOperationException("훨리베이스 홈 통지의 현재 프로세스 NULL 원본 경계");
        Stage(fields, 8); Action<SunAviaryProcessFields> refreshCall = Need(refresh); refreshCall(fields);
    }
    /// <summary>첫 부모 포인터의 현재 좌표로 실제 시작 음향을 요청한다.</summary>
    private void Sound(ushort firstParent)
    {
        ReadOnlySpan<byte> raw = Pool.Slot(firstParent); Need(_hooks.Sound)(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), BinaryPrimitives.ReadSingleLittleEndian(raw[18..]), "apStart.wav");
    }
    /// <summary>진입 stage/frame과 효과 뒤 현재 counter/parent/time을 원문 읽기 위치에서 구별한다.</summary>
    public void Frame(SunAviaryProcessFields fields)
    {
        ushort parent = fields.Parent(); ReadOnlySpan<byte> raw = Pool.Slot(parent); if ((raw[40] & 0x20) != 0 || _globals.Editor() != 0) return;
        uint stage = fields.Stage(); int frame = BinaryPrimitives.ReadInt32LittleEndian(raw[36..]);
        switch (stage)
        {
            case 1: Need(_hooks.SetFrame)(parent, 27); Stage(fields, 8); Sound(parent); return;
            case 2:
                if (frame > 25) { Stage(fields, 6); return; } if (!(_globals.Now() >= fields.Time())) return;
                if (_globals.Checking.IsEnabled) Need(_hooks.CheckFrame)(parent, unchecked(frame + 1), 0x724);
                Need(_hooks.SetFrame)(parent, unchecked(frame + 1)); SetTime(fields, FrameDelay); return;
            case 3: if (_globals.Now() >= fields.Time()) Stage(fields, 4); return;
            case 4:
                if (frame < 1) { Stage(fields, 5); return; } if (!(_globals.Now() >= fields.Time())) return;
                if (_globals.Checking.IsEnabled) Need(_hooks.CheckFrame)(parent, unchecked(frame - 1), 0x751);
                Need(_hooks.SetFrame)(parent, unchecked(frame - 1)); SetTime(fields, FrameDelay); return;
            case 6:
                if (_globals.Server())
                {
                    ushort flyer = Need(_hooks.Create)(_globals.FlyerType.Value, 0); Put(Pool.AllocatedBytes(flyer), 36, 0);
                    Need(_hooks.SetOwner)(flyer, raw[34]); ushort current = fields.Parent();
                    if (BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(current)[6..]) == 0) Need(_hooks.Assertion)(0xa4);
                    BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(current)[12..], flyer);
                    float x = (float)((double)BinaryPrimitives.ReadSingleLittleEndian(raw[14..]) + (double)0.44999998807907104f), y = (float)((double)BinaryPrimitives.ReadSingleLittleEndian(raw[18..]) - 3.0);
                    BinaryPrimitives.WriteUInt16LittleEndian(Pool.AllocatedBytes(flyer)[12..], fields.Parent()); Need(_hooks.Pop)(flyer, x, y, 0);
                    Need(_hooks.BindFlyer)(flyer, fields.Parent()); Stage(fields, 3); SetTime(fields, LaunchDelay); Need(_hooks.RefreshForm)(fields);
                }
                return;
            case 8: case 9:
                if (!(_globals.Now() >= fields.Time())) return; int next = frame;
                if ((fields.Counter() & 1) == 0)
                {
                    if (frame > 29) { Put(fields.Bytes, 44, unchecked(fields.Counter() + 1)); next = unchecked(frame - 1); } else next = unchecked(frame + 1);
                }
                else if (frame > 27) next = unchecked(frame - 1);
                else { if (fields.Stage() == 8) Sound(parent); Put(fields.Bytes, 44, unchecked(fields.Counter() + 1)); next = unchecked(frame + 1); }
                Need(_hooks.SetFrame)(parent, next);
                if (unchecked((int)fields.Counter()) > 9)
                {
                    Put(fields.Bytes, 44, 0); if (fields.Stage() == 8) { Need(_hooks.SetFrame)(parent, 11); Put(fields.Bytes, 24, 2); } else Put(fields.Bytes, 24, 5); fields.Bytes[8] |= 4;
                }
                SetTime(fields, FrameDelay); return;
            default: if (_globals.Checking.IsEnabled && (stage < 1 || stage > 10)) Need(_hooks.Assertion)(0x75e); return;
        }
    }
}
