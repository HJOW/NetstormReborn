using Netstorm.Client.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 일반 객체와 Client가 실제 공통 색표를 공유하는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 밝기 표와 TYPE별 소유자 표를 호출마다 조회한다.</summary>
public sealed class SpriteColorBindingTests
{
    /// <summary>객체 모드별 표와 바뀐 소유자 색 번호는 같은 Client 장치에서 읽는다.</summary>
    [Fact]
    public void UsesActualSharedMapsAndCurrentOwnerPalette()
    {
        var input = new ClientColorSortInput();
        uint[] logical = Enumerable.Range(0, 256).Select(i => (uint)i * 0x010101).ToArray();
        input.CaptureWithPrimary(logical, new byte[0x3800]);
        input.Rebuild(0);
        input.OwnerTables.Initialize(input.Primary, 188, 82, new UIntBox());
        int palette = 1;
        var binding = new SpriteColorBinding(input, owner => { Assert.Equal(3U, owner); return palette; });
        Assert.Same(input.Maps[0], binding.Color(SpriteShapeMode.Bright160, 70, 0));
        Assert.Same(input.Maps[4], binding.Color(SpriteShapeMode.Abstract, 70, 0));
        Assert.Same(input.Maps[6], binding.Color(SpriteShapeMode.Dead, 70, 0));
        Assert.Same(input.Maps[7], binding.Color(SpriteShapeMode.Shadow, 70, 0));
        Assert.Same(input.OwnerTables.Color(82, 1), binding.Color(SpriteShapeMode.Owner, 82, 3));
        palette = 2;
        Assert.Same(input.OwnerTables.Color(82, 2), binding.Color(SpriteShapeMode.Owner, 82, 3));
        palette = 9;
        Assert.Throws<ArgumentOutOfRangeException>(() => binding.Color(SpriteShapeMode.Owner, 82, 3));
        Assert.Throws<ArgumentException>(() => binding.Color(SpriteShapeMode.Plain, 82, 0));
    }

    /// <summary>아직 만들지 않은 공통 표는 임시 색표로 대체하지 않는다.</summary>
    [Fact]
    public void RequiresInitializedSharedMaps()
    {
        var binding = new SpriteColorBinding(new ClientColorSortInput(), _ => 1);
        Assert.Throws<InvalidOperationException>(() => binding.Color(SpriteShapeMode.Shadow, 70, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => binding.Color(SpriteShapeMode.Owner, 82, 3));
    }
}
