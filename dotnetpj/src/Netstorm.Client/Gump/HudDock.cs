namespace Netstorm.Client.Gump; // 실제 HUD와 공통 트리의 지도 영역을 연결하는 영역이다.

/// <summary>004c6530·004c7160의 HUD 명령/지도 영역·정상 슬레이브 배치 본체다.
/// 근거: cpppj/src/client/HudDock.cpp. 실제 부모의 현재 필드를 소비 위치마다 읽는다.</summary>
public sealed class HudDock
{
    /// <summary>생성 중인 실제 this의 현재 자식 필드를 빌리는 getter.</summary>
    private readonly Func<HudDockPart, uint> _token;
    /// <summary>같은 화면/지도/Deck/갱신 전역.</summary>
    private readonly HudDockGlobals _globals;
    /// <summary>같은 기본/파생 가상 호출과 실제 효과.</summary>
    private readonly HudDockHooks _hooks;
    /// <summary>Uber와 생산 패널이 공유하는 전체 구분선 본체.</summary>
    public HudDividerGump DividerGump { get; }
    /// <summary>현재 실제 필드 getter와 전역/색/장치를 빌리며 화면 상태는 바꾸지 않는다.</summary>
    public HudDock(Func<HudDockPart, uint> token, HudDockGlobals globals, Func<sbyte> color, HudDockHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(token); ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(color); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null || hooks.Divider is null || hooks.BaseAttach is null || hooks.Push is null || hooks.Attach is null ||
            hooks.Move is null || hooks.ResolveSerial is null || hooks.Bubble is null || hooks.ClosePopups is null)
            throw new ArgumentException("HUD 실제 노드/트리 처리기 필요", nameof(hooks));
        _token = token; _globals = globals; _hooks = hooks;
        DividerGump = new(() => globals.ScreenWidth, () => globals.ScreenHeight, color, hooks);
    }
    /// <summary>선택한 실제 필드의 현재 토큰을 읽으며 다른 필드를 미리 요구하지 않는다.</summary>
    private uint Token(HudDockPart part) => _token(part);
    /// <summary>원문 유효 포인터 계약에 해당하는 현재 기본 노드.</summary>
    private GumpCreateNode Node(uint token) => (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("HUD Gump 저장소 없음");
    /// <summary>같은 객체의 구분선 확장을 요구한다.</summary>
    private HudDividerNode Divider(uint token)
    {
        HudDividerNode? node = token != 0 ? _hooks.Divider!(token) : null;
        return node is not null && ReferenceEquals(node, Node(token)) ? node : throw new InvalidOperationException("HUD 구분선 동일 저장소 필요");
    }
    /// <summary>원본 signed64 _ftol 결과의 하위DWORD. 범위 밖/비유한 입력은0이다.</summary>
    private static int Truncate(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0 ? 0 : unchecked((int)(long)value);
    /// <summary>전체 구분선 생성의 같은 본체를 호출한다.</summary>
    public uint Construct(uint token, uint axis, int width, int height, uint flags, uint type, uint frame) => DividerGump.Construct(token, axis, width, height, flags, type, frame);
    /// <summary>같은 고정 슬롯 등록 본체를 호출한다.</summary>
    public void RegisterSlave(uint master, uint slave, uint flags) => DividerGump.RegisterSlave(master, slave, flags);
    /// <summary>같은 공통 기본/파생 부착 본체를 호출한다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode) => DividerGump.Attach(token, x, y, parent, mode);
    /// <summary>실제 자식 생성 뒤 원문10등록의 순서와 세 구분선의 현재 제한만 설정한다.</summary>
    public void RegisterNormalLayout()
    {
        RegisterSlave(Token(HudDockPart.Vertical), Token(HudDockPart.MinimapDivider), 1); RegisterSlave(Token(HudDockPart.Vertical), Token(HudDockPart.ChatDivider), 4);
        RegisterSlave(Token(HudDockPart.Vertical), Token(HudDockPart.Minimap), 1); RegisterSlave(Token(HudDockPart.Vertical), Token(HudDockPart.Production), 1);
        RegisterSlave(Token(HudDockPart.Vertical), Token(HudDockPart.Knowledge), 1); RegisterSlave(Token(HudDockPart.Vertical), Token(HudDockPart.Chat), 4);
        RegisterSlave(Token(HudDockPart.MinimapDivider), Token(HudDockPart.Minimap), 8); RegisterSlave(Token(HudDockPart.MinimapDivider), Token(HudDockPart.Production), 2);
        RegisterSlave(Token(HudDockPart.MinimapDivider), Token(HudDockPart.Knowledge), 2); RegisterSlave(Token(HudDockPart.ChatDivider), Token(HudDockPart.Chat), 8);
        HudDividerNode vertical = Divider(Token(HudDockPart.Vertical)); vertical.MinX = 60; vertical.MaxX = 130; vertical.Axis |= 4;
        HudDividerNode minimap = Divider(Token(HudDockPart.MinimapDivider)); minimap.MinY = unchecked(_globals.ScreenHeight - 123); minimap.MaxY = _globals.ScreenHeight; minimap.Axis |= 8;
        HudDividerNode chat = Divider(Token(HudDockPart.ChatDivider)); chat.MinY = 25; chat.MaxY = unchecked(_globals.ScreenHeight - 25); chat.Axis |= 8;
    }
    /// <summary>release도 제거한 명령을 분배하며 효과 뒤 현재 필드/전역을 재조회한다. 이미 쓴 상태는 예외에도 보존한다.</summary>
    public uint Handle(uint eventCode, uint first, uint second)
    {
        uint command = eventCode & 0x03ffffff;
        if (command == 0x02000088)
        {
            if (_hooks.ChatVisible is null || _hooks.ChatInput is null) throw new InvalidOperationException("HUD 실제 채팅 설정 없음");
            if (second != 0 && (second != 2 || (double)_globals.ScreenHeight <= Node(Token(HudDockPart.ChatDivider)).Y))
            {
                _hooks.ChatVisible(true);
                if ((double)_globals.ScreenHeight <= Node(Token(HudDockPart.ChatDivider)).Y)
                {
                    _hooks.Push!(Token(HudDockPart.ChatDivider)); uint token = Token(HudDockPart.ChatDivider);
                    _hooks.Attach!(token, Node(token).X, _globals.SavedChatY, Token(HudDockPart.Root), 0);
                }
            }
            else
            {
                _hooks.ChatVisible(false);
                if (Node(Token(HudDockPart.ChatDivider)).Y < (double)_globals.ScreenHeight) _globals.SavedChatY = Node(Token(HudDockPart.ChatDivider)).Y;
                _hooks.ChatInput(false); _hooks.Push!(Token(HudDockPart.ChatDivider)); uint token = Token(HudDockPart.ChatDivider);
                _hooks.Attach!(token, Node(token).X, _globals.ScreenHeight, Token(HudDockPart.Root), 0);
            }
        }
        if (command is 0x0200003a or 0x02000046)
        {
            if (_hooks.Invalidate is null) throw new InvalidOperationException("HUD dirty 장치 없음");
            if (Token(HudDockPart.Vertical) != 0 && first == Node(Token(HudDockPart.Vertical)).Serial)
            {
                GumpCreateNode node = Node(Token(HudDockPart.Vertical)); int left = Truncate((double)node.X + node.BaseWidth), height = _globals.ScreenHeight;
                _globals.MapWidth = unchecked(_globals.ScreenWidth - left); _globals.MapLeft = left;
                _hooks.Invalidate(0, 0, unchecked(left + 1), height, 0);
                _hooks.Invalidate(_globals.MapLeft, _globals.MapHeight, _globals.MapWidth, unchecked(_globals.ScreenHeight - _globals.MapHeight), 0);
            }
            if (Token(HudDockPart.ChatDivider) != 0 && first == Node(Token(HudDockPart.ChatDivider)).Serial)
            {
                int height = Truncate(Node(Token(HudDockPart.ChatDivider)).Y); _globals.MapHeight = height;
                _hooks.Invalidate(_globals.MapLeft, height, _globals.MapWidth, unchecked(_globals.ScreenHeight - height), 0);
            }
            int leftNow = _globals.MapLeft, top = _globals.MapTop, right = unchecked(leftNow + _globals.MapWidth), bottom = unchecked(top + _globals.MapHeight);
            _globals.MapRect = new(leftNow, top, right, bottom); _globals.MapFloatRect[0] = leftNow; _globals.MapFloatRect[1] = top; _globals.MapFloatRect[2] = right; _globals.MapFloatRect[3] = bottom;
            if (Token(HudDockPart.MinimapDivider) != 0)
            {
                GumpCreateNode node = Node(Token(HudDockPart.MinimapDivider)); _globals.LeftBottomRect = new(0, Truncate(node.Y), Truncate(node.BaseWidth), _globals.ScreenHeight);
            }
            else _globals.LeftBottomRect = default;
            return 1;
        }
        if (command != 0x0200003d)
        {
            if (command == 0x02000005)
            {
                float y = unchecked(_globals.ScreenHeight - unchecked((int)second) - 5);
                _hooks.Push!(Token(HudDockPart.ChatDivider)); uint token = Token(HudDockPart.ChatDivider); _hooks.Attach!(token, Node(token).X, y, Token(HudDockPart.Root), 0); return 1;
            }
            return (_hooks.BaseHandle ?? throw new InvalidOperationException("HUD 기본 입력 없음"))(eventCode, first, second);
        }
        if (first is 1 or 3)
        {
            Node(Token(HudDockPart.Vertical)).Flags |= 1; Node(Token(HudDockPart.MinimapDivider)).Flags |= 1; Node(Token(HudDockPart.Minimap)).Flags |= 1;
            Node(Token(HudDockPart.Production)).Flags |= 1; Node(Token(HudDockPart.Knowledge)).Flags &= ~1U;
            _hooks.Move!(Token(HudDockPart.Vertical), first == 3 || _globals.ShowHud != 0 ? 80 : -Node(Token(HudDockPart.Vertical)).BaseWidth, 0, 0);
            _hooks.Move!(Token(HudDockPart.MinimapDivider), 0, unchecked(_globals.ScreenHeight - 80), 0);
        }
        else if (first == 4)
        {
            Node(Token(HudDockPart.Production)).Flags &= ~1U; Node(Token(HudDockPart.Knowledge)).Flags |= 1; Node(Token(HudDockPart.Minimap)).Flags &= ~1U;
            _hooks.Move!(Token(HudDockPart.Vertical), 170, 0, 0); _hooks.Move!(Token(HudDockPart.MinimapDivider), 0, _globals.ScreenHeight, 0);
        }
        _hooks.Push!(Token(HudDockPart.ChatDivider)); uint chat = Token(HudDockPart.ChatDivider); _hooks.Attach!(chat, Node(chat).X, Node(chat).Y, 0, 0);
        _globals.FullRefresh = unchecked(_globals.FullRefresh + 1);
        (_hooks.FinishResize ?? throw new InvalidOperationException("HUD 화면 후처리 없음"))(); return 0;
    }
}
