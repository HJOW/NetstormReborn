// 커널 프로세스의 부착 식별자와 정기 예약 자료를 공유한다.
namespace Netstorm.Core.Original;

// 프로세스 기본 모듈을 담는 네임스페이스다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.

// 부모의 종속 체인에 들어 있는 form과 부착 대상 객체다. 부착은 Host 본체 단위에서 잇는다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.
public class SquidProcess : BaseProcess
{
    /// <summary>커널 프로세스와 연결된 객체 Form의 SID다. 부착 전에는 0이다. Host 본체 단위에서 붙인다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    private int _form = 0;

    /// <summary>부착·통지 대상인 부모 객체 또는 부모 SID다. 부착 전에는 0이다. Host 본체 단위에서 붙인다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    private int _parent = 0;

    /// <summary>커널의 프로세스 등록 번호다. 부착 전에는 0이다. Host가 Kill 뒤 등록 여부를 물을 때 쓴다. Host 본체 단위에서 붙인다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    private uint _pid = 0u;

    /// <summary>부착한 host다. 부착 전에는 null이다. Host 본체 단위에서 붙인다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.</summary>
    internal SquidProcessHost? HostLink { get; set; }

    /// <summary>독립 월드 최종 fallback은 자기 대여가 만든 프로세스만 반납한다. Host 본체 단위에서 붙인다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.</summary>
    internal ProcessHostRuntimeBindings? RuntimeLink { get; set; }

    /// <summary>부착·재수신 때 식별자를 한 번에 둔다. 원문 friend 쓰기와 같다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.</summary>
    /// <param name="form">쓸 Form SID다.</param>
    /// <param name="parent">쓸 부모 SID다.</param>
    /// <param name="pid">쓸 커널 등록 번호다.</param>
    internal void SetIdentity(int form, int parent, uint pid)
    {
        _form = form;
        _parent = parent;
        _pid = pid;
    }

    /// <summary>부모의 종속 체인에 들어 있는 form이다. 부착 전에는 0이다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <returns>Form SID다.</returns>
    public int Form() => _form;

    /// <summary>부착·통지 대상인 부모 객체 또는 부모 SID다. 부착 전에는 0이다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <returns>부모 SID다.</returns>
    public int Parent() => _parent;

    /// <summary>form 삭제 직전의 가상 통지다. 원본 base는 빈 함수다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <param name="flags">삭제 플래그다.</param>
    public virtual void OnFormDestroy(uint flags)
    {
    }

    /// <summary>부모 삭제 중의 가상 통지다. 원본 base는 빈 함수다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <param name="parent">죽는 부모다.</param>
    public virtual void OnParentDying(int parent)
    {
    }

    /// <summary>부착 직후의 가상 통지다. 원본 base는 빈 함수다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    public virtual void OnAttached()
    {
    }

    /// <summary>ProcessForm 가상 34가 같은 커널 객체의 실제 내장 통지에 전달하는 event·info·value다. 복원하지 않은 파생 통지는 선택 시 예외로 거부한다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <param name="ev">사건 번호다.</param>
    /// <param name="info">부가 정보다.</param>
    /// <param name="value">전달 값이다.</param>
    public virtual void Notify(uint ev, uint info, uint value)
    {
        throw new InvalidOperationException("복원하지 않은 파생 프로세스 통지");
    }

    /// <summary>부착한 host다. 부착 전에는 null이다. Host 본체 단위에서 잇는다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <returns>부착한 호스트다. 부착 전에는 null이다.</returns>
    protected SquidProcessHost? Host() => HostLink;

    /// <summary>부착한 host다. 부착 전에는 null이다. Host 본체 단위에서 잇는다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <returns>커널의 프로세스 등록 번호다. 부착 전에는 0이다.</returns>
    protected uint Pid() => _pid;

    /// <summary>커널 등록 번호를 같은 어셈블리의 Host 실행 본체에 제공한다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    /// <returns>커널의 프로세스 등록 번호다.</returns>
    internal uint KernelId => _pid;
}

// 정해진 시각에 부모의 이벤트 처리기를 부르고 반환값으로 다음 시각을 정한다. 실행 본체는 Host 본체 단위에서 잇는다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.
public sealed class RegularProcess : SquidProcess
{
    /// <summary>다음 실행 시각이다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    private double _time;

    /// <summary>대상 처리기에 전달할 이벤트 ID다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    private uint _event;

    /// <summary>예약 이벤트와 함께 전달할 실수 인자다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    private float _payload;

    /// <summary>현재 항목 또는 실행 횟수다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    private uint _count;

