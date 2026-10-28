using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Input;

namespace Netstorm.Client.Automation;

/// <summary>
/// 자동 UI 스크립트의 한 단계. 줄 형식은 "프레임⇥동작⇥인자"(탭 구분)이며 cpppj <c>--ui-script</c>와 같다.
/// 근거: cpppj/src/app/main.cpp 의 UiStep. (2026-10-28 R1-2에서 이식)
/// </summary>
/// <param name="Frame">이 단계를 실행할 클라이언트 프레임 번호(1부터).</param>
/// <param name="Operation">동작 이름(click, esc, key, snapshot 등).</param>
/// <param name="Argument">동작의 인자. 컨트롤 라벨, 좌표, 키 이름, 저장 경로 등이며 없으면 빈 문자열이다.</param>
public sealed record UiStep(ulong Frame, string Operation, string Argument);

/// <summary>
/// 컨트롤 라벨을 화면 좌표로 바꾸는 조회기. 실제 Gump 트리(R3)가 구현한다. 라벨 클릭을 쓰는 스크립트가 같은 문법으로 두 구현에서 돌게 하는 접점이다.
/// </summary>
public interface IUiControlLocator
{
    /// <summary>라벨에 해당하는 컨트롤의 클릭 좌표를 돌려준다. 현재 화면에 없으면 null이다.</summary>
    /// <param name="label">컨트롤 라벨.</param>
    ScreenPoint? ControlPoint(string label);
}

/// <summary>
/// UI 스크립트 파서. 프레임 번호는 양수이고 오름차순이어야 하며, 순서가 뒤바뀐 입력은 조용히 누락하지 않고 거부한다.
/// 근거: cpppj/src/app/main.cpp 의 ReadUiSteps. (2026-10-28 R1-2에서 이식)
/// </summary>
public static class UiScript
{
    /// <summary>
    /// 스크립트 줄들을 읽는다. 빈 줄과 '#'로 시작하는 줄은 건너뛰고, 줄 끝의 CR은 지운다. 첫 탭 앞이 프레임, 둘째 탭까지가 동작, 그 뒤 전부가 인자다.
    /// 탭이 없거나 프레임이 0이거나 앞 단계보다 작으면 <see cref="ArgumentException"/>이다.
    /// </summary>
    /// <param name="lines">스크립트의 줄들.</param>
    public static List<UiStep> Parse(IEnumerable<string> lines)
    {
        var steps = new List<UiStep>();
        // 줄마다 한 단계를 읽는다.
        foreach (string raw in lines)
        {
            string line = raw.EndsWith('\r') ? raw[..^1] : raw;
            if (line.Length == 0 || line[0] == '#') continue;
            int first = line.IndexOf('\t');
            if (first < 0) throw new ArgumentException("UI script needs tab-separated fields");
            int second = line.IndexOf('\t', first + 1);
            string frameText = line[..first];
            if (!ulong.TryParse(frameText, NumberStyles.None, CultureInfo.InvariantCulture, out ulong frame)) throw new ArgumentException($"UI frame is not a number: {frameText}");
            string operation = second < 0 ? line[(first + 1)..] : line[(first + 1)..second];
            string argument = second < 0 ? string.Empty : line[(second + 1)..];
            if (frame == 0 || (steps.Count > 0 && frame < steps[^1].Frame)) throw new ArgumentException("UI frames must be positive and sorted");
            steps.Add(new UiStep(frame, operation, argument));
        }
        return steps;
    }

    /// <summary>UTF-8 스크립트 파일을 읽는다.</summary>
    /// <param name="path">스크립트 경로.</param>
    public static List<UiStep> ParseFile(string path) => Parse(File.ReadLines(path));
}

/// <summary>
/// UI 스크립트 실행기. 클라이언트 프레임 루프가 프레임마다 <see cref="BeforeInput"/>(입력 주입)과 <see cref="AfterFrame"/>(그리기 뒤 캡처)을 부른다.
/// 모든 입력은 실제 <see cref="InputQueue"/>로만 보내며 게임 상태를 직접 바꾸지 않는다. 같은 스크립트를 cpppj와 dotnetpj가 각자 돌려 캡처를 비교한다.
/// 현재 지원하는 동작: esc, key, keyup, pointer, click/down/up/right/click-if/up-if/outside(좌표 "screen:x,y" 또는 라벨 조회기), snapshot.
/// report·refresh는 cpppj 내부 상태 보고라 dotnetpj에서는 관측 없이 건너뛰며 <see cref="SkippedObservations"/>에 남긴다.
/// 그 밖의 동작(kernel-tell, require, when, snapshot-output, test-* 등)은 해당 계층을 옮기기 전까지 <see cref="NotSupportedException"/>이다.
/// 근거: cpppj/src/app/main.cpp 의 beforeInput/afterFrame. (2026-10-28 R1-2에서 이식)
/// </summary>
public sealed class UiScriptRunner
{
    /// <summary>키 이름 → Win32 가상 키 번호. 검사도 실제 가상 키 번호를 같은 큐에 넣는다.</summary>
    private static readonly Dictionary<string, uint> VirtualKeys = new()
    {
        ["left"] = 0x25, ["up"] = 0x26, ["right"] = 0x27, ["down"] = 0x28, ["home"] = 0x73, ["priest"] = 0x74, ["f9"] = 0x78,
    };

