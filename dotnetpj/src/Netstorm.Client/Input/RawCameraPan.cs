using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Input;

// Alt/가운데 이동과 두 입력 큐 경로 선택을 분리한다.
public sealed class CameraPanGlobals
{
    /// <summary>마우스가 창 밖에 있어도 스크롤하는지다.</summary>
    public int ScrollWhileMouseOut { get; set; }
    /// <summary>대화창·팝업이 열려 있으면 0이 아니다.</summary>
    public int Dialog { get; set; }
    /// <summary>게임 창이 활성 상태이면 0이 아니다.</summary>
    public int Active { get; set; }
    /// <summary>현재 Win32 client 폭이다.</summary>
    public int ClientWidth { get; set; }
    /// <summary>현재 Win32 client 높이다.</summary>
    public int ClientHeight { get; set; }
    /// <summary>현재 지도 원점 X다.</summary>
    public int ViewX { get; set; }
    /// <summary>현재 지도 원점 Y다.</summary>
    public int ViewY { get; set; }
    /// <summary>현재 지도 폭이다.</summary>
    public int ViewWidth { get; set; }
    /// <summary>현재 지도 높이다.</summary>
    public int ViewHeight { get; set; }
}

/// <summary>현재 UI·창 입력을 소비하며 카메라 현재값은 직접 쓰지 않는다.</summary>
public sealed class CameraPanHooks
{
    /// <summary>가운데·Shift를 각각 실제 도달 위치에서 조회한다.</summary>
    public Func<uint, InputEvent>? Poll { get; set; }
    /// <summary>폴링·팝업 callback 뒤 현재 전역을 다시 읽는다.</summary>
    public Func<CameraPanGlobals>? Globals { get; set; }
    /// <summary>실제 팝업 종료 요청이다.</summary>
    public Action? ClosePopups { get; set; }
    /// <summary>현재 view 기준 목표 쓰기이며 즉시 Scroll과 다르다.</summary>
    public Action<int, int>? Request { get; set; }
}

/// <summary>입력·GUI 경계와 독립인 원문 pan 본체다.</summary>
public static class RawCameraPan
{
    /// <summary>Shift 키 상태 조회 의사 코드다. 가상 키 0x10(Shift)을 위로 민 값이다.</summary>
    public const uint ShiftStatePollCode = 0x00100000;

    /// <summary>도달한 장치만 요구하며 미연결은 그 위치에서 실패한다.</summary>
    /// <typeparam name="T">연결된 장치의 형태다.</typeparam>
    /// <param name="callback">요구할 실제 장치다.</param>
    /// <returns>같은 장치다.</returns>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("카메라pan현재장치미연결");

    /// <summary>ADD의 감긴 결과다.</summary>
    /// <param name="first">첫 피연산자다.</param>
    /// <param name="second">더할 값이다.</param>
    /// <returns>감긴 합이다.</returns>
    private static int Add(int first, int second) => unchecked((int)(unchecked((uint)first) + unchecked((uint)second)));

    /// <summary>SUB의 감긴 결과다.</summary>
    /// <param name="first">첫 피연산자다.</param>
    /// <param name="second">뺄 값이다.</param>
    /// <returns>감긴 차다.</returns>
    private static int Sub(int first, int second) => unchecked((int)(unchecked((uint)first) - unchecked((uint)second)));

    /// <summary>원문 ABS다. 최소값의 결과는 여전히 signed 음수다.</summary>
    /// <param name="value">절댓값을 잴 값이다.</param>
    /// <returns>절댓값이다.</returns>
    private static int Abs(int value) => value < 0 ? Sub(0, value) : value;

