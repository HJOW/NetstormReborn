using System.Globalization;
using System.Text;
using static Netstorm.Core.Original.RawMissionPacketFields;

// 로그인 타이머와 실제 Begin/중단/진행 안내가 같은 전역을 읽고 쓴다.
namespace Netstorm.Core.Original;

/// <summary>원문 타이머의 실제 저장소 참조이며 값형 성공 사본을 만들지 않는다.</summary>
public sealed record MissionLoginTimerState
{
    /// <summary>005c85f0의 실제 로그인 벽시각 기한이다.</summary>
    public DoubleBox End { get; init; } = new();
    /// <summary>005c8938의 현재 로그인 제한 정수이다.</summary>
    public IntBox Timeout { get; init; } = new();
    /// <summary>005c7a74의 실제 Begin 송수신 표시이다.</summary>
    public UIntBox Begin { get; init; } = new();
    /// <summary>00542544의 실제 완료0/중단1 전역이다.</summary>
    public UIntBox ScriptBreak { get; init; } = new();
    /// <summary>중단이4를 요청하는005c85ac이며 하위가 다시 변경 가능이다.</summary>
    public UIntBox RequestPhase { get; init; } = new();
    /// <summary>savedGame0에서만 쓰는00594fb4의 실제 기준 clock이다.</summary>
    public UIntBox Clock { get; init; } = new();
}

/// <summary>각 원문 분기에서 현재 모드를 다시 조회한다.</summary>
public sealed record MissionLoginTimerReads
{
    /// <summary>005c85d4의 현재 저장 게임 모드를 조회한다.</summary>
    public Func<bool>? SavedGame { get; init; }
    /// <summary>00540bc0의 현재 서버 여부를 조회한다.</summary>
    public Func<bool>? Server { get; init; }
    /// <summary>00594fa0의 현재 미션 월드 여부를 조회한다.</summary>
    public Func<bool>? Mission { get; init; }
    /// <summary>00594fa4의 현재 미션 후처리 여부를 조회한다.</summary>
    public Func<bool>? MissionPost { get; init; }
    /// <summary>00540bc4의 현재 초기 간헐천 선택를 조회한다.</summary>
    public Func<bool>? InitialGeysers { get; init; }
    /// <summary>005e4794의 현재 진단 활성 여부를 조회한다.</summary>
    public Func<bool>? CheckingAssertions { get; init; }
}

/// <summary>서로 다른 원문 로그 진입을 같은 실제 소비자에 전달할 때 구별한다.</summary>
public enum MissionLoginTimerLog
{
    /// <summary>0045ad00의 로그인 타이머 시작 로그다.</summary>
    TimerStart,
    /// <summary>004a68e0의 사제/카메라 진행 로그다.</summary>
    Status,
}

/// <summary>선택되지 않은 실제 장치는 요구하지 않고 도달한 누락만 거부한다.</summary>
public sealed record MissionLoginTimerHooks
{
    /// <summary>현재 loginTimeout 설정을 조회한다.</summary>
    public Func<string, int>? ConfigInt { get; init; }
    /// <summary>각 실제 호출 위치의 벽시각을 조회한다.</summary>
    public Func<double>? WallSeconds { get; init; }
    /// <summary>현재 CRT clock32비트를 조회한다.</summary>
    public Func<uint>? Clock { get; init; }
    /// <summary>현재 서버9의 실제 연결 존재를 조회한다.</summary>
    public Func<int, bool>? Connection { get; init; }
    /// <summary>004b6180의 실제9/0/0 요청을 처리한다.</summary>
    public Action<int, uint, uint>? RemoteStatus { get; init; }
    /// <summary>실제 Begin8바이트를 전체분배한다.</summary>
    public Action<ReadOnlyMemory<byte>>? Broadcast { get; init; }
    /// <summary>현재 owner1..8의 실제 사제 참여 상태를 재조회한다.</summary>
    public Action<uint>? PriestRefresh { get; init; }
    /// <summary>미션 자연 반환도 같은 실제004b54e0이 결정한다.</summary>
    public Action? SaveReconnect { get; init; }
    /// <summary>실제 로그인 진행 창을 반납한다.</summary>
    public Action? CloseProgress { get; init; }
    /// <summary>비미션 후처리의 실제 카메라 초기화다.</summary>
    public Action? ResetCamera { get; init; }
    /// <summary>실제004b6350 상태 요청/정리를 수행한다.</summary>
    public Action? AbortBattle { get; init; }
    /// <summary>현재 Player/번역/프레임 시각으로 일반 진행 UI를 갱신한다.</summary>
    public Action? Progress { get; init; }
    /// <summary>checking&&!server의 원문 진단 뒤 반환한다.</summary>
    public Action? ServerAssertion { get; init; }
    /// <summary>원문의 서로 다른 실제 로그 종류와 문자열을 전달한다.</summary>
    public Action<MissionLoginTimerLog, string>? Log { get; init; }
    /// <summary>저장 안 된 비미션의 실제 일반 초기화 전체다.</summary>
    public Action? InitializeGeneral { get; init; }
    /// <summary>현재 방장·비미션에서만 실제 초기 간헐천을 생성한다.</summary>
    public Action? CreateInitialGeysers { get; init; }
}