    /// <summary>실제 공유 생성자로 준비한 객체의 가상 writer 구별이다. 실행·예약 수치와 별개다. 공유 부착은 Host 본체 단위에서 잇는다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    private readonly bool _shared = false;

    /// <summary>원본 필드 초기값이다. 시각은 부착 때의 현재 시각으로 정해진다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <param name="ev">대상 처리기에 전달할 이벤트 ID다.</param>
    /// <param name="payload">예약 이벤트와 함께 전달할 실수 인자다.</param>
    public RegularProcess(uint ev, float payload)
    {
        _event = ev;
        _payload = payload;
    }

    /// <summary>Host 본체 단위에서 실제 정기 실행으로 잇는다. 미연결 호출은 명시 실패한다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐고 0.205에서 Host 실행에 연결했다.</summary>
    public override void RunFrame()
    {
        SquidProcessHost? host = HostLink;
        if (host is null)
        {
            throw new InvalidOperationException("정기 프로세스 호스트 미연결");
        }

        host.RunRegular(this);
    }

    /// <summary>원본 이벤트 번호다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <returns>이벤트 ID다.</returns>
    public uint Event() => _event;

    /// <summary>원본 payload다. 재예약 뒤에는 직전 반환값이다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <returns>실수 인자다.</returns>
    public float Payload() => _payload;

    /// <summary>원본 호출 횟수다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <returns>횟수다.</returns>
    public uint Count() => _count;

    /// <summary>원본 다음 실행 시각이다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <returns>시각이다.</returns>
    public double Time() => _time;

    /// <summary>Dude 작업 변경의 event·count 0·다음 double 시각 직접쓰기를 같은 기존 Regular 객체에 적용한다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <param name="ev">쓸 이벤트 ID다.</param>
    /// <param name="time">쓸 다음 시각이다.</param>
    public void ResetTask(uint ev, double time)
    {
        _event = ev;
        _count = 0u;
        _time = time;
    }

    /// <summary>생성자·부착 콜백 뒤 event·payload·count를 보존하며 실제 기한만 덮어쓴다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <param name="time">덮어쓸 기한이다.</param>
    public void SetNextTime(double time)
    {
        _time = time;
    }

    /// <summary>Damageable 삭제 뒤 새 Regular에 원본 초기 count를 쓴다. event·payload·time·Form은 보존한다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <param name="count">쓸 초기 횟수다.</param>
    public void SetInitialCount(uint count)
    {
        _count = count;
    }

    /// <summary>처리기가 돌려준 다음 payload·시각·횟수를 원문 순서대로 둔다. 2026-11-07 F2d 프로세스 호스트 정기 실행 이식에서 추가됐다.</summary>
    /// <param name="result">처리기가 돌려준 다음 payload다.</param>
    /// <param name="now">더할 현재 시각이다.</param>
    internal void ApplyHandlerResult(float result, double now)
    {
        _payload = result;
        _time = (double)result + now;
        ++_count;
    }

    /// <summary>원문 SharedRegular의 실제 writer 가상 표와 일반 Regular를 구별한다. raw TYPE 변경으로 이 값을 추측하지 않는다. 2026-11-07 F2d 프로세스 기본 이식에서 추가됐다.</summary>
    /// <returns>공유 생성분이면 참이다.</returns>
    public bool Shared() => _shared;
}

// 실제 선택 효과가 반환하기 전에 그 월드 대여를 소비하지 못하게 하는 호출 수명이다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.
public sealed class RuntimeCallGuard : IDisposable
{
    /// <summary>호출 중 실제 처리기 묶음을 유지한다. null이면 아무것도 세지 않는다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.</summary>
    private ProcessHostRuntimeBindings? _runtime;

    /// <summary>같은 현재 대여만 증가시킨다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.</summary>
    /// <param name="runtime">유지할 런타임 묶음이다. null이면 빈 guard다.</param>
    public RuntimeCallGuard(ProcessHostRuntimeBindings? runtime)
    {
        _runtime = runtime;
        if (_runtime is not null)
        {
            ++_runtime.ActiveCalls;
        }
    }

    /// <summary>정상·예외 모두 실제 호출이 끝난 뒤 반납한다. 중첩 반납이 진행 중 호출을 소비하지 못하게 한다. 2026-11-07 F2d 프로세스 호출 수명 이식에서 추가됐다.</summary>
    public void Dispose()
    {
        if (_runtime is not null)
        {
            --_runtime.ActiveCalls;
            _runtime = null;
        }
    }
}
