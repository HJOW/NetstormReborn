using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 공통 트리/MetaImage/현재 픽셀/부모 사건/지연 수명 통합 검사다.

/// <summary>목록의 상태 계산을 대신하지 않고 실제 기존 처리기를 조립한다.</summary>
public sealed class MetaImageListWidgetTests
{
    /// <summary>같은 실제 기본/파생/픽셀을 묶는 검사 장면.</summary>
    private sealed class Scene
    {
        /// <summary>실제 공통 트리/등록/가상 분배/수명.</summary>
        public ScrollWidgetScene Common { get; } = new();
        /// <summary>같은 실제 목록 파생.</summary>
        public MetaImageListNode Node { get; } = new();
        /// <summary>같은 실제 동적그림 본체.</summary>
        public MetaImageGump Meta { get; }
        /// <summary>같은 실제 목록 본체.</summary>
        public MetaImageList List { get; }
        /// <summary>현재 실제 목록 장치.콜백 중 상태 변경을 허용한다.</summary>
        public MetaImageListHooks Hooks { get; }
        /// <summary>현재 녹색은 전체DWORD로 읽어 실제 픽셀에서만BYTE가 된다.</summary>
        public uint Green { get; set; } = 0x1234562b;
        /// <summary>실제 자식 할당 순서.</summary>
        public List<uint> Allocated { get; } = [];
        /// <summary>실제 반납 순서.</summary>
        public List<uint> Released { get; } = [];
        /// <summary>다음 실제 MetaImage 토큰.</summary>
        private uint _next = 100;
        /// <summary>호출자가 소유할 동적 Image를 실제 비트맵에서 만든다.</summary>
        public static RawImage Image(bool empty = false) { var image = new RawImage(); image.Convert(new(6, 4, Enumerable.Repeat(empty ? (byte)0 : (byte)7, 24).ToArray()), false); return image; }
        /// <summary>같은 실제 기본 등록표에 MetaImage를 생성한다.</summary>
        private uint CreateImage(RawImage image) { uint token = _next++; Common.Nodes.Add(token, new MetaImageGumpNode()); Allocated.Add(token); return Meta.Construct(token, image); }
        /// <summary>현재 실제 저장소 반납을 공통 수명 분배와 함께 기록한다.</summary>
        private void Release(uint token) { Released.Add(token); Common.Released.Add(token); Common.Nodes.Remove(token); }
        /// <summary>기본/목록/MetaImage의 실제 부착·출력/입력/수명을 같은 노드에 연결한다.</summary>
        public Scene()
        {
            Common.Nodes.Add(30, Node); var pixels = new GumpPixelTarget(Common.Pixels, 64, 128, Common.Clip); Common.LifetimeHooks.Release = Release; var meta = MetaImageGump.BindMetaImagePixels(new() { Node = t => Common.Nodes.GetValueOrDefault(t) as MetaImageGumpNode, Attach = Common.Tree.Attach, Position = Common.Collection.Position }, pixels, new byte[256], new byte[256]); Meta = new(Common.Create, Common.Factory, meta); MetaImageGump.BindMetaImageGump(Common.TreeHooks, Common.DrawHooks, t => Common.Nodes.GetValueOrDefault(t) is MetaImageGumpNode ? Meta : null);
            Hooks = new() { Node = t => t == 30 && Common.Nodes.ContainsKey(t) ? Node : null, Image = t => Common.Nodes.GetValueOrDefault(t) as MetaImageGumpNode, CreateImage = CreateImage, Attach = (t, x, y, p, m) => Common.TreeHooks.Attach!(t, x, y, p, m), Redraw = (t, _) => Common.Redraws.Add(t), Green = () => Green }; MetaImageList.BindMetaImageListDevices(Hooks, Common.Tree, Common.Lifetime, Common.Collection, Common.Events, pixels); List = new(Common.Create, Common.Factory, Hooks); MetaImageList.BindMetaImageList(Common.TreeHooks, Common.EventHooks, Common.InputHooks, Common.DrawHooks, Common.LifetimeHooks, t => t == 30 && Common.Nodes.ContainsKey(t) ? List : null, Release);
        }
        /// <summary>사용할 실제 목록을 기본 생성/루트 부착한다.</summary>
        public void Construct(int columns = 4, int width = 40, int height = 84) { List.Construct(30, columns); Node.BaseWidth = Node.Width = width; Node.BaseHeight = Node.Height = height; Common.TreeHooks.Attach!(30, 3, 5, 1, 0); }
        /// <summary>실제 현재 입력 분배를 통해 좌표 사건을 보낸다.</summary>
        public uint Handle(uint code, int x, int y) => Common.InputHooks.Handle!(30, code, unchecked((uint)x), unchecked((uint)y));
    }
    /// <summary>21개 실제 자식의 같은 RawImage/ID/등록·부모/셀 배치와 한도를 확인한다.</summary>
    [Fact]
    public void ActualTwentyOneChildrenAndDistinctIdsReachSameTree()
    {
        var s = new Scene(); s.Construct(); var images = Enumerable.Range(0, 22).Select(_ => Scene.Image()).ToArray();
        // 각 호출은 실제 MetaImage 공장을 거쳐 같은 트리에 부착한다.
        for (int i = 0; i < 21; i++) Assert.True(s.List.Add(30, images[i], (uint)(1000 + i))); Assert.False(s.List.Add(30, images[21], 2000)); Assert.Equal((21U, 10f, 14f, 23U, 21), (s.Node.Count, s.Node.CellWidth, s.Node.CellHeight, s.Common.Create.Count, s.Allocated.Count));
        // 순번이 아닌 별도ID와 같은 빌린Image를 각 슬롯에서 조회한다.
        for (int i = 0; i < 21; i++) { Assert.Equal((uint)(1000 + i), s.List.Id(30, i)); Assert.Same(images[i], s.List.Image(30, i)); var child = (MetaImageGumpNode)s.Common.Nodes[s.Node.Images[i]]; Assert.Equal(30U, child.Parent); Assert.Equal((i % 4 * 10f, i / 4 * 14f), (child.X, child.Y)); Assert.True(child.Width <= 10.00001f && child.Height <= 14.00001f); } Assert.Throws<ArgumentOutOfRangeException>(() => s.List.Id(30, 21)); Assert.Null(s.List.Image(30, 21));
    }
    /// <summary>열0이하의 가로배치는 폭을 균등분할하고 셀높이는 목록높이와 더 작은 값을 쓴다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(-3)]
    public void ActualHorizontalLayoutUsesSameChildren(int columns)
    {
        var s = new Scene(); s.Construct(columns, 39, 50); s.List.Add(30, Scene.Image(), 71); s.List.Add(30, Scene.Image(), 72); s.List.Add(30, Scene.Image(), 73); Assert.Equal((13f, 13f), (s.Node.CellWidth, s.Node.CellHeight));
        // 실제 자식은 한행의 누적float 위치로 부착된다.
        for (int i = 0; i < 3; i++) { var child = (MetaImageGumpNode)s.Common.Nodes[s.Node.Images[i]]; Assert.Equal((i * 13f, 0f), (child.X, child.Y)); } Assert.Equal(1, s.List.Hit(30, 3 + 14, 5 + 20)); Assert.Equal(-1, s.List.Hit(30, 42, 5));
    }
    /// <summary>같은 입력/부모사건에서 선택/해제를 토글하고 해제사건은 전달하지 않는다.</summary>
    [Fact]
    public void ActualInputTogglesAndNotifiesOutsideIndexWithOriginalValues()
    {
        var s = new Scene(); s.Construct();
        // 실제 21칸을 만든 뒤 grid에서14번을 누른다.
        for (int i = 0; i < 21; i++) s.List.Add(30, Scene.Image(), (uint)(1000 + i)); s.Common.Notices.Clear(); Assert.Equal(1U, s.Handle(0x10000, 24, 48)); Assert.Equal(14, s.Node.Selection); Assert.Equal((0x02000004U, 14U, 1U), s.Common.Notices[^1]); int notices = s.Common.Notices.Count; Assert.Equal(0U, s.Handle(0x80010000, 24, 48)); Assert.Equal(notices, s.Common.Notices.Count); Assert.Equal(1U, s.Handle(0x10000, 24, 48)); Assert.Equal(-1, s.Node.Selection); Assert.Equal((0x02000004U, 14U, 0U), s.Common.Notices[^1]); Assert.Equal(1U, s.Handle(0x10000, -1, -1)); Assert.Equal((0x02000004U, uint.MaxValue, 1U), s.Common.Notices[^1]);
    }
    /// <summary>선택 체크무늬는 절대화면홀짝·현재클립/전체DWORD색을 사용하며 밖과guard를 보존한다.</summary>
    [Fact]
    public void ActualSelectionCheckerPixelsClipAndGuards()
    {
        var s = new Scene(); s.Construct();
        // 빈 실제shape를 빌려 선택픽셀과 그림출력을 구별한다.
        for (int i = 0; i < 21; i++) s.List.Add(30, Scene.Image(true), (uint)i); s.Node.Selection = 6; var clip = new ScreenRect(24, 20, 30, 30); GumpDraw.SetGumpDrawClip(s.Common.Clip, clip); s.Common.Pixels.Span.Fill(0x5a); uint observed = 0; var fill = s.Hooks.Fill; s.Hooks.Fill = (r, c) => { observed = c; fill!(r, c); }; s.Common.Draw.Draw(30);
        // 출력은 원문 선택사각형[23,19..33,33)과 공유클립의 교집합이다.
        for (int y = 0; y < 128; y++) for (int x = 0; x < 64; x++) { byte expected = x >= 24 && x < 30 && y >= 20 && y < 30 && ((x ^ y) & 1) != 0 ? (byte)43 : (byte)0x5a; Assert.Equal(expected, s.Common.Pixels.Span[y * 64 + x]); } Assert.Equal(s.Green, observed); Assert.Equal((7, 7), ((int)s.Common.Guarded[0], (int)s.Common.Guarded[^1])); Assert.Equal(clip, s.Common.Clip.Clip);
    }
    /// <summary>삭제는지연Kill·접두부압축만하고끝중복칸/선택을남기며목록소멸은빌린Image를반납하지않는다.</summary>
    [Fact]
    public void ActualRemovalCompactsPrefixKeepsTailAndChildFirstLifetime()
    {
        var s = new Scene(); s.Construct(); var images = new[] { Scene.Image(), Scene.Image(), Scene.Image() };
        // 실제 부모가 소유한 세 자식과 외부 소유Image를 만든다.
        for (int i = 0; i < 3; i++) s.List.Add(30, images[i], (uint)(70 + i)); uint[] children = (uint[])s.Node.Images.Clone(); s.Node.Selection = 2; Assert.True(s.List.Remove(30, 1)); Assert.Equal((2U, 2, children[2], children[2], 72U, 72U), (s.Node.Count, s.Node.Selection, s.Node.Images[1], s.Node.Images[2], s.Node.Ids[1], s.Node.Ids[2])); Assert.True(s.Common.Nodes.ContainsKey(children[1])); Assert.NotEqual(0U, s.Common.Nodes[children[1]].Flags & 0x10000); s.Common.Lifetime.Cleanup(); Assert.False(s.Common.Nodes.ContainsKey(children[1])); Assert.Equal(4U, s.Common.Create.Count); s.Common.Lifetime.Kill(30); s.Common.Lifetime.Cleanup(); Assert.Equal(1U, s.Common.Create.Count); Assert.Equal(30U, s.Released[^1]); Assert.All(children.Take(3), token => Assert.False(s.Common.Nodes.ContainsKey(token))); Assert.All(images, image => Assert.False(image.Shape().IsEmpty));
    }
    /// <summary>생성전임의count는초기화하되ID/셀의이전값은보존하고실패한Add의선행ID쓰기도유지한다.</summary>
    [Fact]
    public void ConstructorUnwrittenFieldsAndPartialAddStayVisible()
    {
        var s = new Scene(); s.Node.Count = uint.MaxValue; s.Node.CellWidth = 91; s.Node.CellHeight = 93; Array.Fill(s.Node.Ids, 0x12345678U); var ids = s.Node.Ids; s.Construct(); Assert.Same(ids, s.Node.Ids); Assert.All(ids, id => Assert.Equal(0x12345678U, id)); Assert.Equal((91f, 93f, -1, 22U), (s.Node.CellWidth, s.Node.CellHeight, s.Node.Selection, s.Node.Command)); s.Hooks.CreateImage = _ => throw new InvalidOperationException("명시 할당 실패"); Assert.Throws<InvalidOperationException>(() => s.List.Add(30, Scene.Image(), 0x81234567)); Assert.Equal((0U, 0x81234567U), (s.Node.Count, s.Node.Ids[0])); Assert.Empty(s.Allocated); s.Node.Count = 22; Assert.Throws<ArgumentOutOfRangeException>(() => s.List.Image(30, 0)); var cd = new GumpCreateState(); cd.Collection.Edition = OriginalEdition.Cd1072; Assert.Throws<ArgumentException>(() => new MetaImageList(cd, s.Common.Factory, new()));
    }
}
