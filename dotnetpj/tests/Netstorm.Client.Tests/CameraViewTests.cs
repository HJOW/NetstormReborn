using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 독립 PE의 전체64BYTE/목표20BYTE를 현재 카메라 본체와 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 카메라 비트·값복사·목표 전용 쓰기·진단 재조회 검사다.</summary>
public sealed class CameraViewTests
{
    /// <summary>원문 16진수 DWORD를 읽는다.</summary>
    private static uint HexWord(string value) => uint.Parse(value, NumberStyles.HexNumber, CultureInfo.InvariantCulture);
    /// <summary>64BYTE의 각 DWORD를 원래 순서로 읽는다.</summary>
    private static uint[] Words(string value)
    {
        byte[] bytes = Convert.FromHexString(value); var words = new uint[bytes.Length / 4];
        // 원문 리틀엔디언 저장 순서만 해독하고 기대값을 재계산하지 않는다.
        for (int index = 0; index < words.Length; index++) words[index] = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(index * 4)); return words;
    }
    /// <summary>독립64BYTE를 같은 현재 필드에 직접 입력한다.</summary>
    private static void Load(RawCameraViewState view, string image)
    {
        uint[] w = Words(image); Assert.Equal(16, w.Length); view.WorldBits[0] = w[0]; view.WorldBits[1] = w[1]; view.CellPixels[0] = unchecked((int)w[2]); view.CellPixels[1] = unchecked((int)w[3]);
        view.BaseCellPixels[0] = unchecked((int)w[4]); view.BaseCellPixels[1] = unchecked((int)w[5]); view.Zoom = w[6]; view.Pixels[0] = unchecked((int)w[7]); view.Pixels[1] = unchecked((int)w[8]); view.CellFloatBits[0] = w[9]; view.CellFloatBits[1] = w[10];
        view.BaseCellFloatBits[0] = w[11]; view.BaseCellFloatBits[1] = w[12]; view.ZoomFloatBits = w[13]; view.PixelFloatBits[0] = w[14]; view.PixelFloatBits[1] = w[15];
    }
    /// <summary>원래16DWORD를 저장 순서대로 관측한다.</summary>
    private static uint[] Snapshot(RawCameraViewState v) => [v.WorldBits[0], v.WorldBits[1], unchecked((uint)v.CellPixels[0]), unchecked((uint)v.CellPixels[1]), unchecked((uint)v.BaseCellPixels[0]), unchecked((uint)v.BaseCellPixels[1]), v.Zoom,
        unchecked((uint)v.Pixels[0]), unchecked((uint)v.Pixels[1]), v.CellFloatBits[0], v.CellFloatBits[1], v.BaseCellFloatBits[0], v.BaseCellFloatBits[1], v.ZoomFloatBits, v.PixelFloatBits[0], v.PixelFloatBits[1]];
    /// <summary>세 setter/투영의 전체16관찰을 현재 본체에 전수 대조한다.</summary>
    [Fact]
    public void EveryNativeViewImageMatches()
    {
        string[][] rows = X86Fixture.Read("camera-view").Where(row => row[0] != "operation").ToArray(); Assert.Equal(16, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var camera = new ClientCameraGlobals(); var view = camera.RendererView; Load(view, row[1]); uint x = HexWord(row[2]), y = HexWord(row[3]); var diagnostics = new List<uint>();
            // 실제 진단 순서와 줄만 기록하며 계산 결과를 바꾸지 않는다.
            void Assertion(string message, uint line) { Assert.Contains(message, new[] { "scrollPosPixX >= 0", "scrollPosPixY >= 0" }); diagnostics.Add(line); }
            if (row[0] == "world") RawCameraView.SetWorld(view, BitConverter.UInt32BitsToSingle(x), BitConverter.UInt32BitsToSingle(y), Assertion);
            else if (row[0] == "pixels") RawCameraView.SetPixels(view, unchecked((int)x), unchecked((int)y), Assertion);
            else if (row[0] == "init")
            {
                var redraw = new UIntBox { Value = 0x11111111 }; var revision = new UIntBox { Value = uint.MaxValue }; camera.InitializeNativeRenderer(1024, 768, 256, redraw, revision);
                Assert.Equal(Words(row[5]), Snapshot(camera.Snapshot)); Assert.Equal(X86Fixture.UInt(row[7]), redraw.Value); Assert.Equal(X86Fixture.UInt(row[8]), revision.Value); Assert.Equal(new[] { 3072, 2048 }, camera.Target.Limits); Assert.Equal(new[] { 0, 0 }, camera.Target.Pixels); Assert.Equal(65536u, camera.Target.Zoom);
            }
            else if (row[0] == "project")
            {
                ScreenPoint point = RawCameraView.Project(view, BitConverter.UInt32BitsToSingle(x), BitConverter.UInt32BitsToSingle(y)); view.WorldBits[0] = unchecked((uint)point.X); view.WorldBits[1] = unchecked((uint)point.Y); view.CellPixels[0] = point.X; view.CellPixels[1] = point.Y;
            }
            else throw new InvalidOperationException("알수없는원문카메라연산");
            bool xd = row[9].Contains("scrollPosPixX", StringComparison.Ordinal), yd = row[9].Contains("scrollPosPixY", StringComparison.Ordinal);
            Assert.Equal((xd ? 1 : 0) + (yd ? 1 : 0), diagnostics.Count); if (xd) Assert.Equal(0x344u, diagnostics[0]); if (yd) Assert.Equal(0x347u, diagnostics[^1]); return Snapshot(view).SequenceEqual(Words(row[4]));
        });
    }
    /// <summary>목표 전용 이동140원문은 현재 두64BYTE와 별도 분모를 보존한다.</summary>
    [Fact]
    public void EveryNativeCenterTargetMatches()
    {
        string[][] rows = X86Fixture.Read("camera-center"); Assert.Equal(140, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var camera = new ClientCameraGlobals(); Load(camera.RendererView, row[5][..128]); Load(camera.Snapshot, row[6]); uint[] target = Words(row[5].Substring(0x48 * 2, 40));
            camera.Target.Limits[0] = unchecked((int)target[0]); camera.Target.Limits[1] = unchecked((int)target[1]); camera.Target.Pixels[0] = unchecked((int)target[2]); camera.Target.Pixels[1] = unchecked((int)target[3]); camera.Target.Zoom = target[4]; camera.Denominator.Current = 23; camera.Denominator.DefaultValue = 17;
            var revision = new UIntBox { Value = X86Fixture.UInt(row[4]) }; camera.RequestCenter(BitConverter.UInt32BitsToSingle(HexWord(row[2])), BitConverter.UInt32BitsToSingle(HexWord(row[3])), X86Fixture.Int(row[0]), X86Fixture.Int(row[1]), revision);
            uint[] current = [unchecked((uint)camera.Target.Limits[0]), unchecked((uint)camera.Target.Limits[1]), unchecked((uint)camera.Target.Pixels[0]), unchecked((uint)camera.Target.Pixels[1]), camera.Target.Zoom];
            return Snapshot(camera.RendererView).SequenceEqual(Words(row[7][..128])) && Snapshot(camera.Snapshot).SequenceEqual(Words(row[8])) && current.SequenceEqual(Words(row[7].Substring(0x48 * 2, 40))) && revision.Value == X86Fixture.UInt(row[9]) && camera.Denominator.Current == 23 && camera.Denominator.DefaultValue == 17;
        });
    }
    /// <summary>X 진단 뒤 현재 Y를 재조회하며 누락 진단도 여섯 선행 쓰기를 보존한다.</summary>
    [Fact]
    public void AssertionsRequeryAfterStoresAndMissingDeviceKeepsPrefix()
    {
        var view = new RawCameraViewState(); RawCameraView.Initialize(view); int calls = 0;
        RawCameraView.SetWorld(view, BitConverter.UInt32BitsToSingle(0x4d000001), BitConverter.UInt32BitsToSingle(0x4d800001), (message, line) =>
        {
            Assert.Equal("scrollPosPixX >= 0", message); Assert.Equal(0x344u, line); calls++; Assert.True(view.Pixels[0] < 0 && view.Pixels[1] < 0); Assert.Equal(BitConverter.SingleToUInt32Bits(view.Pixels[1]), view.PixelFloatBits[1]); view.WorldBits[1] = 0xbf800000;
        }); Assert.Equal(1, calls); Assert.Equal(0xbf800000u, view.WorldBits[1]);
        Assert.Throws<InvalidOperationException>(() => RawCameraView.SetWorld(view, BitConverter.UInt32BitsToSingle(0x4d000001), 1)); Assert.Equal(0x4d000001u, view.WorldBits[0]); Assert.Equal(0x3f800000u, view.WorldBits[1]); Assert.Equal(11, view.Pixels[1]); Assert.Equal(BitConverter.SingleToUInt32Bits(view.Pixels[0]), view.PixelFloatBits[0]);
    }
    /// <summary>값 복사는 현재 배열을 보존하며 목표 이동/별도 분모와 실제 즉시 commit 경계를 구분한다.</summary>
    [Fact]
    public void CopiesKeepDistinctOwnersAndImmediateMissingCommitKeepsPrefix()
    {
        var camera = new ClientCameraGlobals(); int[] pixels = camera.Snapshot.Pixels; Assert.NotSame(camera.Snapshot, camera.RendererView); Assert.All(Snapshot(camera.Snapshot), word => Assert.Equal(0u, word));
        var redraw = new UIntBox(); var revision = new UIntBox(); camera.InitializeNativeRenderer(1024, 768, 256, redraw, revision); RawCameraView.SetWorld(camera.RendererView, 5, 5); Assert.Equal(0, pixels[0]); camera.CopyRendererSnapshot(); Assert.Equal(80, pixels[0]); Assert.Same(pixels, camera.Snapshot.Pixels);
        RawCameraView.SetWorld(camera.RendererView, 6, 6); Assert.Equal(80, pixels[0]); camera.RequestPixels(123, 456); Assert.Equal(80, pixels[0]); Assert.Equal(96, camera.RendererView.Pixels[0]);
        camera.Denominator.DefaultValue = 17; redraw.Value = 73; camera.SelectDenominator(0, redraw); Assert.Equal(17, camera.Denominator.Current); Assert.Equal(1u, redraw.Value); redraw.Value = 73; camera.SelectDenominator(17, redraw); Assert.Equal(73u, redraw.Value);
        uint[] before = Snapshot(camera.RendererView), snapshot = Snapshot(camera.Snapshot); redraw.Value = revision.Value = uint.MaxValue;
        Assert.Throws<InvalidOperationException>(() => camera.CenterImmediately(100.25f, 101.75f, 1024, 768, redraw, revision, null)); Assert.Equal(0u, redraw.Value); Assert.Equal(0u, revision.Value); Assert.Equal(new[] { 1092, 735 }, camera.Target.Pixels); Assert.Equal(before, Snapshot(camera.RendererView)); Assert.Equal(snapshot, Snapshot(camera.Snapshot));
    }
    /// <summary>실제 Gameplay의 내용물/커서가 같은 현재 snapshot을 빌리고 prepared 왕복은 값을 보존한다.</summary>
    [Fact]
    public void ActualGameplaySharesSnapshotAcrossPreparedRuntime()
    {
        var game = new ClientGameplayGlobals(); var camera = game.Camera; int[] pixels = camera.Snapshot.Pixels; var storage = new RawSurfaceStorage();
        Assert.Same(pixels, game.ContentScreenOrigin); Assert.Equal(0, game.CursorGlobals.Placement.CurrentOriginX());
        camera.InitializeNativeRenderer(1024, 768, 256, new(), game.MapRevision); RawCameraView.SetWorld(camera.Snapshot, 5, 5); Assert.Equal(80, game.ContentScreenOrigin[0]); Assert.Equal(55, game.CursorGlobals.Placement.CurrentOriginY());
        var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), sharedGameplay: game, pool: storage.Pool);
        camera.ConstructionHistory.Points[4] = new(17.25f, 19.5f); camera.ConstructionHistory.Index = 0x80000000; camera.SavedPositions.Points[9] = new(23.25f, 27.5f);
        using (var runtime = HostMissionRuntime.Create(session, storage)) { Assert.Same(camera, game.Camera); RawCameraView.SetWorld(camera.Snapshot, 6, 6); }
        game.ResetCursorForGeneralWorld(); Assert.Same(pixels, game.ContentScreenOrigin); Assert.Equal(96, game.CursorGlobals.Placement.CurrentOriginX()); Assert.Equal(66, game.ContentScreenOrigin[1]); Assert.Equal(0x80000000u, camera.ConstructionHistory.Index); Assert.Equal(17.25f, camera.ConstructionHistory.Points[4].X); Assert.Equal(27.5f, camera.SavedPositions.Points[9].Y);
    }
}
