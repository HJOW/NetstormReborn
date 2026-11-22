using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // HUD 본체와 실제 공통 생성/등록/부착 및 공유 전역의 연결을 검사한다.

/// <summary>독립 기하 관측과 별도로 같은 Gump 객체·현재 owner·실패 prefix 계약을 검증한다.</summary>
public sealed class HudDockBindingTests
{
    /// <summary>원문 자식 기하를 명시 입력으로 공급하고 공통 생성/수명/트리 본체를 실제 사용한다.
    /// 일반 배경 자식을 사용한 본체 검사이며 전체 Uber/미니맵/생산/채팅 UI 조립의 증거로 세지 않는다.</summary>
    private sealed class Rig
    {
        /// <summary>같은 기본/파생 객체를 보유한 공통 노드 저장소.</summary>
        public readonly Dictionary<uint, GumpCreateNode> Nodes = [];
        /// <summary>실제 등록표와 화면 크기.</summary>
        public readonly GumpCreateState Create = new() { ScreenWidth = 1024, ScreenHeight = 768 };
        /// <summary>지도 정렬기와 공유하는 현재 지도 영역.</summary>
        public readonly GumpTreeState TreeState = new();
        /// <summary>현재 Deck 표시 DWORD.</summary>
        public readonly UIntBox ShowHud = new() { Value = 1 };
        /// <summary>현재 미니맵 갱신 DWORD.</summary>
        public readonly UIntBox Revision = new();
        /// <summary>같은 화면/트리/두 DWORD를 빌리는 HUD 전역.</summary>
        public readonly HudDockGlobals Globals;
        /// <summary>원문 순서대로 변하는 현재 자식 필드.</summary>
        public readonly uint[] Parts = [1, 2, 3, 4, 5, 6, 7, 8];
        /// <summary>공통 기본/파생 부착 분배.</summary>
        public readonly GumpTreeHooks TreeHooks;
        /// <summary>전체 공통 트리 본체.</summary>
        public readonly GumpTree Tree;
        /// <summary>같은 등록표의 실제 overlay/수명 상태.</summary>
        public readonly GumpLifetimeState LifetimeState = new();
        /// <summary>실제 overlay 등록 본체.</summary>
        public readonly GumpLifetime Lifetime;
        /// <summary>같은 노드에 결합한 현재 HUD 장치.</summary>
        public readonly HudDockHooks Hooks;
        /// <summary>같은 현재 전역/노드의 실제 HUD 본체.</summary>
        public readonly HudDock Dock;
        /// <summary>실제 dirty 요청의 공개 인자.</summary>
        public readonly List<(int X, int Y, int Width, int Height, uint Flags)> Dirty = [];
        /// <summary>공통 redraw가 관찰한 실제 노드.</summary>
        public readonly List<(uint Token, bool Erase)> Redraw = [];
        /// <summary>팝업 정리 도달 횟수.</summary>
        public int ClosedPopups;
        /// <summary>화면 후처리 도달 횟수.</summary>
        public int Finished;
        /// <summary>현재 채팅 표시 장치 값.</summary>
        public bool ChatVisible = true;
        /// <summary>현재 채팅 입력 표시 장치 값.</summary>
        public bool ChatInput = true;
        /// <summary>실제 현재 객체만 조회한다.</summary>
        public GumpCreateNode? Node(uint token) => Nodes.GetValueOrDefault(token);
        /// <summary>실제 구분선 확장 객체를 빌린다.</summary>
        public HudDividerNode Divider(uint token) => (HudDividerNode)Nodes[token];
        /// <summary>원문 기하를 가진 실제 공통 등록/부착 상태를 구성한다.</summary>
        public Rig()
        {
            Create.Collection.Edition = OriginalEdition.Patch1078; Globals = new(Create, TreeState, ShowHud, Revision);
            var factory = new GumpCreate(Create, new() { Node = Node, Allocate = _ => { Nodes.Add(1, new()); return 1; } }); factory.InitializeRoot();
            TreeHooks = new() { Node = Node, Redraw = (token, erase) => Redraw.Add((token, erase)), Push = token => Tree!.Push(token), Attach = (token, x, y, parent, mode) => Tree!.Attach(token, x, y, parent, mode), Kill = _ => throw new InvalidOperationException("예상하지 않은 교체 삭제") };
            Tree = new(Create, TreeState, TreeHooks);
            Lifetime = new(Create, LifetimeState, new() { Node = Node, Push = Tree.Push, DeletingDestructor = (token, flags) => Lifetime!.DeleteScalar(token, flags), Release = token => Nodes.Remove(token) });
            var backgrounds = new GumpBackgroundCreate(Create, factory, () => 35, new() { Node = token => Node(token) as GumpBackgroundNode });
            Hooks = new()
            {
                Node = Node, Divider = token => Node(token) as HudDividerNode,
                ConstructBackground = (token, width, height, flags, type, frame) => backgrounds.Construct(token, width, height, flags, 1, type, frame), Activate = (token, enabled) => Lifetime.SetOverlay(token, enabled != 0),
                BaseAttach = Tree.Attach, Push = Tree.Push, Attach = (token, x, y, parent, mode) => TreeHooks.Attach!(token, x, y, parent, mode), Move = Tree.Move,
                ResolveSerial = serial => Create.Entries.FirstOrDefault(token => token != 0 && Node(token)?.Serial == serial),
                Bubble = (_, eventCode, first, second) => Dock!.Handle(eventCode, first, second), ClosePopups = () => ClosedPopups++,
                Invalidate = (x, y, width, height, flags) => Dirty.Add((x, y, width, height, flags)), FinishResize = () => Finished++, Assertion = _ => throw new InvalidOperationException("예상하지 않은 슬롯 한도"),
                BaseHandle = (_, _, _) => 73, ChatVisible = value => ChatVisible = value, ChatInput = value => ChatInput = value,
            };
            Dock = new(part => Parts[(int)part], Globals, () => 55, Hooks);
            HudDividerGump.BindHudDividerGump(TreeHooks, token => Node(token) is HudDividerNode ? Dock.DividerGump : null);
            // 세 구분선과 네 일반 배경을 같은 등록표/실제 생성기로 만든다.
            for (uint token = 2; token <= 8; token++)
            {
                Nodes.Add(token, token <= 4 ? new HudDividerNode() : new GumpBackgroundNode());
                if (token <= 4) Dock.Construct(token, token == 2 ? 2U : 0x11U, token == 2 ? 5 : token == 3 ? 80 : 939, token == 2 ? 768 : 5, 0, 0, 0);
                else backgrounds.Construct(token, 80, 80, 0, 0x20001, 0, 0);
            }
            // 같은 공통 가상 부착을 통해 초기 부모/형제 링크를 실제 만든다.
            for (uint token = 2; token <= 8; token++) TreeHooks.Attach!(token, token == 2 ? 80 : token is 4 or 7 ? 85 : 0, token == 3 ? 688 : token is 4 or 7 ? 672 : 0, 1, 0);
            Dock.RegisterNormalLayout();
        }
    }

