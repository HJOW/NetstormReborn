using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 공간 해제(<see cref="RawUnpop"/>)와 그것을 쓰는 Take·반납·최초 Pop 플래그를 원본 x86 기대값에 대조한다.
/// 기대값은 unpop-x86.tsv(원본 PE의 SID·실제 생성자·Unpop·firstPop 을 대체 함수 없이 격리 실행한 3,618회 중 10.78 몫 1,809회)와
/// unpop-vtables.tsv(공통 표시 가상 표 주소별 지원 비트 중 10.78 83행)다.
/// 각 호출 뒤 반환 번호·풀 카운터·목록 머리/꼬리·풀 전체 바이트와 삭제 기록과 네 단계 공간 해시와 65,536 칸 점유 지도의 Adler-32,
/// 대상 슬롯 50바이트 전부를 비교한다. 열 구성은 cpppj/tests/UnpopTests.cpp 와 같다.
/// 입력은 실제 Pop 이 아니라 합성이다(Type/Fill/Head/Space/Next 행). CD 10.72 행은 기준 판본이 아니므로 읽지 않는다 (W02 Unpop 조각, 2026-10-16).
/// </summary>
public sealed class X86RawUnpopTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 의 소용돌이 경로(update8c)를 쓰는 가상 표 후보 주소 4개. 표의 지원 비트가 1 이어도 이 주소들은 소용돌이 호출을 지원한다.</summary>
    private static readonly uint[] VortexAlternates = [0x4DCDA0, 0x4DCF80, 0x4DD150, 0x4DBCD0];

    /// <summary>타입 레코드 188개를 생성자 주소 표와 최소 발자국(1×1)으로 채워 만든다.</summary>
    private static OriginalTypeRecord[] Types()
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 모든 타입 번호의 실제 생성자 주소를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i), FootX = 1, FootY = 1 };
        }
        return types;
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 폭 밖의 이웃 필드는 보존한다.</summary>
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

    /// <summary>삭제 기록(클라이언트·서버 순서, 기록마다 번호·타입 각 4바이트)을 패딩 없이 이어 붙인다.</summary>
    /// <param name="pool">대상 풀.</param>
    private static byte[] DeletionBytes(RawSidPool pool)
    {
        var bytes = new List<byte>();
        // 원본 클라이언트·서버 순서로 각각 20개 기록을 읽는다
        foreach (bool client in new[] { true, false })
        {
            // 기록마다 번호와 타입의 32비트 값을 낮은 바이트부터 놓는다
            foreach (SidDeletion record in pool.Deletions(client))
            {
                // 각 값의 네 바이트를 직접 기록한다
                foreach (uint value in new[] { record.Sid, record.Type })
                {
                    // 하위 바이트부터 네 바이트를 꺼내 bytes 에 기록한다
                    for (int shift = 0; shift < 32; shift += 8)
                    {
                        bytes.Add((byte)(value >> shift));
                    }
                }
            }
        }
        return bytes.ToArray();
    }

    /// <summary>네 단계 공간 해시의 모든 머리를 큰 배열부터 16비트 리틀 엔디언으로 이어 붙인다.</summary>
    /// <param name="hash">대상 해시.</param>
    private static byte[] HashBytes(RawSquidHash hash)
    {
        var bytes = new List<byte>();
        // 0 단계부터 3 단계까지
        for (int level = 0; level < RawSquidHash.Levels; level++)
        {
            // 머리마다 두 바이트
            foreach (ushort value in hash.Entries(level))
            {
                bytes.Add((byte)value);
                bytes.Add((byte)(value >> 8));
            }
        }
        return bytes.ToArray();
    }

    /// <summary>
    /// 공간 해제·Take·반납·최초 Pop 플래그 fixture 의 10.78 몫을 재생한다. Type/Fill/Head/Space/Next 는 합성 입력이며 호출 수에 넣지 않는다.
    /// 호출마다 열 11개(반환값·풀 카운터·목록 머리/꼬리·풀/삭제 기록/해시/점유 지도 체크섬)와 대상 슬롯 50바이트를 대조한다.
    /// </summary>
    [Fact]
    public void UnpopTakeReleaseAndFirstPopMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("unpop");
        RawSidPool? pool = null;
        RawSquidHash? hash = null;
        RawUnpop? unpop = null;
        RawSquidFactory? factory = null;
        OriginalTypeRecord[] types = [];
        byte[] spots = [];
        bool patch = false;
        int begins = 0;
        var calls = new Dictionary<string, int>();
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] == "Begin")
            {
                Assert.Equal(5, row.Length);
                patch = row[1] == PatchEdition;
                if (patch)
                {
                    pool = new RawSidPool(X86Fixture.Int(row[2]), row[3] == "1");
                    hash = new RawSquidHash();
                    spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
                    types = Types();
                    unpop = new RawUnpop(pool, hash, spots);
                    factory = new RawSquidFactory(pool, types, false, unpop);
                    begins++;
                }
                return true;
            }
            if (!patch)
            {
                return true;
            }
            switch (row[0])
            {
                case "Type":
                {
                    Assert.Equal(9, row.Length);
                    OriginalTypeRecord type = types[X86Fixture.Int(row[1])];
                    type.Flags1 = X86Fixture.UInt(row[2]);
                    type.Flags2 = X86Fixture.UInt(row[3]);
                    type.MaxHitPoints = X86Fixture.Int(row[4]);
                    type.ZOrder = X86Fixture.Int(row[5]);
                    type.ConstructorAddress = X86Fixture.UInt(row[6]);
                    type.FootX = X86Fixture.Int(row[7]);
                    type.FootY = X86Fixture.Int(row[8]);
                    return true;
                }
                case "Space":
                    hash!.Reset();
                    Array.Fill(spots, (byte)X86Fixture.UInt(row[1]));
                    return true;
                case "Head":
                    Assert.Equal(5, row.Length);
                    hash!.Bucket(X86Fixture.Int(row[1]), X86Fixture.FloatBits(row[2]), X86Fixture.FloatBits(row[3])) = (ushort)X86Fixture.UInt(row[4]);
                    return true;
                case "Fill":
                {
                    Assert.Equal(11, row.Length);
                    Span<byte> bytes = pool!.AllocatedBytes(X86Fixture.Int(row[1]));
                    int seed = X86Fixture.Int(row[2]);
                    // 오염된 입력 payload 를 놓고 원본 합성 입력이 지정한 필드만 덮는다
                    for (int i = 0; i < bytes.Length; i++)
                    {
                        bytes[i] = (byte)(seed + i * 37);
                    }
                    bytes[11] = (byte)X86Fixture.UInt(row[3]);
                    bytes[10] = (byte)X86Fixture.UInt(row[4]);
                    Put(bytes, 0, X86Fixture.UInt(row[5]));
                    Put(bytes, 4, X86Fixture.UInt(row[6]), 2);
                    Put(bytes, 14, X86Fixture.UInt(row[7]));
                    Put(bytes, 18, X86Fixture.UInt(row[8]));
                    bytes[40] = (byte)X86Fixture.UInt(row[9]);
                    bytes[33] = (byte)X86Fixture.UInt(row[10]);
                    Put(bytes, 36, 0);
                    return true;
                }
                case "Next":
                    Assert.Equal(3, row.Length);
                    Put(pool!.AllocatedBytes(X86Fixture.Int(row[1])), 4, X86Fixture.UInt(row[2]), 2);
                    return true;
            }
            Assert.Equal(17, row.Length);
            int type2 = X86Fixture.Int(row[2]);
            int arg = X86Fixture.Int(row[3]);
            int sid = X86Fixture.Int(row[4]);
            int result = sid;
            switch (row[1])
            {
                case "Create":
                    result = factory!.Create((uint)type2, (SidAllocation)arg);
                    break;
                case "Take":
                    result = factory!.Take((uint)type2, arg);
                    break;
                case "Unpop":
                    unpop!.Unpop(sid, types[type2], (uint)arg);
                    break;
                case "FirstPop":
                    result = (int)factory!.FirstPopFlags(sid);
                    break;
                case "Release":
                    pool!.Release(sid);
                    break;
                default:
                    return false;
            }
            calls[row[1]] = calls.GetValueOrDefault(row[1]) + 1;
            uint[] actual =
            [
                (uint)result, pool!.FreeCount, pool.PredictableCursor, (uint)pool.FirstFree(true), (uint)pool.Tail(true),
                (uint)pool.FirstFree(false), (uint)pool.Tail(false), X86Fixture.Adler32(pool.Bytes),
                X86Fixture.Adler32(DeletionBytes(pool)), X86Fixture.Adler32(HashBytes(hash!)), X86Fixture.Adler32(spots),
            ];
            // 원본 전체 공간 배열과 풀·삭제 기록 결과를 열마다 대조한다
            for (int i = 0; i < actual.Length; i++)
            {
                if (actual[i] != X86Fixture.UInt(row[i + 5]))
                {
                    return false;
                }
            }
            // 대상 자신의 next·좌표·단계·HP·섬 번호를 포함해 모든 바이트를 직접 비교한다
            return Convert.ToHexStringLower(pool.Slot(sid)) == row[16];
        });
        Assert.Equal(9, begins);
        Assert.Equal(
            new Dictionary<string, int> { ["Create"] = 17, ["FirstPop"] = 448, ["Release"] = 192, ["Take"] = 384, ["Unpop"] = 768 }
                .OrderBy(pair => pair.Key).ToArray(),
            calls.OrderBy(pair => pair.Key).ToArray());
    }

    /// <summary>
    /// 표시 지원 표 83개가 원본 PE 의 가상 표별 지원 비트와 같다. 일반 호출은 지원 비트 1, 소용돌이 호출은 지원 비트 2 또는
    /// 소용돌이 후보 주소여야 하고, 표에 없는 값은 지원하지 않는다.
    /// </summary>
    [Fact]
    public void DisplaySupportTableMatchesOriginalVtables()
    {
        string[][] rows = X86Fixture.ReadFile("unpop-vtables.tsv").Where(row => row[0] == PatchEdition).ToArray();
        Assert.Equal(OriginalUnpopDisplay.Count, rows.Length);
        Assert.Equal(83, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(6, row.Length);
            uint vtable = X86Fixture.UInt(row[1]);
            uint mask = X86Fixture.UInt(row[5]);
            uint alternate = X86Fixture.UInt(row[3]);
            bool vortex = Array.IndexOf(VortexAlternates, alternate) >= 0;
            return OriginalUnpopDisplay.Supports(vtable, 0) == ((mask & 1) != 0)
                && OriginalUnpopDisplay.Supports(vtable, OriginalUnpopDisplay.VortexFlag) == (((mask & 2) != 0) || vortex);
        });
        Assert.False(OriginalUnpopDisplay.Supports(0, 0));
        Assert.False(OriginalUnpopDisplay.Supports(0x12345678, OriginalUnpopDisplay.VortexFlag));
    }
}
