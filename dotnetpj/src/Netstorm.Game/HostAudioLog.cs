using System.Diagnostics;

// 실제 음향 로그의 쓰기 실패를 보관하면서 worker 합류와 생성 실패 정리를 계속 허용한다.
namespace Netstorm.Client.Host;

/// <summary>ErrorLog.Note의 실제 owner adapter다. 실패를 성공으로 바꾸지 않고 단계별 첫 예외/횟수를 제한된 저장소에 남긴다.</summary>
public sealed class HostAudioLog
{
    /// <summary>기본은 실제 ErrorLog.Note이며 검사에서는 같은 외부 쓰기 경계의 예외만 주입할 수 있다.</summary>
    private readonly Action<string> _writer;
    /// <summary>기본은 Trace이며 실제 주 기록 실패 뒤에만 호출한다.</summary>
    private readonly Action<string> _fallback;
    /// <summary>기본은 Console.Error이며 fallback 자체의 실패 뒤에만 호출한다.</summary>
    private readonly Action<string> _emergency;
    /// <summary>주 스레드/worker 중 먼저 관측한 실제 주 기록 예외 한 개만 보관한다.</summary>
    private Exception? _firstFailure;
    /// <summary>먼저 관측한 실제 fallback 예외 한 개만 보관한다.</summary>
    private Exception? _firstFallbackFailure;
    /// <summary>먼저 관측한 실제 최후 기록 예외 한 개만 보관한다.</summary>
    private Exception? _firstEmergencyFailure;
    /// <summary>주 기록의 실패 횟수다. 로그 목록을 무제한 저장하지 않는다.</summary>
    private int _failures;
    /// <summary>fallback 자체의 실패 횟수다.</summary>
    private int _fallbackFailures;
    /// <summary>최후 기록 자체의 실패 횟수다.</summary>
    private int _emergencyFailures;

    /// <summary>실제 기본 기록과 보호된 후순위 장치를 연결하며 호출 시점까지 기록/OS 자원을 열지 않는다.</summary>
    public HostAudioLog(Action<string>? writer = null, Action<string>? fallback = null, Action<string>? emergency = null)
    {
        _writer = writer ?? ErrorLog.Note;
        _fallback = fallback ?? (text => Trace.WriteLine(text));
        _emergency = emergency ?? (text => Console.Error.WriteLine(text));
    }

    /// <summary>실제 첫 주 기록 실패를 다른 호출자의 저장과 경쟁하지 않고 읽는다.</summary>
    public Exception? FirstFailure => Volatile.Read(ref _firstFailure);
    /// <summary>실제 첫 fallback 실패다. 주 기록 성공과 혼동하지 않는다.</summary>
    public Exception? FirstFallbackFailure => Volatile.Read(ref _firstFallbackFailure);
    /// <summary>실제 첫 최후 기록 실패다. 정리 정책 밖으로 다시 던지지 않는다.</summary>
    public Exception? FirstEmergencyFailure => Volatile.Read(ref _firstEmergencyFailure);
    /// <summary>현재까지 실제 주 기록의 실패 횟수다.</summary>
    public int FailureCount => Volatile.Read(ref _failures);
    /// <summary>현재까지 실제 fallback의 실패 횟수다.</summary>
    public int FallbackFailureCount => Volatile.Read(ref _fallbackFailures);
    /// <summary>현재까지 실제 최후 기록의 실패 횟수다.</summary>
    public int EmergencyFailureCount => Volatile.Read(ref _emergencyFailures);

    /// <summary>실제 기록을 먼저 시도하고 각 실패를 thread-safe로 보관한다. 모든 후순위 실패도 종료/합류/최초 예외를 중단하지 않는다.</summary>
    public void Write(string text)
    {
        try { _writer(text); }
        catch (Exception error)
        {
            Interlocked.CompareExchange(ref _firstFailure, error, null);
            Interlocked.Increment(ref _failures);
            try { _fallback("음향 주 로그 기록 실패: " + text); }
            catch (Exception fallbackError)
            {
                Interlocked.CompareExchange(ref _firstFallbackFailure, fallbackError, null);
                Interlocked.Increment(ref _fallbackFailures);
                try { _emergency("음향 주 로그/fallback 기록 실패: " + text); }
                catch (Exception emergencyError)
                {
                    Interlocked.CompareExchange(ref _firstEmergencyFailure, emergencyError, null);
                    Interlocked.Increment(ref _emergencyFailures);
                }
            }
        }
    }
}
