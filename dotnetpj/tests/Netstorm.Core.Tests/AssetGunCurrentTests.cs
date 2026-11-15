using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원본 진단 자료와 실제 현재 가상 진입의 Gun 부모를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>같은 원본 장면을 공유해 Gun의 raw/효과/진단을 비교한다.</summary>
public sealed partial class X86RawAssetPostPopTests
{
    /// <summary>원문 1,344행의 생성/판정/복귀 순서와 최종 전역·슬롯을 대조한다.</summary>
    [Fact]
    public void GunNativeDiagnosticOrderAndReturn1344()
    {
        string[][] rows = X86Fixture.Read("gun-postpop-checking"); int count = 0;
        // 원본 입력만 재생하며 기대값은 fixture를 그대로 사용한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(9, row.Length); string[] input = row[0].Split(','), control = row[1].Split(',');
            Assert.Equal(14, input.Length); Assert.Equal(5, control.Length);
            string[] legacy = Enumerable.Repeat("0", 21).ToArray(); legacy[0] = "originals"; input.CopyTo(legacy, 1);
            var scene = new Scene(); scene.Prepare(legacy); uint checking = X86Fixture.UInt(control[0]);
            scene.State.CheckingForms.Bind(() => checking); scene.State.FortMode = control[4] != "0";
            var authority = new RawAuthorityState { Server = input[3] != "0" }; scene.State.SharedAuthority = authority; scene.State.InBattle = !authority.Server;
            uint mutation = X86Fixture.UInt(control[3]); AssetPostPopHooks hooks = scene.Hooks(); Action<int> create = hooks.CreateGunProcess!, remove = hooks.RemoveAnimation!;
            hooks.CreateGunProcess = sid => { if (mutation is 1 or 4) { checking = mutation == 1 ? 0u : 1u; scene.State.LoadingDepth = mutation == 1 ? 1u : 0u; scene.State.FortMode = mutation == 1; } create(sid); };
            hooks.RemoveAnimation = sid => { remove(sid); if (mutation == 5) authority.Server = false; };
            hooks.CheckForms = sid => { scene.Events.Add($"C:{sid}"); if (mutation == 2) checking = 0; return control[1] != "0"; };
            hooks.Assertion = (expression, file, line) =>
            {
                scene.Events.Add($"X:{Convert.ToHexStringLower(Encoding.ASCII.GetBytes(expression))}:{Convert.ToHexStringLower(Encoding.ASCII.GetBytes(file))}:{line}");
                if (control[2] == "0") throw new InvalidOperationException("명시 PostPop 진단 중단");
                if (mutation == 3) { checking = 0; scene.State.LoadingDepth = 1; scene.State.FortMode = true; Put(scene.Pool.RawSlot(Source), 12, 0x1234, 2); Put(scene.Pool.RawSlot(Source), 36, 0x6789); }
            };
            var @base = new RawPostPop(scene.Pool, scene.Types, scene.Book);
            var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, scene.CarrierState, scene.CarrierHooks(@base));
            var assets = new RawAssetPostPop(scene.Pool, scene.Types, @base, damageable, scene.State, hooks);
            uint flags = X86Fixture.UInt(input[1]); assets.Validate(Source, flags); Assert.Empty(scene.Events); string outcome = "return";
            try { assets.PostPop(Source, flags); }
            catch (InvalidOperationException error) when (error.Message == "명시 PostPop 진단 중단") { outcome = "assertion"; }
            string[] output = scene.Output(); Assert.Equal(row[2], output[0]); Assert.Equal(row[3], output[1]); Assert.Equal(row[4], outcome);
            Assert.Equal(X86Fixture.UInt(row[5]), checking); Assert.Equal(X86Fixture.UInt(row[6]), scene.State.LoadingDepth);
            Assert.Equal(row[7] != "0", scene.State.FortMode); Assert.Equal(row[8] != "0", authority.Server); count++;
        }
        Assert.Equal(1344, count);
    }
}

