using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Tests; // 실제 파일/공통 트리/원시 출력과 큐·소유 수명을 함께 검사한다.

/// <summary>창/게임 루프 없이 같은 실제 기본/파생 노드와 원본 자산을 연결한다.</summary>
public sealed class BackgroundGifGumpWidgetTests
{
    /// <summary>전체 메뉴 소유자를 대신하지 않는 명시 헤드리스 조립 입력이다.</summary>
    private sealed class Scene
    {
        /// <summary>동일 객체를 사용하는 실제 공통 UI 장치.</summary>
        internal ScrollWidgetScene Common { get; } = new();
        /// <summary>보호 파일의 읽기 전용 VFS.</summary>
        internal GameFileSystem Files { get; } = GameFileSystem.Open(Path.Combine(BackgroundGifAssetsNativeTests.Root(), "originals"));
        /// <summary>시험에서 명시한 경로 치환 입력이다.전체 ConfigInterface 시작 복원은 후속이다.</summary>
        internal ConfigStore Config { get; } = new();
        /// <summary>행 끝7바이트까지 검사하는 실제 표면.</summary>
        internal byte[] Pixels { get; } = Enumerable.Repeat((byte)0x5a, 647 * 480).ToArray();
        /// <summary>실제 원시 그림 소유 저장소.</summary>
        internal BackgroundGifStorage Storage { get; }
        /// <summary>원문 위젯 본체.</summary>
        internal BackgroundGifGump Body { get; }
        /// <summary>정규화하지 않은 실제 원시 큐 관측.</summary>
        internal List<(uint Code, uint First, uint Second)> Queued { get; } = [];
        /// <summary>기본 처리기 경계에 도달한 나머지 입력 관측.</summary>
        internal List<uint> Delegated { get; } = [];
        /// <summary>큐 호출 중 즉시 해제할 명시 재진입 입력.</summary>
        internal uint DestroyOnQueue { get; set; }
        /// <summary>생성 때 쓰는 단조증가 노드 토큰.</summary>
        private uint _next = 30;
        /// <summary>실제 공통 생성/파일/픽셀/사건/직접 입력/scalar 소멸 분배를 연결한다.</summary>
        internal Scene()
        {
            Common.Create.ScreenWidth = 640; Common.Create.ScreenHeight = 480; Common.Nodes[1].Width = Common.Nodes[1].BaseWidth = 640; Common.Nodes[1].Height = Common.Nodes[1].BaseHeight = 480; Config.Push(new ConfigText("BackgroundGifSpec = \"d/{local.1}.gif\"\n")); Assert.Equal("d/titleMenu.gif", Config.ExpandSpec("BackgroundGifSpec", "titleMenu"));
            var hooks = new BackgroundGifGumpHooks { Node = token => Common.Nodes.GetValueOrDefault(token) as BackgroundGifGumpNode, Base = token => Common.Nodes.GetValueOrDefault(token), PathSpec = name => Config.ExpandSpec("BackgroundGifSpec", name), Exists = Files.Exists, Attach = Common.Tree.Attach, BaseHandle = (_, code, _, _) => { Delegated.Add(code); return 0; }, Dispatch = (code, first, second) => { Queued.Add((code, first, second)); if (DestroyOnQueue != 0) { uint token = DestroyOnQueue; DestroyOnQueue = 0; Common.Lifetime.Kill(token); Common.Lifetime.Cleanup(); } } };
            BackgroundGifGump.BindBackgroundGifGumpDevices(hooks, Common.Factory, Common.Lifetime, Common.Collection); Storage = new(Files.ReadAllBytes); Storage.Bind(hooks, new(Pixels, 647, 480, Common.Clip)); Body = new(new(), hooks); BackgroundGifGump.BindBackgroundGifGump(Common.EventHooks, Common.InputHooks, Common.DrawHooks, Common.LifetimeHooks, token => Common.Nodes.GetValueOrDefault(token) is BackgroundGifGumpNode ? Body : null, token => { Common.Released.Add(token); Assert.True(Common.Nodes.Remove(token)); }); GumpDraw.SetGumpDrawClip(Common.Clip, new(0, 0, 640, 480));
        }
        /// <summary>노드를 동일 저장소에 넣고 원문 생성만 실행한다.부분 생성 예외 때 노드도 남긴다.</summary>
        internal uint Create(string name) { uint token = _next++; Common.Nodes.Add(token, new BackgroundGifGumpNode()); Body.Construct(token, name); return token; }
        /// <summary>현재 동일 파생 노드를 조회한다.</summary>
        internal BackgroundGifGumpNode Node(uint token) => (BackgroundGifGumpNode)Common.Nodes[token];
        /// <summary>현재 등록된 GIF 노드만 자식 우선 지연 삭제로 회수한다.</summary>
        internal void Close()
        {
            // 생성 예외로 남은 노드도 같은 공통 수명 경로로 반납한다.
            foreach (uint token in Common.Nodes.Where(pair => pair.Value is BackgroundGifGumpNode).Select(pair => pair.Key).ToArray()) Common.Lifetime.Kill(token);
            Common.Lifetime.Cleanup();
        }
    }
    /// <summary>동시 생성/교체/없는 파일·잘못된 GIF의 부분 생성과 공통 지연 수명을 확인한다.</summary>
    [Fact]
    public void ActualVfsConcurrentStorageRefreshMissingAndPartialDecodeLifetime()
    {
        var scene = new Scene(); uint first = scene.Create("titleMenu"), second = scene.Create("defeat"); var a = scene.Node(first); var b = scene.Node(second); Assert.Equal((0x501b60U, 3U, 9U, 0U, 0U, 0U), (a.Vtable, a.Command, a.Flags, a.Event, a.First, a.Second)); Assert.Same(a, scene.Common.Nodes[first]); Assert.Equal((639f, 480f, 640f, 480f), (a.BaseWidth, a.BaseHeight, b.BaseWidth, b.BaseHeight)); Assert.NotEqual(a.Bitmap, b.Bitmap); Assert.Equal(2, scene.Storage.ImageCount);
        uint old = b.Bitmap; scene.Body.Refresh(second, "victory"); Assert.NotEqual(old, b.Bitmap); Assert.Throws<ArgumentOutOfRangeException>(() => scene.Storage.Bitmap(old)); Assert.Equal(2, scene.Storage.ImageCount); scene.Common.Lifetime.Kill(first); scene.Common.Lifetime.Cleanup(); Assert.False(scene.Common.Nodes.ContainsKey(first)); Assert.Equal(1, scene.Storage.ImageCount);
        scene.Body.Refresh(second, "nonexistent-background-402"); Assert.Equal((0U, 0f, 0f, 0f, 0f), (b.Bitmap, b.BaseWidth, b.BaseHeight, b.Width, b.Height)); Assert.Equal(0, scene.Storage.ImageCount); scene.Config.Push(ConfigStore.FromPairs([new("BackgroundGifSpec", "d/setup.cfg")])); Assert.Throws<InvalidDataException>(() => scene.Create("invalid")); Assert.Equal(3U, scene.Common.Create.Count); Assert.Equal(0, scene.Storage.ImageCount); scene.Close(); Assert.Equal(1U, scene.Common.Create.Count); Assert.Equal(3, scene.Common.Released.Count); Assert.Equal(0, scene.Storage.ImageCount); Assert.Throws<InvalidOperationException>(() => scene.Storage.Release(old)); Assert.Throws<InvalidOperationException>(() => scene.Storage.Release(0));
    }
    /// <summary>실제 위치/현재 클립과 base 치수로 복사하고 배율/마스크와 행 padding은 구분한다.</summary>
    [Fact]
    public void ActualCommonDrawUsesRawBaseDimensionsCurrentClipAndPadding()
    {
        var scene = new Scene(); uint token = scene.Create("titleMenu"); scene.Common.Tree.Attach(token, 11, 13, 1, 1); var node = scene.Node(token); node.Width = 320; node.Height = 240; var bitmap = scene.Storage.Bitmap(node.Bitmap); Array.Fill(bitmap.Opacity, (byte)0); scene.Common.Draw.Draw(token); byte[] full = scene.Pixels.ToArray(); var clip = new ScreenRect(29, 31, 603, 459); GumpDraw.SetGumpDrawClip(scene.Common.Clip, clip); Array.Fill(scene.Pixels, (byte)0x5a); scene.Common.Draw.Draw(token); int changed = 0;
        // 화면 전체와 행 padding을 직접 소스 색 번호/명시 클립에 대조한다.
        for (int y = 0; y < 480; y++)
        {
            // 공통 출력 폭 밖의7바이트는 언제나 초기 값을 유지해야 한다.
            for (int x = 0; x < 647; x++)
            {
                bool inside = x >= 11 && x < 640 && y >= 13; byte expected = inside ? bitmap.Indices[(y - 13) * bitmap.Width + x - 11] : (byte)0x5a; Assert.Equal(expected, full[y * 647 + x]); bool inClip = x >= clip.Left && x < clip.Right && y >= clip.Top && y < clip.Bottom; Assert.Equal(inClip ? expected : (byte)0x5a, scene.Pixels[y * 647 + x]); if (scene.Pixels[y * 647 + x] != 0x5a) changed++;
            }
        }
        Assert.True(changed > 0); Assert.Equal(bitmap.Indices[(400 - 13) * bitmap.Width + 500 - 11], full[400 * 647 + 500]); scene.Close(); Assert.Equal(0, scene.Storage.ImageCount);
    }
    /// <summary>실제 공통 분배/수정자/해제 구분과 큐 호출 중 삭제 후 재조회 금지를 확인한다.</summary>
    [Fact]
    public void ActualInputPreservesRawTripletAndDispatchCanReleaseCurrentNode()
    {
        var scene = new Scene(); uint token = scene.Create("defeat"); scene.Common.Tree.Attach(token, 0, 0, 1, 0); scene.Body.SetEvent(token, 0xfedcba98, 0x87654321, 0xabcdef01); Assert.Equal(0x600U, scene.Node(token).Flags & 0x600); Assert.Equal(1U, scene.Common.InputHooks.Handle!(token, InputCode.LeftButton, 123, 456)); Assert.Equal(1U, scene.Common.EventHooks.Handle!(token, InputCode.RightButton | InputCode.Shift, 123, 456)); Assert.Equal(1U, scene.Common.InputHooks.Handle!(token, 27 | InputCode.Control, 123, 456));
        scene.Common.InputHooks.Handle!(token, 27 | InputCode.Release, 1, 2); scene.Common.InputHooks.Handle!(token, 27 | InputCode.Command, 1, 2); scene.Common.InputHooks.Handle!(token, 'A', 1, 2); Assert.Equal(3, scene.Queued.Count); Assert.Equal(new[] { 27 | InputCode.Release, 27 | InputCode.Command, (uint)'A' }, scene.Delegated); Assert.Equal(token, scene.Common.Input.Route(27, 5, 7)); Assert.Equal(4, scene.Queued.Count); scene.DestroyOnQueue = token; Assert.Equal(1U, scene.Common.InputHooks.Handle!(token, 27, 5, 7)); Assert.False(scene.Common.Nodes.ContainsKey(token)); Assert.Equal(1U, scene.Common.Create.Count); Assert.Equal(0, scene.Storage.ImageCount); Assert.Equal(5, scene.Queued.Count); Assert.All(scene.Queued, item => Assert.Equal((0xfedcba98U, 0x87654321U, 0xabcdef01U), item));
    }
    /// <summary>변경된 원시 stride/보이는 할당 밖 주소만 물리 경계에서 읽는다.</summary>
    [Fact]
    public void RawCopyHonorsFlatSourceStrideAndOnlyVisibleOutsideReads()
    {
        byte[] source = GameFileSystem.Open(Path.Combine(BackgroundGifAssetsNativeTests.Root(), "originals")).ReadAllBytes("d/titleMenu.gif"); var reads = new List<long>(); var storage = new BackgroundGifStorage(_ => source, (token, offset) => { Assert.NotEqual(0U, token); reads.Add(offset); return 213; }); int width = 0, height = 0; uint bitmap = storage.Load("explicit-input", ref width, ref height); byte[] pixels = new byte[4]; var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 1, 1)); var target = new GumpPixelTarget(pixels, 2, 2, clip); storage.Copy(new(new(0, 0), bitmap, width + 1, new(1, 0, 3, 2)), target); Assert.Equal(storage.Bitmap(bitmap).Indices[1], pixels[0]); Assert.Empty(reads);
        long outside = (long)width * height; storage.Copy(new(new(0, 0), bitmap, checked((int)outside), new(0, 1, 2, 3)), target); Assert.Equal(new[] { outside }, reads); Assert.Equal(213, pixels[0]); Assert.Equal(new byte[3], pixels.AsSpan(1).ToArray()); storage.Release(bitmap); Assert.Equal(0, storage.ImageCount);
    }
}
