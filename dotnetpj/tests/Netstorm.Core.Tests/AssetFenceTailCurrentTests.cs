using Netstorm.Core.Original;
using Netstorm.Tests;

// 현재 바리케이드/펜스/전용 꼬리의 실제 가상 분배와 선택된 부모만 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>CPP AssetFenceCurrent/AssetTailCurrent의 원문 순서와 현재 물리를 대조한다.</summary>
public sealed partial class X86RawAssetPostPopTests
{
    /// <summary>같은 일반 자산 입력에 명시 실제 VT만 공급한다.</summary>
    private static Scene AssetTailScene(uint table, uint flags = 1)
    {
        var scene = VortexScene(0, flags); Put(scene.Pool.RawSlot(Source), 0, table); return scene;
    }
    /// <summary>실제 공통/Carrier 부모를 하위 모듈에 전달하고 같은 가상 진입/깊이를 실행한다.</summary>
    private static void RunAssetTail(Scene scene, Func<RawPostPop, RawCarrierPostPop, AssetPostPopHooks> hooks, uint flags = 1)
    {
        var @base = new RawPostPop(scene.Pool, scene.Types, scene.Book);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, scene.CarrierState, scene.CarrierHooks(@base));
        var assets = new RawAssetPostPop(scene.Pool, scene.Types, @base, damageable, scene.State, hooks(@base, damageable));
        scene.Book.Depth++; @base.SetAssetPostPop(assets); @base.PostPopVirtual(Source, flags);
    }
    /// <summary>기존 자산 원문818행 중 PATCH 바리케이드48행을 실제 현재 진입으로 다시 대조한다.</summary>
    [Fact]
    public void BlockerCurrentExistingPatchNative48()
    {
        int count = 0;
        // 기존 기대값을 수정하지 않고 바리케이드 부모만 선택한다.
        foreach (string[] row in X86Fixture.Read("assetpostpop"))
        {
            if (row[0] != "originals" || row[1] != "7") continue;
            var s = new Scene(); s.Prepare(row); RunAssetTail(s, (_, _) => s.Hooks(), X86Fixture.UInt(row[2])); Assert.Equal(row[15..], s.Output()); Assert.Equal(0u, s.Book.Depth); count++;
        }
        Assert.Equal(48, count);
    }
    /// <summary>두 펜스의 원문 P100행만 실제 현재 VT와 직접 Damageable에 연결한다.</summary>
    [Fact]
    public void TwoFenceCurrentNativePostPop100()
    {
        int count = 0;
        // 아크/애시드의 후처리 외 공격/수신/전송 행은 선택하지 않는다.
        foreach ((string name, uint table) in new[] { ("arc-spire", 0x50e668u), ("acid-fence", 0x50e738u) })
        {
            // 저장된 최초 예약 입력과 기대 trace를 그대로 읽는다.
            foreach (string[] row in X86Fixture.Read(name))
            {
                if (row[0] != "P") continue;
                uint[] input = row[1].Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(3, input.Length); var s = AssetTailScene(table, input[0]); s.Pool.RawSlot(Source)[40] = (byte)input[1]; byte[] before = s.Pool.Slot(Source).ToArray(); var h = s.Hooks();
                h.ReserveRegular = () => { s.Events.Add("N:40"); return input[2] != 0; }; h.AddRegular = (sid, value, delay) => s.Events.Add($"R:{value}:{sid}:{BitConverter.SingleToUInt32Bits(delay)}");
                RunAssetTail(s, (_, _) => h, input[0]); Assert.Equal(row[2], s.Output()[0]); Assert.Equal(before, s.Pool.Slot(Source).ToArray()); Assert.Equal(0u, s.Book.Depth); count++;
            }
        }
        Assert.Equal(100, count);
    }
    /// <summary>세 부모의 미최초는 연결 없는 장치/잘못된 TYPE를 앞에서 요구하지 않는다.</summary>
    [Theory]
    [InlineData(0x50e940u)] [InlineData(0x50e668u)] [InlineData(0x50e738u)]
    public void AssetFenceNoFirstSkipsUnboundFront(uint table) { var s = AssetTailScene(table, 0); s.Pool.RawSlot(Source)[10] = 255; s.State.Temples.Count = 99; RunAssetTail(s, (_, _) => new(), 0); Assert.Equal(new[] { "D:0" }, s.Events); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>서버가 아니거나 로딩 중이면 바리케이드 통지를 요구하지 않으며 extra9는 추가 관문이 아니다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void BlockerNonServerOrLoadingSkipsNotify(bool server) { var s = AssetTailScene(0x50e940); s.State.InBattle = server; s.State.LoadingDepth = server ? 1u : 0u; s.Pool.RawSlot(Source)[40] = 9; RunAssetTail(s, (_, _) => new()); Assert.Equal(new[] { "D:1" }, s.Events); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>선택 통지 누락은 실제 직접 부모와 깊이 감소 전에 실패한다.</summary>
    [Fact]
    public void BlockerMissingSelectedNotifyPrecedesParent() { var s = AssetTailScene(0x50e940); s.State.InBattle = true; Assert.Throws<InvalidOperationException>(() => RunAssetTail(s, (_, _) => new())); Assert.Empty(s.Events); Assert.Equal(1u, s.Book.Depth); }
    /// <summary>통지가 바꾼 현재 TYPE/free/extra/권한을 복구하지 않고 직접 부모로 진행한다.</summary>
    [Fact]
    public void BlockerNoticeKeepsCurrentPhysicalLife() { var s = AssetTailScene(0x50e940); s.State.InBattle = true; var h = new AssetPostPopHooks { StartBlocker = sid => { s.Events.Add("B"); s.Pool.RawSlot(sid)[10] = 75; s.Pool.RawSlot(sid)[11] = 9; s.Pool.RawSlot(sid)[40] = 9; s.State.InBattle = false; s.State.LoadingDepth = 1; } }; RunAssetTail(s, (_, _) => h); Assert.Equal(new[] { "B", "D:1" }, s.Events); Assert.Equal(9, s.Pool.Slot(Source)[11]); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>세 실제 바리케이드 VT 별칭도 extra9와 무관하게 서버 최초 통지를 실행한다.</summary>
    [Theory]
    [InlineData(0x50e940u)] [InlineData(0x50ea08u)] [InlineData(0x50ead0u)]
    public void BlockerAliasesIgnoreExtraAtNoticeGate(uint table) { var s = AssetTailScene(table); s.State.InBattle = true; s.Pool.RawSlot(Source)[40] = 9; RunAssetTail(s, (_, _) => s.Hooks()); Assert.Equal(new[] { "K:50", "D:1" }, s.Events); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>두 펜스의 extra1/8/9는 예약 앞단만 생략하고 같은 직접 부모로 간다.</summary>
    [Theory]
    [InlineData(0x50e668u, 1)] [InlineData(0x50e668u, 8)] [InlineData(0x50e668u, 9)]
    [InlineData(0x50e738u, 1)] [InlineData(0x50e738u, 8)] [InlineData(0x50e738u, 9)]
    public void TwoFencesExtraSkipsUnboundReserve(uint table, byte extra) { var s = AssetTailScene(table); s.Pool.RawSlot(Source)[40] = extra; RunAssetTail(s, (_, _) => new()); Assert.Equal(new[] { "D:1" }, s.Events); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>확보 실패 뒤 누락 생성자를 요구하지 않고 변경된 물리를 부모에 유지한다.</summary>
    [Theory]
    [InlineData(0x50e668u)] [InlineData(0x50e738u)]
    public void TwoFencesFailedReserveKeepsMutation(uint table) { var s = AssetTailScene(table); var h = new AssetPostPopHooks { ReserveRegular = () => { s.Events.Add("A"); s.Pool.RawSlot(Source)[10] = 75; s.Pool.RawSlot(Source)[11] = 9; return false; } }; RunAssetTail(s, (_, _) => h); Assert.Equal(new[] { "A", "D:1" }, s.Events); Assert.Equal(9, s.Pool.Slot(Source)[11]); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>확보나 생성자 누락은 각각 실제 선택 위치까지 완료한 효과만 보존한다.</summary>
    [Theory]
    [InlineData(0x50e668u, false)] [InlineData(0x50e668u, true)] [InlineData(0x50e738u, false)] [InlineData(0x50e738u, true)]
    public void TwoFencesMissingSelectedDevice(uint table, bool reserved) { var s = AssetTailScene(table); s.Pool.RawSlot(Source)[10] = 255; var h = new AssetPostPopHooks(); if (reserved) h.ReserveRegular = () => { s.Events.Add("A"); return true; }; Assert.Throws<InvalidOperationException>(() => RunAssetTail(s, (_, _) => h)); Assert.Equal(reserved ? new[] { "A" } : Array.Empty<string>(), s.Events); Assert.Equal(1u, s.Book.Depth); }
    /// <summary>확보 중 extra9로 바뀌어도 이미 선택한 생성자와 원문 지연은 취소하지 않는다.</summary>
    [Theory]
    [InlineData(0x50e668u, 1f)] [InlineData(0x50e738u, 0.3f)]
    public void TwoFencesReserveKeepsSelectedConstructor(uint table, float delay) { var s = AssetTailScene(table); var h = new AssetPostPopHooks { ReserveRegular = () => { s.Events.Add("A"); s.Pool.RawSlot(Source)[40] = 9; return true; }, AddRegular = (sid, value, seconds) => { Assert.Equal(Source, sid); Assert.Equal(0u, value); Assert.Equal(delay, seconds); s.Events.Add("R"); } }; RunAssetTail(s, (_, _) => h); Assert.Equal(new[] { "A", "R", "D:1" }, s.Events); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>여섯 전용 몸체가 비어 있으면 실제 분배 위치에서 실패하며 현재 TYPE/목록 전수 검사로 바꾸지 않는다.</summary>
    [Theory]
    [InlineData(0x509930u)] [InlineData(0x5099e8u)] [InlineData(0x509aa0u)] [InlineData(0x509470u)] [InlineData(0x50dc30u)] [InlineData(0x50e888u)]
    public void AssetTailMissingWholeBodyFailsAtDispatch(uint table) { var s = AssetTailScene(table, 0); s.Pool.RawSlot(Source)[10] = 255; Assert.Throws<InvalidOperationException>(() => RunAssetTail(s, (_, _) => new(), 0)); Assert.Empty(s.Events); Assert.Equal(1u, s.Book.Depth); }
    /// <summary>미최초는 성장형/너겟의 실제 몸체와 명시 썬 바리케이드 장치를 통해 공통 부모로 간다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)]
    public void AssetTailActualVirtualNoFirstSkipsFrontType(int kind)
    {
        uint[] tables = [0x50e888, 0x509470, 0x50dc30]; var s = AssetTailScene(tables[kind], 0); s.Pool.RawSlot(Source)[10] = 255; s.State.Temples.Count = 99;
        RunAssetTail(s, (parent, _) =>
        {
            Action<int, uint> finish = (sid, flags) => { s.Events.Add("P"); parent.PostPopBase(sid, flags); }; var h = new AssetPostPopHooks();
            if (kind == 0) { var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(); h.GrowingIcePostPop = (sid, flags) => RawGrowingIcePostPop.PostPop(s.Pool, frames, sid, flags, null, null, finish); }
            else if (kind == 1) h.SunFencePostPop = finish;
            else { var nugget = new RawNuggetPostPop(s.Pool, new() { Damageable = finish }); h.NuggetPostPop = nugget.PostPop; }
            return h;
        }, 0);
        Assert.Equal(new[] { "P" }, s.Events); Assert.Equal(0u, s.Book.Depth);
    }
    /// <summary>실제 성장형 확보 실패는 앞선 free 변경 뒤 TYPE/생성자를 생략하고 공통 깊이를 감소한다.</summary>
    [Fact]
    public void GrowingActualVirtualFailedReserveSkipsType()
    {
        var s = AssetTailScene(0x50e888); s.Pool.RawSlot(Source)[10] = 255; var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        RunAssetTail(s, (parent, _) => new() { GrowingIcePostPop = (sid, flags) => RawGrowingIcePostPop.PostPop(s.Pool, frames, sid, flags, () => { s.Events.Add("A"); s.Pool.RawSlot(Source)[11] = 9; return false; }, null, (current, mode) => { s.Events.Add("P"); parent.PostPopBase(current, mode); }) });
        Assert.Equal(new[] { "A", "P" }, s.Events); Assert.Equal(9, s.Pool.Slot(Source)[11]); Assert.Equal(0u, s.Book.Depth);
    }
    /// <summary>실제 너겟의 두 독립 확보 실패는 난수/생성자를 건너뛰고 직접 Damageable로 간다.</summary>
    [Fact]
    public void NuggetActualVirtualIndependentFailedAllocations()
    {
        var s = AssetTailScene(0x50dc30);
        RunAssetTail(s, (_, damageable) => { var nugget = new RawNuggetPostPop(s.Pool, new() { HasRegular = (_, value) => { Assert.Equal(1u, value); s.Events.Add("H"); return false; }, Allocate = () => { s.Events.Add("A"); return false; }, Damageable = damageable.DamageablePostPop }); return new() { NuggetPostPop = nugget.PostPop }; });
        Assert.Equal(new[] { "H", "A", "A", "D:1" }, s.Events); Assert.Equal(0u, s.Book.Depth);
    }
}
