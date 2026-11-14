using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// AI가 읽는 실제 사제 목록과 색 표의 원본 입력·전체 출력을 대조한다.
namespace Netstorm.Tests;

/// <summary>사제 조회 3판본 4851입력과 Player 색 변경216입력을 검증한다.</summary>
public sealed class AiWorldLookupTests
{
    /// <summary>원본 쉼표 구분 DWORD 입력만 읽는다.</summary>
    private static uint[] Numbers(string text) => text.Split(',').Select(X86Fixture.UInt).ToArray();

    /// <summary>현재 풀의 앞128슬롯을 원본 판본의 물리 폭으로 직렬화한다.</summary>
    private static byte[] NativeBytes(RawSidPool pool)
    {
        int stride = pool.Edition == OriginalEdition.Patch1078 ? 50 : 36;
        var result = new byte[128 * stride];
        // C#의 공통50바이트 저장과 CD의 실제36바이트 관찰 사이에서 슬롯 경계만 맞춘다.
        for (int sid = 0; sid < 128; sid++) pool.Slot(sid)[..stride].CopyTo(result.AsSpan(sid * stride, stride));
        return result;
    }

    /// <summary>소유자·상태 외 필드를 포함한 원본 입력 슬롯을 채운다.</summary>
    private static void Node(RawSidPool pool, uint[] node)
    {
        Span<byte> raw = pool.RawSlot(checked((int)node[0])); raw.Clear();
        bool patch = pool.Edition == OriginalEdition.Patch1078;
        raw[10] = unchecked((byte)node[1]); raw[11] = unchecked((byte)node[2]);
        raw[patch ? 34 : 32] = unchecked((byte)node[3]); raw[12] = unchecked((byte)node[4]);
        raw[patch ? 40 : 35] = unchecked((byte)node[5]);
    }

    /// <summary>각 PE의 반환DWORD·원시풀/목록Adler와 조회 순수성을 모두 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    [InlineData("original1037", OriginalEdition.Cd1072)]
    public void PriestNative1617(string edition, OriginalEdition version)
    {
        string[][] all = X86Fixture.Read("priestlookup"); Assert.Equal(4851, all.Length);
        string[][] rows = all.Where(row => row[0] == edition).ToArray(); Assert.Equal(1617, rows.Length);
        var pool = new RawSidPool(24000, false, edition: version);
        var shared = new RawPostPopList();
        var state = new PriestPostPopState { SharedPriests = shared };
        var lookup = new RawPriestLookup(pool, state);
        Assert.Same(pool, lookup.Pool());
        X86Fixture.CheckRows(rows, row =>
        {
            // 입력마다 앞128슬롯만 지우고 fixture의 입력 노드를 그대로 만든다.
            for (int sid = 0; sid < 128; sid++) pool.RawSlot(sid).Clear();
            shared.Entries = Numbers(row[3]); shared.Count = X86Fixture.UInt(row[2]);
            // 타입·원래 소유자·extra도 채워 조회가 이 필드들을 읽지 않는지 대조한다.
            foreach (string text in row[4].Split(';')) Node(pool, Numbers(text));
            byte[] before = NativeBytes(pool);
            var list = new byte[16];
            // 개수 밖의 stale 항목을 포함한 물리4DWORD를 직렬화한다.
            for (int index = 0; index < shared.Entries.Length; index++)
                BinaryPrimitives.WriteUInt32LittleEndian(list.AsSpan(index * 4), shared.Entries[index]);
            uint result = lookup.Find(X86Fixture.UInt(row[1]));
            return result == X86Fixture.UInt(row[5])
                && X86Fixture.Adler32(NativeBytes(pool)) == X86Fixture.UInt(row[6])
                && X86Fixture.Adler32(list) == X86Fixture.UInt(row[7])
                && before.AsSpan().SequenceEqual(NativeBytes(pool))
                && shared.Count == X86Fixture.UInt(row[2]) && shared.Entries.SequenceEqual(Numbers(row[3]));
        });
    }

