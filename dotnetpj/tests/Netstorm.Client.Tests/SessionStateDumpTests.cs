using Netstorm.Client.Automation;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>실제 Session 상태 덤프의 판번호 1 형식과 world 0 파일 쓰기를 검사한다. 창을 열지 않는다.
/// 근거: cpppj/src/client/SessionStateDump.h·SessionStateDump.cpp 와 docs/exe/cpp-session-state-dump-20261109.md.
/// (2026-11-10 L2·L3 장치에서 추가했다)</summary>
public sealed class SessionStateDumpTests
{
    /// <summary>world 0 덤프가 판번호·판본·프레임·난수·Kernel 행을 ASCII·LF·탭으로 쓴다.</summary>
    [Fact]
    public void FormatsWorld0Rows()
    {
        string text = SessionStateDump.FormatWorld0("originals", 5, 0, 0, 1, 0, 39999);
        Assert.Equal(
            "netstorm_session_dump\t1\n" +
            "edition\toriginals\n" +
            "frame\t5\t0\n" +
            "random\t0\t1\n" +
            "world\t0\n" +
            "kernel\t0\t39999\n" +
            "end\n",
            text);
        // LF만 쓰고 CR은 쓰지 않는다.
        Assert.DoesNotContain("\r", text);
    }

    /// <summary>Kernel 프로세스는 pid 순서대로 쓰고 RegularProcess는 사건 칸을 덧붙인다.</summary>
    [Fact]
    public void FormatsProcessAndRegularRows()
    {
        var processes = new List<SessionDumpProcess>
        {
            new(7, 0, 0),
            new(9, 0, 0, true, 3, 2, 1065353216, 4609434218613702656),
        };
        string text = SessionStateDump.FormatWorld0("originalCD", 90, 4621368757626234470, 12, 34, 2, 3999, processes);
        Assert.Equal(
            "netstorm_session_dump\t1\n" +
            "edition\toriginalCD\n" +
            "frame\t90\t4621368757626234470\n" +
            "random\t12\t34\n" +
            "world\t0\n" +
            "kernel\t2\t3999\n" +
            "process\t7\t0\t0\n" +
            "process\t9\t0\t0\n" +
            "regular\t9\t3\t2\t1065353216\t4609434218613702656\n" +
            "end\n",
            text);
    }

    /// <summary>판본 이름이 originals·originalCD가 아니면 거부한다.</summary>
    [Fact]
    public void RejectsUnknownEdition()
    {
        Assert.Throws<ArgumentException>(() => SessionStateDump.FormatWorld0("tell", 1, 0, 0, 1, 0, 39999));
    }

    /// <summary>풀 해시는 FNV-1a 64다. 빈 입력은 offset basis, foobar는 공개 시험값이다.</summary>
    [Fact]
    public void HashesWithFnv1a64()
    {
        Assert.Equal(0xcbf29ce484222325ul, SessionStateDump.SessionDumpHash([]));
        Assert.Equal(0x85944171f73967e8ul, SessionStateDump.SessionDumpHash("foobar"u8.ToArray()));
    }

    /// <summary>호스트 장치가 현재 Kernel·난수·프레임의 world 0 덤프를 ASCII·LF 파일에 쓴다.</summary>
    [Fact]
    public void HostDumpWritesWorld0File()
    {
        var kernel = new Kernel();
        var game = new GameRandom();
        var crt = new CrtRandom();
        var dump = new HostSessionDump(kernel, game, crt, () => 5, () => 0);
        string path = Path.Combine(Path.GetTempPath(), "netstorm-session-" + Guid.NewGuid().ToString("N") + ".tsv");
        try
        {
            dump.Write(path);
            byte[] written = File.ReadAllBytes(path);
            string text = new System.Text.ASCIIEncoding().GetString(written);
            Assert.Equal(
                "netstorm_session_dump\t1\n" +
                "edition\toriginals\n" +
                "frame\t5\t0\n" +
                "random\t0\t1\n" +
                "world\t0\n" +
                "kernel\t0\t39999\n" +
                "end\n",
                text);
            Assert.DoesNotContain("\r", text);
        }
        finally { File.Delete(path); }
        Assert.Throws<ArgumentException>(() => dump.Write(""));
        Assert.Throws<ArgumentException>(() => new HostSessionDump(kernel, game, crt, () => 1, () => 0, "tell"));
    }

