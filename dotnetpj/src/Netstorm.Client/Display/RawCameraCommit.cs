using Netstorm.Client;
using Netstorm.Core.Original;

namespace Netstorm.Client.Display;

// 497960의 현재 view/snapshot·dirty·배율 적용을 외부 화면/커서 장치와 구별한다.
public readonly record struct RawCameraCommitViewport(int X, int Y, int Width, int Height);

/// <summary>같은 Renderer·snapshot과 변경 표를 값 복사 없이 빌린다.</summary>
public sealed class RawCameraCommitState
{
    /// <summary>Renderer 앞64BYTE의 현재 view다.</summary>
    public RawCameraViewState Renderer { get; }
    /// <summary>전역의 별도 현재64BYTE다.</summary>
    public RawCameraViewState Snapshot { get; }
    /// <summary>같은 Renderer의 한계/목표/배율이다.</summary>
    public RawCameraTargetState Target { get; }
    /// <summary>Renderer의 현재 전체 화면 출력 요청이다. 노출 띠의 부분 재합성과 구별한다.</summary>
    public UIntBox Redraw { get; }
    /// <summary>별도 현재 전체 다시 그리기다.</summary>
    public UIntBox WorldRedraw { get; }
    /// <summary>개수와 같은 현재 표이며 장치 효과 뒤에도 다시 읽는다.</summary>
    public List<DirtyRegion> Dirty { get; }

    /// <summary>같은 주소를 빌려 Commit 본체의 현재 저장소로 둔다.</summary>
    /// <param name="renderer">Renderer 앞64BYTE의 현재 view다.</param>
    /// <param name="snapshot">전역의 별도 현재64BYTE다.</param>
    /// <param name="target">같은 Renderer의 한계/목표/배율이다.</param>
    /// <param name="redraw">Renderer의 현재 전체 화면 출력 요청이다.</param>
    /// <param name="worldRedraw">별도 현재 전체 다시 그리기다.</param>
    /// <param name="dirty">개수와 같은 현재 표다.</param>
    public RawCameraCommitState(RawCameraViewState renderer, RawCameraViewState snapshot, RawCameraTargetState target, UIntBox redraw, UIntBox worldRedraw, List<DirtyRegion> dirty)
    {
        Renderer = renderer ?? throw new ArgumentNullException(nameof(renderer));
        Snapshot = snapshot ?? throw new ArgumentNullException(nameof(snapshot));
        Target = target ?? throw new ArgumentNullException(nameof(target));
        Redraw = redraw ?? throw new ArgumentNullException(nameof(redraw));
        WorldRedraw = worldRedraw ?? throw new ArgumentNullException(nameof(worldRedraw));
        Dirty = dirty ?? throw new ArgumentNullException(nameof(dirty));
    }
}

/// <summary>callback 뒤 원문이 재조회하는 장치/viewport 전역의 현재 주소다.</summary>
public sealed class RawCameraCommitGlobals
{
    /// <summary>시차/버퍼 모드 비트다.</summary>
    public uint ScreenFlags { get; }
    /// <summary>현재 signed 시차 분모다.</summary>
    public int Denominator { get; }
    /// <summary>현재 논리 폭이다.</summary>
    public int Width { get; }
    /// <summary>현재 논리 높이다.</summary>
    public int Height { get; }
    /// <summary>네 viewport DWORD의 현재 저장소다.</summary>
    public RawCameraCommitViewport Viewport { get; }
    /// <summary>별도 전체 viewport 사각형이다.</summary>
    public ScreenRect FullViewport { get; }
    /// <summary>현재 실제 마우스 정수 좌표다.</summary>
    public ScreenPoint Mouse { get; }

