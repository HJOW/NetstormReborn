using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 신전 현재 진단 자료와 실제 현재 가상 진입을 같은 원본 장면에서 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 신전 936행과 현재 공유 목록/후처리 관문을 비교한다.</summary>
public sealed partial class X86RawAssetPostPopTests
{
    /// <summary>원문 결과를 재계산하지 않고 일반 신전 입력만 구성한다.</summary>
    private static Scene VortexScene(int kind = 9, uint flags = 1)
    {
        string[] input = ["originals", kind.ToString(), flags.ToString(), "0", "0", "0", "0", "7", "0", "25", "0", "1000", "0", "0", "0", "0", "0", "0", "0", "0", "0"];
        var scene = new Scene(); scene.Prepare(input); scene.State.Temples.Count = 0; scene.State.Residences.Count = 0; return scene;
    }
    /// <summary>실제 공통 깊이 증가와 현재 가상 분배를 실행한다.</summary>
    private static void RunVortex(Scene scene, AssetPostPopHooks hooks, uint flags = 1, byte[]? records = null)
    {
        var @base = new RawPostPop(scene.Pool, scene.Types, scene.Book);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, scene.CarrierState, scene.CarrierHooks(@base));
        var assets = new RawAssetPostPop(scene.Pool, scene.Types, @base, damageable, scene.State, hooks);
        if (records is not null) assets.BindPlayerRecords(records);
        scene.Book.Depth++; @base.SetAssetPostPop(assets); @base.PostPopVirtual(Source, flags);
    }
    /// <summary>원문 진단/음향 장치에 준 입력 쓰기만 공급한다.</summary>
    private static void MutateVortex(Scene scene, ref uint bridge, uint mutation)
    {
        Span<byte> raw = scene.Pool.RawSlot(Source);
        if (mutation == 1) { raw[34] = 8; raw[11] = 9; raw[40] = 9; Put(raw, 12, 0x4567, 2); Put(raw, 36, 0x6789); scene.State.LoadingDepth = 1; bridge = 75; raw[10] = 75; }
        else if (mutation == 2) { raw[34] = 0; scene.State.LocalOwner = 8; bridge = 70; Put(raw, 12, 0x7777, 2); }
        else if (mutation is >= 3 and <= 5) { raw[10] = 75; raw[11] = 9; raw[40] = 0; raw[34] = mutation >= 4 ? (byte)8 : (byte)1; scene.State.LoadingDepth = 0; if (mutation == 4) { scene.State.Selected = 62; scene.State.LocalOwner = 8; } if (mutation == 5) bridge = 75; }
    }
    /// <summary>936개 원문 관측의 진단 복귀·raw·두 목록·전체 Player 기록·전역을 대조한다.</summary>
    [Fact]
    public void VortexNativeDiagnosticsAndReturn936()
    {
        int count = 0;
        // 명시 원문 입력과 기대값을 그대로 재생한다.
        foreach (string[] row in X86Fixture.Read("vortex-current"))
        {
            Assert.Equal(13, row.Length); string[] input = row[0].Split(','), control = row[1].Split(','); Assert.Equal(14, input.Length); Assert.Equal(5, control.Length);
            string[] legacy = Enumerable.Repeat("0", 21).ToArray(); legacy[0] = "originals"; input.CopyTo(legacy, 1);
            var scene = new Scene(); scene.Prepare(legacy); scene.Pool.RawSlot(Source)[34] = (byte)X86Fixture.UInt(control[0]); scene.Types[75].Flags2 = 0x200;
            uint bridge = X86Fixture.UInt(control[4]), mutation = X86Fixture.UInt(control[3]); scene.State.SharedBridgeType = () => bridge; byte[] records = new byte[9 * 0xb4];
            AssetPostPopHooks hooks = scene.Hooks(); Action<uint> sound = hooks.PlayImpactSound!;
            hooks.PlayImpactSound = value => { if (mutation == 5) MutateVortex(scene, ref bridge, 5); sound(value); };
            hooks.IslandExists = _ => control[1] != "0";
            hooks.SetOwner = (sid, owner) => { scene.Events.Add($"O:{sid}:{owner}"); scene.Pool.RawSlot(sid)[34] = (byte)owner; };
            hooks.Assertion = (expression, file, line) =>
            {
                scene.Events.Add($"X:{Convert.ToHexStringLower(Encoding.ASCII.GetBytes(expression))}:{Convert.ToHexStringLower(Encoding.ASCII.GetBytes(file))}:{line}");
                if (mutation is >= 1 and <= 4) MutateVortex(scene, ref bridge, mutation);
                if (control[2] == "0") throw new InvalidOperationException("명시 Vortex 진단 중단");
            };
            string outcome = "return";
            try { RunVortex(scene, hooks, X86Fixture.UInt(input[1]), records); }
            catch (InvalidOperationException error) when (error.Message == "명시 Vortex 진단 중단") { outcome = "assertion"; }
            string[] output = scene.Output(); var fields = new List<uint>();
            // 세 관측 Player의 네 DWORD와 전체9기록 Adler를 함께 비교한다.
            foreach (int owner in new[] { 0, 1, 8 })
            {
                // 초기/복사/새 신전/복사 필드를 물리 순서로 읽는다.
                foreach (int offset in new[] { 0x7c, 0x80, 0x84, 0x88 }) fields.Add(Get(records, owner * 0xb4 + offset));
            }
            Assert.Equal(row[2], output[0]); Assert.Equal(row[3], output[1]); Assert.Equal(row[4], output[2]); Assert.Equal(row[5], output[4]);
            Assert.Equal(X86Fixture.UInt(row[6]), X86Fixture.Adler32(records)); Assert.Equal(row[7], string.Join(',', fields));
            Assert.Equal(X86Fixture.UInt(row[8]), scene.State.LoadingDepth); Assert.Equal(X86Fixture.UInt(row[9]), scene.State.Selected);
            Assert.Equal(X86Fixture.UInt(row[10]), scene.State.LocalOwner); Assert.Equal(X86Fixture.UInt(row[11]), bridge); Assert.Equal(row[12], outcome); Assert.Equal(0u, scene.Book.Depth); count++;
        }
        Assert.Equal(936, count);
    }
    /// <summary>미최초는 공통 부모만 수행하고 미도달 TYPE/장치를 요구하지 않는다.</summary>
    [Theory]
    [InlineData(9)] [InlineData(10)] [InlineData(11)]
    public void VortexNoFirstSkipsDevicesAndType(int kind) { var s = VortexScene(kind, 0); s.Pool.RawSlot(Source)[10] = 255; s.State.Temples.Count = 99; RunVortex(s, new(), 0); Assert.Equal(new[] { "D:0" }, s.Events); Assert.Equal(0u, s.Book.Depth); Assert.Equal(4u, s.State.TempleCount); }
    /// <summary>owner0 진단 누락은 먼저 완료한 공통 깊이 감소를 되돌리지 않는다.</summary>
    [Fact]
    public void VortexMissingOwnerDiagnosticFollowsCommon() { var s = VortexScene(flags: 2); s.Pool.RawSlot(Source)[34] = 0; Assert.Throws<InvalidOperationException>(() => RunVortex(s, new(), 2)); Assert.Equal(new[] { "D:2" }, s.Events); Assert.Equal(0u, s.Book.Depth); Assert.Equal(0u, s.State.Players[0].NewTemple); }
    /// <summary>섬 진단·음향·다리 통지 누락은 각각 원문 위치까지 완료한 효과를 유지한다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)]
    public void VortexMissingSelectedFrontRetainsPrefix(int mode)
    {
        var s = VortexScene(); var h = s.Hooks();
        if (mode == 0) h.IslandExists = _ => false;
        else if (mode == 1) { h.TypeDword = (_, _) => { s.Events.Add("F"); return 17; }; h.PlayImpactSound = null; }
        else { s.State.BridgeType = 70; h.NotifySurface = null; }
        Assert.Throws<InvalidOperationException>(() => RunVortex(s, h)); Assert.Equal(0u, s.Book.Depth); Assert.Equal(5u, s.State.TempleCount);
        Assert.Equal(mode == 1 ? 0xabcdu : 0u, Get(s.Pool.Slot(Source), 12, 2)); Assert.Equal(mode == 0 ? 1u : 0u, s.State.Temples.Count);
        Assert.Equal(mode == 0 ? 50u : 0u, s.State.Players[1].InitialTemple);
    }
    /// <summary>두 실제 공유 목록은 물리 저장 뒤 count를 갱신하고 중복 SID를 다시 넣지 않는다.</summary>
    [Fact]
    public void VortexBoundUniqueListsKeepDuplicatesOut()
    {
        var s = VortexScene(); uint[] temples = [7, 8, 9, 10], residences = [11, 12, 13, 14]; uint tc = 0, rc = 0, stores = 0;
        var first = new RawPostPopList(); first.BindCurrent(new() { Storage = () => temples, Capacity = () => 4, ReadCount = () => tc, WriteCount = value => { Assert.Equal(50u, temples[tc]); stores++; tc = value; } });
        var second = new RawPostPopList(); second.BindCurrent(new() { Storage = () => residences, Capacity = () => 4, ReadCount = () => rc, WriteCount = value => { Assert.Equal(50u, residences[rc]); stores++; rc = value; } });
        s.State.SharedTemples = first; s.State.SharedResidences = second; RunVortex(s, s.Hooks()); RunVortex(s, s.Hooks());
        Assert.Equal(2u, stores); Assert.Equal(1u, tc); Assert.Equal(1u, rc); Assert.Equal(new uint[] { 50, 8, 9, 10 }, temples); Assert.Equal(new uint[] { 50, 12, 13, 14 }, residences); Assert.Equal(0u, s.State.Temples.Count);
    }
    /// <summary>signed 가득 찬 관문은 실제 배열/쓰기를 읽지 않고 음수 저장은 앞선 효과 뒤 실패한다.</summary>
    [Theory]
    [InlineData(4u, 4u, false)] [InlineData(0u, uint.MaxValue, false)] [InlineData(0x80000000u, 0x80000000u, false)] [InlineData(uint.MaxValue, 4u, true)]
    public void VortexSignedUniqueBoundary(uint count, uint capacity, bool fails)
    {
        var s = VortexScene(); var list = new RawPostPopList(); list.BindCurrent(new() { Storage = () => throw new Exception("미도달 배열"), Capacity = () => capacity, ReadCount = () => count, WriteCount = _ => throw new Exception("미도달 쓰기") }); s.State.SharedTemples = list;
        if (fails) Assert.Throws<ArgumentOutOfRangeException>(() => RunVortex(s, s.Hooks())); else RunVortex(s, s.Hooks());
        Assert.Equal(0u, s.Book.Depth); Assert.Equal(50u, s.State.Players[1].InitialTemple); Assert.Equal(fails ? 0u : 1u, s.State.Residences.Count);
    }
    /// <summary>선택 DWORD와 Form TYPE의 물리 오류는 두 목록 뒤에만 거부한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void VortexSelectedPhysicalBoundsAfterLists(bool wide) { var s = VortexScene(); s.State.Selected = wide ? 65598u : 62u; Put(s.Pool.RawSlot(62), 18, wide ? 70u : 255u, 2); Assert.Throws<ArgumentOutOfRangeException>(() => RunVortex(s, s.Hooks())); Assert.Equal(1u, s.State.Temples.Count); Assert.Equal(1u, s.State.Residences.Count); Assert.Equal(25u, Get(s.Pool.Slot(Source), 36)); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>공통 extra8 생략 뒤에도 원소 포장의 독립 현재 프레임/확보를 실행한다.</summary>
    [Theory]
    [InlineData(9)] [InlineData(10)] [InlineData(11)]
    public void VortexWrapperUsesPhysicalFrameAfterExtra8(int kind) { var s = VortexScene(kind); s.Pool.RawSlot(Source)[40] = 8; var h = new AssetPostPopHooks { ReserveRegular = () => { s.Events.Add("A"); return false; } }; if (kind != 10) h.TypeDword = (sid, offset) => { Assert.Equal(0x134u, offset); s.Events.Add("F"); s.Pool.RawSlot(sid)[11] = 9; return 0x87654321; }; RunVortex(s, h); Assert.Equal(kind == 10 ? new[] { "D:1", "A" } : new[] { "D:1", "F", "A" }, s.Events); Assert.Equal(kind == 10 ? 25u : 0x87654321u, Get(s.Pool.Slot(Source), 36)); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>등록이 로딩을 켜면 현재 내용물 관문이 연결 없는 장치를 건너뛴다.</summary>
    [Fact]
    public void VortexRegistrationChangesLoadingBeforeContent() { var s = VortexScene(); var h = s.Hooks(); h.RegisterTemple = (_, _) => s.State.LoadingDepth = 1; h.HasContent = null; h.CreateSlot = null; h.RefreshProduction = null; RunVortex(s, h); Assert.Equal(1u, s.State.LoadingDepth); Assert.Equal(1u, s.State.Temples.Count); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>선택 배치 취소/내용물 생성/생산 갱신의 누락은 앞선 목록/프레임을 유지한다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)]
    public void VortexMissingSelectedTailRetainsEarlierState(int mode) { var s = VortexScene(); var h = s.Hooks(); if (mode == 0) { s.State.Selected = 62; h.ClearPlacement = null; } else { h.HasContent = (_, _) => { s.Events.Add("H"); return mode == 2; }; h.CreateSlot = null; h.RefreshProduction = null; } Assert.Throws<InvalidOperationException>(() => RunVortex(s, h)); Assert.Equal(1u, s.State.Temples.Count); Assert.Equal(1u, s.State.Residences.Count); Assert.Equal(mode == 0 ? 25u : 27u, Get(s.Pool.Slot(Source), 36)); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>현재 owner 효과 뒤 boss 변경은 미도달 삭제 장치를 요구하지 않는다.</summary>
    [Fact]
    public void VortexOwnerEffectChangesCurrentBoss() { var s = VortexScene(); var authority = new RawAuthorityState { Boss = true }; s.State.SharedAuthority = authority; var h = s.Hooks(); h.SetOwner = (_, _) => authority.Boss = false; h.RegionNumber = null; h.Destroy = null; RunVortex(s, h); Assert.False(authority.Boss); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>삭제 전에 확보한 다음 후보는 삭제 효과로 free가 되어도 현재 위치에서 처리한다.</summary>
    [Fact]
    public void VortexIteratorKeepsNextBeforeDestroy()
    {
        var s = VortexScene(); s.State.Boss = true; var destroyed = new List<int>();
        // 원문 iterator에 세 명시 후보만 공급한다.
        foreach (int sid in new[] { 60, 61, 62 }) s.Pool.RawSlot(sid)[10] = 70;
        var h = s.Hooks(); h.Destroy = (sid, flags) => { Assert.Equal(0x100000u, flags); destroyed.Add(sid); if (sid == 60) { s.Pool.RawSlot(61)[11] = 1; s.Pool.RawSlot(62)[11] = 1; } };
        RunVortex(s, h); Assert.Equal(new[] { 60, 61 }, destroyed); Assert.Equal(0u, s.Book.Depth);
    }
}
