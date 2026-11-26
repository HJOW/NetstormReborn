using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Chat;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 실제 자산/공통 트리/Console/공유 로그에서 창별 닫기와 문자열 반납을 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>새 소유권 경계를 실제 기존 호스트 장치와 함께 검증한다.</summary>
public sealed class HostGumpOwnershipTests
{
    /// <summary>창 없이 실제 설치 자산과 같은 공통 UI를 조립한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>실제 팔레트와 픽셀을 가진 논리 화면이다.</summary>
        public readonly Screen Screen;
        /// <summary>실제 메인 메뉴와 공통 트리 소유자다.</summary>
        public readonly HostMainMenu Menu;
        /// <summary>기존 HTML의 현재 닫기 소유자다.</summary>
        public readonly HostHtmlWindows Windows;
        /// <summary>같은 실제 조각/글꼴/공유 로그 저장소다.</summary>
        public readonly HostHtmlText Text;
        /// <summary>같은 실제 편집 버퍼와 Console 처리기다.</summary>
        public readonly HostTextEntry Entry;
        /// <summary>같은 실제 공통 저장소다.</summary>
        public GumpButtonTree Tree => Menu.Tree;
        /// <summary>실제 원본 자료를 읽되 설정 저장/창/소리는 실행하지 않는다.</summary>
        public Scene()
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var startup = new HostStartupScene(data); Screen = new(1024, 768); Assert.True(Screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(Screen);
            Menu = new(startup, Screen, new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 }); Windows = new(Menu, new InputQueue(), () => 1, _ => { }); Text = new(Menu, Windows, Screen, _ => default, (_, _) => { }, _ => { }, _ => { }); Text.Bind(Windows); Entry = new(Menu, Windows, Text, Screen, () => 1, (_, _) => { });
        }
        /// <summary>기존 HTML과 실제 편집 버퍼 잔여를 정상 수명으로 반납한다.</summary>
        public void Dispose() { Windows.CloseAll(); Entry.Dispose(); }
    }
    /// <summary>메뉴·공통 정렬·HUD가 원본 BSS0에서 시작한 같은 현재 지도 사각형을 읽고 쓴다.</summary>
    [Fact]
    public void CurrentMapViewBorrowsSameDockAndAlignmentStorage()
    {
        using var scene = new Scene(); Assert.Equal(default(ScreenRect), scene.Menu.CurrentMapView);
        var dock = new HudDockGlobals(scene.Tree.Create, scene.Tree.TreeState, new(), new());
        dock.MapRect = new(85, 7, 1024, 672); Assert.Equal(dock.MapRect, scene.Menu.CurrentMapView);
        scene.Tree.TreeState.MapView = new(103, 11, 900, 599); Assert.Equal(scene.Tree.TreeState.MapView, dock.MapRect);
        Assert.Equal(dock.MapRect, scene.Menu.CurrentMapView);
        scene.Tree.Create.ScreenWidth = 1920; Assert.Equal(new ScreenRect(103, 11, 900, 599), scene.Menu.CurrentMapView);
    }
    /// <summary>다른 창의 실제 Console/Slider/화살표가 HTML 닫기에서 보존되고 자기 수명에서만 반납된다.</summary>
    [Fact]
    public void ActualConsoleAndNestedChildrenSurviveHtmlClose()
    {
        using var scene = new Scene(); object owner = new(); uint token = scene.Tree.WithNodeOwner(owner, () =>
        {
            uint allocated = scene.Entry.AllocateConsole(); scene.Entry.Entry.ConstructConsole(allocated, 200, 30, null, 0, 0, scene.Screen.Colors().Basic[7]); scene.Tree.TreeHooks.Attach!(allocated, 10, 10, scene.Tree.Create.Entries[0], 0); return allocated;
        });
        var node = Assert.IsType<ConsoleGumpNode>(scene.Tree.Node(token)); uint bufferToken = node.Buffer; var buffer = scene.Entry.FindBuffer(bufferToken); Assert.NotNull(buffer); Assert.True(scene.Tree.OwnedNodes(owner).Length > 2); scene.Windows.CloseAll(); Assert.Same(node, scene.Tree.Node(token)); Assert.Same(buffer, scene.Entry.FindBuffer(bufferToken)); Assert.Equal(1u, scene.Tree.InputHooks.Handle!(token, 'A', 0, 0)); Assert.Equal("A", Encoding.Latin1.GetString(buffer!.Text())); scene.Tree.Lifetime.Kill(token); scene.Tree.Lifetime.Cleanup(); Assert.Null(scene.Tree.Node(token)); Assert.Null(scene.Entry.FindBuffer(bufferToken)); Assert.Empty(scene.Tree.OwnedNodes(owner));
    }
    /// <summary>공통 도움말 교체는 cpppj 1363 순서대로 이전 주소를 반납하고 LOOK 0x1000/HELP 0x200을 함께 갱신한다. 빈 LOOK는 0x1000을 끄고, 빈 HELP는 0x200을 유지한 채 포인터만 반납하며, 첫 NUL 뒤는 소유하지 않는다. 이 검사는 2026-11-26 DN-MP-01에서 교정했다.</summary>
    [Fact]
    public void CommonOwnedHelpTextUsesNativeRangeAndScalarRelease()
    {
        using var scene = new Scene(); uint token = scene.Tree.AddStone(50, 20, "Text\0"u8.ToArray(), 10, 10); var node = scene.Tree.Node(token)!; uint serial = node.Serial; uint baseFlags = node.Flags;
        Assert.Equal(0u, node.TextPointer); Assert.Equal(0u, node.AlternateTextPointer); Assert.Equal(0, scene.Tree.OwnedTextCount);
        scene.Tree.SetHelpText(token, "Chat Helper"); uint first = node.TextPointer; Assert.Equal(0x61100001u, first); Assert.Equal(1, scene.Tree.OwnedTextCount);
        Assert.Equal("Chat Helper", scene.Tree.ReadOwnedText(node.TextPointer)); Assert.NotEqual(0u, node.Flags & 0x1000U); Assert.Equal(baseFlags & ~0x1200U, node.Flags & ~0x1200U);
        scene.Tree.SetHelpWord(token, "chathelper"); Assert.Equal(0x61100002u, node.AlternateTextPointer); Assert.Equal(2, scene.Tree.OwnedTextCount);
        Assert.Equal("chathelper", scene.Tree.ReadOwnedText(node.AlternateTextPointer)); Assert.NotEqual(0u, node.Flags & 0x200U); Assert.NotEqual(0u, node.Flags & 0x1000U);
        Assert.Equal(first, node.TextPointer); Assert.Equal(baseFlags & ~0x1200U, node.Flags & ~0x1200U);
        scene.Tree.SetHelpText(token, "Changed"); Assert.NotEqual(first, node.TextPointer); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Tree.ReadOwnedText(first)); Assert.Equal("Changed", scene.Tree.ReadOwnedText(node.TextPointer)); Assert.Equal(2, scene.Tree.OwnedTextCount); Assert.NotEqual(0u, node.Flags & 0x1000U);
        uint wordPointer = node.AlternateTextPointer; scene.Tree.SetHelpWord(token, ""); Assert.Equal(0u, node.AlternateTextPointer); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Tree.ReadOwnedText(wordPointer)); Assert.Equal(1, scene.Tree.OwnedTextCount);
        Assert.NotEqual(0u, node.Flags & 0x200U); Assert.NotEqual(0u, node.Flags & 0x1000U);
        uint textPointer = node.TextPointer; scene.Tree.SetHelpText(token, ""); Assert.Equal(0u, node.TextPointer); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Tree.ReadOwnedText(textPointer)); Assert.Equal(0, scene.Tree.OwnedTextCount);
        Assert.Equal(0u, node.Flags & 0x1000U); Assert.NotEqual(0u, node.Flags & 0x200U); Assert.Equal(baseFlags & ~0x1200U, node.Flags & ~0x1200U);
        scene.Tree.SetHelpText(token, "ab\0cd"); Assert.Equal("ab", scene.Tree.ReadOwnedText(node.TextPointer)); Assert.NotEqual(0u, node.Flags & 0x1000U);
        uint nulText = node.TextPointer; scene.Tree.SetHelpText(token, "\0xyz"); Assert.Equal(0u, node.TextPointer); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Tree.ReadOwnedText(nulText)); Assert.Equal(0u, node.Flags & 0x1000U);
        scene.Tree.SetHelpWord(token, "ef\0gh"); Assert.Equal("ef", scene.Tree.ReadOwnedText(node.AlternateTextPointer)); Assert.NotEqual(0u, node.Flags & 0x200U);
        Assert.Equal(serial, node.Serial); scene.Tree.Lifetime.Kill(token); scene.Tree.Lifetime.Cleanup(); Assert.Equal(0, scene.Tree.OwnedTextCount); Assert.Null(scene.Tree.Node(token));
    }
    /// <summary>실제 공유128줄 객체를 같은 Store와 반납 추적이 함께 보관하며 HTML 닫기가 지우지 않는다.</summary>
    [Fact]
    public void ActualSharedLogRetiresThroughSameStoreWithoutHtmlLifetime()
    {
        using var scene = new Scene(); var globals = new ClientChatWindowGlobals(); var runtime = new ClientChatWindowRuntime(globals, new(OriginalEdition.Patch1078, () => 100)); object owner = new(); runtime.BindSharedLog(owner, scene.Text.Buffers); scene.Text.TrackBorrowedBuffer(0x548758, globals.Log); Assert.Throws<ArgumentException>(() => scene.Text.TrackBorrowedBuffer(0x548758, new())); Assert.Throws<ArgumentException>(() => scene.Text.TrackBorrowedBuffer(0x548758, globals.Log)); var binding = scene.Text.Buffers.Binding(0x548758); Assert.Same(globals.Log, binding.State); uint[] context = [0, 0, scene.Screen.Colors().Basic[7], 0, 0, 0, 1, 1]; binding.Handler.AppendText("A\n\0"u8, context); uint html = scene.Windows.Create(200, 120); Assert.NotNull(scene.Tree.Node(html)); scene.Windows.CloseAll(); Assert.Null(scene.Tree.Node(html)); Assert.True(globals.Log.Initialized); Assert.Equal(1, globals.Log.LineCount); runtime.ReleaseSharedLog(owner, scene.Text.Buffers); Assert.False(globals.Log.Initialized); Assert.Null(globals.UiOwner); Assert.Throws<InvalidOperationException>(() => scene.Text.Buffers.Binding(0x548758));
    }
    /// <summary>중첩 생성과 예외 후 현재 소유자가 복원되고 미등록 부분 노드도 같은 owner에서 조회/반납한다.</summary>
    [Fact]
    public void NestedOwnerScopeRestoresAfterPartialFailure()
    {
        using var scene = new Scene(); object outer = new(), inner = new(); uint first = 0, second = 0, third = 0;
        Assert.Throws<OperationCanceledException>(() => scene.Tree.WithNodeOwner<int>(outer, () =>
        {
            first = scene.Tree.AllocateNode(new()); Assert.Throws<OperationCanceledException>(() => scene.Tree.WithNodeOwner<int>(inner, () => { second = scene.Tree.AllocateNode(new()); throw new OperationCanceledException(); })); third = scene.Tree.AllocateNode(new()); throw new OperationCanceledException();
        }));
        uint after = scene.Tree.AllocateNode(new()); Assert.Same(outer, scene.Tree.NodeOwner(first)); Assert.Same(inner, scene.Tree.NodeOwner(second)); Assert.Same(outer, scene.Tree.NodeOwner(third)); Assert.Null(scene.Tree.NodeOwner(after)); Assert.Equal(new[] { first, third }, scene.Tree.OwnedNodes(outer));
        // 미등록 부분 생성도 각 실제 주소를 반납한 뒤 소유 표에서 제거한다.
        foreach (uint token in new[] { first, second, third, after }) scene.Tree.ReleaseUnregistered(token);
        Assert.Empty(scene.Tree.OwnedNodes(outer)); Assert.Empty(scene.Tree.OwnedNodes(inner));
    }
}
