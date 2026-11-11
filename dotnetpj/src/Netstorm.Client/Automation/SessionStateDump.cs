using System.Globalization;
using System.Text;

namespace Netstorm.Client.Automation;

/// <summary>덤프에 적는 Kernel 프로세스 한 개의 관측이다.
/// 근거: cpppj/src/client/SessionStateDump.h의 SessionDumpProcess. (2026-11-10 L2·L3 장치에서 추가했다)</summary>
/// <param name="Pid">원본 Kernel 슬롯 번호(1부터).</param>
/// <param name="Form">프로세스 Form SID. SquidProcess가 아니면 0이다.</param>
/// <param name="Parent">부모 SID. SquidProcess가 아니면 0이다.</param>
/// <param name="Regular">RegularProcess이면 뒤의 사건 칸이 유효하다.</param>
/// <param name="Event">사건 번호.</param>
/// <param name="Count">반복 수.</param>
/// <param name="PayloadBits">payload float 비트.</param>
/// <param name="TimeBits">다음 실행 시각 double 비트.</param>
public sealed record SessionDumpProcess(
    uint Pid,
    uint Form,
    uint Parent,
    bool Regular = false,
    uint Event = 0,
    uint Count = 0,
    uint PayloadBits = 0,
    ulong TimeBits = 0);

/// <summary>덤프에 적는 월드 풀 한 개의 관측이다.
/// 근거: cpppj/src/client/SessionStateDump.cpp의 pool 입력(poolBytes·poolCapacity·stride·freeCount·firstFreeServer·firstFreeClient·predictableCursor).
/// (2026-11-12 0.392에서 세션 홀더와 함께 추가했다)</summary>
/// <param name="Capacity">풀 슬롯 수.</param>
/// <param name="Stride">슬롯 바이트 수.</param>
/// <param name="FreeCount">빈 번호 개수.</param>
/// <param name="FirstFreeServer">서버 영역 첫 빈 번호.</param>
/// <param name="FirstFreeClient">클라이언트 영역 첫 빈 번호.</param>
/// <param name="PredictableCursor">다음 예측 번호 커서.</param>
/// <param name="Bytes">풀 전체 원문(FNV-1a 64 해시 대상, free 잔여 포함).</param>
/// <param name="Slots">예약 0~4번과 사용 중 슬롯의 원문 목록(빈 칸 제외).</param>
public sealed record SessionDumpPool(
    int Capacity,
    int Stride,
    uint FreeCount,
    uint FirstFreeServer,
    uint FirstFreeClient,
    uint PredictableCursor,
    byte[] Bytes,
    IReadOnlyList<SessionDumpSlot> Slots);

/// <summary>덤프에 적는 풀 슬롯 한 개의 원문이다.
/// 근거: cpppj/src/client/SessionStateDump.cpp의 slot 행(원문 16진). (2026-11-12 0.392에서 추가했다)</summary>
/// <param name="Sid">슬롯 번호.</param>
/// <param name="Raw">슬롯 원문 바이트.</param>
public sealed record SessionDumpSlot(uint Sid, byte[] Raw);

/// <summary>덤프에 적는 Player 9기록·숨김·동맹 표다.
/// 근거: cpppj/src/client/SessionStateDump.cpp의 player·hidden_players·alliances 행. (2026-11-12 0.393에서 추가했다)</summary>
/// <param name="Records">9기록 원문(기록당 180바이트).</param>
/// <param name="Hidden">Chat 숨김 9바이트.</param>
/// <param name="Alliances">방향별 9×9 동맹 DWORD(행 우선).</param>
public sealed record SessionDumpPlayers(byte[] Records, byte[] Hidden, uint[] Alliances);

/// <summary>덤프에 적는 분산 SP 저장소 관측이다.
/// 근거: cpppj/src/client/SessionStateDump.cpp의 sp 행. (2026-11-12 0.393에서 추가했다)</summary>
/// <param name="Initialized">초기화 여부.</param>
/// <param name="Offset">현재 SP 비트 위치.</param>
/// <param name="Pools">32×9 실제 배열(배열 k의 owner 번째가 k*9+owner).</param>
/// <param name="Permutation">32 순열.</param>
public sealed record SessionDumpSp(bool Initialized, uint Offset, uint[] Pools, short[] Permutation);

/// <summary>덤프에 적는 Graph 표 한 칸이다.
/// 근거: cpppj/src/client/SessionStateDump.cpp의 graph 행. (2026-11-12 0.393에서 추가했다)</summary>
/// <param name="Surfaces">표면 수(signed WORD).</param>
/// <param name="InUse">사용 중(signed WORD).</param>
/// <param name="Reserved">예약(signed WORD).</param>
public sealed record SessionDumpGraphRow(short Surfaces, short InUse, short Reserved);

