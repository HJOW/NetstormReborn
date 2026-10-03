using Microsoft.Xna.Framework.Input;

namespace Netstorm.Game;

/// <summary>클론 자체 입력에 좌표 클릭을 보내고 상태·PNG를 검사한다. OS의 다른 창에 입력하지 않는다.</summary>
internal sealed class UiAutomation
{
    private readonly Queue<string> _commands;
    private MouseState _mouse;
    private int _waitFrames = 6;
    private bool _release;
    private bool _releaseKey;
    private int _dragFrames;
    private int _dragStep;
    private int _dragFromX;
    private int _dragFromY;
    private int _dragToX;
    private int _dragToY;
    /// <summary>클론 안에서만 사용할 검증용 키 상태.</summary>
    public KeyboardState Keyboard { get; private set; }
    /// <summary>다음 그리기 완료 때 저장할 클론 PNG 경로.</summary>
    public string? CapturePath { get; private set; }

    /// <summary>UTF-8 명령 파일의 세미콜론 구분 명령을 읽는다.</summary>
    public UiAutomation(string path) => _commands = new Queue<string>(File.ReadAllText(path).Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries));

    /// <summary>공통 입력이 읽을 절대 좌표 또는 화면 중심 기준 마우스 상태를 한 프레임씩 만든다.</summary>
    public MouseState Update(string state, string detail, Action quit, int width, int height)
    {
        if (_releaseKey) { Keyboard = new KeyboardState(); _releaseKey = false; _waitFrames = 5; return _mouse; }
        if (_dragFrames > 0)
        {
            _dragStep++;
            int x = _dragFromX + (_dragToX - _dragFromX) * _dragStep / _dragFrames;
            int y = _dragFromY + (_dragToY - _dragFromY) * _dragStep / _dragFrames;
            _mouse = MakeMouse(x, y, ButtonState.Pressed);
            if (_dragStep >= _dragFrames) { _dragFrames = 0; _release = true; }
            return _mouse;
        }
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
            case "click-center":
                string[] point = value.Split(',');
                int x = int.Parse(point[0]) + (parts[0] == "click-center" ? width / 2 : 0);
                int y = int.Parse(point[1]) + (parts[0] == "click-center" ? height / 2 : 0);
                _mouse = MakeMouse(x, y, ButtonState.Pressed); _release = true; break;
            case "wait": _waitFrames = int.Parse(value); break;
            case "key":
                Keyboard = new KeyboardState(value.Split('+').Select(k => Enum.Parse<Keys>(k, true)).ToArray());
                _releaseKey = true; break;
            case "right-click":
                string[] rightPoint = value.Split(',');
                _mouse = new MouseState(int.Parse(rightPoint[0]), int.Parse(rightPoint[1]), _mouse.ScrollWheelValue,
                    ButtonState.Released, ButtonState.Released, ButtonState.Pressed, ButtonState.Released, ButtonState.Released);
                _release = true; break;
            case "move":
            case "move-center":
                // 화면비별 같은 지도 위치를 검사할 수 있도록 중심 기준 이동도 지원한다.
                string[] movePoint = value.Split(',');
                _mouse = MakeMouse(int.Parse(movePoint[0]) + (parts[0] == "move-center" ? width / 2 : 0),
                    int.Parse(movePoint[1]) + (parts[0] == "move-center" ? height / 2 : 0), ButtonState.Released); break;
            case "drag":
                string[] drag = value.Split(',');
                _dragFromX = int.Parse(drag[0]); _dragFromY = int.Parse(drag[1]);
                _dragToX = int.Parse(drag[2]); _dragToY = int.Parse(drag[3]);
                _dragFrames = Math.Max(1, int.Parse(drag[4])); _dragStep = 0;
                _mouse = MakeMouse(_dragFromX, _dragFromY, ButtonState.Pressed); break;
            case "capture": CapturePath = value; break;
            case "assert":
                if (state != value) throw new InvalidOperationException($"UI 검사 실패: 예상={value}, 실제={state}");
                Console.WriteLine($"UI PASS: {state}"); break;
            case "assert-detail":
            case "assert-detail-not":
                // 부가 상태(선택·유닛 방향)에 값이 포함돼 있는지 검사한다.
                bool excluded = parts[0] == "assert-detail-not";
                if (detail.Contains(value, StringComparison.OrdinalIgnoreCase) == excluded)
                    throw new InvalidOperationException($"UI 세부 검사 실패: {(excluded ? "제외" : "예상")}={value}, 실제={detail}");
                Console.WriteLine($"UI PASS: {(excluded ? "excluded " : "")}{value}"); break;
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