    /// <summary>생성은 기존 지도/화면/카운터를 초기화하지 않고 이후 같은 owner의 변경을 읽는다.</summary>
    [Fact]
    public void GlobalsBorrowCurrentOwnersWithoutInitializingThem()
    {
        var create = new GumpCreateState { ScreenWidth = 1024, ScreenHeight = 768 }; var tree = new GumpTreeState { MapView = new(7, 9, 99, 111) };
        var show = new UIntBox { Value = 5 }; var refresh = new UIntBox { Value = 27 }; var globals = new HudDockGlobals(create, tree, show, refresh);
        Assert.Equal(new ScreenRect(7, 9, 99, 111), globals.MapRect); Assert.Equal(27U, refresh.Value);
        create.ScreenWidth = 1920; create.ScreenHeight = 1080; show.Value = 0; Assert.Equal(1920, globals.ScreenWidth); Assert.Equal(1080, globals.ScreenHeight); Assert.Equal(0U, globals.ShowHud);
        globals.MapRect = new(85, 0, 1920, 984); Assert.Equal(globals.MapRect, tree.MapView); tree.MapView = new(1, 2, 3, 4); Assert.Equal(tree.MapView, globals.MapRect);
        globals.FullRefresh++; Assert.Equal(28U, refresh.Value); refresh.Value = uint.MaxValue; Assert.Equal(uint.MaxValue, globals.FullRefresh);
    }
    /// <summary>원문10슬롯 관계/제한과 실제 overlay가 같은 생성/수명 표를 공유한다.</summary>
    [Fact]
    public void NormalLayoutUsesActualRegisteredNodesAndTenRelations()
    {
        var rig = new Rig(); Assert.Equal(8U, rig.Create.Count); Assert.Equal(3, rig.LifetimeState.Overlay.Count(token => token != 0));
        Assert.Equal(new uint[] { rig.Nodes[3].Serial, rig.Nodes[4].Serial, rig.Nodes[5].Serial, rig.Nodes[6].Serial, rig.Nodes[8].Serial, rig.Nodes[7].Serial }, rig.Divider(2).Slaves.Take(6).Select(slot => slot.Serial));
        Assert.Equal(new uint[] { 1, 4, 1, 1, 1, 4 }, rig.Divider(2).Slaves.Take(6).Select(slot => slot.Flags));
        Assert.Equal(new uint[] { 8, 2, 2 }, rig.Divider(3).Slaves.Take(3).Select(slot => slot.Flags)); Assert.Equal(8U, rig.Divider(4).Slaves[0].Flags);
        Assert.Equal((60, 130), (rig.Divider(2).MinX, rig.Divider(2).MaxX)); Assert.Equal((645, 768), (rig.Divider(3).MinY, rig.Divider(3).MaxY)); Assert.Equal((25, 743), (rig.Divider(4).MinY, rig.Divider(4).MaxY));
        Assert.All(rig.Nodes.Where(pair => pair.Key is >= 3 and <= 8), pair => Assert.NotEqual(0U, pair.Value.Flags & 0x40000));
    }
    /// <summary>실제 Move/구분선 가상 Attach/부모 통지가 지도 영역을 쓰고 같은 정렬기가 그 영역을 읽는다.</summary>
    [Fact]
    public void CommonVirtualAttachAndAlignmentReadTheDockedMap()
    {
        var rig = new Rig(); rig.Dock.Handle(0x0200003d, 3, 0); Assert.Equal(new ScreenRect(85, 0, 1024, 672), rig.TreeState.MapView);
        Assert.Equal(1U, rig.Revision.Value); Assert.Equal(1, rig.Finished); Assert.NotEmpty(rig.Dirty); Assert.True(rig.ClosedPopups > 0);
        // 크기가 함께 변하는 종속 패널 대신 별도 실제 등록 표식을 같은 루트에 정렬한다.
        var marker = new GumpCreateNode(); rig.Nodes.Add(9, marker); var factory = new GumpCreate(rig.Create, new() { Node = rig.Node });
        factory.Construct(9, 0, 100, 40, 0x20001); marker.AlignFlags = 0xc0; rig.TreeHooks.Attach!(9, 0, 0, 1, 2);
        Assert.Equal((504F, 316F), rig.Tree.Align(9));
        rig.Tree.Move(2, 120, 0, 0); Assert.Equal(new ScreenRect(125, 0, 1024, 672), rig.Globals.MapRect); Assert.Equal((524F, 316F), rig.Tree.Align(9));
        Assert.Same(rig.Nodes[2], rig.Hooks.Divider!(2)); Assert.Equal(1U, rig.Nodes[2].Parent); Assert.Contains(rig.Redraw, item => item.Token == 2 && !item.Erase);
        Assert.Equal(1U, rig.Nodes[8].Parent); Assert.NotEqual(0U, rig.Nodes[1].Child);
    }
    /// <summary>채팅 표시 설정과 구분선 이동을 구별하고 바뀐 같은 화면 크기로 영역을 갱신한다.</summary>
    [Fact]
    public void ChatToggleUsesLiveScreenAndSavedDividerPosition()
    {
        var rig = new Rig(); rig.Dock.Handle(0x02000088, 0, 0); Assert.False(rig.ChatVisible); Assert.False(rig.ChatInput); Assert.Equal(672, rig.Globals.SavedChatY); Assert.Equal(768, rig.Globals.MapRect.Bottom);
        rig.Create.ScreenHeight = 900; rig.Tree.Move(4, rig.Nodes[4].X, 900, 0); rig.Dock.Handle(0x02000088, 0, 1);
        Assert.True(rig.ChatVisible); Assert.False(rig.ChatInput); Assert.Equal(672, rig.Globals.MapRect.Bottom); Assert.Equal(672, rig.Nodes[4].Y);
        rig.Dock.Handle(0x02000005, 0, 100); Assert.Equal(795, rig.Nodes[4].Y); Assert.Equal(795, rig.TreeState.MapView.Bottom);
    }
    /// <summary>기본/파생 사본 혼용은 배경 효과 뒤 거부하며 이미 쓴 실제 노드와 초기화 전 슬롯은 남긴다.</summary>
    [Fact]
    public void DifferentDerivedStorageFailsAfterBackgroundPrefix()
    {
        var node = new HudDividerNode { MinX = 91 }; node.Slaves[0] = (77, 4); bool activated = false;
        var body = new HudDividerGump(() => 1024, () => 768, () => 55, new()
        {
            Node = _ => node, Divider = _ => new HudDividerNode(), ConstructBackground = (_, width, _, _, _, _) => node.BaseWidth = width, Activate = (_, _) => activated = true,
        });
        Assert.Throws<InvalidOperationException>(() => body.Construct(1, 2, 123, 456, 0, 0, 0)); Assert.Equal(123, node.BaseWidth); Assert.Equal(91, node.MinX); Assert.Equal((77U, 4U), node.Slaves[0]); Assert.False(activated);
    }
    /// <summary>후처리 미설치는 실제 부착/갱신 누적 뒤 실패하며 이전 지도 상태로 되돌리지 않는다.</summary>
    [Fact]
    public void MissingResizeDevicePreservesCompletedDockPrefix()
    {
        var rig = new Rig(); rig.Hooks.FinishResize = null; rig.Revision.Value = uint.MaxValue;
        Assert.Throws<InvalidOperationException>(() => rig.Dock.Handle(0x0200003d, 4, 0));
        Assert.Equal(0U, rig.Revision.Value); Assert.Equal(0, rig.Finished); Assert.Equal(175, rig.Globals.MapRect.Left);
        Assert.Equal(0U, rig.Nodes[6].Flags & 1); Assert.Equal(1U, rig.Nodes[8].Flags & 1); Assert.NotEmpty(rig.Dirty);
    }
}