/// <summary>덤프에 적는 장부다.
/// 근거: cpppj/src/client/SessionStateDump.cpp의 totals 행. (2026-11-12 0.393에서 추가했다)</summary>
/// <param name="TotalCost">총 비용(signed).</param>
/// <param name="PostPopDepth">postPop 깊이.</param>
/// <param name="ProductionDirty">생산 재계산 알림.</param>
public sealed record SessionDumpTotals(int TotalCost, uint PostPopDepth, uint ProductionDirty);

/// <summary>덤프에 적는 TotalMade 세 표다.
/// 근거: cpppj/src/client/SessionStateDump.cpp의 totalmade 행. (2026-11-12 0.393에서 추가했다)</summary>
/// <param name="Local">현재 로컬 수(256칸).</param>
/// <param name="Built">누적 로컬 생산 수(256칸).</param>
/// <param name="Global">현재 전역 수(256칸).</param>
public sealed record SessionDumpTotalMade(uint[] Local, uint[] Built, uint[] Global);

/// <summary>실제 Session/World 상태를 기계가 읽는 TSV로 내보내는 검사 전용 관측이다. 상태를 바꾸지 않는다.
/// 근거: cpppj/src/client/SessionStateDump.h·SessionStateDump.cpp의 FormatSessionStateDump·SessionDumpHash.
/// (2026-11-10 L2·L3 장치에서 추가했다)
/// 형식 판번호 1·ASCII·LF·탭 구분이며 float/double은 비트 그대로의 정수로 쓴다.
/// 자세한 행 뜻은 docs/exe/cpp-session-state-dump-20261109.md를 본다.
/// world 1 행은 풀·슬롯·Player·동맹·SP·Graph·장부·TotalMade까지 만든다. 미션 값 채우기는 Start 순서 연결에서 잇는다.</summary>
public static class SessionStateDump
{
    /// <summary>덤프 형식 판번호다. 열 의미가 바뀌면 올린다.</summary>
    public const uint Version = 1;

    /// <summary>풀 전체 비교용 FNV-1a 64비트 해시. 빈 입력은 0xcbf29ce484222325다.</summary>
    /// <param name="bytes">해시할 바이트.</param>
    public static ulong SessionDumpHash(ReadOnlySpan<byte> bytes)
    {
        ulong value = 0xcbf29ce484222325ul;
        // 바이트마다 XOR 뒤 FNV 소수를 곱한다.
        foreach (byte item in bytes)
            value = unchecked((value ^ item) * 0x100000001b3ul);
        return value;
    }

    /// <summary>분산 SP 배열에서 소유자의 현재 SP 값을 복호한다.
/// 근거: cpppj/src/client/SessionStateDump.cpp의 DecodeScrambledSp(0040ec50).
/// (2026-11-12 0.393에서 추가했다)</summary>
    /// <param name="pools">32×9 실제 배열.</param>
    /// <param name="permutation">32 순열.</param>
    /// <param name="offset">현재 SP 비트 위치.</param>
    /// <param name="owner">소유자 0~8.</param>
    public static uint DecodeScrambledSp(ReadOnlySpan<uint> pools, ReadOnlySpan<short> permutation, uint offset, uint owner)
    {
        if (owner >= 9 || pools.Length != 32 * 9 || permutation.Length != 32)
            throw new ArgumentException("세션 덤프 분산 SP 입력 크기", nameof(pools));
        uint value = 0;
        // k번째 비트는 배열 permutation[k]의 (offset+k)&31번째 비트에 있다.
        for (uint k = 0; k < 32; k++)
        {
            short index = permutation[(int)k];
            if (index < 0 || index >= 32) throw new ArgumentOutOfRangeException(nameof(permutation), "세션 덤프 분산 SP 순열 범위");
            if ((pools[index * 9 + (int)owner] & (1u << (int)((offset + k) & 31))) != 0) value |= 1u << (int)k;
        }
        return value;
    }

