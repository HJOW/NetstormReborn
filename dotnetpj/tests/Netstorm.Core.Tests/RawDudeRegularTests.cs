using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 원문1597행 정기/하위 입력과 3행 초기화 입력을 기대값 수정 없이 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 Dude 정기 분배·보행·복귀·드롭·낙하와 초기화 계약을 확인한다.</summary>
public sealed class RawDudeRegularTests
{
    /// <summary>부호있는 입력의 원문 int32다.</summary>
    private static int I(string value) => int.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>부호비트가 있는 원문 DWORD 입력이다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>원문 fixture 행만 읽는다.</summary>
    private static IEnumerable<string[]> Rows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
    /// <summary>같은 풀·프레임 표·공급자 목록을 소유하는 장면이다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>같은 실제 풀이다.</summary>
        internal RawSidPool Pool { get; } = new(24000, false);
        /// <summary>현재 타입의 코드/Run 표다.</summary>
        internal RiftTypeFrames[] Frames { get; } = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        /// <summary>같은 기존 에너지 공급자 목록이다.</summary>
        internal RawPostPopList Providers { get; } = new() { Entries = [51, 52, 53] };
        /// <summary>실제 정기 부모50의 번호를 그대로 확보한다.</summary>
        internal Scene()
        {
            for (int sid = 5; sid <= 53; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            Frames[70] = new RiftTypeFrames([new((byte)'A', (byte)'P', 0, 0), new((byte)'A', (byte)'P', 1, 0), new((byte)'A', (byte)'P', 2, 0), new((byte)'B', (byte)'P', 0, 0), new((byte)'B', (byte)'P', 1, 0), new((byte)'I', (byte)'P', 0, 0)]);
        }
        /// <summary>같은 실제 풀을 반납한다.</summary>
        public void Dispose() { }
    }
    /// <summary>원문 trace 한 줄을 실제 효과 순서대로 기록한다.</summary>
    private sealed class Trace
    {
        /// <summary>기록된 실제 줄이다.</summary>
        private string _text = "";
        /// <summary>이름과 원문 DWORD 인자를 순서대로 보존한다.</summary>
        public void Event(string name, params uint[] values)
        {
            if (_text.Length != 0) _text += ';';
            _text += name;
            foreach (uint value in values) _text += ':' + value.ToString(CultureInfo.InvariantCulture);
        }
        /// <summary>비어 있으면 원문 '-'를 사용한다.</summary>
        public override string ToString() => _text.Length == 0 ? "-" : _text;
    }
    /// <summary>같은 실제 몸체와 원문 입력을 빌리는 한 행 실행이다.</summary>
    private static string Replay(Scene scene, uint[] input, Trace trace, GameRandom random, RawPostPopList providers, int providerIndex, bool checking, bool native)
    {
        Span<byte> Raw50() => scene.Pool.RawSlot(50);
        Raw50().Fill(0xa5);
        BinaryPrimitives.WriteUInt32LittleEndian(Raw50(), 0x508cc8); Raw50()[10] = 70; Raw50()[11] = (byte)input[4];
        BinaryPrimitives.WriteUInt16LittleEndian(Raw50()[12..], 0x5102); BinaryPrimitives.WriteSingleLittleEndian(Raw50()[14..], 20.75F);
        BinaryPrimitives.WriteUInt32LittleEndian(Raw50()[18..], input[16]); Raw50()[34] = 2; BinaryPrimitives.WriteUInt32LittleEndian(Raw50()[36..], input[13]);
        for (ushort target = 51; target <= 53; target++)
        {
            Span<byte> slot = scene.Pool.RawSlot(target); slot.Clear(); slot[10] = 71; slot[11] = (byte)input[15];
        }
        uint At(int at, int width = 4) => at == 14 || at == 18 ? BinaryPrimitives.ReadUInt32LittleEndian(Raw50().Slice(at, width)) : Raw50()[at];
        DudeRegularHooks hooks = new()
        {
            CheckSpace = sid => { trace.Event("space", sid); return input[7] != 0; },
            FindPath = (sid, type) => { trace.Event("lookup", sid, type, 0, 0); return input[6] != 0 ? (ushort)52 : (ushort)0; },
            Task = (sid, task, delay) => { trace.Event("task", sid, task, BitConverter.SingleToUInt32Bits(delay)); BinaryPrimitives.WriteUInt16LittleEndian(Raw50()[12..], (ushort)task); },
            NearestResidence = sid => { trace.Event("nearest", At(14), At(18), 0x59a9d8, Raw50()[34]); return input[8] != 0 ? (ushort)51 : (ushort)0; },
            Entrance = sid => { trace.Event("point", sid); return new RawPathPoint(51.25F, 52.75F); },
            Move = (sid, point, quickly) => { trace.Event("move", sid, BitConverter.SingleToUInt32Bits(point.X), BitConverter.SingleToUInt32Bits(point.Y), quickly ? 1U : 0U); return input[9] != 0; },
            SetFrame = (sid, frame, flags) => { trace.Event("frame", sid, unchecked((uint)frame), flags); BinaryPrimitives.WriteUInt32LittleEndian(Raw50()[36..], unchecked((uint)frame)); },
            Destroy = (sid, flags) => { trace.Event("destroy", sid, flags); Raw50()[11] |= 2; },
            Unpop = (sid, flags) => { trace.Event("unpop", sid, flags); Raw50()[11] |= 4; if (input[14] != 0) { BinaryPrimitives.WriteSingleLittleEndian(Raw50()[14..], 42.5F); BinaryPrimitives.WriteSingleLittleEndian(Raw50()[18..], 43.5F); BinaryPrimitives.WriteUInt32LittleEndian(Raw50()[36..], 3); } },
            Pop = (sid, point, flags) => { trace.Event("pop", sid, BitConverter.SingleToUInt32Bits(point.X), BitConverter.SingleToUInt32Bits(point.Y), flags); BinaryPrimitives.WriteSingleLittleEndian(Raw50()[14..], point.X); BinaryPrimitives.WriteSingleLittleEndian(Raw50()[18..], point.Y); Raw50()[11] &= unchecked((byte)~4); },
            Assertion = line => trace.Event("assert", line),
            SetWord = (sid, word) => { Assert.Equal(50, sid); BinaryPrimitives.WriteUInt16LittleEndian(Raw50()[12..], (ushort)word); },
        };
        var state = new DudeRegularState(new RawCheckingFlag(checking), providers, providerIndex) { Native = native };
        var body = new RawDudeRegular(scene.Pool, scene.Frames, random, state, hooks);
        int count = unchecked((int)input[2]); uint result = input[0] switch
        {
            0 => BitConverter.SingleToUInt32Bits(body.Handle(50, input[1], input[2], BitConverter.UInt32BitsToSingle(input[3]))),
            1 => BitConverter.SingleToUInt32Bits(body.Wander(50, count, input[1] != 0)),
            2 => BitConverter.SingleToUInt32Bits(body.Return(50, input[6] != 0 ? (ushort)52 : (ushort)0, count)),
            3 => BitConverter.SingleToUInt32Bits(body.Drop(50, input[6] != 0 ? (ushort)52 : (ushort)0, count)),
            4 => BitConverter.SingleToUInt32Bits(body.Fall(50, count)),
            5 => body.MoveTo(50, input[8] != 0 ? (ushort)51 : (ushort)0, input[1] != 0) ? 1U : 0U,
            _ => throw new InvalidOperationException("Dude 원문 입력 종류 범위"),
        };
        return result.ToString(CultureInfo.InvariantCulture) + '|' + trace + '|' + Convert.ToHexString(scene.Pool.Slot(50)).ToLowerInvariant()
            + '|' + random.State().ToString(CultureInfo.InvariantCulture) + '|' + unchecked((uint)state.ProviderIndex).ToString(CultureInfo.InvariantCulture) + "|-";
    }
    /// <summary>원문1597행 정기/하위 입력과 3행 초기화를 두 판본에서 전수 대조한다.</summary>
    /// <param name="native">실제 가상 진입이면 true, 독립 typed 입력이면 false다.</param>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void MatchesAllNativeDudeRegularRowsAndInitializers(bool native)
    {
        using var scene = new Scene(); int rows = 0, initializers = 0;
        foreach (string[] row in Rows("dude-regular-x86.tsv"))
        {
            Assert.Equal(3, row.Length); uint[] input = row[0].Split(',').Select(N).ToArray(); Assert.Equal(17, input.Length);
            if (input[0] == 6)
            {
                string[] populationFields = row[1].Split('|')[5].Split(','); Assert.Equal(4, populationFields.Length);
                var population = new RawPostPopList(); RawDudeRegular.InitializeDudePopulation(population);
                Assert.Equal(30, population.Entries.Length); Assert.Equal(0u, population.Count);
                initializers++; continue;
            }
            var trace = new Trace(); var random = new GameRandom(input[12]);
            var providers = new RawPostPopList { Entries = [51, 52, 53], Count = input[10] };
            string observed = Replay(scene, input, trace, random, providers, unchecked((int)input[11]), input[5] != 0, native);
            Assert.Equal(row[1], observed); rows++;
        }
        Assert.Equal(1597, rows); Assert.Equal(3, initializers);
    }
    /// <summary>비어 있지 않은 목록의 중복 초기화를 거부한다.</summary>
    [Fact]
    public void InitializeDudePopulationRequiresEmptyList()
    {
        var population = new RawPostPopList(); RawDudeRegular.InitializeDudePopulation(population);
        Assert.Equal(30, population.Entries.Length); Assert.Equal(0u, population.Count);
        Assert.Throws<InvalidOperationException>(() => RawDudeRegular.InitializeDudePopulation(population));
        Assert.Throws<InvalidOperationException>(() => RawDudeRegular.InitializeDudePopulation(new() { Entries = [51] }));
        Assert.Throws<ArgumentNullException>(() => RawDudeRegular.InitializeDudePopulation(null!));
    }
    /// <summary>현재 poll은 TYPE70/프레임/목록/난수/장치를 선택하지 않는다.</summary>
    [Fact]
    public void CurrentPollIgnoresUnselectedTypeAndDevices()
    {
        var pool = new RawSidPool(24000, true); RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var providers = new RawPostPopList(); uint poll = 17, path = 24; int index = 123;
        var state = new DudeRegularState(new RawCheckingFlag(false), providers, index)
        {
            Native = true,
            SharedPollEvent = new UIntBox { Value = poll },
            SharedPathType = new UIntBox { Value = path },
        };
        var body = new RawDudeRegular(pool, frames, (Func<GameRandom>?)null, state, new());
        Assert.Equal(0f, body.Handle(0, 17, 0, 3)); poll = 19; state.SharedPollEvent!.Value = poll;
        Assert.Equal(0f, body.Handle(0, 19, 0, 3)); Assert.Equal(123, state.ProviderIndex);
    }
    /// <summary>타입/가상 표·SID 범위·장치 미연결·poll/마스크 fallback을 거부하거나 기록한다.</summary>
    [Fact]
    public void BoundsRejectInvalidTypeSidDevicesAndFallback()
    {
        var pool = new RawSidPool(24000, false); RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[70] = new RiftTypeFrames([new((byte)'A', (byte)'P', 0, 0)]);
        for (int sid = 5; sid <= 53; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        Span<byte> raw = pool.RawSlot(50); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x508cc8); raw[10] = 70;
        var state = new DudeRegularState(new RawCheckingFlag(false), new() { Entries = [51] }, 0);
        var body = new RawDudeRegular(pool, frames, new GameRandom(1), state, new());
        Assert.False(body.Handles(51)); Assert.True(body.Handles(50));
        BinaryPrimitives.WriteUInt32LittleEndian(raw, 0); Assert.False(body.Handles(50));
        Assert.Throws<InvalidOperationException>(() => body.Handle(50, 0x5102, 0, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => body.Handle(24000, 0x5102, 0, 0));
        Span<byte> bad = pool.RawSlot(51); bad.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(bad, 0x508cc8); bad[10] = 200;
        Span<byte> dead = pool.RawSlot(52); dead.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(dead, 0x508cc8); dead[10] = 70; dead[11] = 3;
        var mover = new RawDudeRegular(pool, frames, new GameRandom(1), state, new());
        Assert.Throws<ArgumentOutOfRangeException>(() => mover.MoveTo(50, 51, false));
        Assert.False(body.MoveTo(50, 0, false));
        var calls = new List<string>();
        var diagnostic = new RawDudeRegular(pool, frames, new GameRandom(1), new DudeRegularState(new RawCheckingFlag(true), new() { Entries = [51] }, 0), new()
        {
            Assertion = line => calls.Add("assert:" + line),
            Entrance = _ => new RawPathPoint(0, 0),
            Move = (sid, point, quickly) => true,
        });
        Assert.True(diagnostic.MoveTo(50, 52, false)); Assert.Contains("assert:222", calls);
        var dropper = new RawDudeRegular(pool, frames, new GameRandom(1), new DudeRegularState(new RawCheckingFlag(true), new() { Entries = [51] }, 0), new()
        {
            Assertion = line => calls.Add("drop-assert:" + line),
        });
        Assert.Equal(1f, dropper.Drop(50, 0, 3)); Assert.Contains("drop-assert:295", calls);
        var freed = new List<string>();
        var fallback = new RawDudeRegular(pool, frames, new GameRandom(1), new DudeRegularState(new RawCheckingFlag(true), new() { Entries = [51] }, 0), new()
        {
            Assertion = line => freed.Add("assert:" + line),
        });
        BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x508cc8); raw[11] |= 1;
        Assert.Equal(2.5f, fallback.Handle(50, 0x5100, 0, 2.5f)); Assert.Contains("assert:399", freed);
        raw[11] &= unchecked((byte)~1);
        var live = new RawDudeRegular(pool, frames, new GameRandom(1), state, new());
        Assert.Throws<InvalidOperationException>(() => live.Handle(50, 0x5101, 0, 0));
        Assert.Equal(0f, live.Handle(50, state.CurrentPollEvent(), 0, 5));
    }
}
