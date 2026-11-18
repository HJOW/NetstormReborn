using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 원본 후보 권한/방향 관계의 독립 원본 관찰을 대조한다.
public sealed class CanonPlacementPermissionTests
{
    /// <summary>판본별 원본 관찰 행 수다. 2026-11-18 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다.</summary>
    private const int PatchRows = 5850;

    /// <summary>CD판 원본 관찰 행 수다. 2026-11-18 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다.</summary>
    private const int CdRows = 5520;

    /// <summary>통합 장면에 쓰는 합성 후보 타입 번호다. 2026-11-18 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다.</summary>
    private const uint Own = 82u;

    /// <summary>통합 장면에 쓰는 합성 상대 타입 번호다. 2026-11-18 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다.</summary>
    private const uint Other = 83u;

    /// <summary>원본이 기록한 기대 행을 읽는다. C++ 판단을 다시 계산하지 않는다.</summary>
    /// <returns>판본 이름이 첫 칸인 행 목록이다.</returns>
    private static string[][] Fixture() => X86Fixture.Read("canonpermission");

    /// <summary>원본 little endian 바이트를 DWORD로 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 범위다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    private static uint Get(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 낮은 바이트부터 네 바이트만 합치는 반복문이다.
        for (int i = 0; i < 4; ++i)
        {
            value |= (uint)raw[offset + i] << (8 * i);
        }

