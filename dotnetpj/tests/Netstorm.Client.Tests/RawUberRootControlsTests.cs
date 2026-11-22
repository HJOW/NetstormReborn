using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Client.Tests; // 전체 HUD 소유/분배와 같은 공통 트리의 실제 연결을 검증한다.

/// <summary>실제 공통 자산/생성/도킹/입력/수명 조립을 검증한다. 네 패널 공장은 명시 경계이며 실제 게임 UI 공장 실행으로 세지 않는다.</summary>
public sealed class RawUberRootControlsTests
{
    /// <summary>같은 실제 GumpButtonTree와 현재 외부 장치/패널 경계의 검사 장치다.</summary>
    private sealed class Rig : IDisposable
    {
        /// <summary>같은 실제 폰트/배경과 공통 등록/수명/사건/그리기 상태.</summary>
        public readonly GumpButtonTree Tree;
        /// <summary>실제 현재 메뉴 인스턴스 owner.</summary>
        public readonly MenuGumpState Menus = new();
        /// <summary>현재 원문 주소별 명시 전역 입력.</summary>
        public readonly Dictionary<uint, uint> Values = new() { [0x540451] = 43, [0x5411c8] = 82, [0x542664] = 0, [0x565e70] = 0, [0x5949d0] = 0 };
        /// <summary>같은 실제 지도/화면 owner.</summary>
        public readonly HudDockGlobals Globals;
        /// <summary>같은 장기 미니맵 갱신 DWORD.</summary>
        public readonly UIntBox Revision = new();
        /// <summary>현재 실제 바깥 루트 별칭.</summary>
        public uint RootAlias;
        /// <summary>실제 교체 별칭 통지 횟수.</summary>
        public int Replaced;
        /// <summary>실제 UI 변경 통지 횟수.</summary>
        public int Changed;
        /// <summary>화면 후처리 횟수.</summary>
        public int Finished;
        /// <summary>현재 폴링 장치의 release 입력.</summary>
        public uint PollCode;
        /// <summary>실제 폴링 도달 횟수.</summary>
        public int Polled;
        /// <summary>현재 popup 정리 횟수.</summary>
        public int Closed;
        /// <summary>기본 사건 전달 관찰.</summary>
        public readonly List<uint> BaseEvents = [];
        /// <summary>현재 커서 장치 출력.</summary>
        public readonly List<uint> Cursors = [];
        /// <summary>실제 dirty 요청 인자.</summary>
        public readonly List<(int X, int Y, int Width, int Height)> Dirty = [];
        /// <summary>네 명시 패널 공장 호출 순서.</summary>
        public readonly List<string> Factories = [];
        /// <summary>현재 채팅 표시 장치.</summary>
        public bool ChatVisible = true;
        /// <summary>현재 채팅 입력 표시 장치.</summary>
        public bool ChatInput = true;
        /// <summary>실제 전체 조립 입력과 함수 슬롯.</summary>
        public readonly RawUberRootContext Context;
        /// <summary>같은 공통 트리의 전체 부모/구분선 처리기.</summary>
        public readonly RawUberRootControls Controls;
        /// <summary>실제 현재 부모의 같은 확장 필드.</summary>
        public UberRootNode Root => (UberRootNode)Tree.Node(Tree.Create.Entries[0])!;
        /// <summary>현재 실제 구분선 주소.</summary>
        public HudDividerNode Divider(uint token) => (HudDividerNode)Tree.Node(token)!;
        /// <summary>실제 외부 패널 공장 경계의 같은 기본/배경 생성과 등록이다.</summary>
        private uint Panel(string name)
        {
            uint token = Tree.AllocateNode(new GumpBackgroundNode()); Tree.Backgrounds.Construct(token, 0, 0, 0, name == "knowledge" ? 0U : 1U, 0, 0); Factories.Add(name);
            if (name == "minimap") Values[0x5949d0] = token; return token;
        }
        /// <summary>현재 화면/전역은 같은 owner에서 읽는다.</summary>
        private uint Read(uint address) => address switch { 0x5c78f0 or 0x531860 => unchecked((uint)Tree.Create.ScreenWidth), 0x5c78f4 or 0x531864 => unchecked((uint)Tree.Create.ScreenHeight), _ => Values[address] };
        /// <summary>같은 실제 폰트/자산/트리와 명시 외부 장치만 준비하며 전체 부모 생성은 호출자가 선택한다.</summary>
        public Rig(bool replaceAlias = true, bool missingProduction = false)
        {
            var font = new GumpFontFixture("gumptext"); Tree = new(1024, 768, GumpBackgroundTestAssets.Assets(), 82, 0, new(9, 243, 57), font.Fonts,
                new() { Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => BaseEvents.Add(0x7777), Redraw = (_, _) => { }, HandleBase = (_, code, _, _) => { BaseEvents.Add(code); return code == 0x0200002c ? 1U : 0; } });
            Globals = new(Tree.Create, Tree.TreeState, new() { Value = 1 }, Revision);
            Context = new()
            {
                Globals = Globals, Root = new()
                {
                    Read = Read, Write = (address, value) => Values[address] = value, CreateMinimap = () => Panel("minimap"), CreateProduction = missingProduction ? null : () => Panel("production"), CreateKnowledge = () => Panel("knowledge"), CreateChat = () => Panel("chat"),
                    Type = type => { Assert.Equal(82U, type); return type; }, Translate = _ => 0x12345678,
                    Help = (token, value) => { Assert.Equal(0x12345678U, value); Tree.SetHelpText(token, "Chat Window"); }, HelpWord = Tree.SetHelpWord, RefreshTechnology = () => BaseEvents.Add(0x42f540),
                },
                Dock = new() { Invalidate = (x, y, width, height, _) => Dirty.Add((x, y, width, height)), FinishResize = () => Finished++, ChatVisible = value => ChatVisible = value, ChatInput = value => ChatInput = value },
                Poll = code => { Assert.Equal(0x10000U, code); Polled++; return new(PollCode, 0, 0); }, Cursor = value => Cursors.Add(value), BaseHandle = (_, code, _, _) => { BaseEvents.Add(code); return 73; }, ClosePopups = () => Closed++,
                Changed = () => Changed++, Replaced = replaceAlias ? token => { RootAlias = token; Replaced++; } : null,
            };
            Controls = new(Tree, Menus, Context);
        }
        /// <summary>테스트의 실제 전체 UI 수명을 반납한다.</summary>
        public void Dispose() => Controls.Dispose();
    }

