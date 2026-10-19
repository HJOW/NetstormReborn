using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 공간 등록(<see cref="RawPop"/>)을 원본 x86 기대값에 대조한다.
/// 기대값은 pop-x86.tsv(원본 PE의 실제 생성·Pop/Unpop/반납 4,600회 중 10.78 몫 2,456회)와
/// pop-vtables.tsv(파생 firstPop/postPop 지원 주소 중 10.78 83행)다.
/// 각 호출 뒤 반환 번호·풀 전체 바이트와 네 단계 공간 해시와 65,536칸 점유 지도의 Adler-32,
/// 대상 슬롯 50바이트 전부를 비교한다. 열 구성은 cpppj/tests/PopTests.cpp 와 같다.
/// CD 10.72 행은 기준 판본이 아니므로 읽지 않는다 (W02 Pop 조각, 2026-10-20).
/// </summary>
public sealed class X86RawPopTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>마나 대체 주소 3개. 표의 지원 비트가 0 이어도 이 주소들은 Pop 호출을 지원한다.</summary>
    private static readonly uint[] ManaVtables = [0x0050C3B0, 0x00509810, 0x005070C0];

    /// <summary>타입 레코드 188개를 생성자 주소·최소 발자국(1×1)·원본 합성 입력의 HP·zOrder 로 채워 만든다.</summary>
    private static OriginalTypeRecord[] Types()
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 모든 타입 번호의 실제 생성자 주소를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord
            {
                ConstructorAddress = OriginalConstructors.Address(i),
                FootX = 1,
                FootY = 1,
                MaxHitPoints = 91,
                ZOrder = 7,
            };
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

    /// <summary>네 단계 공간 해시의 모든 머리를 큰 배열부터 16비트 리틀 엔디언으로 이어 붙인다.</summary>
    /// <param name="hash">대상 해시.</param>
    private static byte[] HashBytes(RawSquidHash hash)
    {
        var bytes = new List<byte>();
        // 0 단계부터 3 단계까지
        foreach (int level in Enumerable.Range(0, RawSquidHash.Levels))
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
    /// 생성·Pop·Unpop·반납 fixture 의 10.78 몫을 재생한다. Claim/Fill/Seed/Clear/Type 은 합성 입력이며 호출 수에 넣지 않는다.
    /// 호출마다 열 5개(반환값·풀·해시·점유 지도 체크섬)와 대상 슬롯 50바이트를 대조한다.
    /// </summary>
    [Fact]
    public void PopUnpopCreateAndReleaseMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("pop");
        RawSidPool? pool = null;
        RawSquidHash? hash = null;
        RawPop? pop = null;
        RawUnpop? unpop = null;
        RawSquidFactory? factory = null;
        OriginalTypeRecord[] types = [];
        byte[] spots = [];
        float frameWidth = 1;
        float frameHeight = 1;
        bool patch = false;
        int begins = 0;
        var calls = new Dictionary<string, int>();
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] == "Begin")
            {
                Assert.Equal(4, row.Length);
                patch = row[1] == PatchEdition;
                if (patch)
                {
                    pool = new RawSidPool(X86Fixture.Int(row[2]), false);
                    hash = new RawSquidHash();
                    spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
                    types = Types();
                    unpop = new RawUnpop(pool, hash, spots);
                    pop = new RawPop(pool, hash, spots);
                    factory = new RawSquidFactory(pool, types);
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
                    Assert.Equal(8, row.Length);
                    OriginalTypeRecord type = types[X86Fixture.Int(row[1])];
                    type.Flags1 = X86Fixture.UInt(row[2]);
                    type.Flags2 = X86Fixture.UInt(row[3]);
                    type.FootX = X86Fixture.Int(row[4]);
                    type.FootY = X86Fixture.Int(row[5]);
                    frameWidth = X86Fixture.FloatBits(row[6]);
                    frameHeight = X86Fixture.FloatBits(row[7]);
                    return true;
                }
                case "Claim":
                    Assert.Equal(2, row.Length);
                    factory!.Take(74, X86Fixture.Int(row[1]));
                    return true;
                case "Fill":
                {
                    Assert.Equal(7, row.Length);
                    Span<byte> bytes = pool!.AllocatedBytes(X86Fixture.Int(row[1]));
                    int seed = X86Fixture.Int(row[2]);
                    // 오염된 입력 payload 를 놓고 원본 합성 입력이 지정한 필드만 덮는다
                    for (int i = 0; i < bytes.Length; i++)
                    {
                        bytes[i] = (byte)(seed + i * 37);
                    }
                    bytes[10] = (byte)X86Fixture.UInt(row[4]);
                    bytes[11] = (byte)X86Fixture.UInt(row[3]);
                    Put(bytes, 0, X86Fixture.UInt(row[5]));
                    Put(bytes, 4, 0, 2);
                    Put(bytes, 8, 23, 2);
                    Put(bytes, 14, BitConverter.SingleToUInt32Bits(20f));
                    Put(bytes, 18, BitConverter.SingleToUInt32Bits(20f));
                    bytes[40] = (byte)X86Fixture.UInt(row[6]);
                    bytes[33] = 3;
                    Put(bytes, 36, 0, 4);
                    return true;
                }
                case "Seed":
                    Assert.Equal(4, row.Length);
                    spots[X86Fixture.Int(row[2]) * RawSquidHash.WorldCells + X86Fixture.Int(row[1])] = (byte)X86Fixture.UInt(row[3]);
                    return true;
                case "Clear":
                    Assert.Single(row);
                    hash!.Reset();
                    Array.Fill(spots, (byte)0);
                    return true;
            }
            Assert.Equal(11, row.Length);
            int sid = X86Fixture.Int(row[2]);
            uint arg = X86Fixture.UInt(row[3]);
            uint result = 0;
            switch (row[1])
            {
                case "Create":
                    result = (uint)factory!.Create(arg, SidAllocation.Client);
                    break;
                case "Release":
                    pool!.Release(sid);
                    break;
                case "Unpop":
                    unpop!.Unpop(sid, types[pool!.Slot(sid)[RawSidPool.TypeOffset]], arg);
                    break;
                case "Pop":
                    result = (uint)pop!.Pop(sid, types[pool!.Slot(sid)[RawSidPool.TypeOffset]],
                        frameWidth, frameHeight, X86Fixture.FloatBits(row[4]), X86Fixture.FloatBits(row[5]), arg);
                    break;
                default:
                    return false;
            }
            calls[row[1]] = calls.GetValueOrDefault(row[1]) + 1;
            uint[] actual =
            [
                result, X86Fixture.Adler32(pool!.Bytes), X86Fixture.Adler32(HashBytes(hash!)), X86Fixture.Adler32(spots),
            ];
            // 원본 전체 공간 배열과 풀·해시 결과를 열마다 대조한다
            for (int i = 0; i < actual.Length; i++)
            {
                if (actual[i] != X86Fixture.UInt(row[i + 6]))
                {
                    return false;
                }
            }
            // 대상 자신의 next·좌표·단계·화면 좌표·섬 번호를 포함해 모든 바이트를 직접 비교한다
            return Convert.ToHexStringLower(pool.Slot(sid)) == row[10];
        });
        Assert.Equal(4, begins);
        Assert.Equal(
            new Dictionary<string, int> { ["Create"] = 376, ["Pop"] = 1016, ["Release"] = 376, ["Unpop"] = 688 }
                .OrderBy(pair => pair.Key).ToArray(),
            calls.OrderBy(pair => pair.Key).ToArray());
    }

    /// <summary>
    /// 파생 지원 주소 26개가 원본 PE 의 가상 표별 지원 비트와 같다.
    /// 표에 있으면 공통 표시 지원을 따르고, 마나 대체 주소 3개는 표 밖에서도 지원한다.
    /// </summary>
    [Fact]
    public void DerivedSupportTableMatchesOriginalVtables()
    {
        string[][] rows = X86Fixture.ReadFile("pop-vtables.tsv").Where(row => row[0] == PatchEdition).ToArray();
        Assert.Equal(83, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(5, row.Length);
            uint vtable = X86Fixture.UInt(row[1]);
            bool supported = row[4] == "1";
            bool mana = Array.IndexOf(ManaVtables, vtable) >= 0;
            return OriginalPopDisplay.Supports(vtable, 0) == (supported || mana)
                && OriginalPopDisplay.Supports(vtable, OriginalUnpopDisplay.VortexFlag) == (supported || mana);
        });
        Assert.False(OriginalPopDisplay.Supports(0, 0));
        Assert.False(OriginalPopDisplay.Supports(0x12345678, OriginalUnpopDisplay.VortexFlag));
    }
}
