using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 패치판 SP 저장소(RawSpStore)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/SpStoreNoticeTests.cpp 의 SpStoreNotice_ReplaysExistingNativeOffsets 와 같다:
/// 행마다 재시드·fill 로 저장소를 만들고 초기화(I)·쓰기(S)·회전(R) 명령을 차례로 실행한다.
/// 명령마다 offset·추적 offset·난수 상태와 32×9 DWORD 배열 전체를 대조한다.
/// 기준 판본은 10.78 이다 (W05 선행 SP 저장소 조각, 2026-10-22).
/// </summary>
public sealed class X86RawSpStoreTests
{
    /// <summary>fixture 의 행 수.</summary>
    private const int RowCount = 61;

    /// <summary>한 스냅샷의 DWORD 배열 칸 수(32×9).</summary>
    private const int PoolWords = RawSpStore.Bits * RawSpStore.Owners;

    /// <summary>DWORD 배열을 원본과 같은 리틀 엔디언 hex 로 직렬화한다.</summary>
    /// <param name="pools">32×9 DWORD 배열.</param>
    private static string Dump(ReadOnlySpan<uint> pools)
    {
        var text = new StringBuilder(pools.Length * 8);
        // 각 DWORD 를 낮은 바이트부터 쓴다
        foreach (uint word in pools)
        {
            for (int i = 0; i < 4; i++)
            {
                text.Append(((byte)(word >> (8 * i))).ToString("x2"));
            }
        }
        return text.ToString();
    }

    /// <summary>
    /// SP 회전 fixture 의 모든 행을 재생한다.
    /// </summary>
    [Fact]
    public void SpStoreMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("sprotate");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(4, row.Length);
            uint seed = X86Fixture.UInt(row[0]);
            uint fill = X86Fixture.UInt(row[1]);
            var rng = new NetstormRandom(0);
            var store = new RawSpStore(rng, () => seed, fill);
            int tracking = -999;
            store.BindTrackingOffset(value => tracking = value);
            string[] actions = row[2].Split(';');
            string[] snapshots = row[3].Split('|');
            Assert.Equal(actions.Length, snapshots.Length);
            for (int index = 0; index < actions.Length; index++)
            {
                string[] action = actions[index].Split(':');
                switch (action[0])
                {
                    case "I":
                        store.Initialize();
                        break;
                    case "S":
                        store.Set(X86Fixture.UInt(action[1]), X86Fixture.UInt(action[2]));
                        break;
                    case "R":
                        // 회전 횟수는 원본처럼 매번 한 걸음씩 실행한다
                        for (uint step = 0; step < X86Fixture.UInt(action[1]); step++)
                        {
                            store.Rotate();
                        }
                        break;
                    default:
                        throw new InvalidOperationException($"SP 저장소 fixture 명령 오류: {action[0]}");
                }
                string[] halves = snapshots[index].Split(':');
                string[] values = halves[0].Split(',');
                Assert.Equal(X86Fixture.UInt(values[0]), store.Offset);
                Assert.Equal(X86Fixture.Int(values[1]), tracking);
                Assert.Equal(X86Fixture.UInt(values[2]), rng.State);
                Assert.Equal(2304, halves[1].Length);
                Assert.Equal(halves[1], Dump(store.Pools));
                Assert.True(store.Initialized);
                Assert.True(store.FirstPool);
                // 초기화 뒤 순열은 항등이다
                for (short position = 0; position < RawSpStore.Bits; position++)
                {
                    Assert.Equal(position, store.Permutation[position]);
                }
            }
            count++;
            return true;
        });
        Assert.Equal(RowCount, count);
    }

    /// <summary>
    /// 저장소는 소유자 범위·재시드 누락·늦은 추적 연결을 상태를 바꾸기 전에 거부한다.
    /// </summary>
    [Fact]
    public void SpStoreContracts()
    {
        var rng = new NetstormRandom(123);
        var missing = new RawSpStore(rng, null);
        Assert.False(missing.Initialized);
        Assert.Throws<InvalidOperationException>(() => missing.Get(0));
        Assert.Throws<InvalidOperationException>(() => missing.Initialize());
        var store = new RawSpStore(rng, () => 0x12345678u, 0xcdcdcdcd);
        Assert.Throws<ArgumentOutOfRangeException>(() => store.Get(9));
        Assert.Throws<ArgumentOutOfRangeException>(() => store.Set(9, 1));
        Assert.False(store.Initialized);
        int tracking = 0;
        store.BindTrackingOffset(value => tracking = value);
        Assert.Throws<InvalidOperationException>(() => store.BindTrackingOffset(value => tracking = value + 1));
        store.Initialize();
        Assert.Throws<InvalidOperationException>(() => store.BindTrackingOffset(value => tracking = value));
        Assert.True(store.Offset < 100);
        Assert.Equal(unchecked((int)store.Offset), tracking);
    }

    /// <summary>
    /// 소유자마다 다른 값을 쓰고 읽으며, 회전해도 실제 값이 유지된다.
    /// </summary>
    [Fact]
    public void SpStoreRoundtripAndRotateKeepsValues()
    {
        var rng = new NetstormRandom(123);
        var store = new RawSpStore(rng, () => 0x12345678u, 0);
        for (uint owner = 0; owner < RawSpStore.Owners; owner++)
        {
            store.Set(owner, 0x80000001u + owner * 0x01010101u);
        }
        for (uint owner = 0; owner < RawSpStore.Owners; owner++)
        {
            Assert.Equal(0x80000001u + owner * 0x01010101u, store.Get(owner));
        }
        store.Set(4, 0xffffffffu);
        Assert.Equal(0xffffffffu, store.Get(4));
        Assert.Equal(0x80000001u + 3 * 0x01010101u, store.Get(3));
        // 회전은 offset 과 배열을 함께 돌려 값을 보존한다
        for (int i = 0; i < 40; i++)
        {
            store.Rotate();
        }
        for (uint owner = 0; owner < RawSpStore.Owners; owner++)
        {
            uint expected = owner == 4 ? 0xffffffffu : 0x80000001u + owner * 0x01010101u;
            Assert.Equal(expected, store.Get(owner));
        }
        // 9600 경계를 넘으면 offset 이 되돌아간다
        Assert.True(store.Offset < 9600);
    }
}