    /// <summary>Commit 본체가 읽는 현재 전역 값을 묶는다.</summary>
    /// <param name="screenFlags">시차/버퍼 모드 비트다.</param>
    /// <param name="denominator">현재 signed 시차 분모다.</param>
    /// <param name="width">현재 논리 폭이다.</param>
    /// <param name="height">현재 논리 높이다.</param>
    /// <param name="viewport">네 viewport DWORD의 현재 저장소다.</param>
    /// <param name="fullViewport">별도 전체 viewport 사각형이다.</param>
    /// <param name="mouse">현재 실제 마우스 정수 좌표다.</param>
    public RawCameraCommitGlobals(uint screenFlags, int denominator, int width, int height, RawCameraCommitViewport viewport, ScreenRect fullViewport, ScreenPoint mouse)
    {
        ScreenFlags = screenFlags; Denominator = denominator; Width = width; Height = height; Viewport = viewport; FullViewport = fullViewport; Mouse = mouse;
    }
}

/// <summary>복원된 커서/Gump/dirty 몸체와 화면 이동을 실제 호스트가 연결한다. 선택된 경계만 요구한다.</summary>
public sealed class RawCameraCommitHooks
{
    /// <summary>현재 잠금/커서 효과다.</summary>
    public Action<uint>? LockCursor { get; set; }
    /// <summary>잠금 해제다.</summary>
    public Action? UnlockCursor { get; set; }
    /// <summary>현재 Gump 다시 그리기다.</summary>
    public Action? RedrawRoots { get; set; }
    /// <summary>x/y/폭/높이/dx/dy 화면 이동이다.</summary>
    public Action<int, int, int, int, int, int>? Scroll { get; set; }
    /// <summary>x/y/폭/높이/dx/dy 배경 이동이다.</summary>
    public Action<int, int, int, int, int, int>? ScrollBackground { get; set; }
    /// <summary>기존 변경 영역 추가 몸체다.</summary>
    public Action<ScreenRect, uint>? AddDirty { get; set; }
    /// <summary>배경 변경 영역 추가 몸체다.</summary>
    public Action<ScreenRect>? AddBackgroundDirty { get; set; }
    /// <summary>현재 커서 장치 좌표 갱신이다.</summary>
    public Action<ScreenPoint>? UpdateMouse { get; set; }
    /// <summary>두 원문 진단을 같은 호출자에게 전달한다.</summary>
    public Action<string, uint>? Assertion { get; set; }
}

/// <summary>497960 본체이며 Present 제한과 실제 Frame 호출 위치는 호출자가 소유한다.</summary>
public static class RawCameraCommit
{
    /// <summary>도달한 실제 외부 경계만 요구하며 미연결은 그 위치에서 실패한다.</summary>
    /// <typeparam name="T">연결된 장치의 형태다.</typeparam>
    /// <param name="callback">요구할 실제 장치다.</param>
    /// <returns>같은 장치다.</returns>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("카메라적용현재장치미연결");

    /// <summary>원문 DWORD 감김의 ADD 하위32비트다.</summary>
    /// <param name="a">첫 피연산자다.</param>
    /// <param name="b">더할 값이다.</param>
    /// <returns>감긴 합이다.</returns>
    private static int Add(int a, int b) => unchecked((int)((uint)a + (uint)b));

    /// <summary>원문 DWORD 감김의 SUB 하위32비트다.</summary>
    /// <param name="a">첫 피연산자다.</param>
    /// <param name="b">뺄 값이다.</param>
    /// <returns>감긴 차다.</returns>
    private static int Sub(int a, int b) => unchecked((int)((uint)a - (uint)b));

    /// <summary>원문 IDIV의 두 오류를 미정의 동작으로 실행하지 않고 선택된 위치에서 거부한다.</summary>
    /// <param name="value">나눌 값이다.</param>
    /// <param name="denominator">현재 signed 시차 분모다.</param>
    /// <returns>0 방향 절삭 몫이다.</returns>
    private static int Divide(int value, int denominator)
    {
        if (denominator == 0 || (value == int.MinValue && denominator == -1)) throw new DivideByZeroException("카메라시차원문IDIV오류");
        return value / denominator;
    }

