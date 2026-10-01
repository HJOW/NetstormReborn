using Microsoft.Xna.Framework.Input;

namespace Netstorm.Game;

/// <summary>클론 자체 입력에 좌표 클릭을 보내고 상태·PNG를 검사한다. OS의 다른 창에 입력하지 않는다.</summary>
internal sealed class UiAutomation
{
    private readonly Queue<string> _commands;
    private MouseState _mouse;
    private int _waitFrames = 6;
    private bool _release;
    /// <summary>다음 그리기 완료 때 저장할 클론 PNG 경로.</summary>
    public string? CapturePath { get; private set; }

    /// <summary>UTF-8 명령 파일의 세미콜론 구분 명령을 읽는다.</summary>
    public UiAutomation(string path) => _commands = new Queue<string>(File.ReadAllText(path).Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries));

    /// <summary>공통 메뉴·지도 입력이 읽을 마우스 상태를 한 프레임씩 만든다.</summary>
    public MouseState Update(string state, Action quit)
    {
        if (_release)
        {
            _mouse = MakeMouse(_mouse.X, _mouse.Y, ButtonState.Released);
            _release = false; _waitFrames = 5; return _mouse;
        }
        if (_waitFrames-- > 0 || _commands.Count == 0 || CapturePath != null) return _mouse;
        string raw = _commands.Dequeue();
        string[] parts = raw.Split(' ', 2, StringSplitOptions.RemoveEmptyEntries);
        string value = parts.Length == 2 ? parts[1].Trim() : "";
        switch (parts[0])
        {
            case "click":
                string[] point = value.Split(',');
                _mouse = MakeMouse(int.Parse(point[0]), int.Parse(point[1]), ButtonState.Pressed); _release = true; break;
            case "wait": _waitFrames = int.Parse(value); break;
            case "capture": CapturePath = value; break;
            case "assert":
                if (state != value) throw new InvalidOperationException($"UI 검사 실패: 예상={value}, 실제={state}");
                Console.WriteLine($"UI PASS: {state}"); break;
            case "quit": quit(); break;
            default: throw new ArgumentException($"알 수 없는 UI 검사 명령: {raw}");
        }
        return _mouse;
    }

    /// <summary>왼쪽 버튼만 누른 논리 좌표 마우스를 만든다.</summary>
    private static MouseState MakeMouse(int x, int y, ButtonState left) => new(x, y, 0, left, ButtonState.Released,
        ButtonState.Released, ButtonState.Released, ButtonState.Released);

    /// <summary>그리기가 끝난 뒤 캡처 요청을 한 번 꺼낸다.</summary>
    public string? TakeCapture() { string? path = CapturePath; CapturePath = null; return path; }
}
