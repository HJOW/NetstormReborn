using Microsoft.Xna.Framework.Input;
using Netstorm.Core.Display;

namespace Netstorm.Game;

/// <summary>
/// 클론 자체 입력에 좌표 클릭을 보내고 상태·PNG를 검사한다. OS의 다른 창에 입력하지 않는다.
/// 대기(<c>wait N</c>, 조건 대기 한도, 입력 사이의 짧은 쉼)는 프레임 수가 아니라 <b>1/60초 단위의 시간</b>으로 센다.
/// 화면 루프 속도(원본 수준 약 71.4바퀴/초, 이후 60·120)가 달라도 같은 스크립트가 같은 게임 시간만큼 기다린다.
/// </summary>
internal sealed class UiAutomation
{
    /// <summary>조건 대기의 최대 시간, 1/60초 단위 (30초). 게임이 정지하거나 조건이 틀려도 스모크가 무한히 기다리지 않게 한다.</summary>
    private const int DetailWaitFrameLimit = 1800;
    /// <summary>시작 직후 첫 명령 전에 쉬는 시간, 1/60초 단위</summary>
    private const int StartWaitFrames = 6;
    /// <summary>버튼·키를 뗀 뒤 다음 명령 전에 쉬는 시간, 1/60초 단위</summary>
    private const int ReleaseWaitFrames = 5;
    /// <summary>남은 대기 시간을 0 과 비교할 때의 허용 오차(초). 부동소수 누적 오차로 한 프레임 더 기다리지 않게 한다.</summary>
    private const double WaitTolerance = 1e-9;
    /// <summary>조건에 해당하는 세부 상태가 나타날 때까지 기다리는 문자열.</summary>
    private string? _waitDetail;
    /// <summary>조건 대기에 남은 시간(초).</summary>
    private double _waitDetailSeconds;
    private readonly Queue<string> _commands;
    private MouseState _mouse;
    /// <summary>다음 명령을 실행하기 전에 남은 대기 시간(초).</summary>
    private double _waitSeconds = Seconds(StartWaitFrames);
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

    /// <summary>스크립트의 프레임 단위(1/60초)를 초로 바꾼다.</summary>
    /// <param name="frames">1/60초 단위의 길이</param>
    private static double Seconds(int frames) => (double)frames / FramePacing.ScriptFramesPerSecond;

    /// <summary>공통 입력이 읽을 절대 좌표 또는 화면 중심 기준 마우스 상태를 한 프레임씩 만든다.</summary>
    /// <param name="seconds">지난 갱신 이후 경과 시간(초). 대기를 이 시간만큼 줄인다</param>
    public MouseState Update(string state, string detail, Action quit, int width, int height, double seconds)
    {
        if (_releaseKey) { Keyboard = new KeyboardState(); _releaseKey = false; _waitSeconds = Seconds(ReleaseWaitFrames); return _mouse; }
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
            _release = false; _waitSeconds = Seconds(ReleaseWaitFrames); return _mouse;
        }
        if (_waitDetail != null)
        {
            if (detail.Contains(_waitDetail, StringComparison.OrdinalIgnoreCase))
            {
                Console.WriteLine($"UI PASS: waited {_waitDetail}");
                _waitDetail = null;
            }
            else if ((_waitDetailSeconds -= seconds) <= 0)
                throw new InvalidOperationException($"UI 조건 대기 시간 초과: 예상={_waitDetail}, 실제={detail}");
            return _mouse;
        }
        // 남은 대기 시간을 이번 갱신 시간만큼 줄이고, 아직 남았으면 입력을 그대로 둔다.
        if (_waitSeconds > WaitTolerance)
        {
            _waitSeconds -= seconds;
            return _mouse;
        }
        if (_commands.Count == 0 || CapturePath != null) return _mouse;
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
            case "wait":
                // 값은 1/60초 단위다 (wait 60 = 1초). 화면 루프 속도와 무관하게 같은 시간만큼 기다린다.
                _waitSeconds = Seconds(int.Parse(value)); break;
            case "wait-detail":
                // 생산·이동의 실제 완료를 기다려 화면 FPS에 따라 검사 시각이 달라지는 문제를 피한다.
                _waitDetail = value; _waitDetailSeconds = Seconds(DetailWaitFrameLimit); break;
            case "key":
                Keyboard = new KeyboardState(value.Split('+').Select(k => Enum.Parse<Keys>(k, true)).ToArray());
                _releaseKey = true; break;
            case "down":
                // 왼쪽 버튼을 누른 채로 둔다 (떼려면 up). 버튼을 오래 누르는 시험에 쓴다.
                string[] downPoint = value.Split(',');
                _mouse = MakeMouse(int.Parse(downPoint[0]), int.Parse(downPoint[1]), ButtonState.Pressed); break;
            case "held-move":
                // 왼쪽 버튼을 누른 채 커서만 옮긴다 (버튼 밖으로 나갔다 돌아오는 시험).
                string[] heldPoint = value.Split(',');
                _mouse = MakeMouse(int.Parse(heldPoint[0]), int.Parse(heldPoint[1]), ButtonState.Pressed); break;
            case "up":
                // 누르고 있던 왼쪽 버튼을 지금 커서 위치에서 뗀다.
                _mouse = MakeMouse(_mouse.X, _mouse.Y, ButtonState.Released); break;
            case "middle-click":
                string[] middlePoint = value.Split(',');
                _mouse = new MouseState(int.Parse(middlePoint[0]), int.Parse(middlePoint[1]), _mouse.ScrollWheelValue,
                    ButtonState.Released, ButtonState.Pressed, ButtonState.Released, ButtonState.Released, ButtonState.Released);
                _release = true; break;
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