    /// <summary>실행할 단계들(프레임 오름차순).</summary>
    private readonly IReadOnlyList<UiStep> _steps;

    /// <summary>입력을 넣을 실제 입력 큐.</summary>
    private readonly InputQueue _input;

    /// <summary>화면 크기 검사와 기본 캡처에 쓰는 논리 화면.</summary>
    private readonly Screen _screen;

    /// <summary>라벨 클릭의 좌표 조회기. Gump가 없으면 null이다.</summary>
    private readonly IUiControlLocator? _controls;

    /// <summary>snapshot 동작의 저장 방식. 기본은 <see cref="ScreenCapture.WriteBitmap"/>이다.</summary>
    private readonly Action<string> _snapshot;

    /// <summary>다음에 실행할 단계의 위치. AfterFrame에서만 앞으로 간다.</summary>
    private int _next;

    /// <summary>실행기를 만든다.</summary>
    /// <param name="steps"><see cref="UiScript.Parse"/>의 결과.</param>
    /// <param name="input">입력 큐.</param>
    /// <param name="screen">논리 화면.</param>
    /// <param name="controls">컨트롤 라벨 조회기(없으면 라벨 클릭은 실패).</param>
    /// <param name="snapshot">snapshot 저장 동작(없으면 BMP 캡처).</param>
    public UiScriptRunner(IReadOnlyList<UiStep> steps, InputQueue input, Screen screen, IUiControlLocator? controls = null, Action<string>? snapshot = null)
    {
        _steps = steps;
        _input = input;
        _screen = screen;
        _controls = controls;
        _snapshot = snapshot ?? (path => ScreenCapture.WriteBitmap(path, screen));
    }

    /// <summary>
    /// 검사 폴링이 쓰는 마우스 상태. pointer·클릭·outside가 바꾸며 입력 큐에는 넣지 않는다.
    /// 호스트의 폴링 구현이 실제 마우스 대신 이 값을 읽는다. 값이 없으면 null이다.
    /// </summary>
    public InputEvent? ScriptPointer { get; private set; }

    /// <summary>건너뛴 관측 단계("프레임:동작" 형식). 호출자가 보고서에 남긴다.</summary>
    public List<string> SkippedObservations { get; } = [];

    /// <summary>모든 단계를 실행했는가.</summary>
    public bool Finished => _next >= _steps.Count;

    /// <summary>
    /// 이번 프레임의 입력 단계를 실제 입력 큐에 넣는다. 현재 위치부터 프레임 번호가 같은 단계만 처리하며, 앞선 프레임에 못 한 단계가 남아 있으면
    /// 뒤 단계도 막힌다(cpppj와 같은 동작이며 마지막에 <see cref="RequireFinished"/>가 알린다). 위치는 <see cref="AfterFrame"/>에서만 전진한다.
    /// </summary>
    /// <param name="frame">현재 클라이언트 프레임 번호.</param>
    public void BeforeInput(ulong frame)
    {
        // 위치부터 같은 프레임의 단계를 순서대로 입력한다.
        for (int i = _next; i < _steps.Count && _steps[i].Frame == frame; i++)
        {
            UiStep step = _steps[i];
            if (step.Operation is "snapshot" or "report" or "refresh") continue; // 그리기 뒤(AfterFrame)에 처리하는 관측이다.
            Input(step);
        }
    }

    /// <summary>
    /// 그리기를 마친 뒤 같은 프레임의 캡처·관측 단계를 처리하고 위치를 전진한다. snapshot은 저장하고, report·refresh는 건너뛰며 기록한다.
    /// </summary>
    /// <param name="frame">현재 클라이언트 프레임 번호.</param>
    public void AfterFrame(ulong frame)
    {
        // 위치부터 같은 프레임의 단계를 처리한다.
        while (_next < _steps.Count && _steps[_next].Frame == frame)
        {
            UiStep step = _steps[_next++];
            if (step.Operation == "snapshot") _snapshot(step.Argument);
            else if (step.Operation is "report" or "refresh") SkippedObservations.Add($"{step.Frame}:{step.Operation}");
        }
    }

    /// <summary>모든 단계를 실행하지 못했으면 예외를 던진다. 프레임 제한이나 오류로 남은 단계를 조용히 지나치지 않기 위한 마무리 검사다.</summary>
    public void RequireFinished()
    {
        if (!Finished) throw new InvalidOperationException("UI script ended before all steps ran");
    }

