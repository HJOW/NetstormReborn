using System.Buffers.Binary;
using System.Runtime.Versioning;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 포획/공격/승리 없이 실제 제품 자산과 장치의 건설 표시 수명을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>동일 세션·공통 목록·SHP·실제 팔레트·8비트 화면의 건설 표시 결선이다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>실제 건설 효과가 공통 수집·제출과 dirty 클립을 거쳐 현재 화면에 출력된다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductWorldSubmissionDrawsCollectedConstructionWithRestoredClip()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext();
        HostSession session = scene.Session;
        int parent = ConstructionParent(scene);
        scene.Visuals.SetView(0, 0, new(0, 0, 1024, 768));
        ConstructionVisualProcess process = session.WorldConstruction.Add(parent, false);
        // 실제 건설 표시가 픽셀을 만들도록 같은 커널 시각을 전진한다.
        for (uint frame = 0; frame < 24; frame++)
        {
            scene.Visuals.PrepareFrame(frame * .05, frame);
            session.Kernel.RunFrame();
        }
        var shapes = TerrainDisplayShapes.Build(scene.Definitions, scene.SourceShapes);
        session.BuildWorldDrawingDevices(new RawSquidHash(), session.Pool, scene.Types, shapes, () => new(0, 0, 1024, 768), 1024, 768);
        var clip = new ScreenRect(0, 0, 1024, 768);
        var entry = Assert.Single(session.CollectWorldDrawing(clip).Entries);
        Assert.Equal(3, entry.Kind);
        Assert.Equal(process.Registration.Token, entry.Token);
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var database = new Netstorm.Client.Display.ShapeDatabase(startup.Files.ReadAllBytes("d/_shapes.shp"));
        var previous = new ScreenRect(7, 9, 1000, 700);
        scene.Screen.SetClip(previous.Left, previous.Top, previous.Right, previous.Bottom);
        byte[] pixels = scene.Screen.Lock();
        try
        {
            session.SubmitWorldDrawing(pixels, scene.Screen.Pitch, scene.Screen.Height, clip, database, new RawTileDrawModes(),
                effects: scene.Visuals.DrawCollected);
        }
        finally { scene.Screen.Unlock(); }
        Assert.Equal(previous, scene.Screen.Clip);
        int changed = Array.FindIndex(pixels, pixel => pixel != 0);
        Assert.True(changed >= 0);
        byte expected = pixels[changed];
        Array.Clear(pixels);
        int x = changed % scene.Screen.Pitch, y = changed / scene.Screen.Pitch;
        scene.Visuals.DrawCollected(entry.Token, new(x, y, x + 1, y + 1));
        Assert.Equal(expected, pixels[changed]);
        Assert.Equal(1, pixels.Count(pixel => pixel != 0));
        Assert.Equal(previous, scene.Screen.Clip);
    }

    /// <summary>실제 공장 생성자 뒤 독립 건설 상태/HP/좌표를 지정한다. 게임 규칙 요청은 후속 단위다.</summary>
    private static int ConstructionParent(BurningContext scene)
    {
        HostSession session = scene.Session; int parent = session.BuildPlayerWorldHooks().Create!(85, 2);
        Span<byte> raw = session.Pool.AllocatedBytes(parent); raw[11] &= 0xf8; raw[40] |= 1;
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 20); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 20);
        RawHp(session, parent, session.Reward.MaxHitPoints(parent) - 1); return parent;
    }

    /// <summary>일반/즉시 생성이 실제 공장 캡처·팔레트·프레임·현재카메라 출력·자기삭제로 이어진다.</summary>
    [Theory, SupportedOSPlatform("windows")]
    [InlineData(false)]
    [InlineData(true)]
    public void ProductConstructionCapturesFactoryAndCompletesVisual(bool instant)
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = ConstructionParent(scene);
        Assert.Contains("viewport", Assert.Throws<InvalidOperationException>(() => session.WorldConstruction.Add(parent, instant)).Message);
        Assert.Equal(0, session.Kernel.Size()); Assert.Equal(0, session.WorldRendering.Size);
        scene.Visuals.SetView(13, 17, new(0, 0, 1024, 768));
        ConstructionVisualImage expected = scene.Visuals.Capture.Capture(parent);
        ConstructionVisualProcess process = session.WorldConstruction.Add(parent, instant);
        Assert.Same(session.WorldRendering, session.WorldConstruction.Rendering);
        Assert.Equal(17, session.Pool.Slot(process.Form())[10]); Assert.Equal(1, session.WorldConstruction.Size);
        ConstructionVisualFields fields = process.Fields.Visual;
        Assert.Equal(3u, fields.Palette); Assert.Equal(instant, fields.Instant);
        Assert.Equal(expected.Width, fields.Image.Width); Assert.Equal(expected.Height, fields.Image.Height);
        byte[] captured = fields.Image.Pixels.ToArray(); captured[^1] = fields.Output[^1];
        // 초기 입자로 옮긴 색을 원래 위치에 되돌려 실제 캡처 입력 전체를 관찰한다.
        for (int index = 0; index < fields.Total; index++)
            captured[fields.Particles[index].Pixel] = fields.Particles[index].Color;
        Assert.Equal(expected.Pixels, captured);
        Assert.Equal(expected.Pixels[^1], fields.Output[^1]); Assert.Equal(255, fields.Image.Pixels[^1]);
        // 실제 시각을 전진시키며 원본 입자가 처음/중간/끝색과 완성 픽셀을 소비하게 한다.
        for (uint frame = 0; frame < 24; frame++)
        {
            scene.Visuals.PrepareFrame(frame * .05, frame); session.Kernel.RunFrame();
        }
        Assert.NotEmpty(scene.Visuals.Dirty.Entries); Assert.True(fields.Selected > 0);
        session.WorldRendering.Draw(); Assert.Contains(scene.Screen.Pixels.ToArray(), pixel => pixel != 0);
        Assert.Equal(13, scene.Visuals.Construction.FrameState.CameraX); Assert.Equal(17, scene.Visuals.Construction.FrameState.CameraY);
        session.Pool.AllocatedBytes(parent)[40] &= 0xfe; session.Kernel.RunFrame();
        Assert.Equal(0, session.WorldConstruction.Size); Assert.Equal(0, session.WorldRendering.Size); Assert.Equal(0, session.Kernel.Size());
    }

    /// <summary>건설 반납은 같은 목록의 연소/외부Kernel을 보존하고 전체 미션 반납은 두효과를 모두 정리한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductConstructionReleasePreservesSharedBurningUntilWorldShutdown()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = ConstructionParent(scene);
        scene.Visuals.SetView(0, 0, new(0, 0, 1024, 768));
        _ = session.WorldConstruction.Add(parent, false); _ = session.WorldBurning.Add(parent, 0);
        Assert.Equal(2, session.WorldRendering.Size);
        session.ReleaseConstructionWorld(); Assert.Equal(1, session.WorldRendering.Size); Assert.Equal(1, session.WorldBurning.Size);
        Assert.Throws<InvalidOperationException>(() => session.WorldConstruction);
        Assert.Equal(0u, session.WorldRendering.State.Palette);
        uint foreign = session.Kernel.Add(new BaseProcess()); session.ReleaseBurningWorld();
        Assert.Equal(1, session.Kernel.Size()); Assert.NotNull(session.Kernel.Get(foreign)); session.Kernel.Remove(foreign);
        Assert.Throws<InvalidOperationException>(() => session.WorldRendering);
    }

    /// <summary>원본 그리기의끝 제외clip·투명색·화면밖·잠금 해제 경계를 실제 화면에서 확인한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductConstructionBlitUsesExclusiveClipAndTransparentPixels()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); scene.Visuals.SetView(0, 0, new(0, 0, 1024, 768));
        scene.Screen.SetClip(1, 1, 3, 3);
        byte[] pixels = scene.Screen.Lock(); Array.Fill(pixels, (byte)77); scene.Screen.Unlock();
        scene.Visuals.Construction.Draw(new()
        {
            X = 0, Y = 0, Pitch = 4, Clip = new(0, 0, 4, 4),
            Pixels = new byte[] { 1, 2, 3, 4, 5, 6, 255, 8, 9, 10, 11, 12, 13, 14, 15, 16 },
        });
        byte[] actual = scene.Screen.Pixels.ToArray();
        Assert.Equal(6, actual[scene.Screen.Pitch + 1]); Assert.Equal(77, actual[scene.Screen.Pitch + 2]);
        Assert.Equal(10, actual[scene.Screen.Pitch * 2 + 1]); Assert.Equal(11, actual[scene.Screen.Pitch * 2 + 2]);
        Assert.Equal(77, actual[0]); Assert.Equal(77, actual[scene.Screen.Pitch + 3]); Assert.Equal(77, actual[scene.Screen.Pitch * 3 + 1]);
        Assert.Throws<IndexOutOfRangeException>(() => scene.Visuals.Construction.Draw(new() { X = 1, Y = 1, Pitch = 1, Clip = new(0, 0, 1, 1), Pixels = Array.Empty<byte>() }));
        // 실패 경로도 화면 잠금을 반납해 다음 실제 소비자가 사용할 수 있어야 한다.
        _ = scene.Screen.Lock(); scene.Screen.Unlock();
    }
}
