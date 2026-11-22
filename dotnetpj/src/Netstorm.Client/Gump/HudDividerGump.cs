namespace Netstorm.Client.Gump; // HUD와 생산 패널이 같은 구분선 본체를 사용하는 영역이다.

/// <summary>004c6c50·004c4790·004c48f0의 생성/슬레이브/부착 전체 본체다.
/// 근거: cpppj/src/client/HudDock.cpp. callback 뒤 현재 필드와 float 저장 순서를 보존한다.</summary>
public sealed class HudDividerGump
{
    /// <summary>현재 화면 너비 getter이며 표시 영역 사본을 만들지 않는다.</summary>
    private readonly Func<int> _width;
    /// <summary>현재 화면 높이 getter.</summary>
    private readonly Func<int> _height;
    /// <summary>00540452의 현재 signed BYTE 색.</summary>
    private readonly Func<sbyte> _color;
    /// <summary>현재 공통 노드/가상 호출 장치.</summary>
    private readonly HudDockHooks _hooks;
    /// <summary>현재 화면/색과 같은 장치를 빌린다. 각 효과는 원문 도달 위치에서 요구한다.</summary>
    public HudDividerGump(Func<int> width, Func<int> height, Func<sbyte> color, HudDockHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(width); ArgumentNullException.ThrowIfNull(height);
        ArgumentNullException.ThrowIfNull(color); ArgumentNullException.ThrowIfNull(hooks);
        _width = width; _height = height; _color = color; _hooks = hooks;
    }
    /// <summary>실제로 소비한 기본 노드만 조회한다.</summary>
    private GumpCreateNode Node(uint token) => (token != 0 ? _hooks.Node?.Invoke(token) : null) ?? throw new InvalidOperationException("구분선 실제 기본 저장소 없음");
    /// <summary>파생과 기본이 같은 객체인지 요구한다.</summary>
    private HudDividerNode Divider(uint token)
    {
        HudDividerNode? node = token != 0 ? _hooks.Divider?.Invoke(token) : null;
        return node is not null && ReferenceEquals(node, Node(token)) ? node : throw new InvalidOperationException("구분선 실제 파생 저장소 불일치");
    }
    /// <summary>FST 기본 폭 뒤 같은 확장 정밀도 값에 배율을 곱한 폭을 저장한다.</summary>
    private static void Width(GumpCreateNode node, double value) { node.BaseWidth = (float)value; node.Width = (float)(value * node.Scale); }
    /// <summary>기본 높이의 float32 반올림을 배율 계산에 재사용하지 않는다.</summary>
    private static void Height(GumpCreateNode node, double value) { node.BaseHeight = (float)value; node.Height = (float)(value * node.Scale); }
    /// <summary>선택된 현재 가상 Push를 요구하며 미설치도 해당 위치의 부분 효과 뒤 실패한다.</summary>
    private void Push(uint token) => (_hooks.Push ?? throw new InvalidOperationException("HUD 분리 없음"))(token);
    /// <summary>현재 파생 Attach를 매 호출에서 다시 선택한다.</summary>
    private void AttachChild(uint token, float x, float y, uint parent, uint mode) => (_hooks.Attach ?? throw new InvalidOperationException("HUD 파생 부착 없음"))(token, x, y, parent, mode);
    /// <summary>배경 생성 뒤 vtable/flags/색/축과20DWORD 슬롯만 쓰며 네 제한은 보존한다.</summary>
    public uint Construct(uint token, uint axis, int width, int height, uint flags, uint type, uint frame)
    {
        if (_hooks.ConstructBackground is null || _hooks.Activate is null) throw new InvalidOperationException("HUD 구분선 생성 장치 없음");
        _hooks.ConstructBackground(token, width, height, flags, type, frame);
        HudDividerNode node = Divider(token); node.Vtable = 0x514fd8; node.Flags |= 0x60200; node.Command = unchecked((uint)(int)_color()); node.Axis = axis;
        Array.Clear(node.Slaves); _hooks.Activate(token, 1); return token;
    }
    /// <summary>첫 serial0 슬롯에 중복도 등록하며 한도 진단 복귀 후 자식의40000비트를 쓴다.</summary>
    public void RegisterSlave(uint master, uint slave, uint flags)
    {
        if (slave == 0) return; HudDividerNode divider = Divider(master); int index = 0;
        // 원문 고정10슬롯을 순서대로 찾는다.
        for (; index < divider.Slaves.Length; index++) if (divider.Slaves[index].Serial == 0) break;
        if (index == divider.Slaves.Length) (_hooks.Assertion ?? throw new InvalidOperationException("HUD 슬레이브 한도 진단 없음"))(83);
        else divider.Slaves[index] = (Node(slave).Serial, flags);
        Node(slave).Flags |= 0x40000;
    }
    /// <summary>팝업/기본 부착/현재 visible/폭/부모 통지 뒤 현재10관계를1·2·4·8순서로 적용한다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        (_hooks.ClosePopups ?? throw new InvalidOperationException("HUD 팝업 정리 없음"))();
        HudDividerNode master = Divider(token); float oldY = master.Y;
        (_hooks.BaseAttach ?? throw new InvalidOperationException("HUD 기본 부착 없음"))(token, x, y, parent, mode); if ((master.Flags & 1) == 0) return;
        if ((master.Axis & 0x10) != 0) Width(master, (double)_width() - x);
        (_hooks.Bubble ?? throw new InvalidOperationException("HUD 부모 통지 없음"))(token, 0x0200003a, master.Serial, 0);
        // 매 슬롯에서 현재 serial/flags를 다시 읽되 해결된 자식 객체는 네 분기 동안 유지한다.
        for (int index = 0; index < master.Slaves.Length; index++)
        {
            if (master.Slaves[index].Serial == 0) continue;
            uint slave = (_hooks.ResolveSerial ?? throw new InvalidOperationException("HUD serial 조회 없음"))(master.Slaves[index].Serial); if (slave == 0) continue;
            GumpCreateNode child = Node(slave); float preservedY = (float)((double)y - ((double)oldY - child.Y));
            if ((master.Slaves[index].Flags & 1) != 0)
            {
                Push(slave); Width(child, (double)master.X - child.X);
                AttachChild(slave, child.X, child.Y, 0, 1);
            }
            if ((master.Slaves[index].Flags & 2) != 0)
            {
                Push(slave);
                if ((master.Slaves[index].Flags & 0x10) != 0) Height(child, (double)master.Y - child.Y);
                if ((master.Slaves[index].Flags & 0x20) == 0) { Height(child, (double)master.Y - child.Y); AttachChild(slave, child.X, child.Y, 0, 1); }
                else AttachChild(slave, child.X, preservedY, 0, 1);
            }
            if ((master.Slaves[index].Flags & 4) != 0)
            {
                Push(slave); double right = (double)master.BaseWidth + master.X;
                Width(child, (double)_width() - right); AttachChild(slave, (float)right, child.Y, 0, 1);
            }
            if ((master.Slaves[index].Flags & 8) != 0)
            {
                Push(slave); double bottom = (double)master.BaseHeight + master.Y;
                Height(child, (master.Slaves[index].Flags & 0x40) != 0 ? (double)oldY - master.Y + child.BaseHeight : (double)_height() - bottom);
                AttachChild(slave, child.X, (float)bottom, 0, 1);
            }
        }
    }
    /// <summary>같은 공통 가상 Attach에 구분선만 추가하고 이전 파생 분배를 보존한다.</summary>
    public static void BindHudDividerGump(GumpTreeHooks tree, Func<uint, HudDividerGump?> resolve)
    {
        ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(resolve);
        Action<uint, float, float, uint, uint>? previous = tree.Attach;
        tree.Attach = (token, x, y, parent, mode) =>
        {
            if (resolve(token) is { } divider) divider.Attach(token, x, y, parent, mode);
            else (previous ?? throw new InvalidOperationException("Gump 이전 가상 부착 없음"))(token, x, y, parent, mode);
        };
    }
}
