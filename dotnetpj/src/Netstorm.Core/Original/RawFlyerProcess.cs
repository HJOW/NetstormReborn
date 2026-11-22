// 현재 TYPE 참조/서버/시각과 실제 공통 비행 생성·홈·명령 경계를 연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawFlyerProcess.h의 세 원문 전역 참조와 checking의 현재 뷰다.</summary>
public sealed class FlyerProcessGlobals
{
    /// <summary>현재00540bc0의 같은 실제 서버 owner다.</summary>
    public Func<bool> Server { get; }
    /// <summary>현재0055b4d0의 같은 실제 게임 시각 owner다.</summary>
    public Func<double> Now { get; }
    /// <summary>005410ec의 최초31 프로세스 역할 주소다.</summary>
    public UIntBox ProcessType { get; }
    /// <summary>홈 finder 뒤의 별도 현재 진단 DWORD다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>실제 전역 owner를 빌리며 초기값이나 사본을 쓰지 않는다.</summary>
    public FlyerProcessGlobals(Func<bool> server, Func<double> now, UIntBox processType)
    {
        ArgumentNullException.ThrowIfNull(server); ArgumentNullException.ThrowIfNull(now); ArgumentNullException.ThrowIfNull(processType);
        Server = server; Now = now; ProcessType = processType;
    }
    /// <summary>전역 참조는 공유하고 checking 뷰의 값은 원문 구조체처럼 복사한다.</summary>
    internal FlyerProcessGlobals Copy() => new(Server, Now, ProcessType) { Checking = new(Checking) };
}

/// <summary>공통 프로세스가 원문 위치에서 요구하는 실제 부착/조회/큰 비행 장치다.</summary>
public sealed class FlyerProcessHooks
{
    /// <summary>원문 서버/비행 genus/홈 processSid의 줄 번호 진단이다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>같은 이미지와 부모/현재TYPE/flags8의 실제 Host 부착이다.</summary>
    public Action<FlyerProcessFields, ushort, uint, uint>? Attach { get; set; }
    /// <summary>현재 부모 체인의 첫 nondead 현재TYPE를 찾는다.</summary>
    public Func<ushort, uint, ushort>? FindProcess { get; set; }
    /// <summary>현재 Form/PID가 소유한 같은 원문 이미지다.</summary>
    public Func<ushort, FlyerProcessFields>? Fields { get; set; }
    /// <summary>별도로 복원할 실제 비행 Frame이다. 미설치는 실패한다.</summary>
    public Action<FlyerProcessFields>? Frame { get; set; }
    /// <summary>실제 서버 표적 해제의 메시지/인수 두 개다.</summary>
    public Action<FlyerProcessFields, uint, uint, uint>? Notify { get; set; }
    /// <summary>실제 현재 홈의 부모 사망 통지다.</summary>
    public Action<FlyerProcessFields>? ParentDying { get; set; }
    /// <summary>종료 직전 실제 현재 부모의 파편 생성이다.</summary>
    public Action<ushort>? Pieces { get; set; }
    /// <summary>파편 효과 뒤 현재 부모의 가상10 삭제다.</summary>
    public Action<ushort, uint>? Destroy { get; set; }
    /// <summary>함수 값만 복사하고 외부 효과는 실행하지 않는다.</summary>
    internal FlyerProcessHooks Copy() => (FlyerProcessHooks)MemberwiseClone();
}

