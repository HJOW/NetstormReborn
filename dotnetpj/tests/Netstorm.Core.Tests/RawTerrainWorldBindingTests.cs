// 초기 도장 월드 훅의 연결·전달을 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 초기 지형 월드 바인딩을 검증하는 테스트 클래스다. 2026-11-10 R4 월드 도장 훅 이식에서 추가됐다.
public sealed class RawTerrainWorldBindingTests
{
    /// <summary>188개 타입 목록을 만든다. 2026-11-10 R4 월드 도장 훅 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();

    /// <summary>188개 프레임 목록을 만든다. 2026-11-10 R4 월드 도장 훅 이식에서 추가됐다.</summary>
    private static List<RiftTypeFrames> Frames188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new RiftTypeFrames([])).ToList();

    /// <summary>생성·배치 조회까지 되는 전체 물리 저장소를 만든다. 2026-11-10 R4 월드 도장 훅 이식에서 추가됐다.</summary>
    /// <param name="types">같은 전역 TYPE 표다.</param>
    /// <param name="frames">같은 장기 프레임 표다.</param>
    private static RawSurfaceStorage FullStorage(List<OriginalTypeRecord> types, List<RiftTypeFrames> frames)
    {
        var shapes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();
        shapes[70].FrameCount = 1;
        shapes[70].Loaded = true;
        shapes[70].Frames.Add(new SquidDisplayFrame { Width = 10, Height = 12, HotspotX = 2, HotspotY = 3, CellWidth = 8f, CellHeight = 6f });
        return new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
    }

    /// <summary>생성자 주소 표를 채운 타입 표를 만든다. 2026-11-10 R4 월드 도장 훅 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> DisplayTypes()
    {
        List<OriginalTypeRecord> types = Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToList();
        types[70].ConstructorAddress = 0u;
        return types;
    }

    /// <summary>같은 월드에 묶을 호출자 효과를 만든다. 2026-11-10 R4 월드 도장 훅 이식에서 추가됐다.</summary>
    private static TerrainStampHooks Effects() => new()
    {
        ChunkRegion = (_, _) => 127,
        Assertion = _ => { },
    };

    /// <summary>빈 입력·누락 연결을 거부한다. 2026-11-10 R4 월드 도장 훅 이식에서 추가됐다.</summary>
    [Fact]
    public void GuardsInputs()
    {
        var types = Types188();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullStorage(types, frames), frames);
        var state = new TerrainStampState();
        Action<SquidRegionAssert> region = _ => { };
        Assert.Throws<ArgumentNullException>(() => RawTerrainWorldBinding.MakeTerrainStampWorldHooks(null!, state, region, Effects()));
        Assert.Throws<ArgumentNullException>(() => RawTerrainWorldBinding.MakeTerrainStampWorldHooks(world, null!, region, Effects()));
        Assert.Throws<ArgumentNullException>(() => RawTerrainWorldBinding.MakeTerrainStampWorldHooks(world, state, region, null!));
        Assert.Throws<ArgumentException>(() => RawTerrainWorldBinding.MakeTerrainStampWorldHooks(world, state, region, new TerrainStampHooks { Assertion = _ => { } }));
        Assert.Throws<ArgumentException>(() => RawTerrainWorldBinding.MakeTerrainStampWorldHooks(world, state, region, new TerrainStampHooks { ChunkRegion = (_, _) => 127 }));
        Assert.Throws<ArgumentException>(() => RawTerrainWorldBinding.MakeTerrainStampWorldHooks(world, state, null, Effects()));
    }

    /// <summary>묶은 훅이 같은 월드의 생성·소유자·Pop·지역으로 통한다. 2026-11-10 R4 월드 도장 훅 이식에서 추가됐다.</summary>
    [Fact]
    public void HooksReachSameWorld()
    {
        var types = DisplayTypes();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullStorage(types, frames), frames);
        var state = new TerrainStampState();
        TerrainStampHooks hooks = RawTerrainWorldBinding.MakeTerrainStampWorldHooks(world, state, _ => { }, Effects());
        Assert.NotNull(hooks.Create);
        Assert.NotNull(hooks.SetOwner);
        Assert.NotNull(hooks.Pop);
        Assert.NotNull(hooks.SetRegion);
        // 같은 풀에 실제 슬롯을 만들고 타입을 남긴다.
        int sid = hooks.Create!(70u, 0u);
        Assert.True(sid >= 5);
        Assert.Equal(70, world.Core().Pool.Slot(sid)[RawSidPool.TypeOffset]);
        // 같은 슬롯의 소유자 바이트에 쓴다.
        hooks.SetOwner!(sid, 3u);
        Assert.Equal(3, world.Core().Pool.Slot(sid)[34]);
        // 같은 월드의 공간·지역 등록은 예외 없이 돈다.
        hooks.Pop!(sid, 20f, 20f, 0u);
        hooks.SetRegion!(sid, 0);
    }
}