    /// <summary>IMUL 하위32비트 뒤 signed SAR16이다.</summary>
    /// <param name="baseValue">기본 칸 크기다.</param>
    /// <param name="zoom">현재 Q16 배율이다.</param>
    /// <returns>적용된 칸 크기다.</returns>
    private static int Scale(int baseValue, uint zoom) => unchecked((int)((uint)baseValue * zoom)) >> 16;

    /// <summary>원문 FILD32/FSTP32 저장이다.</summary>
    /// <param name="value">적을 정수다.</param>
    /// <returns>단정도 비트다.</returns>
    private static uint FloatBits(int value) => BitConverter.SingleToUInt32Bits(value);

    /// <summary>원문의 두 노출 띠를 계산하며 음수 길이일 때 끝 좌표 자체를 NEG하는 순서를 보존한다.</summary>
    /// <param name="viewport">네 viewport DWORD의 현재 저장소다.</param>
    /// <param name="dx">적용된 X 이동이다.</param>
    /// <param name="dy">적용된 Y 이동이다.</param>
    /// <param name="emit">계산된 노출 띠를 받는 장치다.</param>
    private static void Exposed(RawCameraCommitViewport viewport, int dx, int dy, Action<ScreenRect> emit)
    {
        int left = dx > 0 ? Add(Sub(viewport.X, dx), viewport.Width) : viewport.X;
        int right = dx > 0 ? Add(viewport.X, viewport.Width) : Sub(viewport.X, dx);
        int top = dy > 0 ? Add(Sub(viewport.Y, dy), viewport.Height) : viewport.Y;
        int bottom = dy > 0 ? Add(viewport.Y, viewport.Height) : Add(viewport.Y, dy);
        int horizontal = Sub(right, left), vertical = Sub(bottom, top);
        if (horizontal != 0) { if (horizontal < 0) right = Sub(0, right); emit(new ScreenRect(left, viewport.Y, right, Add(viewport.Y, viewport.Height))); }
        if (vertical != 0) { if (vertical < 0) bottom = Sub(0, bottom); emit(new ScreenRect(viewport.X, top, Add(viewport.X, viewport.Width), bottom)); }
    }