    /// <summary>생성자는 노드/등록/게임 전역을 만들지 않고 첫 실제 Replace가 전체 원문을 실행한다.</summary>
    [Fact]
    public void DeferredConstructionUsesOneRealTreeAndRootAlias()
    {
        using var rig = new Rig(); Assert.Equal(1U, rig.Tree.Create.Count); Assert.Empty(rig.Factories); Assert.False(rig.Controls.Current());
        uint button = rig.Tree.AddStone(75, 19, "Main\0"u8.ToArray(), 300, 10); uint oldRoot = rig.Tree.Create.Entries[0]; rig.Controls.Replace();
        Assert.Equal(new[] { "minimap", "production", "knowledge", "chat" }, rig.Factories); Assert.True(rig.Controls.Current()); Assert.Null(rig.Tree.Node(oldRoot)); Assert.Equal(rig.Tree.Create.Entries[0], rig.RootAlias); Assert.Equal(rig.RootAlias, rig.Tree.Create.Collection.Root);
        Assert.Equal(10U, rig.Tree.Create.Count); Assert.Equal(rig.RootAlias, rig.Tree.Node(button)!.Parent); Assert.Equal(3, rig.Tree.LifeState.Overlay.Count(token => token != 0)); Assert.Equal(2, rig.Tree.OwnedTextCount);
        Assert.Equal(button, rig.Tree.Input.Route(InputCode.LeftButton, 301, 11)); Assert.Equal(button, rig.Tree.Input.Route(InputCode.LeftButton | InputCode.Release, 301, 11)); Assert.Contains(0x0200002cU, rig.BaseEvents);
        Assert.Same(rig.Tree.Node(rig.Root.Vertical), rig.Controls.Background(rig.Root.Vertical)); Assert.Equal(1, rig.Replaced); Assert.Equal(1, rig.Changed);
    }
    /// <summary>캡처/프레임/실제 도킹 부모가 같은 상태를 쓰며 release 폴링 뒤 마지막 이동도 반영한다.</summary>
    [Fact]
    public void CapturedDragReachesActualDockAndFinalReleaseMove()
    {
        using var rig = new Rig(); rig.Controls.Replace(); rig.Controls.Handle(rig.RootAlias, 0x0200003d, 3, 0); uint chat = rig.Root.ChatDivider;
        Assert.Equal(chat, rig.Tree.Input.Route(InputCode.LeftButton, 90, 674)); Assert.Equal(chat, rig.Tree.LifeState.Focus); Assert.NotEqual(0U, rig.Divider(chat).Flags & 0x100);
        rig.Tree.Frame.Broadcast(0x02000001, 90, 700); Assert.Equal(696, rig.Divider(chat).Y); Assert.Equal(696, rig.Globals.MapRect.Bottom);
        int polled = rig.Polled; rig.Menus.Instances = 1; rig.Tree.Frame.Broadcast(0, 90, 720); Assert.Equal(polled, rig.Polled); Assert.Equal(696, rig.Divider(chat).Y);
        rig.Menus.Instances = 0; rig.PollCode = InputCode.Release; rig.Tree.Frame.Broadcast(0, 90, 720); Assert.Equal(716, rig.Divider(chat).Y); Assert.Equal(716, rig.Globals.MapRect.Bottom); Assert.Equal(0U, rig.Divider(chat).Flags & 0x100);
        Assert.Equal(chat, rig.Tree.Input.Route(InputCode.LeftButton | InputCode.Release, 90, 720)); Assert.Equal(0U, rig.Tree.LifeState.Focus);
        rig.Controls.Handle(chat, 0x0200003b, 0, 0); Assert.Equal(5U, rig.Cursors[^1]); Assert.True(rig.Closed > 0); Assert.NotEmpty(rig.Dirty);
    }
    /// <summary>Recreate는 기존 자식/소유 문자열/overlay를 실제 반납하고 새 부모만 같은 등록표에 남긴다.</summary>
    [Fact]
    public void RecreateAndDisposeReleaseTheSamePhysicalStorage()
    {
        var rig = new Rig(); rig.Controls.Replace(); uint oldRoot = rig.RootAlias, oldChat = rig.Root.Chat; rig.Controls.Recreate(); Assert.NotEqual(oldRoot, rig.RootAlias); Assert.Null(rig.Tree.Node(oldRoot)); Assert.Null(rig.Tree.Node(oldChat)); Assert.Equal(8U, rig.Tree.Create.Count); Assert.Equal(2, rig.Tree.OwnedTextCount);
        rig.Controls.Dispose(); rig.Controls.Dispose(); Assert.Equal(0U, rig.Tree.Create.Count); Assert.Equal(0U, rig.Tree.Create.Collection.Root); Assert.Equal(0, rig.Tree.OwnedTextCount); Assert.Empty(rig.Tree.OwnedNodes(rig.Controls)); Assert.False(rig.Controls.Current());
        Assert.Throws<ObjectDisposedException>(rig.Controls.Replace); Assert.All(rig.Tree.LifeState.Overlay, token => Assert.Equal(0U, token));
    }
    /// <summary>미니맵의 옛 도킹 포인터와 현재 표시 포인터의 차이만 허용하며 다른 빠진 필수 자식은 거부한다.</summary>
    [Fact]
    public void CurrentDistinguishesClosedMinimapFromMissingRequiredPanel()
    {
        using var rig = new Rig(); rig.Controls.Replace(); uint mini = rig.Root.Minimap; rig.Tree.Lifetime.Kill(mini); rig.Tree.Lifetime.Cleanup(); Assert.False(rig.Controls.Current()); rig.Values[0x5949d0] = 0; Assert.True(rig.Controls.Current());
        rig.Tree.Lifetime.Kill(rig.Root.Production); rig.Tree.Lifetime.Cleanup(); Assert.False(rig.Controls.Current());
    }
    /// <summary>실제 serial 해결은 등록0/죽은 항목을 제외하고 같은 현재 등록표의 첫 일치만 반환한다.</summary>
    [Fact]
    public void ReferenceSkipsRootAndDeadEntries()
    {
        using var rig = new Rig(); rig.Controls.Replace(); rig.Root.Serial = 777; Assert.Equal(0U, rig.Controls.Reference(777)); uint a = rig.Root.Minimap, b = rig.Root.Production; rig.Tree.Node(a)!.Serial = 777; rig.Tree.Node(b)!.Serial = 777;
        Assert.Equal(a, rig.Controls.Reference(777)); rig.Tree.Node(a)!.Flags |= 0x10000; Assert.Equal(b, rig.Controls.Reference(777)); rig.Tree.Node(a)!.Flags &= ~0x10000U;
    }
    /// <summary>현재 화면 크기를 바꾸면 하단 거리/숨긴 채팅 복원 위치와 같은 지도 영역이 함께 갱신된다.</summary>
    [Fact]
    public void ReflowPreservesBottomDistanceAndHiddenChatRestore()
    {
        using var rig = new Rig(); rig.Controls.Replace(); rig.Controls.Handle(rig.RootAlias, 0x0200003d, 3, 0); rig.Controls.Handle(rig.RootAlias, 0x02000088, 0, 0); Assert.Equal(672, rig.Globals.SavedChatY);
        rig.Tree.Create.ScreenWidth = 1280; rig.Tree.Create.ScreenHeight = 800; rig.Controls.Reflow(); Assert.Equal(800, rig.Divider(rig.Root.ChatDivider).Y); Assert.Equal(704, rig.Globals.SavedChatY); Assert.Equal(720, rig.Divider(rig.Root.MinimapDivider).Y); Assert.Equal(1280, rig.Globals.MapRect.Right); Assert.Equal(800, rig.Globals.MapRect.Bottom);
        rig.Controls.Handle(rig.RootAlias, 0x02000088, 0, 1); Assert.Equal(704, rig.Globals.MapRect.Bottom); int finished = rig.Finished, changed = rig.Changed; rig.Controls.Reflow(); Assert.Equal(finished, rig.Finished); Assert.Equal(changed, rig.Changed);
    }
    /// <summary>부분 생성 실패의 등록된 부모/구분선을 정리하면서 이전 루트/버튼과 그 입력을 보존한다.</summary>
    [Fact]
    public void FailedFactoryDisposalKeepsPreviousTreeAndDispatch()
    {
        var rig = new Rig(missingProduction: true); uint button = rig.Tree.AddStone(75, 19, "Main\0"u8.ToArray(), 300, 10); uint previousRoot = rig.Tree.Create.Entries[0]; Assert.Throws<InvalidOperationException>(rig.Controls.Replace); Assert.False(rig.Controls.Current());
        rig.Controls.Dispose(); Assert.Equal(previousRoot, rig.Tree.Create.Entries[0]); Assert.Equal(3U, rig.Tree.Create.Count); Assert.NotNull(rig.Tree.Node(button)); Assert.Empty(rig.Tree.OwnedNodes(rig.Controls));
        Assert.Equal(button, rig.Tree.Input.Route(InputCode.LeftButton, 301, 11)); Assert.Equal(button, rig.Tree.Input.Route(InputCode.LeftButton | InputCode.Release, 301, 11));
    }
    /// <summary>별칭 장치 미설치는 공통 루트 교체 후 실패하며 그 부분 효과를 되돌리지 않는다.</summary>
    [Fact]
    public void MissingAliasDeviceFailsAfterRealRootReplacement()
    {
        using var rig = new Rig(replaceAlias: false); uint old = rig.Tree.Create.Entries[0]; Assert.Throws<InvalidOperationException>(rig.Controls.Replace); Assert.Null(rig.Tree.Node(old)); Assert.True(rig.Controls.Current()); Assert.Equal(8U, rig.Tree.Create.Count); Assert.Equal(0, rig.Changed); Assert.Equal(0U, rig.RootAlias);
    }
    /// <summary>같은 값인 다른 지도 owner도 설치 전 거부하고 기존 분배/노드를 변경하지 않는다.</summary>
    [Fact]
    public void RejectsDifferentViewOwnerBeforeBinding()
    {
        using var rig = new Rig(); var original = rig.Tree.TreeHooks.Attach; var context = new RawUberRootContext
        {
            Globals = new(rig.Tree.Create, new() { MapView = rig.Tree.TreeState.MapView }, new(), new()), Root = rig.Context.Root, Dock = rig.Context.Dock, Poll = rig.Context.Poll, Cursor = rig.Context.Cursor, BaseHandle = rig.Context.BaseHandle, ClosePopups = rig.Context.ClosePopups, Changed = rig.Context.Changed,
        };
        Assert.Throws<ArgumentException>(() => new RawUberRootControls(rig.Tree, rig.Menus, context)); Assert.Same(original, rig.Tree.TreeHooks.Attach); Assert.Equal(1U, rig.Tree.Create.Count);
    }
    /// <summary>Source의 장치 구조체 값 복사는 외부 함수 슬롯 교체와 구별하고 실제 callback 상태/owner는 계속 공유한다.</summary>
    [Fact]
    public void DeviceSlotsAreCopiedWhileCurrentOwnerStateRemainsShared()
    {
        using var rig = new Rig(); rig.Context.Root.CreateProduction = null; rig.Context.Root.Read = _ => throw new InvalidOperationException("교체된 외부 함수 슬롯"); rig.Context.Dock.FinishResize = () => throw new InvalidOperationException("교체된 외부 후처리 슬롯");
        rig.Controls.Replace(); Assert.True(rig.Controls.Current()); rig.Tree.Create.ScreenHeight = 800; rig.Controls.Reflow(); Assert.Equal(1, rig.Finished); Assert.Equal(800, rig.Divider(rig.Root.Vertical).BaseHeight); Assert.Contains("production", rig.Factories);
    }
    /// <summary>공통 상태를 쓰는 관찰/그리기는 실제 자기 노드만 읽고 부모 Draw는 Source처럼 비어 있다.</summary>
    [Fact]
    public void PointReportAndDrawUseCurrentOwnedNodes()
    {
        using var rig = new Rig(); rig.Controls.Replace(); rig.Controls.Handle(rig.RootAlias, 0x0200003d, 3, 0); int fills = 0; var background = new GumpBackgroundDraw(rig.Tree.Collection, new(), new() { Debug = true }, new() { Node = token => rig.Tree.Node(token) as GumpBackgroundNode, Fill = (_, _, _) => fills++ });
        Assert.True(rig.Controls.TryDraw(rig.RootAlias, background)); Assert.Equal(0, fills); Assert.True(rig.Controls.TryDraw(rig.Root.Vertical, background)); Assert.Equal(1, fills); Assert.False(rig.Controls.TryDraw(rig.Root.Production, background)); Assert.Null(rig.Controls.Point("unknown"));
        ScreenPoint point = rig.Controls.Point("hud:vertical")!.Value; Assert.Equal(new ScreenPoint(82, 384), point); string report = rig.Controls.Report(); Assert.Contains("raw_uber_children\t", report); Assert.Equal(3, report.Split('\n').Count(line => line.StartsWith("raw_hud_divider\t", StringComparison.Ordinal)));
        Assert.Equal(85, rig.Globals.MapRect.Left); Assert.Equal(672, rig.Globals.MapRect.Bottom);
    }
    /// <summary>자기 등록 밖의 물리 저장소는 전체 UI 반납에도 그대로 보존한다.</summary>
    [Fact]
    public void DisposalPreservesUnrelatedUnregisteredStorage()
    {
        var rig = new Rig(); uint other = rig.Tree.AllocateNode(new GumpCreateNode()); rig.Tree.Factory.Construct(other, 0, 1, 1, 0x100001); rig.Controls.Replace(); rig.Controls.Dispose(); Assert.NotNull(rig.Tree.Node(other)); Assert.Equal(0U, rig.Tree.Create.Count);
    }
}
