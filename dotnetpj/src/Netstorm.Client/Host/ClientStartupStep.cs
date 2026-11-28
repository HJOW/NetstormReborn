using System.Runtime.ExceptionServices;

namespace Netstorm.Client.Host; // Native 시작의 부분 효과를 자동 재시도로 반복하지 않는다.

/// <summary>한 Client의 시작 단계를 한 번 실행한다. 완료 접두와 최초 실패를 유지하며 reset은 제공하지 않는다.</summary>
public sealed class ClientStartupStep
{
    /// <summary>중복 진입과 실패를 식별하는 시작 단계 이름이다.</summary>
    public string Name { get; }
    /// <summary>실제 action이 정상 복귀한 뒤에만 참이다.</summary>
    public bool Completed { get; private set; }
    /// <summary>현재 같은 UI 스레드에서 이 단계를 실행 중인지 나타낸다.</summary>
    private bool _running;
    /// <summary>부분 효과를 남긴 최초 실패와 원래 스택이다. 다음 진입도 같은 예외를 전달한다.</summary>
    private ExceptionDispatchInfo? _failure;

    /// <summary>단계의 설명만 보관하며 실제 Client 초기화는 실행하지 않는다.</summary>
    public ClientStartupStep(string name)
    {
        ArgumentException.ThrowIfNullOrEmpty(name); Name = name;
    }

    /// <summary>성공한 단계는 건너뛰고 실패한 단계는 재실행하지 않는다. 잡힌 내부 재진입 오류도 성공으로 바꾸지 않는다.</summary>
    public void Run(Action initialize)
    {
        ArgumentNullException.ThrowIfNull(initialize);
        _failure?.Throw();
        if (Completed) return;
        if (_running)
        {
            _failure = ExceptionDispatchInfo.Capture(new InvalidOperationException("Client 시작 단계 재진입: " + Name));
            _failure.Throw();
        }
        _running = true;
        try
        {
            initialize();
            _failure?.Throw();
            Completed = true;
        }
        catch (Exception error)
        {
            _failure ??= ExceptionDispatchInfo.Capture(error);
            _failure.Throw();
            throw;
        }
        finally { _running = false; }
    }
}