/// <summary>RawMissionLoginTimer.cpp의 전체 현재 모드/타이머·Begin/중단/진행 부모다.</summary>
public sealed class RawMissionLoginTimer
{
    /// <summary>실제 전역 주소를 빌리는 상태 상자다.</summary>
    private readonly MissionLoginTimerState _state;
    /// <summary>로그인과 같은 실제 Player9 집계다.</summary>
    private readonly MissionLoginPlayerCounts _players;
    /// <summary>각 원문 위치에서 다시 읽을 현재 모드다.</summary>
    private readonly MissionLoginTimerReads _reads;
    /// <summary>선택된 실제 시간/전송/사제/UI 효과다.</summary>
    private readonly MissionLoginTimerHooks _hooks;
    /// <summary>원본판본과 필수 현재 모드만 확인하며 타이머나 UI를 실행하지 않는다.</summary>
    public RawMissionLoginTimer(OriginalEdition edition, MissionLoginTimerState state, MissionLoginPlayerCounts players, MissionLoginTimerReads reads, MissionLoginTimerHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(players); ArgumentNullException.ThrowIfNull(reads); ArgumentNullException.ThrowIfNull(hooks);
        if (edition != OriginalEdition.Patch1078 || reads.SavedGame is null || reads.Server is null || reads.Mission is null || reads.MissionPost is null || reads.InitialGeysers is null || reads.CheckingAssertions is null) throw new ArgumentException("로그인 타이머 PATCH 현재 모드 조회 누락");
        _state = state; _players = players; _reads = reads; _hooks = hooks;
    }
    /// <summary>벽시각 효과가 현재 end를 변경하면 복귀 뒤 그 값을 비교한다.</summary>
    public bool Expired() { double now = Need(_hooks.WallSeconds)(); return _state.End.Value <= now; }
    /// <summary>서버 진단 뒤 timer0→실제 Begin 송신→begin1 순서다.</summary>
    public void Begin()
    {
        if (_reads.CheckingAssertions!() && !_reads.Server!()) Need(_hooks.ServerAssertion)();
        _state.End.Value = 0; Need(_hooks.Broadcast)(new byte[] { 1, 0x6e, 8, 0, 0, 0, 0, 0x80 }); _state.Begin.Value = 1;
    }
    /// <summary>현재 begin/end/scriptBreak/phase 쓰기 뒤 실제 중단 하위가 바꾼 값을 덮지 않는다.</summary>
    public void Abort()
    {
        _state.Begin.Value = 0; _state.End.Value = 0; _state.ScriptBreak.Value = 1; _state.RequestPhase.Value = 4;
        Need(_hooks.AbortBattle)(); Need(_hooks.CloseProgress)();
    }
    /// <summary>시작은 현재 설정/벽시각을 예약하고 그 외에는 실제 현재 모드/참여·Ready 조건으로 진행한다.</summary>
    public void Run(bool start)
    {
        if (start)
        {
            Need(_hooks.Log)(MissionLoginTimerLog.TimerStart, "login status timer start\n"); int timeout = Need(_hooks.ConfigInt)("loginTimeout"); _state.Timeout.Value = timeout; double now = Need(_hooks.WallSeconds)(); _state.End.Value = timeout + now; return;
        }
        if (_state.End.Value == 0) return;
        if (!_reads.SavedGame!()) _state.Clock.Value = Need(_hooks.Clock)();
        bool serverBranch = true;
        if (!_reads.Server!())
        {
            if (!Need(_hooks.Connection)(9)) { Abort(); return; }
            if (!_reads.Server!()) { Need(_hooks.RemoteStatus)(9, 0, 0); serverBranch = false; }
        }
        if (serverBranch)
        {
            bool begin = true;
            if (_state.Begin.Value == 0)
            {
                uint dropped = _players.Disconnected(), subscribers = _players.Subscribers(), ready = _players.Ready();
                if (unchecked(subscribers - dropped) != ready)
                {
                    if (!Expired()) begin = false;
                    else if (_players.LoggedIn() < 2 && !_reads.SavedGame!()) { Abort(); return; }
                }
            }
            if (begin) Begin();
        }
        if (_reads.MissionPost!() || _state.Begin.Value != 0)
        {
            if (!_reads.SavedGame!())
            {
                if (!_reads.Mission!()) Need(_hooks.InitializeGeneral)();
                if (_reads.InitialGeysers!() && !_reads.Mission!())
                {
                    Need(_hooks.Log)(MissionLoginTimerLog.Status, "Creating initial geysers\n"); Need(_hooks.CreateInitialGeysers)(); Need(_hooks.Log)(MissionLoginTimerLog.Status, "Done\n");
                }
            }
            Need(_hooks.Log)(MissionLoginTimerLog.Status, "Seting Priest State\n");
            // 같은 실제 owner1..8의 현재 사제 상태를 순서대로 재조회한다.
            for (uint owner = 1; owner < 9; owner++) Need(_hooks.PriestRefresh)(owner);
            Need(_hooks.Log)(MissionLoginTimerLog.Status, "Done\n"); _state.Begin.Value = 0; _state.ScriptBreak.Value = 0; Need(_hooks.SaveReconnect)(); _state.End.Value = 0; Need(_hooks.CloseProgress)();
            Need(_hooks.Log)(MissionLoginTimerLog.Status, "Moveing Camera\n"); if (!_reads.MissionPost!()) Need(_hooks.ResetCamera)(); Need(_hooks.Log)(MissionLoginTimerLog.Status, "Done\n"); return;
        }
        Need(_hooks.Progress)();
    }

