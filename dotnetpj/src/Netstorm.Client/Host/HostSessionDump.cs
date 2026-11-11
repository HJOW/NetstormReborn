using Netstorm.Client.Automation;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>미션 월드가 없을 때의 world 0 상태 덤프를 파일에 쓰는 호스트 장치다.
/// 세션을 연결하면 world 1(머리·난수·Kernel·풀·슬롯) 덤프를 쓴다. Player 이후 행은 세션 조립과 함께 늘린다.
/// 근거: cpppj/src/client/SessionStateDump.cpp의 DumpSessionState(world 0 부분).
/// (2026-11-10 L2·L3 장치에서 추가했고 2026-11-12 0.392에서 세션 연결을 더했다)</summary>
public sealed class HostSessionDump
{
    /// <summary>덤프에 적는 현재 Kernel이다. F3 세션이 생기면 같은 인스턴스를 세션에 넘긴다.</summary>
    private readonly Kernel _kernel;

    /// <summary>덤프에 적는 게임 난수다. F3 세션이 생기면 같은 인스턴스를 세션에 넘긴다.</summary>
    private readonly GameRandom _game;

    /// <summary>덤프에 적는 CRT 난수다. F3 세션이 생기면 같은 인스턴스를 세션에 넘긴다.</summary>
    private readonly CrtRandom _crt;

    /// <summary>덤프에 적는 현재 프레임 번호 읽기다.</summary>
    private readonly Func<ulong> _frame;

    /// <summary>덤프에 적는 현재 게임 시각 double 비트 읽기다. 세션 시계 미연결 상태에서는 0(0.0)이다.</summary>
    private readonly Func<ulong> _gameTimeBits;

    /// <summary>덤프 판본 이름(originals 또는 originalCD)이다. 호스트는 10.78만 다루므로 기본 originals다.</summary>
    private readonly string _edition;

    /// <summary>세션이 있으면 world 1 덤프에 쓰는 미션 세션이다. 없으면 world 0이다.</summary>
    private HostSession? _session;

    /// <summary>같은 Kernel·난수·시각 읽기로 만든다.</summary>
    /// <param name="kernel">현재 Kernel.</param>
    /// <param name="game">게임 난수.</param>
    /// <param name="crt">CRT 난수.</param>
    /// <param name="frame">현재 프레임 번호 읽기.</param>
    /// <param name="gameTimeBits">현재 게임 시각 double 비트 읽기.</param>
    /// <param name="edition">덤프 판본 이름.</param>
    public HostSessionDump(
        Kernel kernel,
        GameRandom game,
        CrtRandom crt,
        Func<ulong> frame,
        Func<ulong> gameTimeBits,
        string edition = "originals")
    {
        ArgumentNullException.ThrowIfNull(kernel);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(crt);
        ArgumentNullException.ThrowIfNull(frame);
        ArgumentNullException.ThrowIfNull(gameTimeBits);
        if (edition is not "originals" and not "originalCD")
            throw new ArgumentException("덤프 판본은 originals 또는 originalCD여야 합니다", nameof(edition));
        _kernel = kernel;
        _game = game;
        _crt = crt;
        _frame = frame;
        _gameTimeBits = gameTimeBits;
        _edition = edition;
    }

    /// <summary>미션 세션을 연결하거나 끊는다. 세션의 Kernel·난수와 덤프 장치는 같은 객체를 써야 한다.</summary>
    /// <param name="session">연결할 세션. null이면 world 0으로 돌아간다.</param>
    /// <remarks>2026-11-12 0.392에서 world 1 덤프를 위해 추가했다. 미션 시작 흐름 연결은 다음 단위다.</remarks>
    public void BindSession(HostSession? session) => _session = session;

    /// <summary>그 프레임의 그리기 뒤 현재 world 0 상태를 인자 경로에 쓴다. 난수·Kernel을 전진시키지 않는다.</summary>
    /// <param name="path">덤프를 쓸 파일 경로.</param>
    public void Write(string path)
    {
        if (string.IsNullOrEmpty(path))
            throw new ArgumentException("session-dump 출력 경로가 비어 있습니다", nameof(path));
        // 현재 Kernel 슬롯을 pid 순서로 읽는다. SquidProcess·RegularProcess의 Form·사건 정보는
        // 월드 저장소 연결(R4) 전까지 없으므로 0으로 둔다(cpppj의 비Squid 행과 같은 모양).
        var processes = new List<SessionDumpProcess>();
        // 등록된 pid만 순서대로 읽는다.
        for (uint pid = 1; pid <= _kernel.Capacity(); pid++)
        {
            if (_kernel.Get(pid) is null)
                continue;
            processes.Add(new SessionDumpProcess(pid, 0, 0));
        }
        if (_session is null)
        {
            string world0 = SessionStateDump.FormatWorld0(
                _edition, _frame(), _gameTimeBits(), _game.State(), _crt.State(), _kernel.Size(), _kernel.Capacity(), processes);
            // cpppj 덤프와 같은 ASCII·LF 바이트로 쓴다.
            File.WriteAllText(path, world0, new System.Text.ASCIIEncoding());
            return;
        }
        string world1 = SessionStateDump.FormatWorld1(
            _edition, _frame(), _gameTimeBits(), _session.Game.State(), _session.Crt.State(),
            _session.Kernel.Size(), _session.Kernel.Capacity(), processes, _session.DumpPool(),
            _session.DumpPlayers(), _session.DumpSp(), _session.DumpGraph(), _session.DumpTotals(), _session.DumpTotalMade());
        // cpppj 덤프와 같은 ASCII·LF 바이트로 쓴다.
        File.WriteAllText(path, world1, new System.Text.ASCIIEncoding());
    }
}
