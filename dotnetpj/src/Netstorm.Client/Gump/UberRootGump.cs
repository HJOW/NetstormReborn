using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 전체 HUD 부모를 같은 공통 노드/등록표/수명에 조립하는 영역이다.

/// <summary>004c7160 생성·004c77c0 교체·004c7880 재생성의 전체 부모다.
/// 근거: cpppj/src/client/UberRootGump.cpp/h. 미설치 하위는 원문에서 선택한 위치에서 실패한다.</summary>
public sealed class UberRootGump
{
    /// <summary>005ca920의 현재 배경 TYPE 주소.</summary>
    private const uint BackgroundType = 0x5ca920;
    /// <summary>005ca924의 현재 배경 프레임 주소.</summary>
    private const uint BackgroundFrame = 0x5ca924;
    /// <summary>005ca928의 현재 패널 프레임 주소.</summary>
    private const uint PanelFrame = 0x5ca928;
    /// <summary>005c78f0의 현재 화면 너비 주소.</summary>
    private const uint Width = 0x5c78f0;
    /// <summary>005c78f4의 현재 화면 높이 주소.</summary>
    private const uint Height = 0x5c78f4;
    /// <summary>005411c8의 현재 fortGump TYPE 주소.</summary>
    private const uint FortGump = 0x5411c8;
    /// <summary>00565e70의 현재 메시지 첫 BYTE 주소.</summary>
    private const uint Title = 0x565e70;
    /// <summary>같은1000등록칸/현재0루트/count/serial.</summary>
    private readonly GumpCreateState _create;
    /// <summary>실제 현재 전역과 전체 하위 몸체.</summary>
    private readonly UberRootHooks _hooks;
    /// <summary>실제 PATCH 공통 저장소를 빌린다. 하위는 도달 시점에만 요구한다.</summary>
    public UberRootGump(GumpCreateState create, UberRootHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(hooks);
        if (create.Collection.Edition != OriginalEdition.Patch1078) throw new ArgumentException("Uber 루트10.78 판본 필요", nameof(create));
        _create = create; _hooks = hooks;
    }
    /// <summary>선택된 실제 하위만 해당 소비 위치에서 요구한다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("Uber 루트 실제 하위 본체 미연결");
    /// <summary>실제 소비한 유효 토큰의 현재 기본 객체를 읽는다.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? Need(_hooks.Base)(token) : null) ?? throw new InvalidOperationException("Uber 루트 실제 기본 노드 없음");
    /// <summary>파생 부모와 기본 주소가 같은 객체임을 요구한다.</summary>
    private UberRootNode Node(uint token)
    {
        UberRootNode? value = token != 0 ? Need(_hooks.Node)(token) : null;
        return value is not null && ReferenceEquals(value, Base(token)) ? value : throw new InvalidOperationException("Uber 루트 파생 저장소 불일치");
    }
    /// <summary>구분선도 공통 기본 주소와 동일해야 한다.</summary>
    private HudDividerNode Divider(uint token)
    {
        HudDividerNode? value = token != 0 ? Need(_hooks.Divider)(token) : null;
        return value is not null && ReferenceEquals(value, Base(token)) ? value : throw new InvalidOperationException("Uber 구분선 파생 저장소 불일치");
    }
    /// <summary>현재 원문 전역을 캐시 없이 읽는다.</summary>
    private uint Read(uint address) => Need(_hooks.Read)(address);
    /// <summary>4437f0의 실제 C3 빈 본문이며 선택적 호출 관측만 실행한다.</summary>
    private void Empty() => _hooks.EmptyObserver?.Invoke();
    /// <summary>현재 생성 중인 this의 필드를 빌린다. 등록0칸이나 자식 사본으로 바꾸지 않는다.</summary>
    public Func<HudDockPart, uint> DockView(uint token)
    {
        Node(token);
        return part =>
        {
            if (part == HudDockPart.Root) return token; UberRootNode node = Node(token);
            return part switch
            {
                HudDockPart.Vertical => node.Vertical, HudDockPart.MinimapDivider => node.MinimapDivider, HudDockPart.ChatDivider => node.ChatDivider,
                HudDockPart.Minimap => node.Minimap, HudDockPart.Production => node.Production, HudDockPart.Chat => node.Chat, HudDockPart.Knowledge => node.Knowledge,
                _ => throw new ArgumentOutOfRangeException(nameof(part), "Uber 현재 HUD 필드 종류 오류"),
            };
        };
    }
    /// <summary>같은 실제 자식의 가상 Attach를 mode0으로 호출한다.</summary>
    private void Attach(uint child, float x, float y, uint parent) { Base(child); Need(_hooks.Attach)(child, x, y, parent, 0); }
    /// <summary>각 등록 효과 뒤 현재 필드를 다시 읽고10호출 및 세 구분선 제한을 원문 순서대로 쓴다.</summary>
    private void Layout(uint token)
    {
        Need(_hooks.RegisterSlave)(Node(token).Vertical, Node(token).MinimapDivider, 1); Need(_hooks.RegisterSlave)(Node(token).Vertical, Node(token).ChatDivider, 4);
        Need(_hooks.RegisterSlave)(Node(token).Vertical, Node(token).Minimap, 1); Need(_hooks.RegisterSlave)(Node(token).Vertical, Node(token).Production, 1);
        Need(_hooks.RegisterSlave)(Node(token).Vertical, Node(token).Knowledge, 1); Need(_hooks.RegisterSlave)(Node(token).Vertical, Node(token).Chat, 4);
        Need(_hooks.RegisterSlave)(Node(token).MinimapDivider, Node(token).Minimap, 8); Need(_hooks.RegisterSlave)(Node(token).MinimapDivider, Node(token).Production, 2);
        Need(_hooks.RegisterSlave)(Node(token).MinimapDivider, Node(token).Knowledge, 2); Need(_hooks.RegisterSlave)(Node(token).ChatDivider, Node(token).Chat, 8);
        HudDividerNode vertical = Divider(Node(token).Vertical); vertical.MinX = 60; vertical.MaxX = 130; vertical.Axis |= 4;
        uint height = Read(Height); HudDividerNode minimap = Divider(Node(token).MinimapDivider); minimap.Axis |= 8; minimap.MinY = unchecked((int)(height - 123)); minimap.MaxY = unchecked((int)height);
        HudDividerNode chat = Divider(Node(token).ChatDivider); chat.Axis |= 8; chat.MinY = 25; chat.MaxY = unchecked((int)(Read(Height) - 25));
    }
    /// <summary>실제 기본 생성·세 구분선·네 공장·기술/관계·재부착·현재 메시지까지 전체200BYTE 부모를 생성한다.</summary>
    public uint Construct(uint token)
    {
        Node(token); Need(_hooks.ConstructBase)(token, 0, 0, 0, 1); Node(token).Vtable = 0x51503c;
        Node(token).Command = unchecked((uint)(int)(sbyte)(byte)Read(0x540451)); Node(token).Flags |= 0x800;
        Need(_hooks.Write)(BackgroundType, Read(FortGump)); Need(_hooks.Write)(BackgroundFrame, 0); Need(_hooks.Write)(PanelFrame, 2);
        Node(token).ChatDivider = 0; Node(token).MinimapDivider = 0; Node(token).Vertical = 0; Node(token).Chat = 0;
        uint next = Need(_hooks.Allocate)(0x11c);
        if (next != 0) { uint frame = Read(0x542664), type = Read(BackgroundType); int height = unchecked((int)Read(Height)); Need(_hooks.ConstructDivider)(next, 0, 5, height, 0x03400000, type, frame); }
        Node(token).Vertical = next; Attach(next, 80, 0, token);
        next = Need(_hooks.Allocate)(0x11c);
        if (next != 0) { uint frame = Read(0x542664), type = Read(BackgroundType); Need(_hooks.ConstructDivider)(next, 0, 80, 5, 0x02a00000, type, frame); }
        Node(token).MinimapDivider = next; Attach(next, 0, 400, token);
        next = Need(_hooks.Allocate)(0x11c);
        if (next != 0) { uint frame = Read(BackgroundFrame), type = Read(BackgroundType); int width = unchecked((int)Read(Width)); Need(_hooks.ConstructDivider)(next, 18, width, 5, 0x02a00000, type, frame); }
        Node(token).ChatDivider = next; Attach(next, 85, unchecked((int)(Read(Height) - 96)), token);
        Empty(); uint made = Need(_hooks.CreateMinimap)(); Node(token).Minimap = made; Base(Node(token).Minimap).Flags |= 0x20000;
        uint child = Node(token).Minimap; float scaledHeight = (float)((double)Base(child).Scale * 128); Base(child).BaseHeight = 128; Base(child).Height = scaledHeight;
        child = Node(token).Minimap; float scaledWidth = (float)((double)Base(child).Scale * 128); Base(child).BaseWidth = 128; Base(child).Width = scaledWidth;
        Attach(Node(token).Minimap, 0, 0, token);
        made = Need(_hooks.CreateProduction)(); Node(token).Production = made; Base(Node(token).Production).Flags |= 0x20000; Attach(Node(token).Production, 0, 0, token);
        made = Need(_hooks.CreateKnowledge)(); Node(token).Knowledge = made; child = Node(token).Knowledge;
        double currentWidth = unchecked((int)Read(Width)); Base(child).BaseWidth = (float)currentWidth; Base(child).Width = (float)(currentWidth * Base(child).Scale);
        child = Node(token).Knowledge; double currentHeight = unchecked((int)Read(Height)); Base(child).BaseHeight = (float)currentHeight; Base(child).Height = (float)(currentHeight * Base(child).Scale);
        Base(Node(token).Knowledge).Flags |= 0x20000; Attach(Node(token).Knowledge, 0, 0, token);
        Need(_hooks.Type)(Read(FortGump)); made = Need(_hooks.CreateChat)(); Node(token).Chat = made; Base(Node(token).Chat).Flags |= 0x20000;
        uint label = Need(_hooks.Translate)("Chat Window"); Need(_hooks.Help)(Node(token).Chat, label); Need(_hooks.HelpWord)(Node(token).Chat, "chatView");
        Empty(); Attach(Node(token).Chat, 0, 0, token); Empty(); Need(_hooks.RefreshTechnology)(); Layout(token);
        Need(_hooks.Cleanup)(); child = Node(token).Vertical; Need(_hooks.Move)(child, Base(child).X, Base(child).Y, 0);
        Need(_hooks.Cleanup)(); child = Node(token).MinimapDivider; Need(_hooks.Move)(child, Base(child).X, Base(child).Y, 0);
        Need(_hooks.Cleanup)(); child = Node(token).ChatDivider; Need(_hooks.Move)(child, Base(child).X, Base(child).Y, 0); Need(_hooks.Cleanup)();
        if ((Read(Title) & 0xff) != 0)
        {
            next = Need(_hooks.Allocate)(0xb0); if (next != 0) Need(_hooks.ConstructMessage)(next, Need(_hooks.SavedMessage)()); Node(token).Message = next;
            float x = (float)((double)(unchecked((int)Read(0x531860)) / 2) - (double)Base(next).BaseWidth * .5);
            float y = (float)((double)(unchecked((int)Read(0x531864)) / 2) - (double)Base(next).BaseHeight * .5); Attach(next, x, y, token);
            if (!Need(_hooks.EqualText)(Need(_hooks.SavedMessage)(), "defeat") && !Need(_hooks.EqualText)(Need(_hooks.SavedMessage)(), "titleMenu") && !Need(_hooks.EqualText)(Need(_hooks.SavedMessage)(), "victory"))
            {
                uint[] expression = [0xfffffffe, Read(0x540cec), 0xfffffffe, Read(0x540cf8), 0xfffffffd, Need(_hooks.SavedMessage)(), 0xfffffffc, 0x51501c, 0];
                uint condition = Need(_hooks.FormatMessage)(expression); if (Need(_hooks.TestMessage)(condition) != 0) Need(_hooks.VisitMessage)(Need(_hooks.SavedMessage)());
                Need(_hooks.MessageEvent)(Node(token).Message, Read(0x542a04), 0, 1); Need(_hooks.Overlay)(Node(token).Message, true); Need(_hooks.SetDepth)(Node(token).Message, 0xffffff9c);
            }
        }
        return token;
    }
    /// <summary>새 생성의 임시 등록을 반납하고 이전 직계 자식의 next를 Push 전에 보관해 실제 새 부모로 이관한다.</summary>
    public void Replace()
    {
        Empty(); uint allocated = Need(_hooks.Allocate)(0xc8); uint token = allocated != 0 ? Construct(allocated) : 0;
        GumpCreateNode node = Base(token); if (node.Id >= _create.Entries.Length) throw new InvalidOperationException("Uber 임시 등록 ID 범위 오류");
        _create.Entries[node.Id] = 0; _create.Collection.Root = _create.Entries[0]; node.Id = 0;
        uint root = _create.Entries[0]; var visited = new HashSet<uint>();
        // 같은 기존 직계 링크만 이관하며 callback이 바꾼 다음 링크로 다시 탐색하지 않는다.
        for (uint child = Base(root).Child; child != 0;)
        {
            if (!visited.Add(child)) throw new InvalidOperationException("Uber 이전 부모 형제 순환"); uint next = Base(child).Next;
            Need(_hooks.Push)(child); Attach(child, Base(child).X, Base(child).Y, token); child = next;
        }
        Need(_hooks.ReplaceRoot)(token);
    }
    /// <summary>기존 부모가0이어도 먼저 정리하고 전체 새 부모를 만든다. 실패 뒤 이전 HUD를 되살리지 않는다.</summary>
    public void Recreate()
    {
        if (_create.Entries[0] != 0) Need(_hooks.Kill)(_create.Entries[0]); Need(_hooks.Cleanup)();
        uint allocated = Need(_hooks.Allocate)(0xc8); uint token = allocated != 0 ? Construct(allocated) : 0; GumpCreateNode node = Base(token);
        if (node.Id >= _create.Entries.Length) throw new InvalidOperationException("Uber 재생성 임시 등록 ID 물리 범위");
        _create.Entries[node.Id] = 0; _create.Collection.Root = _create.Entries[0]; node.Id = 0; Need(_hooks.ReplaceRoot)(token);
    }
}
