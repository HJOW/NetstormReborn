using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

// 요청 크기/실제 창 기하를 Screen 버퍼 재생성과 구별한다.
namespace Netstorm.Client;

/// <summary>InterpretOptions의 변경 분기에서만 선택하는 실제 창 장치다.</summary>
public sealed class ClientWindowOptionHooks
{
    /// <summary>앞선 크기 쓰기 뒤 현재 창 사각형을 읽는다.</summary>
    public Func<ScreenRect>? WindowRectangle { get; init; }
    /// <summary>창 사각형 다음에 같은 창의 Client 사각형을 읽는다.</summary>
    public Func<ScreenRect>? ClientRectangle { get; init; }
    /// <summary>같은 창을 x/y/폭/높이와 repaint로 이동한다.</summary>
    public Action<int, int, int, int, bool>? Move { get; init; }
}

/// <summary>00531860/864의 요청 논리 크기와 별도 표시/창 크기를 같은 Client 수명에 유지한다.</summary>
public sealed class ClientWindowOptions
{
    /// <summary>최초 원문640의 요청 논리 폭이다.</summary>
    public int LogicalWidth { get; set; } = 640;
    /// <summary>최초 원문480의 요청 논리 높이다.</summary>
    public int LogicalHeight { get; set; } = 480;
    /// <summary>아직 선택하지 않은 표시 폭0이다.</summary>
    public int PresentWidth { get; set; }
    /// <summary>아직 선택하지 않은 표시 높이0이다.</summary>
    public int PresentHeight { get; set; }
    /// <summary>00542390의 최초-1이며 창 사각형 효과 뒤 바뀐다.</summary>
    public int WindowWidth { get; private set; } = -1;
    /// <summary>00542394의 최초-1이며 실제 Move의 높이 인자와 구별한다.</summary>
    public int WindowHeight { get; private set; } = -1;
    /// <summary>현재 표시/논리 값→설정→정규화→현재 크기 쓰기→기하→Move 순서다. Screen 버퍼를 재생성하지 않는다.</summary>
    public void Interpret(ConfigInterface configuration, bool fsrAvailable, ClientWindowOptionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(configuration); ArgumentNullException.ThrowIfNull(hooks);
        int width = PresentWidth > 0 ? PresentWidth : LogicalWidth, height = PresentHeight > 0 ? PresentHeight : LogicalHeight;
        configuration.ReadInt("SCREENW", ref width); configuration.ReadInt("SCREENH", ref height);
        var normalized = ScreenResolutions.Normalize(width, height, fsrAvailable);
        width = unchecked((int)normalized.Width); height = unchecked((int)normalized.Height);
        configuration.Configuration.Set("SCREENW", width.ToString(CultureInfo.InvariantCulture));
        configuration.Configuration.Set("SCREENH", height.ToString(CultureInfo.InvariantCulture));
        var logical = ScreenResolutions.LogicalSize(width, height);
        if (unchecked((int)logical.Width) == LogicalWidth && unchecked((int)logical.Height) == LogicalHeight && width == PresentWidth && height == PresentHeight) return;
        LogicalWidth = unchecked((int)logical.Width); LogicalHeight = unchecked((int)logical.Height); PresentWidth = width; PresentHeight = height;
        ScreenRect window = (hooks.WindowRectangle ?? throw new InvalidOperationException("옵션의 실제 창 사각형 미연결"))();
        ScreenRect client = (hooks.ClientRectangle ?? throw new InvalidOperationException("옵션의 실제 Client 사각형 미연결"))();
        WindowWidth = unchecked(((PresentWidth - client.Right) - window.Left) + client.Left + window.Right);
        WindowHeight = unchecked(((PresentHeight - client.Bottom) - window.Top) + client.Top + window.Bottom);
        (hooks.Move ?? throw new InvalidOperationException("옵션의 실제 창 이동 미연결"))(0, 0, WindowWidth, WindowWidth, true);
    }
}
