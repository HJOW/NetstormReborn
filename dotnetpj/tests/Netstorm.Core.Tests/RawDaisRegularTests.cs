using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// Dais 첫6 하위 범위의 원문 독립 출력과 실제604 Host 삭제를 같은 풀에서 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 daisregular-core 668행과 Dais 몸체 계약을 같은 풀에서 대조한다.</summary>
public sealed class RawDaisRegularTests
{
    /// <summary>부호없는 입력의 원문 DWORD다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>원문 fixture 행만 읽는다.</summary>
    private static IEnumerable<string[]> FixtureRows(string name) => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", name)).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
    /// <summary>첫 Native fixture와 같은 코드/첫번호/길이/핫스폿의 합성 입력이다. 출력 구간/프레임은 계산하지 않는다.</summary>
    private static void DaisAssets(OriginalTypeRecord[] types, RiftTypeFrames[] frames, SquidDisplayShape[] shapes, char side)
    {
        (uint Type, string Pattern)[] patterns =
        [
            (158, "AEKLMNOP" + new string('P', 24)), (164, "AEMLP" + new string('P', 11)), (165, new string(side, 180)),
            (166, new string('A', 6)), (169, new string('A', 3) + new string('B', 42) + new string('C', 3) + new string('D', 132)),
        ];
        // 원본 입력의 number/variant와 signed short 핫스폿을 준비한다.
        foreach ((uint type, string pattern) in patterns)
        {
            types[type].FootX = types[type].FootY = type == 165 ? 3 : 1;
            var codes = new List<RiftFrameCode>();
            for (int index = 0; index < pattern.Length; index++)
            {
                codes.Add(new RiftFrameCode((byte)pattern[index], (byte)'P', (byte)(index + 1), 0));
                shapes[type].Frames.Add(new SquidDisplayFrame { Width = 16, Height = 11, MuzzleX = (short)((index % 5) * 16), MuzzleY = (short)((index % 5) * 11) });
            }
            frames[type] = new RiftTypeFrames(codes); shapes[type].Loaded = true; shapes[type].FrameCount = pattern.Length;
        }
        // Temple helper의 세 자산 HP 입력을 앞668 관찰과 동일하게 공급한다.
        foreach (uint type in new uint[] { 158, 165, 166 }) { types[type].MaxHitPoints = 200; types[type].Flags1 = 16; }
    }
    /// <summary>원문668행을 두 판본에서 전수 대조한다.</summary>
    [Fact]
    public void MatchesAllNativeDaisRegularCoreRows()
    {
        int rows = 0;
        foreach (string[] row in FixtureRows("daisregular-core-x86.tsv"))
        {
            Assert.Equal(5, row.Length); string[] cells = row[1].Split(','); Assert.Equal(11, cells.Length);
            bool patch = row[0] == "originals";
            string mode = cells[0]; uint frame = N(cells[1]), priestFrame = N(cells[3]);
            bool eventExists = cells[4] != "0"; uint reserve = N(cells[5]); bool altarExists = cells[6] != "0";
            uint captured = N(cells[7]), genus = N(cells[8]), priestOwner = N(cells[9]); int alliance = int.Parse(cells[10], CultureInfo.InvariantCulture);
            char side = cells[2][0];
            var pool = new RawSidPool(24000, false, edition: patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            int count = patch ? 188 : 171;
            OriginalTypeRecord[] types = Enumerable.Range(0, count).Select(_ => new OriginalTypeRecord()).ToArray();
            RiftTypeFrames[] frames = Enumerable.Range(0, count).Select(_ => new RiftTypeFrames([])).ToArray();
            SquidDisplayShape[] shapes = Enumerable.Range(0, count).Select(_ => new SquidDisplayShape()).ToArray();
            DaisAssets(types, frames, shapes, side);
            var empty = Enumerable.Range(0, count).Select(_ => (IReadOnlyList<FrameCode>)new List<FrameCode>()).ToArray();
            byte[] spots = new byte[65536];
            var hash = new RawSquidHash(); var effects = new List<string>(); bool pending = false;
            for (uint number = 5; number <= 11; number++) Assert.Equal((int)number, pool.Allocate(SidAllocation.Client));
            for (int clear = 5; clear <= 11; clear++) pool.RawSlot(clear).Clear();
            void Insert(int sid, uint type)
            {
                Span<byte> raw = pool.RawSlot(sid); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x11010000);
                raw[10] = (byte)type;
                BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 40.75F); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 41.25F);
                ref ushort head = ref hash.Bucket(1, 40.75F, 41.25F); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)sid;
            }
            Insert(5, 165); Insert(6, 158); if (altarExists) Insert(7, 164);
            BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(5), patch ? 0x00507950u : 0x00504090u);
            if (patch) BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(5)[36..], frame); else pool.RawSlot(5)[34] = (byte)frame;
            if (patch) BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(6)[36..], mode == "stop" ? priestFrame : 0); else pool.RawSlot(6)[34] = (byte)(mode == "stop" ? priestFrame : 0);
            if (patch) BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(7)[36..], mode == "can" ? priestFrame : 1); else pool.RawSlot(7)[34] = (byte)(mode == "can" ? priestFrame : 1);
            pool.RawSlot(5)[patch ? 34 : 32] = 1; pool.RawSlot(6)[patch ? 34 : 32] = (byte)priestOwner; pool.RawSlot(7)[patch ? 34 : 32] = 1;
            RawSquidHitPoints.Set(pool, 6, 100);
            types[158].Flags2 = genus;
            // Host 공유 예약의 Form 생성은 실제 생성자 주소를 요구한다. 0.886 입증 방식이다.
            types[80].ConstructorAddress = OriginalConstructors.Address(pool.Edition, 2);
            BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(5)[6..], (ushort)(captured != 0 ? 10 : 0));
            Span<byte> contained = pool.RawSlot(10); BinaryPrimitives.WriteUInt32LittleEndian(contained, 0x11010000);
            contained[10] = 6;
            BinaryPrimitives.WriteUInt16LittleEndian(contained[12..], 2);
            BinaryPrimitives.WriteUInt16LittleEndian(contained[18..], 158);
            BinaryPrimitives.WriteUInt16LittleEndian(contained[20..], 32);
            var book = new RawPostPopState { LocalOwner = 1 };
            OriginalTypeRecord[] baseTypes = patch ? types : Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
            var native = new RawPostPop(pool, baseTypes, book);
            var unpop = new RawUnpop(pool, hash, spots);
            // 삭제기·팩토리는 DN 188타입 고정의 한계로 CD행에 더미 188표를 사용한다. 폼 생성자 주소는 판본대로 둔다.
            OriginalTypeRecord[] sharedTypes = patch ? types : Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
            if (!patch) sharedTypes[80].ConstructorAddress = OriginalConstructors.Address(pool.Edition, 2);
            var destroy = new RawSquidDestroy(pool, unpop, sharedTypes);
            var kernel = new Kernel(); var clock = new SquidProcessState { CurrentProcessFormType = new UIntBox { Value = 80 } };
            var host = new SquidProcessHost(pool, types, kernel, destroy, clock);
            // Host 내부 Form 생성의 원문 진단은 기록형으로 묶는다. 독립 팩토리의 미연결 실패와 구별한다.
            var factory = new RawSquidFactory(pool, sharedTypes, false, unpop);
            var diagnostics = new List<string>();
            factory.BindDiagnostics(new() { Assertion = (expression, file, line) => diagnostics.Add(expression + "@" + line) });
            host.BindFormFactory(factory);
            if (mode == "stop" && eventExists)
            {
                // 604 선행 부착은 입증된 공유 예약 경로로 수행한다. 직접 Attach의 폼 VT 복원은 R4-6 후속이다.
                Assert.NotNull(host.AddSharedRegular(6, 604, 0.01F));
                Assert.NotNull(host.FindEvent(6, 604));
                Assert.Equal(12, BinaryPrimitives.ReadUInt16LittleEndian(pool.Slot(6)[6..]));
                pending = true;
            }
            void SetFrame(int sid, int value, uint flags)
            {
                if (pending && host.FindEvent(6, 604) is null) { effects.Add("K:604,6,0"); pending = false; }
                Assert.Equal(0u, flags);
                effects.Add("F:" + sid + "," + unchecked((uint)value) + ",0");
                if (patch) BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(sid)[36..], unchecked((uint)value)); else pool.RawSlot(sid)[34] = (byte)value;
            }
            var frameObj = new SquidFrame(pool, types, new()
            {
                FrameSize = (_, _) => throw new InvalidOperationException("첫Dais범위의프레임지정은원본명시경계다"),
                Update = (_, _) => throw new InvalidOperationException("첫Dais범위는실제프레임표시를실행하지않는다"),
                Unpop = (_, _) => throw new InvalidOperationException("첫Dais범위는실제Frame Unpop을실행하지않는다"),
                Pop = (_, _, _, _) => throw new InvalidOperationException("첫Dais범위는실제Frame Pop을실행하지않는다"),
            });
            var carrier = new RawCarrierPreDestroy(pool, new ContainedFinderState(), new()
            {
                ValidateDamageable = (_, _) => throw new InvalidOperationException("선택조회는Carrier삭제검사를실행하지않는다"),
                DamageablePre = (_, _) => throw new InvalidOperationException("선택조회는Carrier삭제를실행하지않는다"),
                DependentFound = () => throw new InvalidOperationException("선택조회는Carrier후처리를실행하지않는다"),
            });
            var lookup = new RawContainedLookup(pool, (_, _) => throw new InvalidOperationException("첫Dais범위는경로종속삭제를실행하지않는다"));
            var path = new RawPathStart(pool, hash, types, host, lookup, frameObj, new RawPathStartState(clock), new()
            {
                ProcessType = _ => throw new InvalidOperationException("첫Dais범위는Path mapper를실행하지않는다"),
                Create = _ => throw new InvalidOperationException("첫Dais범위는Path생성을실행하지않는다"),
                Metadata = _ => throw new InvalidOperationException("첫Dais범위는Path메타를실행하지않는다"),
                ClearBlocked = (_, _, _) => throw new InvalidOperationException("첫Dais범위는경로막힘정리를실행하지않는다"),
            });
            var footprint = new RawSquidNeighbors(pool, hash, spots, types, empty);
            OriginalTypeRecord[] rewardTypes = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
            rewardTypes[158].MaxHitPoints = 200;
            var store = new RawSpStore(new GameRandom(1), () => 1); store.Initialize();
            var reward = new RawSquidReward(pool, rewardTypes, new RawPostPopState(), new SquidRewardState(), store);
            var templeState = new TempleRitualState();
            RawTempleRitual? ritual = null;
            var templeHooks = new TempleRitualHooks
            {
                Create = (_, _) => throw new InvalidOperationException("첫Dais범위는Rune생성을실행하지않는다"),
                SetHitPoints = (_, _) => throw new InvalidOperationException("첫Dais범위는RuneHP를실행하지않는다"),
                SetOwner = (_, _) => throw new InvalidOperationException("첫Dais범위는Runeowner를실행하지않는다"),
                Pop = (_, _, _, _) => throw new InvalidOperationException("첫Dais범위는RunePop을실행하지않는다"),
                PlayAt = (_, _, _, _) => throw new InvalidOperationException("첫Dais범위는의식위치음향을실행하지않는다"),
                ReserveShared = () => throw new InvalidOperationException("첫Dais범위는Temple307예약을실행하지않는다"),
                AddShared = (_, _, _) => throw new InvalidOperationException("첫Dais범위는Temple307부착을실행하지않는다"),
                Count = sid => ritual!.Count(sid),
            };
            ritual = new RawTempleRitual(pool, hash, types, frames, shapes, spots, footprint, reward,
                new RawPriestRitualActions(pool, frames, new()
                {
                    SetFrame = (_, _, _) => throw new InvalidOperationException("첫Dais범위는사제의식Begin을실행하지않는다"),
                    ReserveShared = () => throw new InvalidOperationException("첫Dais범위는사제604예약을실행하지않는다"),
                    AddShared = (_, _, _) => throw new InvalidOperationException("첫Dais범위는사제604부착을실행하지않는다"),
                }), templeState, templeHooks);
            var runeState = new RunePostPopState();
            var rune = new RawRunePostPop(pool, hash, types, runeState, native, new()
            {
                ReserveRegular = () => throw new InvalidOperationException("첫Dais범위는Rune304확보를실행하지않는다"),
                AddRegular = (_, _, _) => throw new InvalidOperationException("첫Dais범위는Rune304부착을실행하지않는다"),
                StartConstruction = (_, _) => throw new InvalidOperationException("첫Dais범위는Rune건설을실행하지않는다"),
            });
            uint localOwner = book.LocalOwner;
            var state = new DaisRegularState(templeState, runeState, clock, new SquidRewardState(), localOwner);
            state.Alliances[2 + 9] = alliance;
            var hooks = new DaisRegularHooks
            {
                SetFrame = SetFrame,
                ReserveShared = () => { effects.Add("N:40"); return reserve != 0; },
                AddShared = (sid, ev, payload) => effects.Add("S:" + ev + "," + sid + "," + BitConverter.SingleToUInt32Bits(payload)),
                PlayAt = (x, y, name, flags) => effects.Add("A:" + BitConverter.SingleToUInt32Bits(x) + "," + BitConverter.SingleToUInt32Bits(y) + "," + name + "," + flags),
                Destroy = (_, _) => throw new InvalidOperationException("첫Dais범위는삭제를실행하지않는다"),
                ReserveSound = () => throw new InvalidOperationException("첫Dais범위는희생소리확보를실행하지않는다"),
                LoadSound = (_, _, _) => throw new InvalidOperationException("첫Dais범위는희생소리조회를실행하지않는다"),
                AttachSound = (_, _, _, _) => throw new InvalidOperationException("첫Dais범위는희생소리부착을실행하지않는다"),
                WallSeconds = () => throw new InvalidOperationException("첫Dais범위는벽시계를실행하지않는다"),
                SetSoundStart = (_, _) => throw new InvalidOperationException("첫Dais범위는지연시각을실행하지않는다"),
                StopVoice = (_, _, _) => throw new InvalidOperationException("첫Dais범위는희생종료를실행하지않는다"),
                ProcessType = _ => throw new InvalidOperationException("첫Dais범위는Dais Path조회를실행하지않는다"),
                PathMetadata = _ => throw new InvalidOperationException("첫Dais범위는Dais Path메타를실행하지않는다"),
                Damageable = (_, _, _, _) => throw new InvalidOperationException("첫Dais범위는나머지Damageable를실행하지않는다"),
                AddLocalSp = _ => throw new InvalidOperationException("첫Dais범위는SP획득을실행하지않는다"),
                NormalizeForeignSp = _ => throw new InvalidOperationException("첫Dais범위는다른SP변환을실행하지않는다"),
                GrantForeignType = (_, _) => throw new InvalidOperationException("첫Dais범위는다른타입획득을실행하지않는다"),
                LocalSpecial = _ => throw new InvalidOperationException("첫Dais범위는로컬특수획득을실행하지않는다"),
                LocalType = _ => throw new InvalidOperationException("첫Dais범위는로컬타입획득을실행하지않는다"),
                RefreshOwner = _ => throw new InvalidOperationException("첫Dais범위는owner갱신을실행하지않는다"),
                RefreshLocal = () => throw new InvalidOperationException("첫Dais범위는메뉴갱신을실행하지않는다"),
                PriestExists = _ => throw new InvalidOperationException("첫Dais범위는player사제조회를실행하지않는다"),
                MultiplayerStatus = () => throw new InvalidOperationException("첫Dais범위는멀티상태조회를실행하지않는다"),
                Chat = _ => throw new InvalidOperationException("첫Dais범위는희생채팅을실행하지않는다"),
            };
            var dais = new RawDaisRegular(pool, hash, types, frames, ritual, rune, carrier, host, lookup, path, state, hooks);
            string returned = "-";
            if (mode == "stop") dais.StopPriestRitual(6);
            else if (mode == "collapse") dais.Collapse(5);
            else if (mode == "start") dais.StartSacrifice(5);
            else if (mode == "advance") returned = BitConverter.SingleToUInt32Bits(dais.SacrificeAdvance(5)).ToString(CultureInfo.InvariantCulture);
            else if (mode == "handle305") returned = BitConverter.SingleToUInt32Bits(dais.Handle(5, 0x305, 1, 9.25F)).ToString(CultureInfo.InvariantCulture);
            else if (mode == "can") returned = dais.CanBridge(5, 6) ? "1" : "0";
            else throw new InvalidOperationException("Dais 첫fixture 모드 누락");
            Assert.Equal(row[2], returned);
            Assert.Equal(row[3], effects.Count == 0 ? "-" : string.Join(';', effects));
            string[] images = row[4].Split(';'); Assert.Equal(4, images.Length);
            foreach (int number in new int[] { 5, 6, 7, 10 })
            {
                string image = Convert.ToHexString(pool.Slot(number)).ToLowerInvariant();
                string expected = images[number == 10 ? 3 : number - 5];
                // CD 원문 슬롯은 36바이트이며 DN 슬롯 50바이트와 길이가 다르다. VT 8자 제외 구간만 대조한다.
                if (patch) { Assert.True(expected.Length == image.Length, $"row {rows} sid {number}"); Assert.Equal(expected.Substring(8), image.Substring(8)); }
                else Assert.Equal(expected.Substring(8), image.Substring(8, expected.Length - 8));
            }
            Assert.Empty(diagnostics);
            rows++;
        }
        Assert.Equal(668, rows);
    }
    /// <summary>같은 풀의 실제 몸체 계약과 Cd 판본 개수 불일치를 거부한다.</summary>
    [Fact]
    public void RejectsNullAndMismatchedPool()
    {
        var pool = new RawSidPool(24000, false);
        var hash = new RawSquidHash(); byte[] spots = new byte[65536];
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        SquidDisplayShape[] shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        DaisAssets(types, frames, shapes, 'A');
        var empty = Enumerable.Range(0, 188).Select(_ => (IReadOnlyList<FrameCode>)new List<FrameCode>()).ToArray();
        var store = new RawSpStore(new GameRandom(1), () => 1); store.Initialize();
        var reward = new RawSquidReward(pool, types, new RawPostPopState(), new SquidRewardState(), store);
        var footprint = new RawSquidNeighbors(pool, hash, spots, types, empty);
        var templeState = new TempleRitualState();
        var ritual = new RawTempleRitual(pool, hash, types, frames, shapes, spots, footprint, reward,
            new RawPriestRitualActions(pool, frames, new() { SetFrame = (_, _, _) => { }, ReserveShared = () => false, AddShared = (_, _, _) => { } }),
            templeState, new() { Count = sid => 1 });
        var book = new RawPostPopState();
        var rune = new RawRunePostPop(pool, hash, types, new RunePostPopState(), new RawPostPop(pool, types, book), new()
        {
            ReserveRegular = () => false, AddRegular = (_, _, _) => { }, StartConstruction = (_, _) => { },
        });
        var carrier = new RawCarrierPreDestroy(pool, new ContainedFinderState(), new()
        {
            ValidateDamageable = (_, _) => throw new InvalidOperationException("계약 검사는Carrier삭제검사를실행하지않는다"),
            DamageablePre = (_, _) => throw new InvalidOperationException("계약 검사는Carrier삭제를실행하지않는다"),
            DependentFound = () => throw new InvalidOperationException("계약 검사는Carrier후처리를실행하지않는다"),
        });
        var host = new SquidProcessHost(pool, types, new Kernel(), new RawSquidDestroy(pool, new RawUnpop(pool, hash, spots), types), new SquidProcessState());
        var lookup = new RawContainedLookup(pool, (_, _) => { });
        var frameObj = new SquidFrame(pool, types, new()
        {
            FrameSize = (_, _) => new float[] { 1, 1 }, Update = (_, _) => { },
            Unpop = (_, _) => { }, Pop = (_, _, _, _) => { },
        });
        var path = new RawPathStart(pool, hash, types, host, lookup, frameObj, new RawPathStartState(new SquidProcessState()), new()
        {
            ProcessType = _ => 0, Create = _ => null, Metadata = _ => new RawPathStartType(0, 0, 0), ClearBlocked = (_, _, _) => { },
        });
        var state = new DaisRegularState(templeState, new RunePostPopState(), new SquidProcessState(), new SquidRewardState(), 1);
        var hooks = new DaisRegularHooks { SetFrame = (_, _, _) => { } };
        Assert.Throws<ArgumentNullException>(() => new RawDaisRegular(null!, hash, types, frames, ritual, rune, carrier, host, lookup, path, state, hooks));
        var other = new RawSidPool(24000, false);
        Assert.Throws<ArgumentException>(() => new RawDaisRegular(other, hash, types, frames, ritual, rune, carrier, host, lookup, path, state, hooks));
        var cd = new RawSidPool(24000, false, edition: OriginalEdition.Cd1072);
        OriginalTypeRecord[] cdTypes = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToArray();
        RiftTypeFrames[] cdFrames = Enumerable.Range(0, 171).Select(_ => new RiftTypeFrames([])).ToArray();
        Assert.Throws<ArgumentException>(() => new RawDaisRegular(cd, hash, cdTypes, cdFrames, ritual, rune, carrier, host, lookup, path, state, hooks));
        var cdHooks = new DaisRegularHooks
        {
            SetFrame = (_, _, _) => { }, Destroy = (_, _) => { }, ReserveShared = () => false, AddShared = (_, _, _) => { },
            PlayAt = (_, _, _, _) => { }, ReserveSound = () => false, LoadSound = (_, _, _) => 0, AttachSound = (_, _, _, _) => 0,
            WallSeconds = () => 0, SetSoundStart = (_, _) => { }, StopVoice = (_, _, _) => { }, ProcessType = _ => 0,
            PathMetadata = _ => new RawPathStartType(0, 0, 0), Damageable = (_, _, _, _) => 0, AddLocalSp = _ => { },
            NormalizeForeignSp = _ => { }, GrantForeignType = (_, _) => { }, LocalSpecial = _ => { }, LocalType = _ => { },
            RefreshOwner = _ => { }, RefreshLocal = () => { }, PriestExists = _ => false, MultiplayerStatus = () => 0,
            Chat = _ => { },
        };
        Assert.Throws<ArgumentException>(() => new RawDaisRegular(cd, hash, cdTypes, cdFrames, ritual, rune, carrier, host, lookup, path, state, cdHooks));
    }
}
