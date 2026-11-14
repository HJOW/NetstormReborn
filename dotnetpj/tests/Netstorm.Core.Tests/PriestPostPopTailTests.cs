using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 사제 후반의 현재 상태 재조회와 일반 Pop/Activate 분배를 검증한다.
namespace Netstorm.Tests;

/// <summary>PATCH 원본2048입력과 같은 실제 모듈의 가상 분배를 대조한다.</summary>
public sealed class PriestPostPopTailTests
{
    /// <summary>정수 비트를 주변 바이트 변경 없이 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>원본 입력만 부호 없는 DWORD로 읽는다.</summary>
    private static uint Number(string text) => X86Fixture.UInt(text);

    /// <summary>실제 접두·HP·사제 상태와 원본 외부 입력/관측을 소유하는 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>공유 원시 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>HP·Carrier·사제 상태가 같은 TYPE를 읽는다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>원문 방향 코드의 입력 표다.</summary>
        public readonly RiftTypeFrames[] Frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        /// <summary>현재 지면 점유 지도다.</summary>
        public readonly byte[] Spots = new byte[65536];
        /// <summary>회복 예약과 전체 장부가 공유할 상태다.</summary>
        public readonly RawPostPopState Books = new();
        /// <summary>목록·소유자 상태다.</summary>
        public readonly PriestPostPopState Priests = new();
        /// <summary>초기화되지 않아야 하는 실제 SP다.</summary>
        public readonly RawSpStore Sp = new(new GameRandom(), () => 1u);
        /// <summary>접두와 사제 상태가 같은 실제 HP를 읽는다.</summary>
        public readonly RawSquidReward Hp;
        /// <summary>접두와 상태 조회가 함께 읽는 현재 사제HP 배율이다.</summary>
        public readonly SquidRewardState HpMode = new();
        /// <summary>도달 효과의 실제 인자와 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>원본 입력의 기존 예약 유무다.</summary>
        public bool Found;
        /// <summary>원본 입력의 메모리 확보 유무다.</summary>
        public bool Allocated;
        /// <summary>Carrier·낙하 효과가 바꾸는 원본 입력 프로필이다.</summary>
        public uint Change;

        /// <summary>현재 TYPE·HP·장부의 동일 주소를 연결한다.</summary>
        public Scene() => Hp = new RawSquidReward(Pool, Types, Books, HpMode, Sp);

        /// <summary>판정 결과를 계산하지 않고 원본 행의 raw/지면/프레임을 공급한다.</summary>
        public void Prepare(string[] row)
        {
            Assert.Equal(14, row.Length); Events.Clear(); Array.Clear(Spots);
            Found = Number(row[7]) != 0; Allocated = Number(row[8]) != 0; Change = Number(row[9]);
            Types[158].Flags1 = 0x69012; Types[158].Flags2 = 0x210000; Types[158].MaxHitPoints = 200;
            Frames[158] = new RiftTypeFrames([new RiftFrameCode((byte)Number(row[6]), 80, 1, 0)]);
            Spots[21 * 256 + 20] = (byte)Number(row[4]); Spots[22 * 256 + 21] = (byte)Number(row[5]);
            Priests.Priests.Entries = [60, 50, 70, 80]; Priests.Priests.Count = 0;
            // 0번과 변경되지 않은 소유자까지 구별하는 원본 입력이다.
            for (int owner = 0; owner < 9; owner++) Priests.OwnerState[owner] = 0xabc000u + (uint)owner;
            Span<byte> raw = Pool.RawSlot(50); raw.Clear(); Put(raw, 0, 0x50f210); raw[10] = 158;
            raw[34] = 1; raw[12] = 1; raw[40] = (byte)Number(row[2]); Put(raw, 26, Number(row[3]));
            Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
        }

        /// <summary>접두 검색·확보·부착의 인자와 목록 갱신 시점을 관찰한다.</summary>
        public PriestPostPopHooks PrefixHooks() => new()
        {
            FindRegular = (sid, ev) => { Events.Add($"F:{sid}:{ev}:{Priests.GetPriests().Count}"); return Found; },
            ReserveRegular = () => { Events.Add("A:40"); return Allocated; },
            AddRegular = (sid, ev, payload) => Events.Add($"R:{sid}:{ev}:{BitConverter.SingleToUInt32Bits(payload)}"),
        };