    /// <summary>world 1 덤프가 풀 행·예약 슬롯 행을 쓰고 사용 중·반납 슬롯을 따라간다.</summary>
    [Fact]
    public void FormatsWorld1PoolAndSlotRows()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.Equal(HostSession.MissionPoolCapacity, session.Pool.Capacity);
        string PoolText() => SessionStateDump.FormatWorld1(
            "originals", 90, 4622156887561024307, session.Game.State(), session.Crt.State(),
            session.Kernel.Size(), session.Kernel.Capacity(), null, session.DumpPool(),
            session.DumpPlayers(), session.DumpSp(), session.DumpGraph(), session.DumpTotals(), session.DumpTotalMade());
        string zeros100 = new('0', 100);
        string zeros60 = new('0', 60);
        string zeros76 = new('0', 76);
        string freshHash = SessionStateDump.SessionDumpHash(session.Pool.Bytes.ToArray()).ToString("x", System.Globalization.CultureInfo.InvariantCulture);
        // 빈 풀은 예약 0~4번만 내놓는다. cpppj pool 행 순서(크기·폭·빈 수·서버·클라이언트·커서·해시)와 같다.
        var expected = new System.Text.StringBuilder();
        expected.Append("netstorm_session_dump\t1\nedition\toriginals\nframe\t90\t4622156887561024307\n");
        expected.Append("random\t").Append(session.Game.State()).Append('\t').Append(session.Crt.State()).Append('\n');
        expected.Append("world\t1\nkernel\t0\t39999\n");
        expected.Append("pool\t32768\t50\t32763\t15000\t5\t1\t").Append(freshHash).Append('\n');
        // 슬롯 0은 빈 목록 머리(바이트 4·5 클라이언트 첫 번호 5, 바이트 18·19 서버 첫 번호 15000)다.
        expected.Append("slot\t0\t00000000" + "0500" + new string('0', 24) + "983a" + zeros60 + "\n");
        // 슬롯 1은 0이다.
        expected.Append("slot\t1\t" + zeros100 + "\n");
        // 슬롯 2~4는 빈 칸이다(free 비트만 켜지고 목록 연결은 5번부터다). 덤프에는 나온다(예약).
        // 각 슬롯의 free 바이트를 순회하며 쓴다.
        for (uint sid = 2; sid < 5; sid++)
            expected.Append("slot\t").Append(sid).Append('\t')
                .Append(new string('0', 22)).Append("01").Append(zeros76).Append('\n');
        // 빈 세션의 Player 9기록·숨김·동맹은 0이다.
        for (int index = 0; index < 9; index++)
            expected.Append("player\t").Append(index).Append('\t').Append(new string('0', 360)).Append('\n');
        expected.Append("hidden_players\t").Append(new string('0', 18)).Append('\n');
        expected.Append("alliances").Append(string.Concat(Enumerable.Repeat("\t0", 81))).Append('\n');
        // 미초기화 SP는 대시 9칸이다.
        expected.Append("sp\t0\t0").Append(string.Concat(Enumerable.Repeat("\t-", 9))).Append('\n');
        // 설치 초기화(ResetTable) Graph 표 255칸이다. 0번 예약·254번 sentinel 외에는 0이다.
        // (2026-11-12 0.476에서 세션 설치 초기화를 잇었다. 근거 cpppj 세션 저장소의 Acquire ResetTable.)
        for (int index = 0; index < 255; index++)
            expected.Append("graph\t").Append(index).Append(
                index == 0 ? "\t0\t1\t1\n" : index == 254 ? "\t32253\t32253\t0\n" : "\t0\t0\t0\n");
        expected.Append("totals\t0\t0\t0\n");
        expected.Append("end\n");
        Assert.Equal(expected.ToString(), PoolText());
        // 클라이언트 영역 첫 번호를 쓰면 슬롯 행이 생기고, 반납하면 다시 빠진다.
        int sid5 = session.Pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        Assert.Equal(5, sid5);
        string taken = PoolText();
        string slot5 = "slot\t5\t" + new string('0', 22) + "04" + new string('0', 76) + "\n";
        Assert.Contains(slot5, taken, StringComparison.Ordinal);
        session.Pool.Release(sid5);
        Assert.DoesNotContain("slot\t5\t", PoolText(), StringComparison.Ordinal);
    }

    /// <summary>world 1 덤프의 난수 행이 세션이 실제 소비하는 게임·지형 CRT 상태를 쓴다. 미소비 호스트 CRT 상수가 아니다.</summary>
    [Fact]
    public void HostDumpWritesWorld1SessionStreams()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        var dump = new HostSessionDump(session.Kernel, session.Game, session.Crt, () => 5, () => 0);
        dump.BindSession(session);
        session.Game.SetState(12u);
        session.TerrainCrt.Value = 34u;
        string path = Path.Combine(Path.GetTempPath(), "netstorm-session-" + Guid.NewGuid().ToString("N") + ".tsv");
        try
        {
            dump.Write(path);
            string text = new System.Text.ASCIIEncoding().GetString(File.ReadAllBytes(path));
            Assert.Contains("\nrandom\t12\t34\n", text, StringComparison.Ordinal);
        }
        finally { File.Delete(path); }
    }

    /// <summary>분산 SP 복호가 원본 비트 순서와 같다.</summary>
    [Fact]
    public void DecodesScrambledSpLikeNative()
    {
        var pools = new uint[32 * 9];
        var permutation = new short[32];
        // 항등 순열을 순서대로 둔다.
        for (int index = 0; index < 32; index++) permutation[index] = (short)index;
        // 소유자 3의 0·5·31번째 비트를 해당 배열 칸에 둔다.
        pools[0 * 9 + 3] |= 1u << 0;
        pools[5 * 9 + 3] |= 1u << 5;
        pools[31 * 9 + 3] |= 1u << 31;
        Assert.Equal(1u | (1u << 5) | (1u << 31), SessionStateDump.DecodeScrambledSp(pools, permutation, 0, 3));
        // 다른 소유자는 읽지 않는다.
        Assert.Equal(0u, SessionStateDump.DecodeScrambledSp(pools, permutation, 0, 4));
        // 어긋난 입력은 거부한다.
        Assert.Throws<ArgumentException>(() => SessionStateDump.DecodeScrambledSp(new uint[10], permutation, 0, 0));
        Assert.Throws<ArgumentException>(() => SessionStateDump.DecodeScrambledSp(pools, new short[10], 0, 0));
        Assert.Throws<ArgumentException>(() => SessionStateDump.DecodeScrambledSp(pools, permutation, 0, 9));
        var badPermutation = (short[])permutation.Clone();
        badPermutation[7] = 40;
        Assert.Throws<ArgumentOutOfRangeException>(() => SessionStateDump.DecodeScrambledSp(pools, badPermutation, 0, 0));
    }

    /// <summary>0이 아닌 저장소 값이 덤프 행으로 나온다.</summary>
    [Fact]
    public void FormatsNonzeroSessionStores()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        session.PlayerRecords[0] = 0xAB;
        session.HiddenPlayers[2] = 7;
        session.Alliances[0] = 7;
        session.Alliances[80] = 1;
        session.TotalMade.LocalCounts[5] = 3;
        session.TotalMade.LocalSecondaryCounts[5] = 4;
        session.TotalMade.GlobalCounts[70] = 9;
        session.TotalCost = -12;
        string text = SessionStateDump.FormatWorld1(
            "originals", 1, 0, 0, 0, 0, 39999, null, session.DumpPool(),
            session.DumpPlayers(), session.DumpSp(), session.DumpGraph(), session.DumpTotals(), session.DumpTotalMade());
        Assert.Contains("\nplayer\t0\tab", text, StringComparison.Ordinal);
        Assert.Contains("\nhidden_players\t000007000000000000\n", text, StringComparison.Ordinal);
        Assert.Contains("\nalliances\t7", text, StringComparison.Ordinal);
        Assert.Contains("\ntotalmade\t5\t3\t4\t0\n", text, StringComparison.Ordinal);
        Assert.Contains("\ntotalmade\t70\t0\t0\t9\n", text, StringComparison.Ordinal);
        Assert.Contains("\ntotals\t-12\t0\t0\n", text, StringComparison.Ordinal);
    }

    /// <summary>초기화한 SP 저장소는 소유자별 복호 값을 행에 쓴다.</summary>
    [Fact]
    public void FormatsInitializedSpRow()
    {
        var pools = new uint[32 * 9];
        var permutation = new short[32];
        // 항등 순열을 순서대로 둔다.
        for (int index = 0; index < 32; index++) permutation[index] = (short)index;
        // 소유자 1의 2번째 비트를 해당 배열 칸에 둔다(비트 위치 (7+2)&31).
        pools[2 * 9 + 1] |= 1u << 9;
        var sp = new SessionDumpSp(true, 7, pools, permutation);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        string text = SessionStateDump.FormatWorld1(
            "originals", 1, 0, 0, 0, 0, 39999, null, session.DumpPool(),
            session.DumpPlayers(), sp, session.DumpGraph(), session.DumpTotals(), session.DumpTotalMade());
        Assert.Contains("\nsp\t1\t7\t0\t4\t0\t0\t0\t0\t0\t0\t0\n", text, StringComparison.Ordinal);
    }

    /// <summary>세션을 연결하면 world 1 파일을 쓰고 끊으면 world 0으로 돌아간다.</summary>
    [Fact]
    public void HostSessionBindsWorld1File()
    {
        var kernel = new Kernel();
        var game = new GameRandom();
        var crt = new CrtRandom();
        var dump = new HostSessionDump(kernel, game, crt, () => 90, () => 0);
        var session = new HostSession(kernel, game, crt);
        string path = Path.Combine(Path.GetTempPath(), "netstorm-session-" + Guid.NewGuid().ToString("N") + ".tsv");
        try
        {
            dump.BindSession(session);
            dump.Write(path);
            string world1 = File.ReadAllText(path, System.Text.ASCIIEncoding.ASCII);
            Assert.Contains("\nworld\t1\n", world1, StringComparison.Ordinal);
            Assert.Contains("\npool\t32768\t50\t", world1, StringComparison.Ordinal);
            Assert.Contains("\nend\n", world1, StringComparison.Ordinal);
            Assert.DoesNotContain("\r", world1);
            dump.BindSession(null);
            dump.Write(path);
            string world0 = File.ReadAllText(path, System.Text.ASCIIEncoding.ASCII);
            Assert.Contains("\nworld\t0\n", world0, StringComparison.Ordinal);
            Assert.DoesNotContain("\npool\t", world0, StringComparison.Ordinal);
        }
        finally { File.Delete(path); }
    }
}
