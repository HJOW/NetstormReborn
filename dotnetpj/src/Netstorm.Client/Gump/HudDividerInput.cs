using Netstorm.Client.Input;

namespace Netstorm.Client.Gump; // 같은 HUD 구분선의 실제 입력과 프레임 통지를 연결하는 영역이다.

/// <summary>현재 같은 기본/파생 객체와 메뉴 수·폴링·커서·기본 Help 장치를 빌린다.</summary>
public sealed class HudDividerInputHooks
{
    /// <summary>같은 실제 파생 구분선.</summary>
    public Func<uint, HudDividerNode?>? Divider { get; set; }
    /// <summary>공통 트리의 동일한 기본 객체.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>미처리 사건의 실제 기본 Help.</summary>
    public Func<uint, uint, uint, uint, uint>? BaseHandle { get; set; }
    /// <summary>현재 커서4/5의 실제 장치.</summary>
    public Action<uint>? Cursor { get; set; }
    /// <summary>00593024의 같은 MenuGumpState.Instances 현재값.</summary>
    public Func<uint>? MenuInstances { get; set; }
    /// <summary>00452e00의 실제 현재 좌클릭 폴링.</summary>
    public Func<uint, InputEvent>? Poll { get; set; }
    /// <summary>005427e4의 현재 hover 사건.</summary>
    public uint CursorEvent { get; set; } = 0x0200003b;
}

/// <summary>004c49f0 가상+28과004c4b00 가상+2c 전체 본체다. 별도 idle 등록은 없다.
/// 근거: cpppj/src/client/HudDividerInput.cpp/h. 같은 공통 좌표/캡처/이동을 사용한다.</summary>
public sealed class HudDividerInput
{
    /// <summary>같은 부모 체인의 현재 로컬 좌표 변환.</summary>
    private readonly GumpCollection _collection;
    /// <summary>첫 캡처를 유지하는 같은 실제 focus.</summary>
    private readonly GumpInput _input;
    /// <summary>anchor 보정·Push·파생 Attach의 같은 실제 Move.</summary>
    private readonly GumpTree _tree;
    /// <summary>현재 실제 노드와 외부 효과 장치.</summary>
    private readonly HudDividerInputHooks _hooks;
    /// <summary>같은 실제 처리기를 빌린다. 외부 효과는 해당 원문 분기에서 요구한다.</summary>
    public HudDividerInput(GumpCollection collection, GumpInput input, GumpTree tree, HudDividerInputHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(input); ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Divider is null || hooks.Base is null) throw new ArgumentException("구분선 입력 공유 저장소 필요", nameof(hooks));
        _collection = collection; _input = input; _tree = tree; _hooks = hooks;
    }
    /// <summary>선택된 실제 하위만 해당 위치에서 요구한다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("구분선 입력 실제 장치 미연결");
    /// <summary>같은 공통 기본 객체와 파생 객체의 일치를 요구한다.</summary>
    private HudDividerNode Node(uint token)
    {
        HudDividerNode? node = token != 0 ? _hooks.Divider!(token) : null;
        return node is not null && ReferenceEquals(node, _hooks.Base!(token)) ? node : throw new InvalidOperationException("구분선 입력 공유 노드 없음");
    }
    /// <summary>signed 정수 제한과 비교한 뒤 선택된 제한만 float32로 변환한다. 역전 제한도 원문 두 비교 순서를 유지한다.</summary>
    private static double Limited(double value, int minimum, int maximum)
    {
        if (value < minimum) value = (float)minimum;
        if (maximum < value) value = (float)maximum;
        return value;
    }
    /// <summary>항상 로컬 변환 후 press 기준점/캡처·release·hover·실제 기본 Help 순서로 분배한다.</summary>
    public uint Handle(uint token, uint eventCode, uint first, uint second)
    {
        HudDividerNode node = Node(token); var local = _collection.ScreenToLocal(token, new(unchecked((int)first), unchecked((int)second))); uint code = eventCode & 0x03ffffff;
        if ((eventCode & 0x80000000) == 0 && code == 0x10000 && (node.Axis & 3) != 0)
        {
            node.Flags |= 0x10; node.Positioning[0] = BitConverter.SingleToUInt32Bits(local.X); node.Positioning[1] = BitConverter.SingleToUInt32Bits(local.Y);
            node.Flags |= 0x100; _input.Capture(token); return 1;
        }
        if ((eventCode & 0x80000000) != 0 && code == 0x10000) { node.Flags &= ~0x100U; return 1; }
        if (code == _hooks.CursorEvent)
        {
            if ((node.Axis & 2) != 0) { Need(_hooks.Cursor)(5); return 1; }
            if ((node.Axis & 1) != 0) { Need(_hooks.Cursor)(4); return 1; }
        }
        return Need(_hooks.BaseHandle)(token, eventCode, first, second);
    }
    /// <summary>공통 Broadcast의 visible/notify/비dead 관문 뒤 호출한다. release 폴링으로 notify를 지워도 마지막 Move를 수행한다.</summary>
    public void Broadcast(uint token, uint eventCode, uint first, uint second)
    {
        HudDividerNode node = Node(token); if (Need(_hooks.MenuInstances)() != 0) return;
        if ((Need(_hooks.Poll)(0x10000).Code & 0x80000000) != 0) node.Flags &= ~0x100U;
        float x, y;
        if ((node.Axis & 2) != 0)
        {
            double position = (double)unchecked((int)second) - 2;
            if ((node.Axis & 8) != 0) position = Limited(position, node.MinY, node.MaxY);
            y = (float)position; x = (float)((double)BitConverter.UInt32BitsToSingle(node.Positioning[0]) + node.X);
        }
        else
        {
            if ((node.Axis & 1) == 0) return;
            double position = (double)unchecked((int)first) - 2;
            if ((node.Axis & 4) != 0) position = Limited(position, node.MinX, node.MaxX);
            x = (float)position; y = (float)((double)BitConverter.UInt32BitsToSingle(node.Positioning[1]) + node.Y);
        }
        _tree.Move(token, x, y, 0);
    }
    /// <summary>같은 사건/직접 캡처/프레임 가상 분배에 구분선만 추가하고 다른 파생의 이전 처리를 보존한다.</summary>
    public static void BindHudDividerInput(GumpEventHooks events, GumpInputHooks input, GumpFrameHooks frame, Func<uint, HudDividerInput?> resolve)
    {
        ArgumentNullException.ThrowIfNull(events); ArgumentNullException.ThrowIfNull(input); ArgumentNullException.ThrowIfNull(frame); ArgumentNullException.ThrowIfNull(resolve);
        Func<uint, uint, uint, uint, uint>? previousEvent = events.Handle, previousInput = input.Handle; Action<uint, uint, uint, uint>? previousBroadcast = frame.Broadcast;
        events.Handle = (token, code, first, second) => resolve(token) is { } handler ? handler.Handle(token, code, first, second) : Need(previousEvent)(token, code, first, second);
        input.Handle = (token, code, first, second) => resolve(token) is { } handler ? handler.Handle(token, code, first, second) : Need(previousInput)(token, code, first, second);
        frame.Broadcast = (token, code, first, second) => { if (resolve(token) is { } handler) handler.Broadcast(token, code, first, second); else Need(previousBroadcast)(token, code, first, second); };
    }
}