    /// <summary>zoom 분기와 일반 이동 분기를 원문과 같이 분리하고 모든 외부 효과 뒤 현재 저장소를 다시 읽는다.</summary>
    /// <param name="state">같은 Renderer·snapshot과 변경 표다.</param>
    /// <param name="globals">장치/viewport 전역의 현재 주소다.</param>
    /// <param name="hooks">복원된 커서/Gump/dirty 몸체와 화면 이동이다.</param>
    /// <remarks>2026-11-26 1.047에서 DI-4a 카메라 Commit용으로 추가했다. 근거: cpppj RawCameraCommit.cpp의 CommitCameraTarget(497960)이다. Present 제한과 실제 Frame 호출 위치는 호출자가 소유한다.</remarks>
    public static void CommitCameraTarget(RawCameraCommitState state, RawCameraCommitGlobals globals, RawCameraCommitHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(hooks);
        RawCameraViewState view = state.Renderer, snapshot = state.Snapshot; RawCameraTargetState target = state.Target;
        if (target.Zoom != view.Zoom)
        {
            Need(hooks.RedrawRoots)(); uint zoom = target.Zoom;
            view.CellPixels[0] = Scale(view.BaseCellPixels[0], zoom);
            view.ZoomFloatBits = BitConverter.SingleToUInt32Bits(unchecked((float)unchecked((int)zoom)) * (1f / 65536f));
            view.CellFloatBits[0] = FloatBits(view.CellPixels[0]); view.Zoom = zoom; view.CellPixels[1] = Scale(view.BaseCellPixels[1], zoom); view.CellFloatBits[1] = FloatBits(view.CellPixels[1]);
            snapshot.ZoomFloatBits = BitConverter.SingleToUInt32Bits(unchecked((float)unchecked((int)zoom)) * (1f / 65536f));
            snapshot.CellPixels[0] = Scale(snapshot.BaseCellPixels[0], zoom); snapshot.CellPixels[1] = Scale(snapshot.BaseCellPixels[1], zoom); snapshot.Zoom = zoom;
            snapshot.CellFloatBits[0] = FloatBits(snapshot.CellPixels[0]); snapshot.CellFloatBits[1] = FloatBits(snapshot.CellPixels[1]);
            RawCameraView.SetPixels(view, target.Pixels[0], target.Pixels[1], hooks.Assertion); RawCameraView.SetPixels(snapshot, target.Pixels[0], target.Pixels[1], hooks.Assertion);
            Need(hooks.RedrawRoots)(); state.WorldRedraw.Value = 1; return;
        }
        if (target.Pixels[0] != view.Pixels[0] || target.Pixels[1] != view.Pixels[1]) { Need(hooks.LockCursor)(0); Need(hooks.RedrawRoots)(); }
        int dy = Sub(target.Pixels[1], view.Pixels[1]), dx = Sub(target.Pixels[0], view.Pixels[0]);
        Need(hooks.Scroll)(globals.Viewport.X, globals.Viewport.Y, globals.Viewport.Width, globals.Viewport.Height, dx, dy);
        if (globals.Viewport.X > 0) Need(hooks.Scroll)(0, 0, globals.Viewport.X, globals.Height, 0, 0);
        if (globals.Viewport.Height < globals.Height) Need(hooks.Scroll)(globals.Viewport.X, globals.Viewport.Height, Sub(globals.Width, globals.Viewport.X), Sub(globals.Height, globals.Viewport.Height), 0, 0);
        if ((globals.ScreenFlags & 16u) != 0)
        {
            if ((globals.ScreenFlags & 2u) == 0)
            {
                int oldX = Divide(view.Pixels[0], globals.Denominator), newX = Divide(target.Pixels[0], globals.Denominator);
                int oldY = Divide(view.Pixels[1], globals.Denominator), newY = Divide(target.Pixels[1], globals.Denominator); int sx = Sub(newX, oldX), sy = Sub(newY, oldY);
                Need(hooks.ScrollBackground)(globals.Viewport.X, globals.Viewport.Y, globals.Viewport.Width, globals.Viewport.Height, sx, sy);
                Exposed(globals.Viewport, sx, sy, rect => Need(hooks.AddBackgroundDirty)(rect));
            }
            else if (dx != 0 || dy != 0) Need(hooks.AddDirty)(globals.FullViewport, 0);
        }
        RawCameraView.SetPixels(view, target.Pixels[0], target.Pixels[1], hooks.Assertion); RawCameraView.SetPixels(snapshot, target.Pixels[0], target.Pixels[1], hooks.Assertion);
        if (dx == 0 && dy == 0) return;
        Need(hooks.UpdateMouse)(globals.Mouse);
        // 마우스 효과 뒤 현재 표와 매 반복의 현재 개수를 읽는다.
        for (int index = 0; index < state.Dirty.Count; index++)
        {
            DirtyRegion entry = state.Dirty[index]; if ((entry.Flags & 1u) != 0) continue; ScreenRect old = entry.Rect;
            var shifted = new ScreenRect(Sub(old.Left, dx), Sub(old.Top, dy), Sub(old.Right, dx), Sub(old.Bottom, dy));
            if ((entry.Flags & 2u) != 0) state.Dirty[index] = entry with { Rect = new ScreenRect(Math.Min(old.Left, shifted.Left), Math.Min(old.Top, shifted.Top), Math.Max(old.Right, shifted.Right), Math.Max(old.Bottom, shifted.Bottom)) };
            else state.Dirty[index] = entry with { Rect = shifted };
        }
        Exposed(globals.Viewport, dx, dy, rect => Need(hooks.AddDirty)(rect, 0));
        Need(hooks.UnlockCursor)(); Need(hooks.RedrawRoots)(); state.Redraw.Value = 1;
    }
}
