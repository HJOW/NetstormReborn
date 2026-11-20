using Netstorm.Core.Original;

// 원문 선택과 강조의 실제 전역을 빌리며 서로 다른 DWORD를 합치지 않는다.
namespace Netstorm.Client.Input;

/// <summary>004d5bb0/004d5480이 공유하는 현재 강조·선택·마우스 필드다.</summary>
public sealed class UserHighlightState
{
    /// <summary>005cae90의 현재 전체 강조 모드다.</summary>
    public required UIntBox Mode { get; init; }
    /// <summary>005caea0의 현재 선택 SID다. 강조는 이 필드를 직접 쓰지 않는다.</summary>
    public required UIntBox SelectedSid { get; init; }
    /// <summary>005caea4의 마우스 대상 TYPE이며 ResetMouse의 유일한 쓰기다.</summary>
    public required UIntBox MouseType { get; init; }
    /// <summary>005cae94의 원시 signed 화면 X다.</summary>
    public required IntBox MouseX { get; init; }
    /// <summary>005cae98의 원시 signed 화면 Y다.</summary>
    public required IntBox MouseY { get; init; }
}

/// <summary>선택된 실제 커서·역투영·이동 강조를 호출 위치에서 공급한다.</summary>
public sealed class UserHighlightHooks
{
    /// <summary>004a4410의 실제 커서 종류 선택이다.</summary>
    public Action<uint>? SelectCursor { get; init; }
    /// <summary>004971e0의 현재 카메라 역투영이다. 반환은 float 두 개의 원문 비트다.</summary>
    public Func<int, int, (uint X, uint Y)>? Project { get; init; }
    /// <summary>004284a0의 선택 SID·좌표 비트·force0 이동 강조다.</summary>
    public Action<uint, uint, uint, uint>? Move { get; init; }
}

/// <summary>현재 공유 필드의 순서와 역투영 전 선택 SID 보관을 유지하는 원문 강조 본체다.</summary>
public sealed class RawUserHighlight
{
    /// <summary>호출자보다 짧게 빌리는 실제 장기 전역이다.</summary>
    private readonly UserHighlightState _state;
    /// <summary>도달한 효과만 요구하는 현재 외부 장치다.</summary>
    private readonly UserHighlightHooks _hooks;

    /// <summary>참조만 보관하며 어떤 상태나 커서도 초기화하지 않는다.</summary>
    public RawUserHighlight(UserHighlightState state, UserHighlightHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        _state = state; _hooks = hooks;
    }

    /// <summary>미연결 장치는 원문 선행 쓰기 뒤 선택된 위치에서 실패한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("실제사용자강조장치미연결");

    /// <summary>004d5480처럼 TYPE 한 DWORD만0으로 쓴다.</summary>
    public void ResetMouse() => _state.MouseType.Value = 0;

    /// <summary>같은 모드는 반환하고, 변경된 현재 모드에 해당하는 실제 효과만 호출한다.</summary>
    public void Highlight(uint mode)
    {
        if (mode == _state.Mode.Value) return;
        _state.Mode.Value = mode; ResetMouse();
        switch (_state.Mode.Value)
        {
            case 1: case 2: case 3: Need(_hooks.SelectCursor)(3); return;
            case 4: Need(_hooks.SelectCursor)(2); return;
            case 5:
                uint selected = _state.SelectedSid.Value;
                (uint x, uint y) = Need(_hooks.Project)(_state.MouseX.Value, _state.MouseY.Value);
                Need(_hooks.Move)(selected, x, y, 0); return;
            default: Need(_hooks.SelectCursor)(1); return;
        }
    }
}