/// <summary>cpppj/src/o/RawFlyerProcess.cpp의 공통 생성/홈/명령/실제 장치 경계다.</summary>
public sealed partial class RawFlyerProcess
{
    /// <summary>비행체와 Form이 공유하는 같은 실제 Pool이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>원문 span처럼 현재 TYPE를 참조한다. Gun의 vector 값 복사와 다르다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>객체보다 오래 사는 현재 전역 참조다.</summary>
    private readonly FlyerProcessGlobals _globals;
    /// <summary>원문 함수 슬롯의 값 사본이다.</summary>
    private readonly FlyerProcessHooks _hooks;
    /// <summary>PATCH188 계약만 확인하며 현재 TYPE와 전역은 복제하지 않는다.</summary>
    public RawFlyerProcess(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, FlyerProcessGlobals globals, FlyerProcessHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188) throw new ArgumentException("비행 프로세스의 PATCH188 계약");
        Pool = pool; _types = types; _globals = globals.Copy(); _hooks = hooks.Copy();
    }
    /// <summary>선택된 누락만 앞선 필드 효과를 보존하며 거부한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("비행체 실제 하위 몸체 미연결");
    /// <summary>기본 표로 실제 부착한 뒤 caller가 Rain/Sun/Wind 전문화를 수행한다.</summary>
    public void Construct(FlyerProcessFields fields, ushort parent, ushort home, uint speed, float parameter)
    {
        Time(fields, 32, 0); Time(fields, 40, 0); fields.Bytes[8] = 0xff; Put(fields.Bytes, 48, 0x5019ec); Put(fields.Bytes, 56, 0); Put(fields.Bytes, 52, 0);
        Put(fields.Bytes, 0, 0x509cc8); Put(fields.Bytes, 48, 0x509ca8); Time(fields, 64, 0); Time(fields, 72, 0); Put(fields.Bytes, 84, 0);
        if (!_globals.Server()) Need(_hooks.Assertion)(0x2e3);
        Word(fields.Bytes, 10, home); fields.Bytes[8] |= 0x1e; Put(fields.Bytes, 16, speed); Word(fields.Bytes, 12, 0); Put(fields.Bytes, 20, BitConverter.SingleToUInt32Bits(parameter));
        fields.Bytes[8] |= 0x60; Time(fields, 32, _globals.Now()); Put(fields.Bytes, 24, 0);
        Need(_hooks.Attach)(fields, parent, _globals.ProcessType.Value, 8);
    }
    /// <summary>현재 부모 TYPE의 실제 비행 genus를 확인하고 진단이 복귀하면 계속한다.</summary>
    private void CheckGenus(ushort flyer, uint line)
    {
        byte type = Pool.Slot(flyer)[10]; if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(flyer), "비행 홈 TYPE 범위 오류");
        if ((_types[type].Flags2 & 0x100000) == 0) Need(_hooks.Assertion)(line);
    }
    /// <summary>현재 첫 nondead Form만 갱신하며 없으면 자연 반환한다.</summary>
    public void Bind(ushort flyer, ushort home)
    {
        CheckGenus(flyer, 0x904); ushort form = Need(_hooks.FindProcess)(flyer, _globals.ProcessType.Value); if (form == 0) return;
        FlyerProcessFields fields = Need(_hooks.Fields)(form); fields.Bytes[8] |= 2; Word(fields.Bytes, 10, home);
    }
    /// <summary>genus 진단은 checking 밖이며 finder 뒤 현재 checking과 저장한 Form을 사용한다.</summary>
    public ushort Home(ushort flyer)
    {
        CheckGenus(flyer, 0x8ee); if ((Pool.Slot(flyer)[11] & 3) != 0) return 0;
        ushort form = Need(_hooks.FindProcess)(flyer, _globals.ProcessType.Value);
        if (form == 0) { if (_globals.Checking.IsEnabled) Need(_hooks.Assertion)(0x8f5); return 0; }
        return Need(_hooks.Fields)(form).Home();
    }
    /// <summary>같은 이미지의 실제 큰 Frame을 명시 장치로 전달한다.</summary>
    public void Frame(FlyerProcessFields fields) => Need(_hooks.Frame)(fields);
    /// <summary>원문 메시지와 두 인수를 실제 서버 표적 해제에 전달한다.</summary>
    public void Notify(FlyerProcessFields fields, uint message, uint first, uint second) => Need(_hooks.Notify)(fields, message, first, second);
    /// <summary>현재 이미지의 홈 사망 통지를 실제 본체에 전달한다.</summary>
    public void ParentDying(FlyerProcessFields fields) => Need(_hooks.ParentDying)(fields);
    /// <summary>현재 outer+c의 실제 leaf만 선택하며 자산TYPE나 테마로 추정하지 않는다.</summary>
    public static double ReturnDelay(FlyerProcessFields fields) => Read(fields.Bytes, 0) switch
    {
        0x509cc8 or 0x509f60 => 10000000.0, 0x509f24 => 60.0, 0x509f98 => 25.0,
        _ => throw new InvalidOperationException("비행 명령의 현재 outer+c 실제 leaf 미연결"),
    };
    /// <summary>다음 자연 Frame이 읽을 현재8BYTE 기한만 쓰며 dirty/예약/즉시 Frame은 추가하지 않는다.</summary>
    public void Command(FlyerProcessFields fields, uint command)
    {
        if (command == 1) { double remaining = (Time(fields, 104) - _globals.Now()) + 15.0; double limited = remaining < 60.0 ? remaining : 60.0; Time(fields, 104, limited + _globals.Now()); return; }
        if (command == 2) { double delay = ReturnDelay(fields); Time(fields, 40, delay + _globals.Now()); return; }
        if (command == 3) Time(fields, 40, 0);
    }
    /// <summary>서버 진단·현재 부모 파편·효과 뒤 현재 부모의 flags8 가상 삭제 순서다.</summary>
    public void Finish(FlyerProcessFields fields)
    {
        if (!_globals.Server()) Need(_hooks.Assertion)(0x41d); Need(_hooks.Pieces)(fields.Parent()); Need(_hooks.Destroy)(fields.Parent(), 8);
    }
}
