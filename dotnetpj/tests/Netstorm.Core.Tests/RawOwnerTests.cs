using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 소유자 지정(<see cref="SquidOwner"/>)의 보호 경로와 목록 계약을 확인한다.
/// 원본 기대값 재생(X86RawOwnerTests)은 변경 결과·목록·체크섬을 대조하고,
/// 이 검사들은 cpppj/tests/OwnerTests.cpp 의 보호 검사만 10.78 몫으로 옮겼다 (W02 Owner 조각, 2026-10-20).
/// </summary>
public sealed class RawOwnerTests
{
    /// <summary>관찰 대상 타입 번호.</summary>
    private const int ObjectType = 82;

    /// <summary>검사 대상 번호.</summary>
    private const int Target = 50;

    /// <summary>작업장 목록 부류 마스크(소용돌이·공장).</summary>
    private const uint FactoryMask = 0x4200;

    /// <summary>번호 50 을 클라이언트 영역에 확보한 풀을 만든다.</summary>
    private static RawSidPool Pool()
    {
        var pool = new RawSidPool(24000, false);
        // 원본 관찰의 번호 범위를 클라이언트 영역에 확보한다
        for (int id = 5; id <= 70; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
        return pool;
    }

    /// <summary>타입 레코드 188개를 만들고 관찰 타입의 부류를 정한다.</summary>
    /// <param name="genus">82번 타입의 플래그2.</param>
    private static OriginalTypeRecord[] Types(uint genus = FactoryMask)
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 쓰지 않는 번호도 슬롯 검사의 범위 안에 들어야 한다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        types[ObjectType].Flags2 = genus;
        return types;
    }

    /// <summary>기본 슬롯(관찰 타입·빈 상태·중립 소유자)을 채운다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="state">상태 바이트.</param>
    /// <param name="owner">소유자 바이트.</param>
    /// <param name="extra">추가 비트 바이트.</param>
    private static void Fill(RawSidPool pool, byte state = 0, byte owner = 0, byte extra = 0)
    {
        Span<byte> raw = pool.AllocatedBytes(Target);
        raw.Clear();
        raw[RawSidPool.TypeOffset] = ObjectType;
        raw[RawSidPool.StateOffset] = state;
        raw[34] = owner;
        raw[40] = extra;
    }

    /// <summary>
    /// 판본 assert 가 보고하는 범위 밖 소유자는 목록이 없는 번호까지 쓰기 전에 거부한다.
    /// 일반 9·도전 0·20·40 을 확인한다.
    /// </summary>
    [Fact]
    public void RejectsOutOfRangePlayersBeforeWriting()
    {
        var pool = Pool();
        var types = Types();
        var book = new RawPostPopState();
        var mode = new SquidOwnerMode { Battle = true };
        Fill(pool, 0, 3, 0);
        // 짧은 타입 표(CD 171개)는 받지 않는다
        Assert.ThrowsAny<Exception>(() => new SquidOwner(pool, types.Take(100).ToList(), book, mode));
        var owner = new SquidOwner(pool, types, book, mode);
        book.OwnerFactories[3].Entries = new uint[] { Target, 0 };
        book.OwnerFactories[3].Count = 1;
        string slotBefore = Convert.ToHexStringLower(pool.Slot(Target));
        string listBefore = string.Join(',', book.OwnerFactories[3].Entries);
        // 목록이 없는 9번은 쓰기 전에 거부한다
        Assert.ThrowsAny<Exception>(() => owner.Set(Target, 9));
        mode.Challenge = true;
        // 도전 모드에서 0·40 은 범위 밖, 20 은 목록이 없어서 거부한다
        Assert.ThrowsAny<Exception>(() => owner.Set(Target, 0));
        Assert.ThrowsAny<Exception>(() => owner.Set(Target, 20));
        Assert.ThrowsAny<Exception>(() => owner.Set(Target, 40));
        // 거부된 호출은 소유자 바이트와 목록을 바꾸지 않는다
        Assert.Equal(3, pool.Slot(Target)[34]);
        Assert.Equal(1u, book.OwnerFactories[3].Count);
        Assert.Equal(slotBefore, Convert.ToHexStringLower(pool.Slot(Target)));
        Assert.Equal(listBefore, string.Join(',', book.OwnerFactories[3].Entries));
    }

