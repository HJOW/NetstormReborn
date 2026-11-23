using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// Rune 후처리의 조회·시간 helper와 PostPop 분기를 기대값 수정 없이 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 Rune 예약/신전조회/건설시간과 공통 호출 순서를 확인한다.</summary>
public sealed class RawRunePostPopTests
{
    /// <summary>같은 풀/해시/TYPE 표를 공유하는 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 실제 풀이다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>같은 현재 해시다.</summary>
        internal RawSquidHash Hash { get; } = new();
        /// <summary>현재 TYPE 표다.</summary>
        internal OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>공통 후처리의 실제 몸체다. 표시는 억제해 공간 쓰기만 둔다.</summary>
        internal RawPostPop Base { get; }
        /// <summary>현재 원본 전역이다.</summary>
        internal RunePostPopState State { get; } = new();
        /// <summary>실제 몸체와 효과를 빌리는 공장이다.</summary>
        internal RawRunePostPop Body { get; }
        /// <summary>같은 풀에 Rune166을 배치한다.</summary>
        internal Scene()
        {
            for (int sid = 5; sid <= 53; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            Base = new RawPostPop(Pool, Types, new RawPostPopState { Suppressed = true });
            Body = new RawRunePostPop(Pool, Hash, Types, State, Base, new());
        }
        /// <summary>같은 실제 풀을 반납한다.</summary>
        public void Dispose() { }
    }
    /// <summary>부호없는 입력의 원문 DWORD다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>지정 슬롯의 원문 필드를 직접 쓰고 같은 해시 셀에 등록한다.</summary>
    private static void Place(Scene scene, int sid, uint type, float x, float y, int owner)
    {
        Span<byte> raw = scene.Pool.RawSlot(sid); raw.Clear();
        ref ushort head = ref scene.Hash.Bucket(1, x, y); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)sid;
        raw[10] = (byte)type; BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[34..], (ushort)owner);
    }
    /// <summary>지정 슬롯을 Rune166 free 형태로 둔다.</summary>
    private static void PlaceRune(Scene scene, int sid, float x, float y, int owner)
    {
        Span<byte> raw = scene.Pool.RawSlot(sid); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x005075d8);
        raw[10] = 166; raw[11] = 1; BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[34..], (ushort)owner);
    }
    /// <summary>PATCH 가상 진입은 현재 TYPE와 무관하다. CD 결합은 RawPostPop이 188 고정을 풀 때까지 후속이다.</summary>
    [Fact]
    public void HandlesSelectsPatchVtableRegardlessOfType()
    {
        using var scene = new Scene(); PlaceRune(scene, 50, 20.75F, 21.25F, 1);
        Assert.True(scene.Body.Handles(50));
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Pool.RawSlot(50), 0); Assert.False(scene.Body.Handles(50));
    }
    /// <summary>타입/가상표와 몸체/판본 연결을 거부한다. CD 결합은 RawPostPop이 188 고정을 풀 때까지 후속이다.</summary>
    [Fact]
    public void ValidateRejectsTypeMismatchAndUnconnectedBody()
    {
        using var scene = new Scene(); PlaceRune(scene, 50, 20.75F, 21.25F, 1);
        scene.Pool.RawSlot(50)[10] = 165; Assert.Throws<InvalidOperationException>(() => scene.Body.Validate(50, 0));
        var other = new RawSidPool(24000, false);
        Assert.Throws<ArgumentException>(() => new RawRunePostPop(other, scene.Hash, scene.Types, new(), scene.Base, new()));
        Assert.Throws<ArgumentNullException>(() => new RawRunePostPop(null!, scene.Hash, scene.Types, new(), scene.Base, new()));
    }
    /// <summary>현재 좌표와 patch+34/CD+32 owner를 읽는다.</summary>
    [Fact]
    public void PositionAndOwnerReadCurrentFields()
    {
        using var scene = new Scene(); PlaceRune(scene, 50, 20.75F, 21.25F, 3);
        Assert.Equal(new RawPathPoint(20.75F, 21.25F), scene.Body.Position(50)); Assert.Equal(3u, scene.Body.Owner(50));
    }
    /// <summary>반경1 finder에서 같은 owner의 첫 Dais만 읽는다.</summary>
    [Fact]
    public void FindOwnedDaisSelectsSameOwnerInRadius()
    {
        using var scene = new Scene();
        PlaceRune(scene, 50, 40.75F, 41.25F, 2);
        Place(scene, 51, 165, 40.75F, 41.25F, 2);
        Assert.Equal((ushort)51, scene.Body.FindOwnedDais(50));
        scene.Pool.RawSlot(51)[34] = 3; Assert.Equal(0, scene.Body.FindOwnedDais(50));
        scene.Pool.RawSlot(51)[34] = 2; scene.Pool.RawSlot(51)[10] = 164; Assert.Equal(0, scene.Body.FindOwnedDais(50));
    }
    /// <summary>같은 셀의 현재 타입 altar를 읽고 없으면 0을 둔다.</summary>
    [Fact]
    public void AltarAtFindsCurrentTypeAtSameCell()
    {
        using var scene = new Scene();
        PlaceRune(scene, 50, 40.75F, 41.25F, 2);
        Place(scene, 53, 164, 40.75F, 41.25F, 2);
        Assert.Equal((ushort)53, scene.Body.AltarAt(50));
        scene.State.AltarType = 999; Assert.Throws<ArgumentOutOfRangeException>(() => scene.Body.AltarAt(50));
        scene.State.AltarType = 164; scene.Pool.RawSlot(53)[10] = 163; Assert.Equal(0, scene.Body.AltarAt(50));
    }
    /// <summary>altar 부재·owner 0·억제·깃발·기록 순서로 7/10/25를 고른다.</summary>
    [Fact]
    public void ConstructionSecondsUsesAltarOwnerAndFlags()
    {
        using var scene = new Scene(); PlaceRune(scene, 50, 40.75F, 41.25F, 2);
        Assert.Equal(10, scene.Body.ConstructionSeconds(50));
        Place(scene, 53, 164, 40.75F, 41.25F, 0);
        Assert.Equal(10, scene.Body.ConstructionSeconds(50));
        BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(53)[12..], 2);
        uint flags = 0; var hooked = new RawRunePostPop(scene.Pool, scene.Hash, scene.Types, scene.State, scene.Base, new() { PlayerFlags = _ => flags });
        Assert.Equal(7, hooked.ConstructionSeconds(50)); flags = 1; Assert.Equal(25, hooked.ConstructionSeconds(50));
        var suppressed = new RawRunePostPop(scene.Pool, scene.Hash, scene.Types, scene.State, scene.Base, new() { SuppressDuration = () => true });
        Assert.Equal(10, suppressed.ConstructionSeconds(50));
        scene.State.SuppressDuration = true; Assert.Equal(10, scene.Body.ConstructionSeconds(50)); scene.State.SuppressDuration = false;
        scene.State.PlayerFlags[2] = 1; Assert.Equal(25, scene.Body.ConstructionSeconds(50));
        scene.State.PlayerFlags[2] = 0; Assert.Equal(7, scene.Body.ConstructionSeconds(50));
        BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(53)[12..], 9);
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Body.ConstructionSeconds(50));
    }
    /// <summary>예약·건설 분기와 공통 후처리를 원문 순서로 실행한다.</summary>
    [Fact]
    public void PostPopBranchesReserveConstructAndBase()
    {
        using var scene = new Scene(); PlaceRune(scene, 50, 40.75F, 41.25F, 2);
        Place(scene, 51, 165, 40.75F, 41.25F, 2); Place(scene, 53, 164, 40.75F, 41.25F, 2);
        BinaryPrimitives.WriteUInt16LittleEndian(scene.Pool.RawSlot(53)[12..], 2);
        var calls = new List<string>(); bool reserve = true;
        var body = new RawRunePostPop(scene.Pool, scene.Hash, scene.Types, scene.State, scene.Base, new()
        {
            ReserveRegular = () => { calls.Add("reserve"); return reserve; },
            AddRegular = (sid, ev, payload) => calls.Add($"add:{sid}:{ev}:{payload}"),
            StartConstruction = (sid, seconds) => calls.Add($"start:{sid}:{seconds}"),
            PlayerFlags = _ => 0,
        });
        body.PostPop(50, 0); Assert.Empty(calls);
        body.PostPop(50, 1); Assert.Equal(["reserve", "add:50:772:0.01"], calls);
        calls.Clear(); reserve = false; body.PostPop(50, 1); Assert.Equal(["reserve"], calls);
        calls.Clear(); reserve = true; body.PostPop(50, 3); Assert.Equal(["reserve", "add:50:772:0.01", "start:50:7"], calls);
    }
    /// <summary>같은 풀의 Host/건설 몸체를 묶고 빈 공급을 거부한다.</summary>
    [Fact]
    public void MakeRunePostPopHooksBindsHostAndConstruction()
    {
        using var scene = new Scene();
        var kernel = new Kernel();
        var destroy = new RawSquidDestroy(scene.Pool, new RawUnpop(scene.Pool, new RawSquidHash(), new byte[65536]), scene.Types);
        var host = new SquidProcessHost(scene.Pool, scene.Types, kernel, destroy, new SquidProcessState());
        var construction = new RawConstructionProcess(scene.Pool, scene.Types, new(), new ConstructionProcessHooks
        {
            Reserve = () => true,
            Effect = (_, _) => { },
            LoadSound = _ => 0,
            ReserveSound = () => false,
            AttachSound = (_, _, _, _) => { },
            Refresh = () => { },
            MaxHitPoints = _ => 0,
            UpdateDisplay = _ => { },
            RemoveContained = (_, _) => { },
            PlayAt = (_, _, _, _) => { },
            ReadySound = _ => 0,
            Play = _ => { },
            Unpop = (_, _) => { },
            Repop = (_, _) => { },
            Push = (_, _, _, _) => { },
        }, host);
        var hooks = RawRunePostPop.MakeRunePostPopHooks(scene.Pool, host, construction, new());
        Assert.NotNull(hooks.AddRegular); Assert.NotNull(hooks.StartConstruction);
        Assert.Throws<ArgumentNullException>(() => RawRunePostPop.MakeRunePostPopHooks(null!, host, construction, new()));
        var other = new RawSidPool(24000, false);
        Assert.Throws<ArgumentException>(() => RawRunePostPop.MakeRunePostPopHooks(other, host, construction, new()));
        kernel.DrainOwnedForShutdown();
    }
    /// <summary>원문 fixture 행만 읽는다.</summary>
    private static IEnumerable<string[]> FixtureRows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
    /// <summary>원문1920행을 두 판본에서 전수 대조한다. CD행은 앞 36바이트만 대조한다.</summary>
    [Fact]
    public void MatchesAllNativeRunePostPopRows()
    {
        int rows = 0;
        foreach (string[] row in FixtureRows("runepostpop-x86.tsv"))
        {
            Assert.Equal(7, row.Length); uint[] input = row[1].Split(',').Select(N).ToArray(); Assert.Equal(8, input.Length);
            bool patch = row[0] == "originals";
            uint flags = input[0], profile = input[1], reserve = input[2], word = input[3], playerFlag = input[4], suppress = input[5], mutation = input[6], extra = input[7];
            var pool = new RawSidPool(24000, false, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            int count = patch ? 188 : 171;
            OriginalTypeRecord[] types = Enumerable.Range(0, count).Select(_ => new OriginalTypeRecord()).ToArray();
            for (uint number = 5; number <= 8; number++) Assert.Equal((int)number, pool.Allocate(SidAllocation.Client));
            for (int clear = 5; clear <= 8; clear++) pool.RawSlot(clear).Clear();
            var hash = new RawSquidHash(); var state = new RunePostPopState(); var effects = new List<string>();
            void Place(int sid, uint type, float x, float y, uint currentExtra, uint rawState, int foot)
            {
                Span<byte> raw = pool.RawSlot(sid); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x11010000);
                raw[10] = (byte)type; raw[11] = (byte)rawState; raw[patch ? 40 : 35] = (byte)currentExtra;
                BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
                types[type].FootX = types[type].FootY = foot;
                ref ushort head = ref hash.Bucket(1, x, y); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)sid;
            }
            Place(8, 166, 40.75F, 41.25F, extra == 0 ? 0u : extra == 1 ? 1u : 8u, 0, 1);
            if (profile != 0) Place(6, 165, profile == 7 ? 80.75F : profile == 6 ? 42.75F : 40.75F, profile == 7 ? 81.25F : profile == 6 ? 43.25F : 41.25F, profile == 3 ? 8u : profile == 5 ? 1u : 0u, profile == 4 ? 2u : 0u, profile == 6 ? 3 : 1);
            if (profile != 8) Place(7, 164, profile == 9 ? 80.75F : 40.75F, profile == 9 ? 81.25F : 41.25F, 0, 0, 1);
            // 공통 후처리 몸체는 거의 모든 표 항목을 읽으므로 패치 188표로 묶는다. 실제 CD 171표 대조는 후속에서 잇는다.
            var base188 = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
            var book = new RawPostPopState(); var native = new RawPostPop(pool, base188, book);
            var body = new RawRunePostPop(pool, hash, types, state, native, new RunePostPopHooks
            {
                ReserveRegular = () => { effects.Add("N:40"); if (mutation == 1) state.SuppressDuration = !state.SuppressDuration; return reserve != 0; },
                AddRegular = (sid, ev, payload) => { effects.Add("R:" + ev + "," + sid + "," + BitConverter.SingleToUInt32Bits(payload)); if (mutation == 2) pool.RawSlot(8)[patch ? 34 : 32] = 2; },
                StartConstruction = (sid, seconds) => effects.Add("C:" + sid + "," + BitConverter.SingleToUInt32Bits(seconds)),
            });
            Span<byte> rune = pool.RawSlot(8);
            BinaryPrimitives.WriteUInt32LittleEndian(rune, patch ? 0x005075d8u : 0x00504148u); rune[patch ? 34 : 32] = 1;
            pool.RawSlot(6)[patch ? 34 : 32] = (byte)(profile == 2 ? 2 : 1);
            BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(7)[12..], (ushort)word);
            state.SuppressDuration = suppress != 0; state.PlayerFlags[(int)word] = playerFlag;
            book.ActivationDepth = 1;
            Assert.Equal(int.Parse(row[2], CultureInfo.InvariantCulture), body.ConstructionSeconds(8));
            if (patch) Assert.Equal(N(row[3]), body.FindOwnedDais(8));
            body.PostPop(8, flags);
            Assert.Equal(0u, book.ActivationDepth);
            effects.Add("B:8," + flags);
            Assert.Equal(row[4], string.Join(';', effects));
            Assert.Equal(row[6] != "0", state.SuppressDuration);
            string[] images = row[5].Split(';'); Assert.Equal(3, images.Length);
            foreach (ushort number in new ushort[] { 6, 7, 8 })
            {
                string image = Convert.ToHexString(pool.Slot(number)).ToLowerInvariant();
                string expected = images[number - 6];
                if (patch) { Assert.True(expected.Length == image.Length, $"row {rows} sid {number}"); Assert.Equal(expected.Substring(8), image.Substring(8)); }
                else Assert.Equal(expected.Substring(8), image.Substring(8, expected.Length - 8));
            }
            rows++;
        }
        Assert.True(rows > 1000);
    }
}