    /// <summary>축의 원문 signed 크기·비정상 FTOL 결과를 보존한다.</summary>
    /// <param name="pointer">현재 포인터 좌표다.</param>
    /// <param name="origin">지도 원점이다.</param>
    /// <param name="extent">지도 크기다.</param>
    /// <param name="client">Win32 client 크기다.</param>
    /// <returns>축 이동량이다.</returns>
    private static int Axis(int pointer, int origin, int extent, int client)
    {
        int center = Add(origin, extent / 2); int bounded = Math.Min(Math.Max(origin, pointer), Add(origin, extent));
        double ratio = (double)client / (double)extent;
        return RawCameraView.FtolLow(ratio * (double)Sub(bounded, center));
    }

    /// <summary>진입 관문을 지나면 팝업을 정리하고 가운데 이동량을 목표 쓰기로 보낸다. 이동 전용 큐 경로이면 참이다.</summary>
    /// <param name="pointer">소비할 포인터 사건이다.</param>
    /// <param name="hooks">폴링·전역·팝업·목표 장치다.</param>
    /// <returns>원문 이동 전용 큐 경로이면 참이다. 좌표 -1·비활성에서도 팝업 정리 뒤 참이며 일반 입력을 재분배하지 않는다.</returns>
    /// <remarks>2026-11-26 1.050에서 DI-4d Pan용으로 추가했다. 근거: cpppj RawCameraPan.cpp의 ProcessCameraPan(4d6b54)이다. 프레임 delta나 임시 속도 대신 원문 signed 좌표·FTOL·Shift 축 선택을 사용한다.</remarks>
    public static bool ProcessCameraPan(InputEvent pointer, CameraPanHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        InputEvent middle = Need(hooks.Poll)(InputCode.MiddleButton); CameraPanGlobals gate = Need(hooks.Globals)();
        if ((!Convert.ToBoolean(gate.ScrollWhileMouseOut) && (pointer.Code & InputCode.Outside) != 0) ||
            ((pointer.Code & InputCode.Release) != 0 && (middle.Code & InputCode.Release) != 0) || gate.Dialog != 0) return false;
        Need(hooks.ClosePopups)(); CameraPanGlobals current = Need(hooks.Globals)();
        if (pointer.X == -1 || pointer.Y == -1 || current.Active == 0) return true;
        int x = Axis(pointer.X, current.ViewX, current.ViewWidth, current.ClientWidth), y = Axis(pointer.Y, current.ViewY, current.ViewHeight, current.ClientHeight);
        if ((Need(hooks.Poll)(ShiftStatePollCode).Code & InputCode.Release) == 0) { if (Abs(x) > Abs(y)) y = 0; else x = 0; }
        Need(hooks.Request)(x / 8, y / 8); return true;
    }

    /// <summary>목표는 현재 Renderer의 1c/20을 사용하며 이전 목표 50/54에 이동량을 누적하지 않는다.</summary>
    /// <param name="view">현재 Renderer 앞64BYTE다.</param>
    /// <param name="target">같은 Renderer의 목표다.</param>
    /// <param name="height">현재 논리 화면 높이다.</param>
    /// <param name="dx">X 이동량이다.</param>
    /// <param name="dy">Y 이동량이다.</param>
    /// <param name="metaRebuild">증가시킬 재구성 횟수다.</param>
    /// <remarks>2026-11-26 1.050에서 DI-4d Pan용으로 추가했다. 근거: cpppj RawEdgeScroll.cpp의 RequestCameraDelta(497e00)다. 절반을 더했다 빼므로 결과는 현재 view와 이동량의 합이다.</remarks>
    public static void RequestCameraDelta(RawCameraViewState view, RawCameraTargetState target, int height, int dx, int dy, UIntBox metaRebuild)
    {
        ArgumentNullException.ThrowIfNull(view); ArgumentNullException.ThrowIfNull(target); ArgumentNullException.ThrowIfNull(metaRebuild);
        int half = height / 2; target.Pixels[0] = Add(view.Pixels[0], dx);
        target.Pixels[1] = Add(Add(Add(half, view.Pixels[1]), dy), unchecked((int)(0u - unchecked((uint)half)))); metaRebuild.Value++;
    }
}
