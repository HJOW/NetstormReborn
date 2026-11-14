using System.Buffers.Binary;
using Netstorm.Core.Original;

// 원본대조 외에 현재 owner/선택경계/함수 사본을 같은 실제 몸체에 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>현재 생성/연소점의 장기 대여와 선택시점 추가 검사다.</summary>
public sealed partial class BurningVisualOracleTests
{
    /// <summary>cold팔레트는한번만 생성하며 원문 부분효과/현재토큰/함수 사본을 보존한다.</summary>
    [Fact]
    public void ColdPaletteUsesCurrentSharedTokenAndCopiedCallbacksOnlyOnce()
    {
        var pool = new RawSidPool(24000, false); var events = new List<string>();
        var registry = new RawRenderRegistry(pool, new() { Reserve = amount => { events.Add($"R:{amount}"); return true; }, ReleaseArray = () => Assert.Fail("40칸 내 기존배열 반납"), Attach = (_, _, _, _) => Assert.Fail("부모0부착") });
        var state = new BurningVisualState(); var hooks = new BurningVisualHooks
        {
            Reserve = amount => { events.Add($"V:{amount}"); return true; }, CreatePalette = () => { events.Add("C"); return 7; },
            Gradient = (first, last, count, noise) =>
            {
                Assert.Equal(new double[] { 1, 115.0 / 255, 7.0 / 255 }, first); Assert.Equal(new double[] { 1, 245.0 / 255, 117.0 / 255 }, last); Assert.Equal(8u, count); Assert.Equal(2.4, noise);
                state.Palette = 9; events.Add("G");
            },
            BuildPalette = token => events.Add($"B:{token}"),
        };
        var visual = new RawBurningVisual(pool, registry, state, hooks); hooks.Reserve = _ => false; hooks.CreatePalette = () => throw new InvalidOperationException("입력함수 교체");
        var a = new BurningVisualFields(); visual.Initialize(a, 1, 0, uint.MaxValue); var b = new BurningVisualFields(); visual.Initialize(b, 2, 0, 1);
        Assert.Equal(new[] { "R:160", "V:3600", "V:8", "C", "G", "B:9", "V:3600" }, events);
        Assert.Equal(9u, state.Palette); Assert.Equal(0u, a.Palette); Assert.Equal(uint.MaxValue, a.Mode); Assert.Equal(150, a.Particles.Length); Assert.Equal(150, b.Particles.Length);
        Assert.Equal(new uint[] { 1, 2 }, registry.Slots[..2].ToArray());
    }
    /// <summary>누락된cold팔레트는 실제 등록/150칸 초기화 뒤 선택지점에서 실패한다.</summary>
    [Fact]
    public void MissingColdPalettePreservesEarlierRegistrationAndParticleInitialization()
    {
        var pool = new RawSidPool(24000, false); var registry = new RawRenderRegistry(pool, new() { Reserve = _ => true, ReleaseArray = () => { }, Attach = (_, _, _, _) => { } });
        var visual = new RawBurningVisual(pool, registry, new(), new() { Reserve = _ => true }); var fields = new BurningVisualFields();
        Assert.Throws<InvalidOperationException>(() => visual.Initialize(fields, 17, 0, 1)); Assert.Equal(150u, fields.Capacity); Assert.Equal(150, fields.Particles.Length); Assert.Equal(17u, registry.Slots[0]);
    }
    /// <summary>현재TYPE/extra/VT와 실제 장치 교체를 읽으며 checking0에 불필요한 상세 장치를 요구하지 않는다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void VictimAdapterUsesCurrentRawRolesAndCurrentDiagnosticDevices(string edition)
    {
        var s = new Scene(edition); var state = new BurningVictimState(); var hooks = RawBurningVictim.MakeHooks(s.Pool, s.Types, state); int extra = edition == "originals" ? 40 : 35;
        s.Pool.RawSlot(50).Clear(); s.Pool.RawSlot(50)[10] = 165; s.Types[165].Flags2 = 0x400000; s.Pool.RawSlot(50)[extra] = 1;
        Assert.True(hooks.HasHitPoints!(50)); s.Pool.RawSlot(50)[extra] = 0; Assert.False(hooks.HasHitPoints(50)); s.Types[165].Flags2 = 0; s.Types[165].Flags1 = 16; Assert.True(hooks.HasHitPoints(50));
        Assert.False(hooks.Vtable!(50)); s.Pool.RawSlot(50)[0] = 1; Assert.True(hooks.Vtable(50)); Assert.True(hooks.Robust!(65534, true));
        state.Checking = new(0x100u); Assert.Throws<InvalidOperationException>(() => hooks.Robust(50, true));
        state.Robust = (sid, live) => sid == 50 && live; Assert.True(hooks.Robust(50, true)); state.Robust = (_, _) => false; Assert.False(hooks.Robust(50, true));
    }
    /// <summary>실제getter 진입에서 선택한 bool/TYPE주소는 진단이 어댑터 포인터를 교체해도 유지한다.</summary>
    [Fact]
    public void GetterKeepsSelectedOwnerAddressesAcrossDiagnosticRebinding()
    {
        var s = new Scene("originals"); bool originalQuarter = false; var priest = new UIntBox { Value = 74 }; var dais = new UIntBox { Value = 165 };
        s.State.SharedWeakenedMana = () => originalQuarter; s.State.SharedManaType = priest; s.State.SharedDaisType = dais;
        s.State.Victim.Checking = new(0x100u); s.State.Victim.Robust = (_, _) => true; s.Types[74].Flags2 = 0x400000; s.Types[74].MaxHitPoints = 200;
        s.Pool.RawSlot(50).Clear(); s.Pool.RawSlot(50)[10] = 74; s.Pool.RawSlot(50)[40] = 1; BinaryPrimitives.WriteInt32LittleEndian(s.Pool.RawSlot(50)[26..], 50);
        var fields = new BurningVisualFields { Victim = 50 }; fields.Image.Width = 30; fields.Image.Height = 2; int assertions = 0;
        s.State.Victim.Assertion = (_, _, line) =>
        {
            Assert.Equal(0x1e3u, line); assertions++; originalQuarter = true;
            s.State.SharedWeakenedMana = () => false; s.State.SharedManaType = new() { Value = 75 }; s.State.SharedDaisType = new() { Value = 74 };
            s.State.Victim.Assertion = null;
        };
        s.Sources.Select(fields); Assert.Equal(1, assertions); Assert.Equal(1, fields.Selected); Assert.Equal(50, BinaryPrimitives.ReadInt32LittleEndian(s.Pool.Slot(50)[26..]));
        Assert.Equal(74u, priest.Value); Assert.Equal(165u, dais.Value); Assert.Equal(75u, s.State.SharedManaType.Value);
    }
}