    /// <summary>단계 하나의 입력 동작을 실행한다. 지원하지 않는 동작은 <see cref="NotSupportedException"/>이다.</summary>
    private void Input(UiStep step)
    {
        switch (step.Operation)
        {
            case "esc":
                _input.Push(0x1b, 0, 0);
                return;
            case "key":
            case "keyup":
                Key(step);
                return;
            case "pointer":
                ScriptPointer = ParsePointer(step.Argument);
                return;
            case "click" or "click-if" or "down" or "up" or "up-if" or "right" or "outside":
                Pointer(step);
                return;
            default:
                throw new NotSupportedException($"UI 동작 미이식: {step.Operation} (해당 계층을 옮긴 뒤 지원한다)");
        }
    }

    /// <summary>key·keyup 단계: 한 글자는 WM_CHAR처럼 아래 코드로, 이름은 실제 가상 키 번호를 위로 밀어 넣는다. shift+ 접두는 Shift 비트만 더한다.</summary>
    private void Key(UiStep step)
    {
        uint release = step.Operation == "keyup" ? InputCode.Release : 0;
        if (step.Argument.Length == 1 && step.Argument[0] >= 0x20 && step.Argument[0] < 0x7f)
        {
            _input.Push(step.Argument[0] | release, 0, 0); // 가상 키 번호로 바꾸지 않는다.
            return;
        }
        bool shifted = step.Argument.StartsWith("shift+", StringComparison.Ordinal);
        string name = shifted ? step.Argument[6..] : step.Argument;
        if (!VirtualKeys.TryGetValue(name, out uint virtualKey)) throw new ArgumentException("Unknown UI key: " + step.Argument);
        _input.Push((virtualKey << InputCode.VirtualKeyShift) | (shifted ? InputCode.Shift : 0) | release, 0, 0);
    }

    /// <summary>마우스 단계: 좌표는 "screen:x,y" 또는 컨트롤 라벨이다. 일반 입력 큐에만 보내고 폴링 상태를 같은 위치로 맞춘다.</summary>
    private void Pointer(UiStep step)
    {
        string operation = step.Operation;
        bool optional = operation is "click-if" or "up-if";
        if (operation == "click-if") operation = "click";
        else if (operation == "up-if") operation = "up";
        ScreenPoint point = default;
        if (operation != "outside")
        {
            if (step.Argument.StartsWith("screen:", StringComparison.Ordinal))
            {
                string[] parts = step.Argument[7..].Split(',');
                if (parts.Length != 2 || !int.TryParse(parts[0], NumberStyles.Integer, CultureInfo.InvariantCulture, out int x)
                    || !int.TryParse(parts[1], NumberStyles.Integer, CultureInfo.InvariantCulture, out int y)
                    || x < 0 || y < 0 || x >= _screen.Width || y >= _screen.Height)
                    throw new ArgumentException("UI 화면 좌표는 현재 화면 안의 screen:x,y 여야 한다");
                point = new ScreenPoint(x, y);
            }
            else
            {
                ScreenPoint? found = _controls?.ControlPoint(step.Argument);
                if (found is null && optional) return; // 결과나 게임 상태를 바꾸지 않고 없는 조작만 생략한다.
                if (found is null) throw new InvalidOperationException("UI control not found: " + step.Argument);
                point = found.Value;
            }
        }
        // down 이후 프레임에서도 같은 위치를 폴링해 누름 캡처가 실제 바탕 화면 커서 때문에 취소되지 않게 한다.
        ScriptPointer = new InputEvent(InputCode.LeftButton | (operation is "down" or "outside" ? 0u : InputCode.Release), point.X, point.Y);
        if (operation is "click" or "down" or "outside") _input.Push(InputCode.LeftButton, point.X, point.Y);
        if (operation is "click" or "up") _input.Push(InputCode.LeftButton | InputCode.Release, point.X, point.Y);
        if (operation == "right")
        {
            _input.Push(InputCode.RightButton, point.X, point.Y);
            _input.Push(InputCode.RightButton | InputCode.Release, point.X, point.Y);
        }
    }

    /// <summary>pointer 인자 "code,x,y"를 읽는다. 코드는 C++ 스트림의 setbase(0)처럼 0x 16진수·0 시작 8진수·10진수를 받는다.</summary>
    private static InputEvent ParsePointer(string argument)
    {
        string[] parts = argument.Split(',');
        if (parts.Length != 3 || !TryParseBase0(parts[0].Trim(), out uint code)
            || !int.TryParse(parts[1].Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out int x)
            || !int.TryParse(parts[2].Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out int y))
            throw new ArgumentException("UI pointer requires code,x,y");
        return new InputEvent(code, x, y);
    }

    /// <summary>접두에 따라 진법을 고르는 부호 없는 정수 읽기(0x → 16진수, 0 → 8진수, 그 밖 → 10진수).</summary>
    private static bool TryParseBase0(string text, out uint value)
    {
        value = 0;
        if (text.Length == 0) return false;
        if (text.StartsWith("0x", StringComparison.OrdinalIgnoreCase)) return uint.TryParse(text[2..], NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out value);
        if (text.Length > 1 && text[0] == '0')
        {
            try { value = Convert.ToUInt32(text, 8); return true; }
            catch (Exception e) when (e is FormatException or OverflowException or ArgumentException) { return false; }
        }
        return uint.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out value);
    }
}