        /// <summary>Carrier의 외부 효과가 바꾸는 raw 입력만 공급한다.</summary>
        public void CarrierEffect(int sid, uint flags)
        {
            Events.Add($"C:{sid}:{flags}"); Span<byte> raw = Pool.RawSlot(sid);
            if (Change == 1) raw[40] = 1;
            else if (Change is 2 or 3)
            {
                raw[40] = Change == 2 ? (byte)32 : (byte)0; Put(raw, 26, 150);
                Put(raw, 14, BitConverter.SingleToUInt32Bits(30.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(31.9f));
                Spots[31 * 256 + 30] = Change == 2 ? (byte)0 : (byte)6; Spots[32 * 256 + 31] = 6;
            }
        }

        /// <summary>낙하/보호막 요청의 실제 순서와 낙하 콜백 이후 변이를 관측한다.</summary>
        public PriestPostPopTailHooks TailHooks() => new()
        {
            Fall = sid => { Events.Add($"D:{sid}"); if (Change == 4) Pool.RawSlot(sid)[40] = 1; },
            EnsureShield = sid => Events.Add($"S:{sid}"), ClearShield = sid => Events.Add($"X:{sid}"),
        };

        /// <summary>실제 Carrier를 공급된 원본 외부 경계와 연결한다.</summary>
        public RawCarrierPostPop Carrier(Action<int, uint> effect) => new(Pool, Types, new CarrierPostPopState(), new CarrierPostPopHooks
        {
            CarrierCheck = _ => throw new InvalidOperationException("검사 입력에서 선택되지 않는 Carrier 조회"),
            ClaimGround = (_, _, _) => throw new InvalidOperationException("검사 입력에서 선택되지 않는 지면 소유자 변경"),
            ValidateBase = (_, _) => { }, Base = effect,
        });
    }

