using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 신전 의식의 현재 단계·SHP 목표점·사제/룬 조회와 실제 하위 요청 순서를 원문과 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 templeritual 6720행과 실제 Factory/owner/Host 사슬을 같은 풀에서 대조한다.</summary>
public sealed class RawTempleRitualTests
{
    /// <summary>부호없는 입력의 원문 DWORD다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>원문 fixture 행만 읽는다.</summary>
    private static IEnumerable<string[]> FixtureRows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
    /// <summary>단정도 비트를 원문 10진 그대로 읽는다.</summary>
    private static float Bits(uint value) => BitConverter.UInt32BitsToSingle(value);
    /// <summary>원문 목표점의 두 단정도 비트를 묶는다.</summary>
    private static string PointBits(float x, float y) => BitConverter.SingleToUInt32Bits(x) + "," + BitConverter.SingleToUInt32Bits(y);
    /// <summary>합성 SHP와 code/first-table 입력을 판본 공통 타입 인덱스로 준비한다. 출력 선택은 계산하지 않는다.</summary>
    private static void TempleAssets(OriginalTypeRecord[] types, RiftTypeFrames[] frames, SquidDisplayShape[] shapes, char side)
    {
        // native와같은 세타입의 기본HP/핫스폿과 방향코드다.
        foreach (uint type in new uint[] { 158, 165, 166 })
        {
            int count = type == 165 ? 180 : type == 166 ? 6 : 32;
            types[type].MaxHitPoints = 200; types[type].Flags1 = 16; types[type].FootX = types[type].FootY = type == 165 ? 3 : 1;
            var codes = new List<RiftFrameCode>();
            // SHP 정수 핫스폿은입력이며목표점의계산은원본/복원몸체가각각한다.
            for (int index = 0; index < count; index++)
            {
                shapes[type].Frames.Add(new SquidDisplayFrame { Width = 16, Height = 11, MuzzleX = (short)((index % 5) * 16), MuzzleY = (short)((index % 5) * 11) });
                codes.Add(new RiftFrameCode((byte)(type == 165 ? side : 'A'), (byte)'P', (byte)(index + 1), 0));
            }
            // 사제first-table은A둘/K..P각하나다. 이후P는해당첫프레임을바꾸지않는다.
            if (type == 158) for (int index = 2; index < count; index++) codes[index] = codes[index] with { Side = (byte)(index < 8 ? 'K' + index - 2 : 'P') };
            frames[type] = new RiftTypeFrames(codes);
            shapes[type].Loaded = true; shapes[type].FrameCount = count;
        }
        types[158].Flags2 = 0x210000;
    }
    /// <summary>원문6720행을 두 판본에서 전수 대조한다. CD행은 DN 188타입 고정의 한계로 앞 판본 갈림만 둔다.</summary>
    [Fact]
    public void MatchesAllNativeTempleRitualRows()
    {
        int rows = 0;
        foreach (string[] row in FixtureRows("templeritual-x86.tsv"))
        {
            Assert.Equal(7, row.Length); string[] cells = row[1].Split(','); Assert.Equal(9, cells.Length);
            bool patch = row[0] == "originals";
            uint mode = N(cells[0]), frame = N(cells[1]), profile = N(cells[3]), boss = N(cells[4]), reserve = N(cells[5]), spot = N(cells[6]), mutation = N(cells[8]);
            char side = cells[2][0];
            uint[] xy = row[2].Split(',').Select(N).ToArray(); Assert.Equal(4, xy.Length);
            var pool = new RawSidPool(24000, false, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            int count = patch ? 188 : 171;
            OriginalTypeRecord[] types = Enumerable.Range(0, count).Select(_ => new OriginalTypeRecord()).ToArray();
            RiftTypeFrames[] frames = Enumerable.Range(0, count).Select(_ => new RiftTypeFrames([])).ToArray();
            SquidDisplayShape[] shapes = Enumerable.Range(0, count).Select(_ => new SquidDisplayShape()).ToArray();
            TempleAssets(types, frames, shapes, side);
            var empty = Enumerable.Range(0, count).Select(_ => (IReadOnlyList<FrameCode>)new List<FrameCode>()).ToArray();
            byte[] spots = Enumerable.Repeat((byte)spot, 65536).ToArray();
            for (uint number = 5; number <= 8; number++) Assert.Equal((int)number, pool.Allocate(SidAllocation.Client));
            for (int clear = 5; clear <= 8; clear++) pool.RawSlot(clear).Clear();
            var hash = new RawSquidHash(); var effects = new List<string>();
            void Place(int sid, uint type, float x, float y, uint currentExtra, uint rawState)
            {
                Span<byte> raw = pool.RawSlot(sid); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x11010000);
                raw[10] = (byte)type; raw[11] = (byte)rawState; raw[patch ? 40 : 35] = (byte)currentExtra;
                BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
                ref ushort head = ref hash.Bucket(1, x, y); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)sid;
            }
            float x = Bits(xy[0]), y = Bits(xy[1]);
            Place(5, 165, 40.75F, 41.25F, 0, 0); Place(6, 158, x, y, 0, 0);
            if (patch) BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(5)[36..], frame); else pool.RawSlot(5)[34] = (byte)frame;
            pool.RawSlot(5)[patch ? 34 : 32] = 1; pool.RawSlot(6)[patch ? 34 : 32] = (byte)(profile == 2 ? 2 : 1);
            RawSquidHitPoints.Set(pool, 6, profile == 1 ? 99 : 100);
            if (profile >= 3) Place(7, 166, Bits(xy[2]), Bits(xy[3]), profile == 6 ? 8u : profile == 5 ? 1u : 0u, profile == 4 ? 2u : 0u);
            // 공통 HP 몸체는 DN 188타입 고정의 한계로 더미 188표에 두며 판본 갈림은 의식 본체가 가진다.
            OriginalTypeRecord[] rewardTypes = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
            rewardTypes[158].MaxHitPoints = 200;
            var store = new RawSpStore(new GameRandom(1), () => 1); store.Initialize();
            var reward = new RawSquidReward(pool, rewardTypes, new RawPostPopState(), new SquidRewardState(), store);
            var footprint = new RawSquidNeighbors(pool, hash, spots, types, empty);
            var state = new TempleRitualState { Boss = boss != 0 };
            var actionHooks = new PriestRitualActionHooks
            {
                SetFrame = (sid, index, flags) =>
                {
                    Assert.Equal(6, sid); Assert.Equal(0u, flags);
                    effects.Add("F:6," + index + "," + flags);
                    if (patch) BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(6)[36..], unchecked((uint)index)); else pool.RawSlot(6)[34] = (byte)index;
                    if (mutation == 1) state.Boss = !state.Boss;
                    if (mutation == 2) { if (patch) BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(5)[36..], 14); else pool.RawSlot(5)[34] = 14; }
                },
                ReserveShared = () => { effects.Add("N:40"); if (mutation == 3) state.Boss = !state.Boss; return reserve != 0; },
                AddShared = (sid, ev, payload) => effects.Add("S:" + ev + "," + sid + "," + BitConverter.SingleToUInt32Bits(payload)),
            };
            var actions = new RawPriestRitualActions(pool, frames, actionHooks);
            RawTempleRitual? ritual = null;
            var hooks = new TempleRitualHooks
            {
                Create = (type, flags) =>
                {
                    Assert.Equal(166u, type); Assert.Equal(0u, flags);
                    effects.Add("C:166,0");
                    Span<byte> raw = pool.RawSlot(8); raw.Fill(0xa5);
                    BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x11010000); raw[10] = 166; raw[11] = 4; return 8;
                },
                SetHitPoints = (sid, value) => { Assert.Equal(8, sid); Assert.Equal(0, value); RawSquidHitPoints.Set(pool, sid, value); },
                SetOwner = (sid, player) => { Assert.Equal(8, sid); effects.Add("O:8," + player); pool.RawSlot(8)[patch ? 34 : 32] = (byte)player; },
                Pop = (sid, px, py, flags) =>
                {
                    Assert.Equal(8, sid); Assert.Equal(0u, flags);
                    effects.Add("P:8," + PointBits(px, py) + "," + flags);
                    Span<byte> raw = pool.RawSlot(8);
                    BinaryPrimitives.WriteSingleLittleEndian(raw[14..], px); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], py);
                    raw[11] &= unchecked((byte)~4);
                },
                PlayAt = (px, py, sound, flags) => effects.Add("A:" + PointBits(px, py) + "," + sound + "," + flags),
                ReserveShared = actionHooks.ReserveShared,
                AddShared = actionHooks.AddShared,
                Count = sid => ritual!.Count(sid),
            };
            ritual = new RawTempleRitual(pool, hash, types, frames, shapes, spots, footprint, reward, actions, state, hooks);
            uint stage = ritual.Stage(5);
            uint limited = stage > 4 ? 4 : stage;
            TempleRitualPoint anchor = ritual.StageAnchor(5, limited), point = ritual.StagePoint(5, limited);
            string actual = ritual.Count(5) + "," + stage + "," + PointBits(anchor.X, anchor.Y) + "," + PointBits(point.X, point.Y)
                + "," + ritual.RuneAt(5, new(x, y)) + "," + ritual.FindStrongOwnedPriest(5) + "," + (ritual.HasAbstractRune(5) ? 1 : 0);
            Assert.Equal(row[3], actual);
            ritual.Ritual(5, 6, mode);
            Assert.Equal(row[4], effects.Count == 0 ? "-" : string.Join(';', effects));
            Assert.Equal(row[6] != "0", state.Boss);
            string[] images = row[5].Split(';'); Assert.Equal(4, images.Length);
            foreach (int number in new int[] { 5, 6, 7, 8 })
            {
                string image = Convert.ToHexString(pool.Slot(number)).ToLowerInvariant();
                string expected = images[number - 5];
                if (patch) { Assert.True(expected.Length == image.Length, $"row {rows} sid {number}"); Assert.Equal(expected.Substring(8), image.Substring(8)); }
                else Assert.Equal(expected.Substring(8), image.Substring(8, expected.Length - 8));
            }
            rows++;
        }
        Assert.True(rows > 1000);
    }
    /// <summary>실제Factory/Owner→같은Frame의mode0사슬과 Make 묶음의 실제 결선을 실행한다. 실제0x307 Host 부착은 R4-6 후속이며 기록형으로 둔다.</summary>
    [Fact]
    public void MakeTempleRitualHooksBindsRealChain()
    {
        var pool = new RawSidPool(24000, false, edition: OriginalEdition.Patch1078);
        var hash = new RawSquidHash(); byte[] spots = new byte[65536];
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        SquidDisplayShape[] shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        TempleAssets(types, frames, shapes, 'A');
        // 165/158/166의실제생성자주소를입력으로공급하고base vtable로대체하지않는다.
        foreach (uint type in new uint[] { 158, 165, 166 }) types[type].ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, (int)type);
        var factory = new RawSquidFactory(pool, types, false, new RawUnpop(pool, hash, spots));
        var diagnostics = new List<string>();
        factory.BindDiagnostics(new() { Assertion = (expression, file, line) => diagnostics.Add(expression + "@" + line) });
        int temple = factory.Create(165, SidAllocation.Client);
        int priest = factory.Create(158, SidAllocation.Client);
        var owner = new SquidOwner(pool, types, new RawPostPopState(), new SquidOwnerMode());
        owner.Set(temple, 1); owner.Set(priest, 1);
        RawSquidHitPoints.Set(pool, priest, 100);
        Span<byte> templeRaw = pool.AllocatedBytes(temple), priestRaw = pool.AllocatedBytes(priest);
        BinaryPrimitives.WriteSingleLittleEndian(templeRaw[14..], 40.75F); BinaryPrimitives.WriteSingleLittleEndian(templeRaw[18..], 41.25F);
        BinaryPrimitives.WriteUInt32LittleEndian(templeRaw[36..], 25); templeRaw[11] = 0; priestRaw[11] = 0; priestRaw[33] = 1;
        var empty = Enumerable.Range(0, 188).Select(_ => (IReadOnlyList<FrameCode>)new List<FrameCode>()).ToArray();
        var footprint = new RawSquidNeighbors(pool, hash, spots, types, empty);
        var store = new RawSpStore(new GameRandom(1), () => 1); store.Initialize();
        var reward = new RawSquidReward(pool, types, new RawPostPopState(), new SquidRewardState(), store);
        int updates = 0, sounds = 0, reservations = 0;
        var frame = new SquidFrame(pool, types, new()
        {
            FrameSize = (sid, index) => { Assert.Equal(priest, sid); Assert.True(index >= 0 && index < frames[158].Codes.Count); return new float[] { 1, 1 }; },
            Update = (sid, flags) => { Assert.Equal(priest, sid); Assert.Equal(0u, flags); updates++; },
            Unpop = (_, _) => throw new InvalidOperationException("같은단계의식프레임은Unpop을요청하지않는다"),
            Pop = (_, _, _, _) => throw new InvalidOperationException("같은단계의식프레임은Pop을요청하지않는다"),
        });
        var kernel = new Kernel(); var clock = new SquidProcessState { Now = 177 };
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, hash, spots), types);
        var host = new SquidProcessHost(pool, types, kernel, destroy, clock, null, new() { Selected = () => 0, Emit = _ => throw new InvalidOperationException("신전의식mode0은자산삭제를요청하지않는다") });
        host.BindFormFactory(factory);
        // Make 묶음은 실제 장치를 묶고 빈 공급/풀 불일치를 거부한다.
        var wired = RawTempleRitual.MakeTempleRitualHooks(pool, factory, owner, host, new TempleRitualHooks());
        Assert.NotNull(wired.Create); Assert.NotNull(wired.SetHitPoints); Assert.NotNull(wired.SetOwner); Assert.NotNull(wired.AddShared);
        Assert.Throws<ArgumentNullException>(() => RawTempleRitual.MakeTempleRitualHooks(null!, factory, owner, host, new TempleRitualHooks()));
        var other = new RawSidPool(24000, false);
        Assert.Throws<ArgumentException>(() => RawTempleRitual.MakeTempleRitualHooks(other, factory, owner, host, new TempleRitualHooks()));
        var actionHooks = new PriestRitualActionHooks { ReserveShared = () => throw new InvalidOperationException("신전의식mode0은사제604확보를요청하지않는다") };
        var actions = new RawPriestRitualActions(pool, frames, RawPriestRitualActions.MakePriestRitualActionHooks(pool, frame, host, actionHooks));
        var state = new TempleRitualState();
        var effects = new List<string>();
        // 실제0x307 Host 부착은 R4-6 후속이므로 기록형으로 두고 단계·음향·효과 순서를 실행한다.
        var hooks = new TempleRitualHooks
        {
            ReserveShared = () => { reservations++; return true; },
            AddShared = (sid, ev, payload) => effects.Add("S:" + ev + "," + sid + "," + BitConverter.SingleToUInt32Bits(payload)),
            Pop = (_, _, _, _) => throw new InvalidOperationException("신전의식mode0은Rune생성/Pop을요청하지않는다"),
            PlayAt = (x, y, sound, flags) => { Assert.Equal(42.75F, x); Assert.Equal(43.25F, y); Assert.Equal("forRain2.wav", sound); Assert.Equal(0u, flags); Assert.Equal(2, updates); sounds++; },
        };
        var ritual = new RawTempleRitual(pool, hash, types, frames, shapes, spots, footprint, reward, actions, state, hooks);
        TempleRitualPoint point = ritual.StagePoint(temple, 2);
        Span<byte> priestMove = pool.AllocatedBytes(priest);
        BinaryPrimitives.WriteSingleLittleEndian(priestMove[14..], point.X); BinaryPrimitives.WriteSingleLittleEndian(priestMove[18..], point.Y);
        hash.Bucket(1, point.X, point.Y) = (ushort)priest;
        ritual.Ritual(temple, priest, 0);
        Assert.Equal(2, updates); Assert.Equal(1, sounds); Assert.Equal(1, reservations);
        Assert.Equal(["S:775," + temple + "," + BitConverter.SingleToUInt32Bits(1.8799999952316284F)], effects);
        Assert.Same(pool, ritual.Pool); Assert.Same(pool, host.Pool());
        Assert.Equal(4, BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(priest)[36..]));
        Assert.Empty(diagnostics);
        kernel.DrainOwnedForShutdown();
    }
}
