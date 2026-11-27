using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 일반 객체 표시용 자산과 원본 TYPE 번호의 연결을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>자산 속성의 번호·절삭과 셰이프 사본 수명을 확인한다.</summary>
public sealed class SpriteAssetViewsTests
{
    /// <summary>내장 TYPE을 건너뛰고 기본 프레임과 막대 속성을 실제 파일 순서에 맞춘다.</summary>
    [Fact]
    public void AssetDefinitionsPreserveTypeNumbersAndIndependentShapes()
    {
        var shapes = Enumerable.Range(0, 72).Select(_ => new SquidDisplayShape()).ToArray();
        shapes[70].Loaded = true;
        shapes[70].FrameCount = 1;
        shapes[70].Frames.Add(new() { Width = 12, Height = 9 });
        var first = new RiftTypeDefinition();
        first.SpecialFrames.DefaultFrame = 7;
        first.SpecialFrames.GumpFrame = 3;
        first.Properties.Add(new("casttime", 13.9));
        first.Properties.Add(new("praytime", -2.9));
        first.Properties.Add(new("effecttime", 25.5));
        var second = new RiftTypeDefinition();
        second.SpecialFrames.DefaultFrame = -1;
        SpriteTypeView[] views = SpriteAssetViews.Build(shapes, [first, second]);
        Assert.Equal(72, views.Length);
        Assert.Equal(0u, views[69].GumpFrame);
        Assert.Equal((7u, 13, -2, 25), (views[70].GumpFrame, views[70].Capacity, views[70].ConstructionTime, views[70].Duration));
        Assert.Equal(uint.MaxValue, views[71].GumpFrame);
        Assert.Equal((0, 0, 0), (views[71].Capacity, views[71].ConstructionTime, views[71].Duration));
        Assert.NotSame(shapes[70], views[70].Shape);
        Assert.NotSame(shapes[70].Frames[0], views[70].Shape.Frames[0]);
        shapes[70].Frames[0].Width = 3;
        Assert.Equal(12, views[70].Shape.Frames[0].Width);
    }

    /// <summary>정의가 실제 TYPE 표를 넘으면 잘못된 번호를 조용히 만들지 않는다.</summary>
    [Fact]
    public void RejectsDefinitionsOutsideTypeTable()
    {
        var shapes = Enumerable.Range(0, 70).Select(_ => new SquidDisplayShape()).ToArray();
        Assert.Throws<ArgumentException>(() => SpriteAssetViews.Build(shapes, [new RiftTypeDefinition()]));
    }

    /// <summary>실제 배포 자산의 전체 TYPE·SHP가 같은 번호 공간으로 조립되는지 확인한다.</summary>
    [Fact]
    public void ActualAssetTablesFitTheDisplayTypeSpace()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var storage = new RawSurfaceStorage();
        var assets = new HostAssetTables(new HostStartupScene(data), storage);
        SpriteTypeView[] views = SpriteAssetViews.Build(storage.Shapes, assets.Definitions);
        Assert.Equal(storage.Types.Count, views.Length);
        Assert.Equal(storage.Shapes.Count, views.Length);
        Assert.True(assets.Definitions.Count > 0);
        Assert.Contains(views.Skip(70), view => view.Shape.Loaded);
        Assert.Equal(unchecked((uint)assets.Definitions[0].SpecialFrames.DefaultFrame), views[70].GumpFrame);
    }
}
