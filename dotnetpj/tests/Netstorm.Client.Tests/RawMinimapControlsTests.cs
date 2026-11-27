using System.Buffers.Binary;
using System.Reflection;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Client.Tests; // 실제 공통 미니맵의 입력·부분 출력·통지 대여 수명을 검증한다.

/// <summary>실제 공통 트리/물리/bitmap을 사용하며 카메라·선택·기본 Help는 명시 외부 장치다.</summary>
public sealed class RawMinimapControlsTests
{
    /// <summary>같은 현재 입력과 실제 공통 UI 소유자다.</summary>
    private sealed class Rig : IDisposable
    {
        /// <summary>같은 실제 자산/글꼴과 공통 등록표/입력/출력이다.</summary>
        public readonly GumpButtonTree Tree;
        /// <summary>Session 이전에도 존재하는 같은 장기 Pool/TYPE/Hash다.</summary>
        public readonly RawSurfaceStorage Storage;
        /// <summary>실제 현재 장기 Gameplay의 공유 갱신 owner다.</summary>
        public readonly ClientGameplayGlobals Gameplay = new();
        /// <summary>같은 공유 bitmap/큐다.</summary>
        public readonly MetadisplayDirtyState State = new();
        /// <summary>같은 전용 팔레트/현재 토큰/serial이다.</summary>
        public readonly RawMinimapGlobals Globals = new();
        /// <summary>서로 구별되는 명시 팔레트 입력이다.</summary>
        public readonly PaletteColorTable Palette = new();
        /// <summary>현재 실제 카메라/지도/칸 배율 입력이다.</summary>
        public MinimapGeometryView View = new(BitConverter.SingleToUInt32Bits(120), BitConverter.SingleToUInt32Bits(80), new(84, 0, 320, 240));
        /// <summary>현재 마우스 폴링이다.</summary>
        public InputEvent Pointer = new(0x10000, 50, 187);
        /// <summary>선택 이동의 현재 모드다.</summary>
        public uint Mode;
        /// <summary>현재 중심 장치 호출 수다.</summary>
        public int Centers;
        /// <summary>선택 이동 호출 수다.</summary>
        public int Moves;
        /// <summary>공통 구조 변경 호출 수다.</summary>
        public int Changed;
        /// <summary>중심 장치 반환 전 현재 표시를 바꾸는 경계 입력이다.</summary>
        public bool HideOnCenter;
        /// <summary>실제 영역 요청과 원래flags다.</summary>
        public readonly List<(ScreenRect Rect, uint Flags)> Rectangles = [];
        /// <summary>카메라 호출 당시 실제 영역 수와 현재 focus다.</summary>
        public readonly List<(int Rectangles, uint Focus)> CenterOrder = [];
        /// <summary>선택 이동/현재 카메라와 실제 영역을 공급하는 장치다.</summary>
        public readonly RawMinimapHooks Hooks;
        /// <summary>현재 실제 공통 미니맵 처리기다.</summary>
        public readonly RawMinimapControls Mini;
        /// <summary>같은 현재 기본 주소다.</summary>
        public GumpCreateNode Node => Mini.Node(Mini.RootToken)!;
        /// <summary>처음에는 같은 장기 물리/트리만 준비하며 open은 호출자가 선택한다.</summary>
        public Rig(bool open = true)
        {
            var font = new GumpFontFixture("gumptext"); Tree = new(320, 240, GumpBackgroundTestAssets.Assets(), 82, 0, new(9, 243, 57), font.Fonts,
                new() { Now = () => 1, Poll = _ => Pointer, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(); var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray(); Storage = new(OriginalEdition.Patch1078, types, frames, shapes);
            // 일반색과 전용 이름색의 선택 순서를 분명하게 구별한다.
            for (int i = 0; i < 9; i++) Palette.Basic[i] = (uint)(10 + i);
            // 기존 파일 팔레트와 같은 이름색 자리만 공급한다.
            for (int i = 0; i < 20; i++) Palette.Named.Add(new(0, 0, 0, (uint)(100 + i)));
            Hooks = new() { View = () => View, InputMode = () => Mode, Poll = () => Pointer, Move = (_, _) => Moves++, BaseHandle = (_, _, _, _) => 0,
                Center = (x, y) => { CenterOrder.Add((Rectangles.Count, Tree.LifeState.Focus)); Centers++; View = View with { CameraX = BitConverter.SingleToUInt32Bits(x), CameraY = BitConverter.SingleToUInt32Bits(y) }; if (HideOnCenter) Node.Flags &= ~1U; },
                Invalidate = (rect, flags) => Rectangles.Add((rect, flags)), Changed = () => Changed++ };
            Mini = new(Tree, Storage, State, Globals, Gameplay.MapRevision, Hooks);
            if (open) { Mini.Open(); Node.BaseWidth = Node.Width = 80; Node.BaseHeight = Node.Height = 75; Tree.Tree.Push(Mini.RootToken); Mini.Attach(Mini.RootToken, 10, 150, Tree.Create.Entries[0], 0); Mini.SetPalette(Palette); Assert.True(Mini.Refresh()); Rectangles.Clear(); }
        }
        /// <summary>자기 UI만 반납한다. 물리/전역은 따로 소유한다.</summary>
        public void Dispose() => Mini.Dispose();
        /// <summary>실제 공통 수집/정렬/가상 Draw를 현재 clip과 같은 표면에 수행한다.</summary>
        public void Draw(byte[] pixels, ScreenRect clip)
        {
            var state = new GumpDrawState(); GumpDraw.SetGumpDrawClip(state, clip); var target = new GumpPixelTarget(pixels, 327, 240, state); var colors = new GumpGlyphColorState(); var background = new GumpBackgroundDraw(Tree.Collection, state, new(), new() { Node = token => Tree.Node(token) as GumpBackgroundNode });
            var entries = new RenderCollectionEntry[5000]; int count = Tree.Collection.Collect(clip, entries, 5000); DrawSort.Sort(entries.AsSpan(0, count), OriginalEdition.Patch1078); var draw = new GumpDraw(Tree.Collection, state, new() { Draw = token => Tree.ExternalDrawing.TryDraw(token, target, colors, background) });
            // 실제 수집 결과의 순서로 현재 외부 그림을 한 번씩 분배한다.
            for (int i = 0; i < count; i++) draw.Draw(entries[i].Token);
        }
        /// <summary>실제 전달한 부분만 지우고 다시 그린 결과를 독립 전체 출력과 비교한다.</summary>
        public void ComparePartial(byte[] pixels)
        {
            var request = Assert.Single(Rectangles); Assert.Equal(new ScreenRect(10, 150, 90, 225), request.Rect); Assert.Equal(0U, request.Flags);
            // 요청 사각형 밖과 각 행의7BYTE padding은 보존한다.
            for (int y = request.Rect.Top; y < request.Rect.Bottom; y++) pixels.AsSpan(y * 327 + request.Rect.Left, request.Rect.Right - request.Rect.Left).Fill(0x5a);
            Draw(pixels, request.Rect); var whole = new byte[327 * 240]; Array.Fill(whole, (byte)0x5a); Draw(whole, new(0, 0, 320, 240)); Assert.Equal(whole, pixels);
        }
        /// <summary>같은 장기 물리의 실제 현재 모듈을 만든다. Game Root 설치의 증거로 세지 않는다.</summary>
        public RawSurfaceWorld World() => new(OriginalEdition.Patch1078, Storage.Types, 97, Storage, Storage.Frames);
    }
    /// <summary>생성은 bitmap/등록값을 초기화하지 않고 실제 공유DWORD만 빌리며 Construct는 현재 bitmap 폭을 쓴다.</summary>
    [Fact]
    public void DeferredConstructionBorrowsActualMapRevisionAndCurrentBitmapWidth()
    {
        using var rig = new Rig(false); Assert.Equal(0U, rig.Mini.RootToken); Assert.Empty(rig.State.Bitmap); Assert.Equal(1, rig.Tree.ExternalDrawing.RegistrationCount); Assert.Same(rig.Gameplay.MapRevision, rig.State.SharedFullRefresh);
        rig.Gameplay.MapRevision.Value = 7; rig.State.BitmapWidth = 13; uint token = rig.Mini.Construct(); Assert.Equal(13, rig.Node.BaseWidth); Assert.Empty(rig.State.Bitmap); Assert.Equal(29U, rig.Node.Command); Assert.Equal(0x50d51cU, rig.Node.Vtable); Assert.Equal(token, rig.Globals.Current); Assert.Equal(rig.Node.Serial, rig.Globals.Serial); Assert.Equal(7U, rig.Gameplay.MapRevision.Value);
        Assert.Equal("Sky Overview", rig.Tree.ReadOwnedText(rig.Node.TextPointer)); Assert.Equal("minimapGump", rig.Tree.ReadOwnedText(rig.Node.AlternateTextPointer)); Assert.Throws<InvalidOperationException>(() => rig.Mini.Construct());
        Assert.Throws<ArgumentException>(() => new RawMinimapControls(rig.Tree, rig.Storage, rig.State, rig.Globals, new(), rig.Hooks)); Assert.Equal(1, rig.Tree.ExternalDrawing.RegistrationCount);
    }
    /// <summary>전용8색/중립2색의 정확한 선택과 같은팔레트 재설정의 자연 반환을 확인한다.</summary>
    [Fact]
    public void PaletteUsesDedicatedOwnerOrderAndOnlyChangedValuesIncrementSharedWord()
    {
        using var rig = new Rig(); Assert.Equal(new uint[] { 111, 11, 17, 110, 14, 15, 112, 16 }, rig.Globals.Colors.Owners); Assert.Equal(118U, rig.Globals.Colors.NeutralIsle); Assert.Equal(119U, rig.Globals.Colors.NeutralNoIsland);
        rig.Gameplay.MapRevision.Value = 37; rig.Mini.SetPalette(rig.Palette); Assert.Equal(37U, rig.Gameplay.MapRevision.Value); rig.Palette.Named[11] = new(0, 0, 0, 233); rig.Mini.SetPalette(rig.Palette); Assert.Equal(38U, rig.Gameplay.MapRevision.Value); Assert.Equal(233U, rig.Globals.Colors.Owners[0]);
    }
    /// <summary>실제 Route/Frame의 카메라→영역→캡처 순서와 바깥 드래그 후 부분 출력 전체 일치를 확인한다.</summary>
    [Fact]
    public void ActualInputFrameAndPartialDrawingKeepCaptureOrderAndEveryPixel()
    {
        using var rig = new Rig(); var pixels = new byte[327 * 240]; Array.Fill(pixels, (byte)0x5a); rig.Draw(pixels, new(0, 0, 320, 240)); uint token = rig.Mini.RootToken;
        rig.Tree.Input.Route(0x10000, 50, 187); Assert.Equal((0, 0U), Assert.Single(rig.CenterOrder)); Assert.Equal(token, rig.Tree.LifeState.Focus); Assert.NotEqual(0U, rig.Node.Flags & 0x80); rig.ComparePartial(pixels);
        // 실제 공통 프레임은 오른쪽/위쪽 바깥에서도 같은 캡처 토큰의 Idle을 호출한다.
        foreach (var point in new[] { new ScreenPoint(105, 187), new ScreenPoint(50, 145) })
        {
            rig.Rectangles.Clear(); rig.Pointer = new(0x10000, point.X, point.Y); rig.Tree.Frame.Idle(); Assert.Equal((0, token), rig.CenterOrder[^1]); rig.ComparePartial(pixels);
        }
        rig.Rectangles.Clear(); rig.Tree.Input.Route(0x80010000, 50, 145); Assert.Equal(0U, rig.Tree.LifeState.Focus); Assert.Equal(0U, rig.Node.Flags & 0x80); Assert.Empty(rig.Rectangles); Assert.DoesNotContain(token, rig.Tree.LifeState.Idle);
    }
    /// <summary>선택 모드2/5와 카메라 반환 중 숨김에서 불필요한 카메라/영역 요청을 하지 않는다.</summary>
    [Fact]
    public void MoveModesAndCenterCallbackUseCurrentVisibility()
    {
        using var rig = new Rig(); uint token = rig.Mini.RootToken; rig.Node.Flags &= ~1U; rig.View = rig.View with { CameraX = BitConverter.SingleToUInt32Bits(144) }; Assert.False(rig.Mini.Refresh()); Assert.Empty(rig.Rectangles); rig.Node.Flags |= 1;
        // 원문 두 선택 이동 모드만 현재 클릭 시점에 선택한다.
        foreach (uint mode in new uint[] { 2, 5 }) { rig.Mode = mode; Assert.Equal(1U, rig.Mini.Handle(token, 0x10000, 50, 187)); Assert.Equal(0, rig.Centers); Assert.Equal(0U, rig.Tree.LifeState.Focus); Assert.Empty(rig.Rectangles); }
        Assert.Equal(2, rig.Moves); rig.Mode = 0; rig.HideOnCenter = true; rig.Tree.Input.Route(0x10000, 50, 187); Assert.Equal(1, rig.Centers); Assert.Equal(0U, rig.Node.Flags & 1); Assert.Equal(token, rig.Tree.LifeState.Focus); Assert.Empty(rig.Rectangles); rig.Tree.Input.Route(0x80010000, 50, 187); Assert.Equal(0U, rig.Tree.LifeState.Focus);
    }
    /// <summary>현재 공유 MapRevision의 지면/소유자/제거 변경이 실제 셀·영역·픽셀까지 전파된다.</summary>
    [Fact]
    public void SharedOwnerAndTerrainChangesReachActualPartialPixelsOnce()
    {
        using var rig = new Rig(); var pixels = new byte[327 * 240]; Array.Fill(pixels, (byte)0x5a); rig.Draw(pixels, new(0, 0, 320, 240)); int sid = rig.Storage.Pool.Allocate(); var raw = rig.Storage.Pool.AllocatedBytes(sid); raw.Clear(); raw[10] = 97; BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 120); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 80); rig.Storage.Hash.Cell(0, 120, 80) = checked((ushort)sid);
        // 같은 카메라에서 owner1→2→중립을 원본 전용 색으로 표시한다.
        foreach (var item in new[] { (Owner: (byte)1, Color: (byte)111), (Owner: (byte)2, Color: (byte)11), (Owner: (byte)0, Color: (byte)118) })
        {
            raw[34] = item.Owner; rig.Rectangles.Clear(); rig.Gameplay.MapRevision.Value = 1; Assert.True(rig.Mini.Refresh()); Assert.Equal(0U, rig.Gameplay.MapRevision.Value); rig.ComparePartial(pixels); Assert.Equal(item.Color, pixels[182 * 327 + 44]); rig.Rectangles.Clear(); Assert.False(rig.Mini.Refresh()); Assert.Empty(rig.Rectangles);
        }
        rig.Storage.Hash.Cell(0, 120, 80) = 0; rig.Gameplay.MapRevision.Value = 1; Assert.True(rig.Mini.Refresh()); rig.ComparePartial(pixels); Assert.Equal((byte)10, pixels[182 * 327 + 44]);
    }
    /// <summary>이전 표시의 scalar/Dispose가 새 current/bitmap/외부 출력과 다른 노드를 지우지 않는다.</summary>
    [Fact]
    public void OlderDisplayDisposalPreservesNewCurrentBitmapStringsAndDrawRegistration()
    {
        using var rig = new Rig(); byte[] bitmap = rig.State.Bitmap; uint old = rig.Mini.RootToken; using var newer = new RawMinimapControls(rig.Tree, rig.Storage, rig.State, rig.Globals, rig.Gameplay.MapRevision, rig.Hooks); uint token = newer.Construct();
        Assert.Equal(2, rig.Tree.ExternalDrawing.RegistrationCount); rig.Mini.Dispose(); Assert.Null(rig.Tree.Node(old)); Assert.Equal(token, rig.Globals.Current); Assert.True(rig.State.DisplayPresent); Assert.Same(bitmap, rig.State.Bitmap); Assert.NotNull(newer.Node(token)); Assert.Equal(1, rig.Tree.ExternalDrawing.RegistrationCount); Assert.Equal(2, rig.Tree.OwnedTextCount);
        newer.Close(); Assert.False(rig.State.DisplayPresent); Assert.Same(bitmap, rig.State.Bitmap); Assert.Empty(newer.Report()); Assert.Equal(0, rig.Tree.OwnedTextCount);
    }
    /// <summary>같은 실제 World만 연결하고 중복 대여를 거부하며 후속으로 설치한 다른 큐는 반납하지 않는다.</summary>
    [Fact]
    public void BindWorldChecksPhysicalIdentityAndUnbindOnlyReleasesExpectedQueue()
    {
        using var rig = new Rig(); using var world = rig.World(); var core = world.Core(); rig.Mini.BindWorld(core); Assert.Equal(1U, rig.Gameplay.MapRevision.Value); Assert.Throws<ArgumentException>(() => rig.Mini.BindWorld(core));
        var other = new RawMetadisplayDirty(rig.Storage.Pool, new()); core.Pop.SetMetadisplay(other); core.Unpop.SetMinimapQueue(other); rig.Mini.UnbindWorld(); Assert.True(core.Pop.ReleaseMetadisplay(other)); Assert.True(core.Unpop.ReleaseMinimapQueue(other));
        rig.Mini.BindWorld(core); rig.Mini.UnbindWorld(); Assert.False(core.Unpop.ReleaseMinimapQueue(other));
        using var different = new Rig(false); using var otherWorld = different.World(); Assert.Throws<ArgumentException>(() => rig.Mini.BindWorld(otherWorld.Core()));
    }
    /// <summary>장기 저장소 실행기의 등록과 해제가 미니맵 큐에 도달하고 대여 반납 뒤에는 더 이상 전달되지 않는다.</summary>
    [Fact]
    public void StorageExecutorsNotifyMinimapAndUnbindStopsNotifications()
    {
        using var rig = new Rig();
        rig.Storage.BindDisplayDevices(new() { View = () => new() { Zoom = 65536 }, Suppressed = () => false, Invalidate = (_, _) => { } });
        var pop = rig.Storage.PopExecutor();
        var unpop = rig.Storage.UnpopExecutor();
        rig.Mini.BindStorageExecutors();
        Assert.Throws<InvalidOperationException>(() => rig.Mini.BindStorageExecutors());
        rig.Mini.Refresh();
        rig.Storage.Types[74].Flags2 = 0x4002;
        rig.Storage.Types[74].FootX = 1;
        rig.Storage.Types[74].FootY = 1;
        rig.Storage.Shapes[74].Loaded = true;
        rig.Storage.Shapes[74].FrameCount = 1;
        rig.Storage.Shapes[74].Frames.Add(new() { Width = 16, Height = 11, CellWidth = 1, CellHeight = 1 });
        int sid = rig.Storage.FactoryExecutor().Create(74, SidAllocation.Client);
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, rig.Storage.Types[74], 1, 1, 120, 80));
        Assert.True(rig.State.Count > 0);
        rig.Mini.Refresh();
        unpop.Unpop(sid, rig.Storage.Types[74]);
        Assert.True(rig.State.Count > 0);
        rig.Mini.UnbindWorld();
        rig.Mini.Refresh();
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, rig.Storage.Types[74], 1, 1, 120, 80));
        Assert.Equal(0, rig.State.Count);
    }
    /// <summary>같은 물리의 추가 실행기 알림과 중복·타 월드 거부 및 교체된 큐 보존을 검증한다.</summary>
    [Fact]
    public void AdditionalWorldPairNotifiesAndReleasesOnlyItsOwnQueue()
    {
        using var rig = new Rig();
        rig.Storage.BindDisplayDevices(new() { View = () => new() { Zoom = 65536 }, Suppressed = () => false, Invalidate = (_, _) => { } });
        var display = rig.Storage.DisplayExecutor();
        var pop = new RawPop(rig.Storage.Pool, rig.Storage.Hash, rig.Storage.Spots, display, rig.Storage.PostPopExecutor());
        var unpop = new RawUnpop(rig.Storage.Pool, rig.Storage.Hash, rig.Storage.Spots, display);
        pop.BindDiagnostics(new() { Types = rig.Storage.Types });
        unpop.BindDiagnostics(new() { Types = rig.Storage.Types });
        pop.SetBuildingSide(rig.Storage.BuildingExecutor());
        unpop.SetBuildingSide(rig.Storage.BuildingExecutor());
        Assert.Throws<InvalidOperationException>(() => rig.Mini.BindAdditionalWorldPair(pop, unpop));
        rig.Mini.BindStorageExecutors();
        using var foreign = new Rig(false);
        var foreignPop = new RawPop(foreign.Storage.Pool, foreign.Storage.Hash, foreign.Storage.Spots);
        var foreignUnpop = new RawUnpop(foreign.Storage.Pool, foreign.Storage.Hash, foreign.Storage.Spots);
        Assert.Throws<ArgumentException>(() => rig.Mini.BindAdditionalWorldPairs((pop, unpop), (foreignPop, foreignUnpop)));
        // 뒤쪽 쌍이 실패해도 앞쪽 쌍은 설치되지 않아 정상 대여가 가능하다.
        rig.Mini.BindAdditionalWorldPair(pop, unpop);
        Assert.Throws<ArgumentException>(() => rig.Mini.BindAdditionalWorldPair(pop, unpop));
        Assert.Throws<ArgumentException>(() => rig.Mini.BindAdditionalWorldPair(pop, foreignUnpop));
        rig.Mini.Refresh();
        rig.Storage.Types[74].Flags2 = 0x4002;
        rig.Storage.Types[74].FootX = rig.Storage.Types[74].FootY = 1;
        rig.Storage.Shapes[74].Loaded = true;
        rig.Storage.Shapes[74].FrameCount = 1;
        rig.Storage.Shapes[74].Frames.Add(new() { Width = 16, Height = 11, CellWidth = 1, CellHeight = 1 });
        int sid = rig.Storage.FactoryExecutor().Create(74, SidAllocation.Client);
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, rig.Storage.Types[74], 1, 1, 120, 80));
        Assert.True(rig.State.Count > 0);
        rig.Mini.Refresh();
        unpop.Unpop(sid, rig.Storage.Types[74]);
        Assert.True(rig.State.Count > 0);
        var other = new RawMetadisplayDirty(rig.Storage.Pool, new());
        pop.SetMetadisplay(other); unpop.SetMinimapQueue(other);
        var nextPop = new RawPop(rig.Storage.Pool, rig.Storage.Hash, rig.Storage.Spots);
        var nextUnpop = new RawUnpop(rig.Storage.Pool, rig.Storage.Hash, rig.Storage.Spots);
        Assert.True(rig.Mini.TryReplaceAdditionalWorldPair(pop, unpop, nextPop, nextUnpop));
        Assert.True(pop.ReleaseMetadisplay(other));
        Assert.True(unpop.ReleaseMinimapQueue(other));
        rig.Mini.UnbindWorld();
        Assert.False(rig.Mini.TryReplaceAdditionalWorldPair(nextPop, nextUnpop, pop, unpop));
    }
    /// <summary>실제 HostSession의 지형과 Player 실행기에서 미니맵 큐까지 변경이 전달된다.</summary>
    [Fact]
    public void HostSessionTerrainAndPlayerExecutorsNotifyMinimap()
    {
        using var rig = new Rig();
        rig.Storage.BindDisplayDevices(new() { View = () => new() { Zoom = 65536 }, Suppressed = () => false, Invalidate = (_, _) => { } });
        var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: rig.Storage.Pool);
        try
        {
            _ = HostPostPopRuntimeTests.Build(session, rig.Storage);
            _ = session.BuildPlayerWorldHooks();
            rig.Mini.BindStorageExecutors();
            session.BindMinimapNotifications(rig.Mini);
            Assert.Throws<ArgumentException>(() => session.BindMinimapNotifications(rig.Mini));
            rig.Mini.Refresh();
            rig.Storage.Types[74].Flags2 = 0x4002;
            rig.Storage.Types[74].FootX = rig.Storage.Types[74].FootY = 1;
            rig.Storage.Shapes[74].Loaded = true;
            rig.Storage.Shapes[74].FrameCount = 1;
            rig.Storage.Shapes[74].Frames.Add(new() { Width = 16, Height = 11, CellWidth = 1, CellHeight = 1 });
            int sid = rig.Storage.FactoryExecutor().Create(74, SidAllocation.Client);
            session.PopWorldObject(sid, 120, 80, 0);
            Assert.True(rig.State.Count > 0);
            rig.Mini.Refresh();
            session.UnpopWorldObject(sid, 0);
            Assert.True(rig.State.Count > 0);
            rig.Mini.Refresh();
            // 실제 그래프 재결선이 지형 실행기를 교체해도 같은 미니맵이 변경을 받는다.
            var oldTerrainPop = Assert.IsType<RawPop>(typeof(HostSession).GetField("_terrainPop", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(session));
            var oldTerrainUnpop = Assert.IsType<RawUnpop>(typeof(HostSession).GetField("_terrainUnpop", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(session));
            session.BuildWorldGraphDevices(rig.Storage.Types);
            session.WireTerrainPostPopGraph();
            rig.Mini.Refresh();
            Assert.Equal(RawPopResult.Registered, oldTerrainPop.Pop(sid, rig.Storage.Types[74], 1, 1, 120, 80));
            oldTerrainUnpop.Unpop(sid, rig.Storage.Types[74]);
            Assert.Equal(0, rig.State.Count);
            var terrainPop = Assert.IsType<RawPop>(typeof(HostSession).GetField("_terrainPop", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(session));
            Assert.Equal(RawPopResult.Registered, terrainPop.Pop(sid, rig.Storage.Types[74], 1, 1, 120, 80));
            Assert.True(rig.State.Count > 0);
            rig.Mini.Refresh();
            var terrainUnpop = Assert.IsType<RawUnpop>(typeof(HostSession).GetField("_terrainUnpop", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(session));
            terrainUnpop.Unpop(sid, rig.Storage.Types[74]);
            Assert.True(rig.State.Count > 0);
            rig.Mini.UnbindWorld();
            rig.Mini.Refresh();
            session.PopWorldObject(sid, 120, 80, 0);
            Assert.Equal(0, rig.State.Count);
        }
        finally { session.ReleasePlayerRecordViews(); }
    }
    /// <summary>Capture/Idle 중 닫아도 같은 공통 수명이 둘을 반납하고 독립 bitmap은 남긴다.</summary>
    [Fact]
    public void CloseAndReopenKeepBitmapAndSharedWordButReleaseCaptureIdleAndOwnedStrings()
    {
        using var rig = new Rig(); byte[] bitmap = rig.State.Bitmap; uint old = rig.Mini.RootToken; rig.Tree.Input.Route(0x10000, 50, 187); rig.Mini.Close(); Assert.Equal(0U, rig.Tree.LifeState.Focus); Assert.DoesNotContain(old, rig.Tree.LifeState.Idle); Assert.Null(rig.Tree.Node(old)); Assert.Equal(0, rig.Tree.OwnedTextCount); Assert.Same(bitmap, rig.State.Bitmap);
        rig.Mini.Open(); Assert.NotEqual(old, rig.Mini.RootToken); Assert.Same(bitmap, rig.State.Bitmap); Assert.Equal(1U, rig.Gameplay.MapRevision.Value); rig.Mini.Resize(240); Assert.Equal(new ScreenPoint(0, 165), rig.Tree.Collection.Position(rig.Mini.RootToken)); Assert.Equal(80, rig.Node.BaseWidth); Assert.Equal(75, rig.Node.BaseHeight); Assert.Equal(1, rig.Changed); rig.Mini.Resize(240); Assert.Equal(1, rig.Changed); Assert.Equal(new ScreenPoint(12, 172), rig.Mini.Point("minimap:12,7"));
    }
}
