using Netstorm.Client.Gump;

namespace Netstorm.Client.Tests; // 같은 공통 출력의 외부 소유 순서와 반납을 검증한다.

/// <summary>Source의 공통/고정/독립 장치와 값 전달·단조 번호·실제 문맥을 검증한다.</summary>
public sealed class GumpExternalDrawingTests
{
    /// <summary>실제 동일 픽셀/배경 문맥을 공급한다. 출력은 각 검사에서 관측한다.</summary>
    private static (GumpPixelTarget Target, GumpGlyphColorState Colors, GumpBackgroundDraw Background) Context()
    {
        var collection = new GumpCollection(new(), new() { Node = _ => null }); var clip = new GumpDrawState();
        return (new(new byte[16], 4, 4, clip), new(), new(collection, clip, new(), new() { Node = _ => null }));
    }
    /// <summary>공통→고정 번호순→독립 순서와 현재 owner를 보며 함수 슬롯 변경은 등록 뒤 영향을 주지 않는다.</summary>
    [Fact]
    public void PriorityAndFunctionSlotCopiesKeepOtherOwnersAlive()
    {
        var registry = new GumpExternalDrawing(); var context = Context(); string result = ""; bool common = true, fixedOwner = true;
        // 같은 현재 출력 문맥을 관찰하는 독립 소유자 함수 슬롯이다.
        GumpExternalGumps Device(string label, Func<uint, bool> owns) => new() { Owns = owns, Draw = (_, target, colors, background) => { Assert.Same(context.Target, target); Assert.Same(context.Colors, colors); Assert.Same(context.Background, background); result += label; } };
        registry.BindCommonButtons(Device("C", _ => common)); registry.SetExternalGumps(8, Device("8", _ => fixedOwner)); registry.SetExternalGumps(2, Device("2", _ => fixedOwner));
        var hooks = Device("A", _ => true); uint a = registry.RegisterExternalGumps(hooks), b = registry.RegisterExternalGumps(Device("B", _ => true)); hooks.Draw = (_, _, _, _) => Assert.Fail("등록 뒤 함수 슬롯은 값 복사해야 함");
        Assert.True(registry.TryDraw(7, context.Target, context.Colors, context.Background)); common = false; Assert.True(registry.TryDraw(7, context.Target, context.Colors, context.Background)); fixedOwner = false; Assert.True(registry.TryDraw(7, context.Target, context.Colors, context.Background));
        registry.UnregisterExternalGumps(a); Assert.True(registry.TryDraw(7, context.Target, context.Colors, context.Background)); Assert.Equal("C2AB", result); Assert.Equal(1, registry.RegistrationCount);
        registry.UnregisterExternalGumps(99); registry.UnregisterExternalGumps(b); uint c = registry.RegisterExternalGumps(Device("N", _ => true)); Assert.True(c > b && b > a);
    }
    /// <summary>외부 텍스트는 Owns와 별도이며 고정 슬롯 뒤 독립 등록 순서를 사용한다.</summary>
    [Fact]
    public void TextLookupUsesItsOwnCurrentOrderAndFixedRemovalIsScoped()
    {
        var registry = new GumpExternalDrawing(); var first = new TextGumpNode(); var second = new TextGumpNode(); var common = new TextGumpNode();
        registry.BindCommonButtons(new() { Owns = _ => true, Text = _ => common, Draw = (_, _, _, _) => { } });
        registry.SetExternalGumps(0, new() { Owns = _ => false, Text = _ => first }); uint registration = registry.RegisterExternalGumps(new() { Owns = _ => true, Text = _ => second, Draw = (_, _, _, _) => { } });
        Assert.Same(first, registry.Text(3)); registry.SetExternalGumps(new()); Assert.Same(second, registry.Text(3)); registry.UnregisterExternalGumps(registration); Assert.Null(registry.Text(3));
    }
    /// <summary>미선택 출력은 자연 반환하지만 선택한 빈 출력과 잘못된 독립 등록은 거부한다.</summary>
    [Fact]
    public void MissingSelectedDrawFailsWithoutInventingSuccess()
    {
        var registry = new GumpExternalDrawing(); var context = Context(); Assert.False(registry.TryDraw(1, context.Target, context.Colors, context.Background));
        Assert.Throws<ArgumentException>(() => registry.RegisterExternalGumps(new() { Owns = _ => true })); registry.SetExternalGumps(new() { Owns = _ => true });
        Assert.Throws<InvalidOperationException>(() => registry.TryDraw(1, context.Target, context.Colors, context.Background));
    }
}