    /// <summary>
    /// 작업장 부류의 소유자 변경은 이전 목록에서 빼고 새 목록에 넣는다.
    /// 빈·매립·모드 밖이면 소유자 바이트만 바꾼다.
    /// </summary>
    [Fact]
    public void MovesFactoryEntriesBetweenOwnerLists()
    {
        // 전투 모드의 작업장 부류는 이전 목록에서 빼고 새 목록에 넣는다
        {
            var pool = Pool();
            var types = Types();
            var book = new RawPostPopState();
            var mode = new SquidOwnerMode { Battle = true };
            Fill(pool, 0, 3, 0);
            var owner = new SquidOwner(pool, types, book, mode);
            book.OwnerFactories[3].Entries = new uint[] { Target };
            book.OwnerFactories[3].Count = 1;
            book.OwnerFactories[5].Entries = new uint[4];
            book.OwnerFactories[5].Count = 0;
            owner.Set(Target, 5);
            Assert.Equal(5, pool.Slot(Target)[34]);
            Assert.Equal(0u, book.OwnerFactories[3].Count);
            Assert.Equal(1u, book.OwnerFactories[5].Count);
            Assert.Equal((uint)Target, book.OwnerFactories[5].Entries[0]);
            // 소유자 0으로 바꾸면 목록에서만 빠진다
            owner.Set(Target, 0);
            Assert.Equal(0, pool.Slot(Target)[34]);
            Assert.Equal(0u, book.OwnerFactories[5].Count);
        }
        // 빈 상태·매립 추가이면 목록에 넣지 않는다
        foreach (byte blocked in new byte[] { 0, 1 })
        {
            var pool = Pool();
            var types = Types();
            var book = new RawPostPopState();
            var mode = new SquidOwnerMode { Battle = true };
            Fill(pool, blocked == 0 ? (byte)4 : (byte)0, 3, blocked == 0 ? (byte)0 : (byte)8);
            var owner = new SquidOwner(pool, types, book, mode);
            book.OwnerFactories[3].Entries = new uint[] { Target };
            book.OwnerFactories[3].Count = 1;
            book.OwnerFactories[5].Entries = new uint[4];
            book.OwnerFactories[5].Count = 0;
            owner.Set(Target, 5);
            Assert.Equal(5, pool.Slot(Target)[34]);
            Assert.Equal(0u, book.OwnerFactories[3].Count);
            Assert.Equal(0u, book.OwnerFactories[5].Count);
        }
        // 모드 밖이면 목록을 건드리지 않는다
        {
            var pool = Pool();
            var types = Types();
            var book = new RawPostPopState();
            var mode = new SquidOwnerMode();
            Fill(pool, 0, 3, 0);
            var owner = new SquidOwner(pool, types, book, mode);
            book.OwnerFactories[3].Entries = new uint[] { Target };
            book.OwnerFactories[3].Count = 1;
            owner.Set(Target, 5);
            Assert.Equal(5, pool.Slot(Target)[34]);
            Assert.Equal(1u, book.OwnerFactories[3].Count);
            Assert.Equal(0u, book.OwnerFactories[5].Count);
        }
        // 가득 찬 목록은 조용히 생략하고 소유자 바이트는 쓴다
        {
            var pool = Pool();
            var types = Types();
            var book = new RawPostPopState();
            var mode = new SquidOwnerMode { Battle = true };
            Fill(pool, 0, 0, 0);
            var owner = new SquidOwner(pool, types, book, mode);
            book.OwnerFactories[5].Entries = new uint[] { 7, 8 };
            book.OwnerFactories[5].Count = 2;
            owner.Set(Target, 5);
            Assert.Equal(5, pool.Slot(Target)[34]);
            Assert.Equal(2u, book.OwnerFactories[5].Count);
        }
    }
}