    /// <summary>원본2048입력의 호출순서·현재 raw50바이트·목록·9DWORD를 모두 비교한다.</summary>
    [Fact]
    public void NativePatch2048()
    {
        string[][] all = X86Fixture.Read("priestpostpoptail"); Assert.Equal(6144, all.Length);
        string[][] rows = all.Where(row => row[0] == "originals").ToArray(); Assert.Equal(2048, rows.Length);
        var scene = new Scene();
        X86Fixture.CheckRows(rows, row =>
        {
            scene.Prepare(row); var prefix = new RawPriestPostPop(scene.Pool, scene.Hp, scene.Priests, scene.PrefixHooks());
            var state = new RawPriestState(scene.Pool, scene.Hp, scene.Types, scene.Spots);
            var tail = new RawPriestPostPopTail(scene.Pool, prefix, scene.Carrier(scene.CarrierEffect), state, scene.Frames, scene.Spots, scene.TailHooks());
            tail.PostPop(50, Number(row[1]));
            return (scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)) == row[10]
                && scene.Pool.Slot(50).SequenceEqual(Convert.FromHexString(row[11]))
                && $"{scene.Priests.Priests.Count}:{string.Join(',', scene.Priests.Priests.Entries)}" == row[12]
                && string.Join(',', scene.Priests.OwnerState) == row[13];
        });
        Assert.False(scene.Sp.Initialized);
    }

    /// <summary>일반 Pop의 공간 변경→실제 사제 분배→공통 깊이·장부 후처리를 같은 풀에서 검증한다.</summary>
    [Fact]
    public void OrdinaryPopSharesPriestBooksAndHash()
    {
        var scene = new Scene(); scene.Prepare(X86Fixture.Read("priestpostpoptail")[0]);
        Span<byte> raw = scene.Pool.RawSlot(50); raw[11] = RawSidPool.VoidBit;
        scene.Types[158].FootX = 1; scene.Types[158].FootY = 1; scene.Types[158].Cost = 123;
        var common = new RawPostPop(scene.Pool, scene.Types, scene.Books);
        var carrier = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState(),
            RawCarrierPostPop.MakeCarrierPostPopHooks(scene.Pool, common, _ => { }, (_, _, _) => { }));
        var prefix = new RawPriestPostPop(scene.Pool, scene.Hp, scene.Priests, scene.PrefixHooks());
        var state = new RawPriestState(scene.Pool, scene.Hp, scene.Types, scene.Spots);
        var tail = new RawPriestPostPopTail(scene.Pool, prefix, carrier, state, scene.Frames, scene.Spots, scene.TailHooks());
        Assert.False(common.HandlesDerived(50)); common.SetPriestPostPop(tail); Assert.True(common.HandlesPriest(50));
        var hash = new RawSquidHash(); var pop = new RawPop(scene.Pool, hash, scene.Spots, null, common);
        pop.Pop(50, scene.Types[158], 1f, 1f, 20.75f, 21.9f);
        Assert.Equal(0u, scene.Books.Depth); Assert.Equal(123, scene.Books.TotalCost); Assert.Equal(1u, scene.Books.GlobalCounts[158]);
        Assert.Equal(1u, scene.Priests.Priests.Count); Assert.Equal(50u, new RawPriestLookup(scene.Pool, scene.Priests).Find(1));
        Assert.Equal(0, scene.Pool.Slot(50)[11] & RawSidPool.VoidBit);
        var finder = new RawSquidFinder(scene.Pool, hash, scene.Types);
        Assert.Equal(50, finder.Begin(new SquidSearchArea(20, 21, 20, 21)));
        Assert.Equal(new[] { "D:50", "S:50" }, scene.Events.TakeLast(2));
        Put(scene.Pool.RawSlot(50), 26, 40); scene.Spots[22 * 256 + 21] = 6;
        Assert.True(state.Immobile(50)); scene.HpMode.PriestQuarterHp = true; Assert.False(state.Immobile(50));
        common.SetPriestPostPop(null); Assert.False(common.HandlesDerived(50));
        Assert.Throws<InvalidOperationException>(() => common.Activate(50, 0x800));
    }

    /// <summary>낙하 플래그와 Carrier 뒤 extra가 불필요한 좌표·프레임·소비자 조회를 차단하는지 확인한다.</summary>
    [Fact]
    public void LazyReadsAndNativeVtableDispatch()
    {
        var scene = new Scene(); scene.Prepare(X86Fixture.Read("priestpostpoptail")[0]);
        var prefix = new RawPriestPostPop(scene.Pool, scene.Hp, scene.Priests, new PriestPostPopHooks());
        var state = new RawPriestState(scene.Pool, scene.Hp, scene.Types, scene.Spots); int shields = 0;
        var hooks = new PriestPostPopTailHooks { EnsureShield = _ => shields++ };
        var tail = new RawPriestPostPopTail(scene.Pool, prefix, scene.Carrier((_, _) => { }), state, scene.Frames, scene.Spots, hooks);
        Span<byte> raw = scene.Pool.RawSlot(50); raw[40] = 32; Put(raw, 14, 0x7fc12345); Put(raw, 36, 0xffffffff);
        tail.PostPop(50, 0x800); Assert.Equal(1, shields);
        Assert.Throws<ArgumentOutOfRangeException>(() => tail.PostPop(50, 0)); Assert.Equal(1, shields);
        raw[40] = 8; tail.PostPop(50, 0); Assert.Equal(1, shields);
        var common = new RawPostPop(scene.Pool, scene.Types, scene.Books); common.SetPriestPostPop(tail);
        raw[10] = 159; raw[40] = 32; scene.Books.Depth = 1;
        Assert.True(common.HandlesPriest(50));
        // 직접 PATCH 가상 분배는 TYPE158 추가 관문 없이 현재 VT의 실제 몸체로 간다.
        var directCarrier = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState(),
            RawCarrierPostPop.MakeCarrierPostPopHooks(scene.Pool, common, _ => { }, (_, _, _) => { }));
        var directTail = new RawPriestPostPopTail(scene.Pool, prefix, directCarrier, state, scene.Frames, scene.Spots, hooks);
        common.SetPriestPostPop(directTail); common.PostPopVirtual(50, 0x800);
        Assert.Equal(0u, scene.Books.Depth); Assert.Equal(2, shields);
        Assert.Throws<ArgumentException>(() => tail.Validate(50));
        Assert.Throws<ArgumentException>(() => new RawPriestPostPopTail(new RawSidPool(24000), prefix,
            directCarrier, state, scene.Frames, scene.Spots, hooks));
    }
}