    /// <summary>signed 개수·늦은 잘못된SID·DWORD 소유자·공유 목록 교체를 원문 순서로 검사한다.</summary>
    [Fact]
    public void PriestLazyGuardsAndSharedList()
    {
        var pool = new RawSidPool(24000, false);
        var state = new PriestPostPopState(); var lookup = new RawPriestLookup(pool, state);
        state.Priests.Count = 0xffffffff; Assert.Equal(0u, lookup.Find(3));
        state.Priests.Count = 0x80000000; Assert.Equal(0u, lookup.Find(3));
        state.Priests.Count = 1; Assert.Throws<InvalidOperationException>(() => lookup.Find(3));
        Node(pool, [50, 82, 5, 3, 1, 255]);
        state.SharedPriests = new RawPostPopList { Entries = [50, 0x10032], Count = 2 };
        Assert.Equal(50u, lookup.Find(3));
        Assert.Throws<ArgumentOutOfRangeException>(() => lookup.Find(259));
        state.SharedPriests.Entries = [0x10032]; state.SharedPriests.Count = 1;
        Assert.Throws<ArgumentOutOfRangeException>(() => lookup.Find(3));
        state.SharedPriests.Entries = [24000]; Assert.Throws<ArgumentOutOfRangeException>(() => lookup.Find(3));
        state.SharedPriests.Entries = [50]; Assert.Equal(0u, lookup.Find(259));
        state.SharedPriests.Entries = [0, 0x10032]; state.SharedPriests.Count = 2;
        Node(pool, [0, 1, 0, 3, 0, 0]); Assert.Equal(0u, lookup.Find(3));
        state.SharedPriests.Entries = [50]; state.SharedPriests.Count = 1;
        pool.RawSlot(50)[11] = 2; Assert.Equal(0u, lookup.Find(3));
        pool.RawSlot(50)[11] = 5; pool.RawSlot(50)[34] = 4; Assert.Equal(50u, lookup.Find(4));
    }

    /// <summary>색 표36바이트·전체16슬롯800바이트·호출 순서를 원본216입력과 비교한다.</summary>
    [Fact]
    public void PlayerColorNative216()
    {
        string[][] rows = X86Fixture.Read("player-color").Where(row => row[0] != "input").ToArray(); Assert.Equal(216, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] input = Numbers(row[0]); int layout = (int)input[3];
            var pool = new RawSidPool(24000, false);
            var state = new BridgeConnectState { IslandType = 94, StalagType = 95, Battle = input[2] != 0 };
            var events = new List<string>();
            // 실제 클라이언트 할당 뒤 원본 관찰기의 동일한 모든 입력 바이트를 채운다.
            for (int sid = 5; sid <= 20; sid++)
            {
                Assert.Equal(sid, pool.Allocate(SidAllocation.Client)); Span<byte> raw = pool.RawSlot(sid);
                // 프레임 외 필드 보존을 함께 검사하는 입력 패턴이다.
                for (int index = 0; index < raw.Length; index++) raw[index] = unchecked((byte)(sid * 13 + index * 29 + layout * 17));
                raw[10] = new byte[] { 94, 95, 82, 96 }[(sid + layout) % 4];
                raw[11] = new byte[] { 0, 1, 2, 4, 128, 34 }[(sid + layout) % 6];
                raw[34] = (byte)((sid * 3 + layout) % 9);
            }
            RawPlayerColor.ChangePlayerColor(pool, state, input[0], input[1], (sid, frame, flags) =>
            {
                events.Add($"{sid},{frame},{flags}");
                BinaryPrimitives.WriteInt32LittleEndian(pool.RawSlot(sid)[36..], frame);
            });
            var colors = new byte[36];
            // 원본 signed 색 표9DWORD의 비트를 그대로 직렬화한다.
            for (int owner = 0; owner < 9; owner++) BinaryPrimitives.WriteInt32LittleEndian(colors.AsSpan(owner * 4), state.OwnerColors[owner]);
            return colors.AsSpan().SequenceEqual(Convert.FromHexString(row[1]))
                && pool.Bytes.Slice(250, 800).SequenceEqual(Convert.FromHexString(row[2]))
                && (events.Count == 0 ? "-" : string.Join(';', events)) == row[3];
        });
    }

    /// <summary>선택 소비자의 실패 전 색 표 쓰기와 콜백 뒤 현재 타입·소유자 재조회를 확인한다.</summary>
    [Fact]
    public void PlayerColorCurrentFieldsAndFailureOrder()
    {
        var pool = new RawSidPool(24000, false);
        var state = new BridgeConnectState { IslandType = 94, StalagType = 95, Battle = true };
        Node(pool, [5, 94, 0, 2, 0, 0]);
        Assert.Throws<InvalidOperationException>(() => RawPlayerColor.ChangePlayerColor(pool, state, 2, 8, null));
        Assert.Equal(8, state.OwnerColors[2]);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawPlayerColor.ChangePlayerColor(pool, state, 9, 1, null));
        Node(pool, [6, 95, 2, 3, 0, 0]); var events = new List<(int, int)>();
        RawPlayerColor.ChangePlayerColor(pool, state, 2, 4, (sid, frame, _) =>
        {
            events.Add((sid, frame));
            if (sid == 5) { state.OwnerColors[3] = 7; pool.RawSlot(6)[34] = 3; }
        });
        Assert.Equal(new[] { (5, 3), (6, 6) }, events);
    }
}
