using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // Wind 뒤의 현재 시간 알림을 실제 Chat 제출에 연결한다.

/// <summary>같은 로컬 기한·옵션·현재 이름·manager를 주 스레드에서 빌린다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>실제 최초 옵션 해석과 같은 Config 변경 구독이다.</summary>
    private readonly ClientMonitorOptions? _monitorOptions;
    /// <summary>현재 실제 시각/달력/Config를 읽고 같은 일반 Chat 부모로 시간 알림을 제출한다.</summary>
    private void UpdateNativeMonitorTime()
    {
        var startup = _startup ?? throw new InvalidOperationException("시간 알림의 실제 시작 설정 미설치");
        RawMonitorTime.Step(_localState.MonitorTime, new()
        {
            WallSeconds = () => _clientClock.Clock.WallSeconds(_clock.Milliseconds),
            Calendar = WindowsMonitorCalendar.Read,
            Handle = () =>
            {
                if (_mainMenu is null) throw new InvalidOperationException("시간 알림의 현재 이름 UI 소유자 없음");
                return CurrentNativeChatName();
            },
            Format = () => Encoding.Latin1.GetString(OriginalText.EncodeWindows1252(startup.Configuration.PathSpec("monitorFormat"))),
            Submit = SubmitNativeMonitorChat,
        });
    }
    /// <summary>현재 front를 같은 manager에 임시 적용하고 원문 일반 제출을 실행한 뒤에만 복원한다.</summary>
    private void SubmitNativeMonitorChat(string format, string line)
    {
        RawMonitorTime.SubmitTemporary(_chatManagerGlobals.Current, format, line, OriginalEdition.Patch1078, new()
        {
            Memory = token => _chatManagerGlobals.Bytes(token),
            Submit = (manager, text) => SubmitNativeChat(manager, Encoding.Latin1.GetBytes(text + '\0')),
        });
    }
}