        return value;
    }

    /// <summary>원본 DWORD를 기록 끝에 덧붙인다.</summary>
    /// <param name="trace">쓸 기록이다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(List<byte> trace, uint value)
    {
        // 낮은 바이트부터 네 개를 잇는 반복문이다.
        for (int i = 0; i < 4; ++i)
        {
            trace.Add((byte)(value >> (i * 8)));
        }
    }

    /// <summary>원본 little endian 슬롯 필드에 DWORD를 쓴다.</summary>
    /// <param name="raw">쓸 슬롯 범위다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트만 쓰는 반복문이다.
        for (int i = 0; i < 4; ++i)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>슬롯 전체를 원본 관측의 소문자 16진수로 표현한다.</summary>
    /// <param name="bytes">바꿀 바이트다.</param>
    /// <returns>16진수 문자열이다.</returns>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>같은 십진수 문자열을 원본 DWORD로 복원한다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    private static uint Number(string value) => uint.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>원본 후보 권한/방향 관계 helper의 전체 정상 반환을 대조한다.</summary>
    /// <param name="name">판본 이름이다. originals·originalCD·original1037 중 하나다.</param>
    private static void Replay(string name)
    {
        OriginalEdition edition = name == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        bool patch = edition == OriginalEdition.Patch1078;
        var pool = new RawSidPool(24000, false, null, edition);
        var state = new CanonPlacementPermissionState();
        uint calls = 0;
        uint argOwner = 0, argType = 0, argX = 0, argY = 0;
        string[][] rows = Fixture();
        int count = 0;
        // 이전 입력의 전역·콜백 결과·raw 바이트가 다음 입력에 남지 않게 초기화한다.
        foreach (string[] row in rows)
        {
            if (row[0] != name)
            {
                continue;
            }

            Assert.Equal(25, row.Length);
            state.GraphReady = Number(row[2]);
            state.Editor = Number(row[3]);
            state.UseAlliances = Number(row[4]);
            state.AllowOtherOwners = Number(row[5]);
            Array.Clear(state.Alliances);
            uint current = Number(row[6]), owner = Number(row[7]);
            uint forward = current * 9u + owner, reverse = owner * 9u + current;
            if (forward < state.Alliances.Length)
            {
                state.Alliances[(int)forward] = Number(row[8]);
            }

            if (reverse < state.Alliances.Length && reverse != forward)
            {
                state.Alliances[(int)reverse] = Number(row[9]);
            }

            uint sidValue = Number(row[11]);
            int sid = sidValue < 128 ? (int)sidValue : 50;
            Span<byte> raw = pool.RawSlot(sid);
            raw.Fill(0xcd);
            raw[patch ? 34 : 32] = (byte)current;
            raw[RawSidPool.TypeOffset] = (byte)Number(row[12]);
            Put(raw, 14, Number(row[13]));
            Put(raw, 18, Number(row[14]));
            calls = 0;
            argOwner = argType = argX = argY = 0;
            var permission = new RawCanonPlacementPermission(pool, state, (player, type, x, y) =>
            {
                ++calls;
                argOwner = player;
                argType = type;
                argX = BitConverter.SingleToUInt32Bits(x);
                argY = BitConverter.SingleToUInt32Bits(y);
                if (Number(row[15]) != 0)
                {
                    Span<byte> inner = pool.RawSlot(sid);
                    inner[patch ? 34 : 32] = 3;
                    inner[10] = 84;
                    Put(inner, 14, 0x41f00000);
                    Put(inner, 18, 0x41f80000);
                    state.Editor ^= 1u;
                }

                return Number(row[10]);
            });
            uint result = row[1] == "R"
                ? (permission.Related(current, owner) ? 1u : 0u)
                : permission.Candidate((int)sidValue, owner);
            var relationBytes = new List<byte>();
            // 기계어 실행기의 little endian 관계 표 checksum과 같은 바이트 순서로 직렬화한다.
            for (int i = 0; i < state.Alliances.Length; ++i)
            {
                Put(relationBytes, state.Alliances[i]);
            }

            bool same = result == Number(row[16]) && calls == Number(row[17])
                && argOwner == Number(row[18]) && argType == Number(row[19])
                && argX == Number(row[20]) && argY == Number(row[21])
                && Hex(pool.Slot(sid).Slice(0, patch ? 50 : 36)) == row[22] && state.Editor == Number(row[23])
                && X86Fixture.Adler32(relationBytes.ToArray()) == Number(row[24]);
            Assert.True(same, $"캐논 권한 {name} 행 {count}({row[1]}) 불일치");
            ++count;
        }

        Assert.Equal(patch ? PatchRows : CdRows, count);
    }

    /// <summary>패치 후보와 방향 관계 helper의 전체 정상 반환을 대조한다.</summary>
    [Fact]
    public void PatchX86()
    {
        Replay("originals");
    }

    /// <summary>CD 후보 helper의 BYTE 소유자·좌표와 DWORD 반환을 대조한다.</summary>
    [Fact]
    public void CdX86()
    {
        Replay("originalCD");
    }

    /// <summary>추가 판본의 독립 원본 관찰을 대조한다.</summary>
    [Fact]
    public void ThirdX86()
    {
        Replay("original1037");
    }

    /// <summary>원본 밖의 관계 주소·누락 locator·다른 풀 SID 풀을 명확한 오류로 진단한다.</summary>
    [Fact]
    public void RequiredBoundaryAndRelationGuard()
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var other = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var state = new CanonPlacementPermissionState();
        Assert.Throws<ArgumentNullException>(() => new RawCanonPlacementPermission(pool, state, null!));
        var permission = new RawCanonPlacementPermission(pool, state, (_, _, _, _) => { Assert.Fail("호출하지 않아야 한다"); return 0u; });
        Assert.Throws<ArgumentException>(() => RawCanonPlacementPermission.MakeCanonPermissionTerrainHooks(other, permission, new CanonPlacementTerrainHooks()));
        int sid = pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[34] = 255;
        state.GraphReady = state.UseAlliances = state.AllowOtherOwners = 1;
        Assert.Throws<ArgumentOutOfRangeException>(() => permission.Candidate(sid, 128));
        Assert.Throws<ArgumentOutOfRangeException>(() => permission.Related(255, 128));
        state.Editor = 1;
        Assert.True(permission.Related(255, 128));
    }

    /// <summary>원본 요청 DWORD를 유지하고 후보 helper에는 부호 있는 소유자를 전달한다.</summary>
    [Fact]
    public void SignedOwnerAndCurrentRead()
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        int sid = pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[34] = 1;
        raw[RawSidPool.TypeOffset] = (byte)Own;
        Put(raw, 14, 0x80000000u);
        Put(raw, 18, 0x7fc12345u);
        var state = new CanonPlacementPermissionState();
        state.GraphReady = 1;
        state.UseAlliances = 1;
        state.Alliances[8] = 1;
        uint calls = 0;
        var permission = new RawCanonPlacementPermission(pool, state, (owner, type, x, y) =>
        {
            ++calls;
            Assert.Equal(0xffffffffu, owner);
            Assert.Equal(Own, type);
            Assert.Equal(0x80000000u, BitConverter.SingleToUInt32Bits(x));
            Assert.Equal(0x7fc12345u, BitConverter.SingleToUInt32Bits(y));
            pool.AllocatedBytes(sid)[34] = 2;
            state.UseAlliances = 0;
            return 0x80000000u;
        });
        CanonPlacementTerrainHooks hooks = RawCanonPlacementPermission.MakeCanonPermissionTerrainHooks(pool, permission, new CanonPlacementTerrainHooks());
        var query = new CanonPlacementQuery(Own, 25, 20f, 21f, 0, 0xabff, 7);
        Assert.True(hooks.CandidatePermission!(query, sid));
        Assert.Equal(0xabffu, query.Owner);
        Assert.Equal(1u, calls);
        Assert.False(hooks.CandidatePermission(query, sid));
        Assert.Equal(1u, calls);
        state.GraphReady = 0;
        Assert.Equal(0u, permission.Candidate(65535, 1));
        state.GraphReady = 1;
        Assert.Equal(1u, permission.Candidate(65535, 0));
        Assert.Equal(1u, calls);
    }
}