    /// <summary>머리(판번호·판본·프레임·난수) 행을 쓴다.</summary>
    private static void AppendHeader(
        StringBuilder output,
        string edition,
        ulong frame,
        ulong gameTimeBits,
        uint gameRandom,
        uint crtRandom)
    {
        output.Append("netstorm_session_dump\t").Append(Version.ToString(CultureInfo.InvariantCulture)).Append('\n');
        output.Append("edition\t").Append(edition).Append('\n');
        output.Append("frame\t").Append(frame.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(gameTimeBits.ToString(CultureInfo.InvariantCulture)).Append('\n');
        output.Append("random\t").Append(gameRandom.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(crtRandom.ToString(CultureInfo.InvariantCulture)).Append('\n');
    }

    /// <summary>Kernel 행과 pid 순서 프로세스·Regular 행을 쓴다.</summary>
    private static void AppendKernel(
        StringBuilder output,
        int kernelSize,
        uint kernelCapacity,
        IReadOnlyList<SessionDumpProcess>? processes)
    {
        output.Append("kernel\t").Append(kernelSize.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(kernelCapacity.ToString(CultureInfo.InvariantCulture)).Append('\n');
        // Kernel 프로세스는 pid 순서 그대로 둔다. RegularProcess는 사건 칸을 덧붙인다.
        if (processes is not null)
        {
            // 목록의 각 프로세스를 순서대로 쓴다.
            foreach (SessionDumpProcess process in processes)
            {
                output.Append("process\t").Append(process.Pid.ToString(CultureInfo.InvariantCulture)).Append('\t')
                    .Append(process.Form.ToString(CultureInfo.InvariantCulture)).Append('\t')
                    .Append(process.Parent.ToString(CultureInfo.InvariantCulture)).Append('\n');
                if (process.Regular)
                    output.Append("regular\t").Append(process.Pid.ToString(CultureInfo.InvariantCulture)).Append('\t')
                        .Append(process.Event.ToString(CultureInfo.InvariantCulture)).Append('\t')
                        .Append(process.Count.ToString(CultureInfo.InvariantCulture)).Append('\t')
                        .Append(process.PayloadBits.ToString(CultureInfo.InvariantCulture)).Append('\t')
                        .Append(process.TimeBits.ToString(CultureInfo.InvariantCulture)).Append('\n');
            }
        }
    }

    /// <summary>바이트 배열을 소문자 16진 문자열로 쓴다. cpppj WriteHex와 같다.</summary>
    private static void AppendHex(StringBuilder output, ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        // 각 바이트를 상위·하위 nibble 순으로 쓴다.
        foreach (byte item in bytes)
        {
            output.Append(digits[item >> 4]).Append(digits[item & 15]);
        }
    }

    /// <summary>판본 이름을 확인한다.</summary>
    private static void RequireEdition(string edition)
    {
        if (edition is not "originals" and not "originalCD")
            throw new ArgumentException("덤프 판본은 originals 또는 originalCD여야 합니다", nameof(edition));
    }
    /// <summary>world 0 덤프(머리·난수·Kernel까지)를 판번호 1 TSV 문자열로 만든다.</summary>
    /// <param name="edition">판본 이름(originals 또는 originalCD).</param>
    /// <param name="frame">현재 프레임 번호.</param>
    /// <param name="gameTimeBits">현재 게임 시각 double 비트.</param>
    /// <param name="gameRandom">게임 난수 상태.</param>
    /// <param name="crtRandom">CRT holdrand 상태.</param>
    /// <param name="kernelSize">현재 Kernel 등록 수.</param>
    /// <param name="kernelCapacity">Kernel 끝 번호.</param>
    /// <param name="processes">pid 오름차순 프로세스 목록.</param>
    public static string FormatWorld0(
        string edition,
        ulong frame,
        ulong gameTimeBits,
        uint gameRandom,
        uint crtRandom,
        int kernelSize,
        uint kernelCapacity,
        IReadOnlyList<SessionDumpProcess>? processes = null)
    {
        RequireEdition(edition);
        var output = new StringBuilder();
        AppendHeader(output, edition, frame, gameTimeBits, gameRandom, crtRandom);
        output.Append("world\t0\n");
        AppendKernel(output, kernelSize, kernelCapacity, processes);
        output.Append("end\n");
        return output.ToString();
    }

    /// <summary>world 1 덤프(머리·난수·Kernel·풀·슬롯·Player·동맹·SP·Graph·장부·TotalMade)를 판번호 1 TSV 문자열로 만든다.</summary>
    /// <param name="edition">판본 이름(originals 또는 originalCD).</param>
    /// <param name="frame">현재 프레임 번호.</param>
    /// <param name="gameTimeBits">현재 게임 시각 double 비트.</param>
    /// <param name="gameRandom">게임 난수 상태.</param>
    /// <param name="crtRandom">CRT holdrand 상태.</param>
    /// <param name="kernelSize">현재 Kernel 등록 수.</param>
    /// <param name="kernelCapacity">Kernel 끝 번호.</param>
    /// <param name="processes">pid 오름차순 프로세스 목록.</param>
    /// <param name="pool">월드 풀 관측(전체 원문·슬롯 목록 포함).</param>
    /// <param name="players">Player 9기록·숨김·동맹 관측.</param>
    /// <param name="sp">분산 SP 관측.</param>
    /// <param name="graph">Graph 표 전체.</param>
    /// <param name="totals">장부.</param>
    /// <param name="totalMade">TotalMade 세 표.</param>
    public static string FormatWorld1(
        string edition,
        ulong frame,
        ulong gameTimeBits,
        uint gameRandom,
        uint crtRandom,
        int kernelSize,
        uint kernelCapacity,
        IReadOnlyList<SessionDumpProcess>? processes,
        SessionDumpPool pool,
        SessionDumpPlayers players,
        SessionDumpSp sp,
        IReadOnlyList<SessionDumpGraphRow> graph,
        SessionDumpTotals totals,
        SessionDumpTotalMade totalMade)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(sp);
        ArgumentNullException.ThrowIfNull(graph);
        ArgumentNullException.ThrowIfNull(totals);
        ArgumentNullException.ThrowIfNull(totalMade);
        if (players.Records.Length % 180 != 0)
            throw new ArgumentException("세션 덤프 Player 원문 크기", nameof(players));
        if (totalMade.Local.Length != totalMade.Built.Length || totalMade.Local.Length != totalMade.Global.Length)
            throw new ArgumentException("세션 덤프 TotalMade 표 크기", nameof(totalMade));
        RequireEdition(edition);
        var output = new StringBuilder();
        AppendHeader(output, edition, frame, gameTimeBits, gameRandom, crtRandom);
        output.Append("world\t1\n");
        AppendKernel(output, kernelSize, kernelCapacity, processes);
        output.Append("pool\t").Append(pool.Capacity.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(pool.Stride.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(pool.FreeCount.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(pool.FirstFreeServer.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(pool.FirstFreeClient.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(pool.PredictableCursor.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(SessionDumpHash(pool.Bytes).ToString("x", CultureInfo.InvariantCulture)).Append('\n');
        // 슬롯은 번호 순서대로 쓰고 원문은 소문자 16진이다.
        foreach (SessionDumpSlot slot in pool.Slots)
        {
            output.Append("slot\t").Append(slot.Sid.ToString(CultureInfo.InvariantCulture)).Append('\t');
            AppendHex(output, slot.Raw);
            output.Append('\n');
        }
        // Player 9기록을 번호 순서로 쓴다.
        for (int index = 0; index < players.Records.Length / 180; index++)
        {
            output.Append("player\t").Append(index.ToString(CultureInfo.InvariantCulture)).Append('\t');
            AppendHex(output, players.Records.AsSpan(index * 180, 180));
            output.Append('\n');
        }
        output.Append("hidden_players\t");
        AppendHex(output, players.Hidden);
        output.Append('\n');
        output.Append("alliances");
        // 방향별 9×9 DWORD를 행 우선 순서로 쓴다.
        foreach (uint value in players.Alliances) output.Append('\t').Append(value.ToString(CultureInfo.InvariantCulture));
        output.Append('\n');
        output.Append("sp\t").Append(sp.Initialized ? 1 : 0).Append('\t').Append(sp.Offset.ToString(CultureInfo.InvariantCulture));
        // 소유자 0~8의 현재 SP다. 미초기화 저장소는 값을 읽지 않는다.
        for (uint owner = 0; owner < 9; owner++)
        {
            if (sp.Initialized) output.Append('\t').Append(DecodeScrambledSp(sp.Pools, sp.Permutation, sp.Offset, owner).ToString(CultureInfo.InvariantCulture));
            else output.Append("\t-");
        }
        output.Append('\n');
        // Graph 표의 모든 칸을 그대로 쓴다.
        for (int index = 0; index < graph.Count; index++)
            output.Append("graph\t").Append(index.ToString(CultureInfo.InvariantCulture)).Append('\t')
                .Append(graph[index].Surfaces.ToString(CultureInfo.InvariantCulture)).Append('\t')
                .Append(graph[index].InUse.ToString(CultureInfo.InvariantCulture)).Append('\t')
                .Append(graph[index].Reserved.ToString(CultureInfo.InvariantCulture)).Append('\n');
        output.Append("totals\t").Append(totals.TotalCost.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(totals.PostPopDepth.ToString(CultureInfo.InvariantCulture)).Append('\t')
            .Append(totals.ProductionDirty.ToString(CultureInfo.InvariantCulture)).Append('\n');
        // 세 표 가운데 하나라도 0이 아닌 TYPE만 쓴다.
        for (int type = 0; type < totalMade.Local.Length; type++)
        {
            if (totalMade.Local[type] != 0 || totalMade.Built[type] != 0 || totalMade.Global[type] != 0)
                output.Append("totalmade\t").Append(type.ToString(CultureInfo.InvariantCulture)).Append('\t')
                    .Append(totalMade.Local[type].ToString(CultureInfo.InvariantCulture)).Append('\t')
                    .Append(totalMade.Built[type].ToString(CultureInfo.InvariantCulture)).Append('\t')
                    .Append(totalMade.Global[type].ToString(CultureInfo.InvariantCulture)).Append('\n');
        }
        output.Append("end\n");
        return output.ToString();
    }
}
