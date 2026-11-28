using Netstorm.Client.Chat;
using Netstorm.Core.Original;

namespace Netstorm.Client; // 같은 Config writer가 현재 monitor/alert와 활동 owner를 갱신한다.

/// <summary>ClientMain::InterpretMonitorOptions의 최초/변경 해석과 자기 구독 수명을 보유한다.</summary>
public sealed class ClientMonitorOptions : IDisposable
{
    /// <summary>기한을 지우지 않고 두 현재 옵션만 해석할 실제 상태다.</summary>
    private readonly MonitorTimeState _state;
    /// <summary>옵션 해석이1을 쓰는 같은 AutoRestart owner다.</summary>
    private readonly ClientConnectionActivityGlobals _activity;
    /// <summary>현재 Config와 다른 소비자의 구독을 보존한다.</summary>
    private readonly ConfigInterface _configuration;
    /// <summary>자신의 구독만 한 번 반납하는 표식이다.</summary>
    private bool _disposed;
    /// <summary>실제 최초 값을 해석하고 정상 복귀 뒤 자신의 변경 구독만 설치한다.</summary>
    public ClientMonitorOptions(MonitorTimeState state, ClientConnectionActivityGlobals activity, ConfigInterface configuration)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(activity); ArgumentNullException.ThrowIfNull(configuration);
        _state = state; _activity = activity; _configuration = configuration;
        Interpret(); _configuration.OnChanged += Interpret;
    }
    /// <summary>원문 AutoRestart1→monitor→alert 순서다. 누락 값·기한·초기화 비트는 유지한다.</summary>
    public void Interpret()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        _activity.AutoRestart.Value = 1;
        _configuration.ReadInt("monitor", ref _state.Monitor); _configuration.ReadInt("alert", ref _state.Alert);
    }
    /// <summary>상태를 읽거나 변경하지 않고 부모가 같은 감독/활동/설정을 빌리는지 확인한다.</summary>
    internal bool Borrows(MonitorTimeState state, ClientConnectionActivityGlobals activity, ConfigInterface configuration) =>
        ReferenceEquals(_state, state) && ReferenceEquals(_activity, activity) && ReferenceEquals(_configuration, configuration);
    /// <summary>같은 Config에서 자기 해석만 해제하고 다른 구독/상태/기한은 유지한다.</summary>
    public void Dispose() { if (_disposed) return; _configuration.OnChanged -= Interpret; _disposed = true; }
}
