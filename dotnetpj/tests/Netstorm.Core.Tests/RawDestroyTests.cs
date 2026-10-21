using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 공통 삭제 본체(<see cref="RawSquidDestroy"/>)의 경계·보호 경로를 합성 입력으로 확인한다.
/// 원본 재생(X86RawDestroyTests)이 다루지 않는 죽음 재진입·완료 누락·권한·종속 사슬 오류가 대상이며,
/// 원본 동작은 cpppj/tests/RawDestroyTests.cpp 의 경계 검사를 따른다 (W03 삭제·수명 첫 조각, 2026-10-21).
/// </summary>
public sealed class RawDestroyTests
{
    /// <summary>삭제 뿌리·종속의 기본 타입 번호(자산).</summary>
    private const int RootType = 74;

    /// <summary>풀·해시·점유 지도·Unpop·삭제 본체를 묶어 돌려준다.</summary>
    /// <param name="server">서버 풀인지.</param>
    private static (RawSidPool Pool, RawSquidHash Hash, byte[] Spots, OriginalTypeRecord[] Types, RawUnpop Unpop, RawSquidDestroy Destroy) Setup(bool server = true)
    {
        var pool = new RawSidPool(32768, server);
        var hash = new RawSquidHash();
        var spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 타입 번호 전체에 빈 타입 레코드를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        var unpop = new RawUnpop(pool, hash, spots);
        return (pool, hash, spots, types, unpop, new RawSquidDestroy(pool, unpop, types));
    }

    /// <summary>클라이언트 슬롯 7개를 확보하고 번호 목록을 돌려준다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    private static int[] AllocateIds(RawSidPool pool)
    {
        var ids = new int[7];
        // 원본처럼 5번부터 순서대로 확보한다
        for (int i = 0; i < ids.Length; i++)
        {
            ids[i] = pool.Allocate(SidAllocation.Client);
        }
        return ids;
    }