    /// <summary>원문 단일바이트 문자열의 실제 첫NUL 앞만 printf 인자로 사용한다.</summary>
    private static string CString(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>고정 숫자 폭/정밀도의 유효 범위를 읽되 넘치는 표기는 안전한 상한으로 보존한다.</summary>
    private static int Number(string pattern, ref int at)
    {
        int value = 0;
        // 원본의 상수 폭/정밀도만 허용하며 별표로 미공급 인자를 읽지 않는다.
        while (at < pattern.Length && pattern[at] is >= '0' and <= '9') { value = (int)Math.Min(int.MaxValue, (long)value * 10 + pattern[at] - '0'); at++; }
        return value;
    }
    /// <summary>원문이 허용한 문자열/문자열/정수/문자열 네 인자와 %%·고정 폭/정밀도를 실제1024바이트 안에서 출력한다.</summary>
    private static string FormatProgress(string pattern, string title, string list, int remaining, string extra)
    {
        int arguments = 0;
        // CRT에 미공급 인자를 읽게 하는 종류/개수를 먼저 거부하는 원문 검사를 보존한다.
        for (int at = 0; at < pattern.Length; at++) if (pattern[at] == '%')
        {
            if (++at < pattern.Length && pattern[at] == '%') continue;
            // 원문 validator가 허용한 플래그·숫자·점을 같은 범위에서 검사한다.
            while (at < pattern.Length && (pattern[at] is '-' or '+' or ' ' or '#' or '0' or '.' || pattern[at] is >= '0' and <= '9')) at++;
            if (arguments >= 4 || at == pattern.Length || pattern[at] != (arguments == 2 ? 'd' : 's')) throw new ArgumentException("로그인 진행 번역의 네 인자 서식 오류"); arguments++;
        }
        if (arguments != 4) throw new ArgumentException("로그인 진행 번역의 인자 개수 오류");
        var output = new StringBuilder(); int argument = 0; pattern = CString(pattern);
        // 현재 번역 서식의 각 실제 변환만 처리하며 다른 런타임 포맷 엔진을 호출하지 않는다.
        for (int at = 0; at < pattern.Length; at++)
        {
            if (pattern[at] != '%') { output.Append(pattern[at]); if (output.Length >= 1024) throw new ArgumentOutOfRangeException(nameof(pattern), "로그인 진행 HTML1024바이트 초과"); continue; }
            if (++at < pattern.Length && pattern[at] == '%') { output.Append('%'); continue; }
            bool left = false, plus = false, blank = false, zero = false;
            // 원문 상수 printf 플래그의 유효 의미를 구별한다.
            while (at < pattern.Length && pattern[at] is '-' or '+' or ' ' or '#' or '0')
            {
                left |= pattern[at] == '-'; plus |= pattern[at] == '+'; blank |= pattern[at] == ' '; zero |= pattern[at] == '0'; at++;
            }
            int width = Number(pattern, ref at), precision = -1;
            if (at < pattern.Length && pattern[at] == '.') { at++; precision = Number(pattern, ref at); }
            if (at >= pattern.Length || pattern[at] != (argument == 2 ? 'd' : 's')) throw new ArgumentException("로그인 진행 번역의 상수 폭/정밀도 오류");
            string text;
            if (argument != 2)
            {
                text = CString(argument switch { 0 => title, 1 => list, 3 => extra, _ => throw new ArgumentException("로그인 진행 번역의 초과 인자") }); if (precision >= 0 && precision < text.Length) text = text[..precision];
            }
            else
            {
                string digits = Math.Abs((long)remaining).ToString(CultureInfo.InvariantCulture); if (remaining == 0 && precision == 0) digits = "";
                if (precision >= 1024) throw new ArgumentOutOfRangeException(nameof(pattern), "로그인 진행 HTML1024바이트 초과");
                if (precision > digits.Length) digits = new string('0', precision - digits.Length) + digits;
                string sign = remaining < 0 ? "-" : plus ? "+" : blank ? " " : ""; text = sign + digits;
                if (zero && !left && precision < 0 && width > text.Length)
                {
                    if (width >= 1024) throw new ArgumentOutOfRangeException(nameof(pattern), "로그인 진행 HTML1024바이트 초과"); text = sign + new string('0', width - text.Length) + digits;
                }
            }
            if (width >= 1024 || output.Length + Math.Max(width, text.Length) >= 1024) throw new ArgumentOutOfRangeException(nameof(pattern), "로그인 진행 HTML1024바이트 초과");
            int padding = Math.Max(0, width - text.Length); if (!left) output.Append(' ', padding); output.Append(text); if (left) output.Append(' ', padding); argument++;
        }
        if (output.Length >= 1024) throw new ArgumentOutOfRangeException(nameof(pattern), "로그인 진행 HTML1024바이트 초과"); return output.ToString();
    }

    /// <summary>현재 Player/프레임 벽시각과 단일바이트 번역을 원문1024바이트 진행문구로 작성한다.</summary>
    public string ProgressText(double frameWall, Func<string, string>? translate)
    {
        string list = "";
        // 현재 subscriber가 있는 실제1..8의 이름과 현재 flags 상태를 순서대로 표시한다.
        for (uint owner = 1; owner < 9; owner++)
        {
            ReadOnlyMemory<byte> record = _players.Record(owner); if (Get(record.Span, 0x2c) == 0) continue; ReadOnlySpan<byte> name = record.Span.Slice(12, 32); int end = name.IndexOf((byte)0);
            if (end < 0) throw new ArgumentOutOfRangeException(nameof(owner), "로그인 진행 이름 NUL 누락"); list += Encoding.Latin1.GetString(name[..end]); uint flags = Get(record.Span, 0x50);
            string key = (flags & 2) == 0 ? ": ~iConnecting~." : (flags & 1) == 0 ? (flags & 0x100) != 0 ? ": ~iDisconnected~." : ": ~iPending~." : (flags & 0x40) == 0 ? ": ~iTransferring~." : ": ~iReady~.";
            list += Need(translate)(key); list += '\n'; if (list.Length >= 1024) throw new ArgumentOutOfRangeException(nameof(owner), "로그인 진행 명단1024바이트 초과");
        }
        double difference = _state.End.Value - frameWall; uint bits = !double.IsFinite(difference) || difference >= 9223372036854775808.0 || difference < -9223372036854775808.0 ? 0u : unchecked((uint)(long)difference); int remaining = unchecked((int)bits); if (remaining < 0) remaining = 0; string extra = "";
        if (remaining < 1) extra = Need(translate)("\n\nThere may be Internet traffic, or the Server\nmay have a slow Internet connection which is\ntaking longer than expected. You can abort\nthis game, or continue waiting to get in. Most\ngames reconnect correctly even after long delays.\n$ButtonInstant=Abort,LeaveBattle,3\n");
        string titleKey = "Starting Game";
        if (_reads.Server!())
        {
            uint disconnected = _players.Disconnected(), subscribers = _players.Subscribers(), pending = _players.Pending(), connecting = _players.Connecting(), ready = _players.Ready();
            if (unchecked(subscribers - disconnected - pending - connecting) == ready && unchecked((int)(unchecked((uint)_state.Timeout.Value) - unchecked((uint)remaining))) > 10)
                extra = Need(translate)("\n\nSome players are not connected and may\nnot make it back. Press Start to begin now.$ButtonInstant=Start,ServerForceBattleStart,0 ");
            if (_reads.Server!()) titleKey = "Becoming Server";
        }
        string title = Need(translate)(titleKey), pattern = Need(translate)("~.~3%s~*~.\n\n%s\nTime Remaining: ~i%d~.%s"); return FormatProgress(pattern, title, list, remaining, extra);
    }
}
