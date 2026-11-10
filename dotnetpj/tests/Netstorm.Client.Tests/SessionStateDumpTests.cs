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
}