    /// <summary>슬롯을 뿌리 기본값(가상 표·타입·상태·좌표)으로 채운다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="sid">채울 번호.</param>
    /// <param name="type">타입 번호.</param>
    /// <param name="state">상태 바이트.</param>
    private static void Fill(RawSidPool pool, int sid, int type, int state)
    {
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Clear();
        Put(raw, 0, OriginalConstructors.BaseVtable);
        Put(raw, 4, 0, 2);
        raw[10] = (byte)type;
        raw[11] = (byte)state;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>해시 체인에 뿌리를 걸고 바깥 효과를 원본 시험처럼 처리하는 콜백 묶음을 만든다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="hash">공간 해시.</param>
    /// <param name="destroy">삭제 본체.</param>
    /// <param name="selected">고른 번호 상자.</param>
    private static SquidDestroyHooks Completing(RawSidPool pool, RawSquidHash hash, RawSquidDestroy destroy, int selected)
    {
        return new SquidDestroyHooks
        {
            Emit = effect =>
            {
                switch (effect.Effect)
                {
                    case SquidDestroyEffect.PreDestroy:
                        destroy.CompletePreDestroy();
                        break;
                    case SquidDestroyEffect.PostDestroy:
                        destroy.CompletePostDestroy();
                        break;
                    case SquidDestroyEffect.ReleaseDependent:
                        Put(pool.AllocatedBytes(effect.Sid), 4, 0, 2);
                        break;
                    case SquidDestroyEffect.DestroyDependent:
                        pool.AllocatedBytes(effect.Sid)[11] |= 2;
                        break;
                    default:
                        break;
                }
            },
            Selected = () => selected,
        };
    }

    /// <summary>
    /// 죽은 뿌리의 삭제는 바깥 효과 없이 그대로 돌아오고, 같은 빈 콜백 묶음의 반복 호출도 풀을 바꾸지 않는다.
    /// </summary>
    [Fact]
    public void DeadRootReturnsWithoutEffectsAndRepeatedReleaseChangesNothing()
    {
        var (pool, hash, _, _, unpop, _) = Setup();
        int[] ids = AllocateIds(pool);
        Fill(pool, ids[0], RootType, 0);
        ref ushort head = ref hash.Bucket(0, 20.75f, 21.9f);
        head = (ushort)ids[0];
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 타입 번호 전체에 빈 타입 레코드를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        var destroy = new RawSquidDestroy(pool, unpop, types);
        var hooks = Completing(pool, hash, destroy, 0);
        destroy.Destroy(ids[0], 0, hooks);
        string before = Convert.ToHexStringLower(pool.Bytes.ToArray());
        int selected = 0;
        destroy.Destroy(ids[0], 0, new SquidDestroyHooks { Emit = _ => { }, Selected = () => selected });
        Assert.Equal(before, Convert.ToHexStringLower(pool.Bytes.ToArray()));
        Assert.Equal(0U, destroy.PreDepth);
        Assert.Equal(0U, destroy.PostDepth);
    }

    /// <summary>
    /// Pre·Post 안에서 완료 호출을 빼먹으면 예외로 멈추고, 뒷정리(반납·깊이 복구)를 건너뛴다.
    /// </summary>
    [Fact]
    public void MissingCompletionStopsBeforeFinish()
    {
        var (pool, hash, _, types, unpop, _) = Setup();
        int[] ids = AllocateIds(pool);
        Fill(pool, ids[0], RootType, 0);
        ref ushort head = ref hash.Bucket(0, 20.75f, 21.9f);
        head = (ushort)ids[0];
        var pre = new RawSquidDestroy(pool, unpop, types);
        Assert.ThrowsAny<Exception>(() => pre.Destroy(ids[0], 0, new SquidDestroyHooks { Emit = _ => { }, Selected = () => 0 }));
        Assert.Equal(2, pool.Slot(ids[0])[11]);
        Assert.Equal(1U, pre.PreDepth);
        Assert.Equal(0U, pool.Deletions(true)[0].Sid);
        var (pool2, hash2, _, types2, unpop2, _) = Setup();
        int[] ids2 = AllocateIds(pool2);
        Fill(pool2, ids2[0], RootType, 0);
        ref ushort head2 = ref hash2.Bucket(0, 20.75f, 21.9f);
        head2 = (ushort)ids2[0];
        var post = new RawSquidDestroy(pool2, unpop2, types2);
        Assert.ThrowsAny<Exception>(() => post.Destroy(ids2[0], 0, new SquidDestroyHooks
        {
            Emit = effect =>
            {
                if (effect.Effect == SquidDestroyEffect.PreDestroy)
                {
                    post.CompletePreDestroy();
                }
            },
            Selected = () => 0,
        }));
        Assert.Equal(6, pool2.Slot(ids2[0])[11]);
        Assert.Equal(1U, post.PostDepth);
        Assert.Equal(0U, pool2.Deletions(true)[0].Sid);
    }

    /// <summary>
    /// 서버 권한·빈 슬롯·범위 밖 번호·다른 풀·빈/Form 뿌리는 풀을 바꾸기 전에 거부한다.
    /// </summary>
    [Fact]
    public void AuthorityPoolAndRootValidatedBeforeMutation()
    {
        var (pool, _, _, types, unpop, destroy) = Setup(server: false);
        var factory = new RawSquidFactory(pool, types);
        int serverRoot = RawSidPool.ServerFirst + 9;
        factory.Take(RootType, serverRoot);
        Fill(pool, serverRoot, RootType, 0);
        string before = Convert.ToHexStringLower(pool.Bytes.ToArray());
        Assert.ThrowsAny<Exception>(() => destroy.Destroy(serverRoot, 0, Completing(pool, new RawSquidHash(), destroy, 0)));
        Assert.ThrowsAny<Exception>(() => destroy.Destroy(serverRoot, 8, new SquidDestroyHooks()));
        Assert.ThrowsAny<Exception>(() => destroy.Destroy(0, 8, Completing(pool, new RawSquidHash(), destroy, 0)));
        Assert.ThrowsAny<Exception>(() => destroy.Destroy(32768, 8, Completing(pool, new RawSquidHash(), destroy, 0)));
        Assert.Equal(before, Convert.ToHexStringLower(pool.Bytes.ToArray()));
        var other = new RawSidPool(32768, true);
        var otherHash = new RawSquidHash();
        var otherSpots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
        var otherUnpop = new RawUnpop(other, otherHash, otherSpots);
        Assert.ThrowsAny<Exception>(() => new RawSquidDestroy(other, unpop, types));
        Assert.ThrowsAny<Exception>(() => new RawSquidDestroy(pool, otherUnpop, types));
        int[] ids = AllocateIds(pool);
        Fill(pool, ids[0], RootType, 1);
        Assert.ThrowsAny<Exception>(() => destroy.Destroy(ids[0], 8, Completing(pool, new RawSquidHash(), destroy, 0)));
        Assert.Equal(1, pool.Slot(ids[0])[11]);
        Fill(pool, ids[1], RootType, 8);
        Assert.ThrowsAny<Exception>(() => destroy.Destroy(ids[1], 8, Completing(pool, new RawSquidHash(), destroy, 0)));
        Assert.Equal(8, pool.Slot(ids[1])[11]);
        Fill(pool, ids[2], 20, 0);
        Assert.ThrowsAny<Exception>(() => destroy.Destroy(ids[2], 8, Completing(pool, new RawSquidHash(), destroy, 0)));
        Assert.Equal(0, pool.Slot(ids[2])[11]);
    }

    /// <summary>
    /// Pre 뒤에 종속이 비었거나 자산인데 내용물이 아니거나 순환이면 예외이며 뿌리는 죽음으로 남는다.
    /// </summary>
    [Fact]
    public void InvalidOrCyclicDependentThrowsAfterPre()
    {
        // 갈래 4의 기본 체인(뿌리→6→7)을 세 가지로 망가뜨린다
        foreach (int broken in new[] { 0, 1, 2 })
        {
            var (pool, hash, _, types, unpop, _) = Setup();
            int[] ids = AllocateIds(pool);
            Fill(pool, ids[0], RootType, 0);
            Fill(pool, ids[1], 75, 8);
            Fill(pool, ids[2], 75, 8);
            ref ushort head = ref hash.Bucket(0, 20.75f, 21.9f);
            head = (ushort)ids[0];
            Put(pool.AllocatedBytes(ids[0]), 6, (uint)ids[1], 2);
            Put(pool.AllocatedBytes(ids[1]), 4, (uint)ids[2], 2);
            Put(pool.AllocatedBytes(ids[2]), 4, 0, 2);
            if (broken == 0)
            {
                pool.AllocatedBytes(ids[1])[11] = 1;
            }
            if (broken == 1)
            {
                pool.AllocatedBytes(ids[1])[11] = 0;
            }
            if (broken == 2)
            {
                Put(pool.AllocatedBytes(ids[1]), 4, (uint)ids[1], 2);
            }
            var destroy = new RawSquidDestroy(pool, unpop, types);
            Assert.ThrowsAny<Exception>(() => destroy.Destroy(ids[0], 0, Completing(pool, hash, destroy, 0)));
            Assert.Equal(2, pool.Slot(ids[0])[11]);
            Assert.Equal(0U, pool.Deletions(true)[0].Sid);
        }
    }
}