/// <summary>실제 현재 가상 진입에서 미도달 장치와 앞선 물리 효과를 검사한다.</summary>
public sealed class AssetGunCurrentTests
{
    /// <summary>원본 번호50과 실제 공통/Carrier 부모를 공유한다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 슬롯 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>콜백이 바꾸는 현재 타입 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>선택 본문 뒤 깊이 감소만 관찰하는 실제 공통 상태다.</summary>
        public readonly RawPostPopState Book = new() { Suppressed = true };
        /// <summary>현재 서버/로딩/진단 상태다.</summary>
        public readonly AssetPostPopState State = new();
        /// <summary>선택된 장치의 호출 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>공통 부모다.</summary>
        public readonly RawPostPop Base;
        /// <summary>직접 Damageable 부모다.</summary>
        public readonly RawCarrierPostPop Damageable;
        /// <summary>여섯 실제 VT 중 지정 입력을 구성한다.</summary>
        public Scene(int kind)
        {
            // 명시 원본 번호50까지 같은 실제 풀을 확보한다.
            for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            uint[] tables = [0x501ea0, 0x50b6e8, 0x50b7c0, 0x50b9b8, 0x50b888, 0x50b570];
            Span<byte> raw = Pool.RawSlot(50); raw.Clear(); Put(raw, 0, tables[kind]); raw[10] = 70; raw[34] = 1; Put(raw, 12, 0xabcd, 2); Put(raw, 36, 25);
            Base = new(Pool, Types, Book); Damageable = new(Pool, Types, new() { Boss = true }, new() { Base = (sid, flags) => { Events.Add("D" + flags); Base.PostPopBase(sid, flags); } });
        }
        /// <summary>현재 각도·생성·진단 장치의 입력이다.</summary>
        public AssetPostPopHooks Hooks() => new()
        {
            GunFrame = angle => { Events.Add("F" + angle); return 2; }, SetGraph = (sid, graph) => { Events.Add("Q" + graph); Pool.RawSlot(sid)[30] = (byte)graph; },
            CreateGunProcess = _ => Events.Add("P"), RemoveAnimation = _ => Events.Add("M"), CheckForms = _ => true,
            Assertion = (_, _, line) => Events.Add("X" + line), NotifySurface = _ => Events.Add("N"), ReserveRegular = () => { Events.Add("A"); return false; },
        };
        /// <summary>실제 가상 분배와 공통 깊이 수명을 실행한다.</summary>
        public void Run(AssetPostPopHooks hooks, uint flags = 1) { var body = new RawAssetPostPop(Pool, Types, Base, Damageable, State, hooks); Book.Depth++; Base.SetAssetPostPop(body); Base.PostPopVirtual(50, flags); }
        /// <summary>원문 물리 DWORD/WORD를 쓴다.</summary>
        public static void Put(Span<byte> raw, int offset, uint value, int width = 4)
        {
            // 지정 폭만 낮은 바이트부터 저장하여 이웃 필드를 보존한다.
            for (int index = 0; index < width; index++) raw[offset + index] = (byte)(value >> (index * 8));
        }
        /// <summary>원문 물리 DWORD/WORD를 읽는다.</summary>
        public uint Read(int offset, int width = 4)
        {
            uint value = 0;
            // 지정 폭만 낮은 바이트부터 조합한다.
            for (int index = 0; index < width; index++) value |= (uint)Pool.Slot(50)[offset + index] << (index * 8);
            return value;
        }
    }
    /// <summary>최초가 아니면 Sun의 필수 TYPE 조회 이외에는 미도달 장치를 요구하지 않는다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(4)] [InlineData(5)]
    public void NoFirstSkipsDevicesAndType(int kind) { var s = new Scene(kind); s.Pool.RawSlot(50)[10] = 255; s.State.Temples.Count = 99; s.Run(new(), 0); Assert.Equal(0u, s.Book.Depth); Assert.Equal(kind == 5 ? Array.Empty<string>() : new[] { "D0" }, s.Events); }
    /// <summary>그래프 setter 누락은 먼저 도달한 각도 getter 효과를 보존한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void AngleMissingGetterOrSetter(bool getter) { var s = new Scene(1); var h = s.Hooks(); h.SetGraph = null; if (!getter) h.GunFrame = null; Assert.Throws<InvalidOperationException>(() => s.Run(h, 2)); Assert.Equal(getter ? new[] { "F25" } : Array.Empty<string>(), s.Events); Assert.Equal(1u, s.Book.Depth); }
    /// <summary>그래프 효과 뒤 현재 권한을 읽고 free 물리 슬롯을 유지한다.</summary>
    [Fact]
    public void GraphChangesCurrentAuthorityAndLife() { var s = new Scene(1); var authority = new RawAuthorityState { Server = true }; s.State.SharedAuthority = authority; var h = s.Hooks(); h.CreateGunProcess = null; h.SetGraph = (sid, graph) => { s.Events.Add("Q" + graph); s.Pool.RawSlot(sid)[30] = (byte)graph; s.Pool.RawSlot(sid)[10] = 255; s.Pool.RawSlot(sid)[11] = 9; authority.Server = false; }; s.Run(h, 2); Assert.Equal(new[] { "F25", "Q2", "D2" }, s.Events); Assert.Equal(9, s.Pool.Slot(50)[11]); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>생성 누락은 각도/제거 효과 뒤 선택 위치에서만 중단한다.</summary>
    [Theory]
    [InlineData(1)] [InlineData(4)]
    public void MissingCreationRetainsPrefix(int kind) { var s = new Scene(kind); s.State.InBattle = true; var h = s.Hooks(); h.CreateGunProcess = null; Assert.Throws<InvalidOperationException>(() => s.Run(h)); Assert.Equal(kind == 1 ? new[] { "F25", "Q2" } : new[] { "M" }, s.Events); Assert.Equal(1u, s.Book.Depth); }
    /// <summary>extra나 비서버 관문이 선택하지 않은 생성/진단 장치는 요구하지 않는다.</summary>
    [Theory]
    [InlineData(1, false)] [InlineData(1, true)] [InlineData(2, false)] [InlineData(2, true)]
    [InlineData(3, false)] [InlineData(3, true)] [InlineData(4, false)] [InlineData(4, true)]
    public void NonServerOrExtraSkipsCreate(int kind, bool server) { var s = new Scene(kind); s.State.InBattle = server; s.Pool.RawSlot(50)[40] = server ? (byte)1 : (byte)0; var h = s.Hooks(); h.CreateGunProcess = null; h.RemoveAnimation = null; h.CheckForms = null; h.Assertion = null; s.Run(h); Assert.DoesNotContain("P", s.Events); Assert.Equal("D1", s.Events[^1]); Assert.Equal(0u, s.Book.Depth); }
    /// <summary>제거 효과로 권한이 바뀌어도 이미 선택한 생성은 취소하지 않는다.</summary>
    [Fact]
    public void ArcherRemovalKeepsSelectedCreation() { var s = new Scene(4); s.State.InBattle = true; s.State.FortMode = true; var h = s.Hooks(); h.RemoveAnimation = sid => { s.Events.Add("M"); s.State.InBattle = false; s.Pool.RawSlot(sid)[40] = 1; }; s.Run(h); Assert.Equal(new[] { "M", "P", "D1" }, s.Events); }
    /// <summary>장식 확보 실패는 생성자 누락을 요구하지 않고 성공 뒤 누락은 확보 효과를 유지한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void DecorationReserveControlsConstructor(bool reserved) { var s = new Scene(5); s.Pool.RawSlot(50)[10] = 255; var h = s.Hooks(); h.ReserveRegular = () => { s.Events.Add("A"); return reserved; }; if (reserved) Assert.Throws<InvalidOperationException>(() => s.Run(h)); else s.Run(h); Assert.Equal(new[] { "A" }, s.Events); Assert.Equal(reserved ? 1u : 0u, s.Book.Depth); }
    /// <summary>Sun의 flags0도 현재 TYPE를 읽고 WORD 저장 뒤 현재 다리 통지를 선택한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void SunWordAndCurrentBridgeNotice(bool installed) { var s = new Scene(3); s.Types[70].Flags1 = 0x1000; s.State.SharedBridgeType = () => 70; var h = s.Hooks(); h.NotifySurface = installed ? sid => { s.Events.Add("N"); s.Pool.RawSlot(sid)[10] = 255; s.Pool.RawSlot(sid)[11] = 9; Scene.Put(s.Pool.RawSlot(sid), 12, 0x4321, 2); } : null; if (installed) s.Run(h, 0); else Assert.Throws<InvalidOperationException>(() => s.Run(h, 0)); Assert.Equal(installed ? 0x4321u : 25u, s.Read(12, 2)); Assert.Equal(installed ? 0u : 1u, s.Book.Depth); }
    /// <summary>생성이 바꾼 현재 로딩/TYPE/HP를 DWORD 백업한 뒤 free 물리 WORD와 통지를 처리한다.</summary>
    [Fact]
    public void SunLoadingUsesCurrentWordAfterCreate() { var s = new Scene(3); s.State.InBattle = true; var h = s.Hooks(); h.CreateGunProcess = sid => { s.Events.Add("P"); s.State.LoadingDepth = 1; s.Pool.RawSlot(sid)[10] = 75; s.Pool.RawSlot(sid)[11] = 9; Scene.Put(s.Pool.RawSlot(sid), 12, 0x8765, 2); s.Types[75].Flags1 = 0x1000; s.State.BridgeType = 75; }; s.Run(h); Assert.Equal(new[] { "P", "N", "D1" }, s.Events); Assert.Equal(0x8765u, s.Read(36)); Assert.Equal(0x8765u, s.Read(12, 2)); }
    /// <summary>잘못된 TYPE는 제네레이터 최초/캐논 마지막 조회 위치에서만 중단한다.</summary>
    [Theory]
    [InlineData(0, 1u)] [InlineData(3, 0u)]
    public void InvalidCurrentTypeFailsAtSelectedLookup(int kind, uint flags) { var s = new Scene(kind); s.Pool.RawSlot(50)[10] = 255; Assert.Throws<ArgumentOutOfRangeException>(() => s.Run(new(), flags)); Assert.Empty(s.Events); Assert.Equal(1u, s.Book.Depth); }
    /// <summary>checking 누락과 실패 진단은 생성 뒤에만 요구되며 getter가 관문을 꺼도 선택 실패는 유지한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void MissingSelectedGetterOrAssertionAfterCreation(bool getter) { var s = new Scene(2); s.State.InBattle = true; uint checking = 0x80000000; s.State.CheckingForms.Bind(() => checking); var h = s.Hooks(); h.CheckForms = getter ? _ => { checking = 0; return false; } : null; h.Assertion = null; Assert.Throws<InvalidOperationException>(() => s.Run(h)); Assert.Equal(new[] { "P" }, s.Events); Assert.Equal(1u, s.Book.Depth); Assert.Equal(getter ? 0u : 0x80000000u, checking); }
}
