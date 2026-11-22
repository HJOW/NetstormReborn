// 7차(오류/예외 처리와 상세 로그) 공통 구조다. 원본에는 대응 코드가 없는 새 장치다.
// 사용자 홈 아래 logs\yyyyMMddHHmmss.log 에 비정상 종료 직전까지의 상세 정보를 남긴다.
// 2026-11-11 0.360에서 cpppj platform/ErrorLog.h/.cpp를 이식했다.
using System.Runtime.InteropServices;
using System.Text;

namespace Netstorm.Client.Host;

/// <summary>
/// 한 줄 기록의 심각도다. Info는 메모리 활동 기록에만 남고, Warning 이상이 처음 나오면 로그 파일이 만들어진다.
/// </summary>
/// <remarks>2026-11-11 0.360에서 cpppj <c>platform/ErrorLog.h</c>의 수준을 이식했다.</remarks>
public enum LogLevel
{
    /// <summary>정상 동작 중의 활동 기록이다(시작 정보·미션 진입 등). 파일을 만들지 않는다.</summary>
    Info,
    /// <summary>복귀 가능한 진단이다(예: 원문 assertion). 처음 나오면 로그 파일을 만들고 이전 활동 기록을 함께 쓴다.</summary>
    Warning,
    /// <summary>처리된 오류 또는 던져진 예외다. 즉시 디스크에 반영한다.</summary>
    Error,
    /// <summary>프로세스 종료로 이어지는 치명 오류다. 즉시 디스크에 반영한다.</summary>
    Fatal,
}

/// <summary>
/// 프로세스 전체가 공유하는 오류 로그다. 모든 함수는 스레드 안전하며, 구성되지 않았으면 메모리 기록만 하고 파일은 만들지 않는다
/// (cpppj <c>platform/ErrorLog</c>).
/// 사용 순서: 프로그램 시작에서 Configure → InstallCrashHandlers → CaptureStandardStreams, 이후 필요한 곳에서 Note/Record*를 호출한다.
/// </summary>
/// <remarks>2026-11-11 0.360에서 이식했다. Win32 SEH·CRT 갈고리 대신 .NET 처리되지 않은 예외를 받고, 표준 스트림은 Console 경계에서 복제한다.</remarks>
public static class ErrorLog
{
    /// <summary>메모리 활동 기록의 최대 줄 수다. 치명 오류 직전 상황을 충분히 남기되 메모리를 제한한다.</summary>
    private const int RingLimit = 1024;
    /// <summary>한 줄의 최대 문자 수다. 너무 긴 진단은 잘라 로그 파일이 한 줄로 폭주하지 않게 한다.</summary>
    private const int LineLimit = 4096;
    /// <summary>모든 기록 함수가 공유하는 잠금이다.</summary>
    private static readonly object Mutex = new();
    /// <summary>Configure가 호출됐는지 여부다.</summary>
    private static bool _configured;
    /// <summary>로그 파일을 만들 폴더의 절대 경로다.</summary>
    private static string _directory = "";
    /// <summary>제품 버전 문자열(예: 11.01)이다.</summary>
    private static string _version = "";
    /// <summary>열린 로그 파일 스트림이다. 아직 만들지 않았으면 null이다.</summary>
    private static FileStream? _file;
    /// <summary>쓸 때 쓰는 라이터다. 파일과 수명을 같이 한다.</summary>
    private static StreamWriter? _writer;
    /// <summary>만든 로그 파일 경로다.</summary>
    private static string _filePath = "";
    /// <summary>최근 활동 기록(한 줄씩)이다.</summary>
    private static readonly Queue<string> Ring = new();
    /// <summary>핸들러를 중복 설치하지 않기 위한 표식이다.</summary>
    private static bool _crashHandlersInstalled;
    /// <summary>표준 스트림 복제를 중복 설치하지 않기 위한 표식이다.</summary>
    private static bool _streamsCaptured;
    /// <summary>치명 기록의 재귀(핸들러 안의 추가 오류)를 막는 깊이다.</summary>
    private static int _fatalDepth;

    /// <summary>표준 스트림 한 줄을 원래 출력으로 전달하면서 메모리 활동 기록에도 복제한다.</summary>
    /// <param name="inner">원래 출력이다.</param><param name="tag">기록에 붙일 스트림 이름이다.</param>
    private sealed class TeeWriter(TextWriter inner, string tag) : TextWriter
    {
        /// <summary>원래 출력의 부호화를 그대로 쓴다.</summary>
        public override Encoding Encoding => inner.Encoding;
        /// <summary>아직 개행을 받지 못한 현재 줄이다.</summary>
        private readonly StringBuilder _line = new();
        /// <summary>줄 모으기와 전달을 보호한다.</summary>
        private readonly object _gate = new();
        /// <summary>한 글자를 원래 출력으로 보내고 줄을 모은다.</summary>
        /// <param name="value">쓸 글자다.</param>
        public override void Write(char value)
        {
            lock (_gate)
            {
                inner.Write(value);
                Collect(value);
            }
        }
        /// <summary>여러 글자를 원래 출력으로 보내고 줄을 모은다.</summary>
        /// <param name="value">쓸 문자열이다.</param>
        public override void Write(string? value)
        {
            if (value is null)
            {
                return;
            }
            lock (_gate)
            {
                inner.Write(value);
                // 받은 글자를 줄 단위로 모으는 반복이다
                foreach (char ch in value)
                {
                    Collect(ch);
                }
            }
        }
        /// <summary>동기화 요청을 원래 출력으로 넘긴다.</summary>
        public override void Flush()
        {
            lock (_gate)
            {
                inner.Flush();
            }
        }
        /// <summary>개행을 만나면 모은 줄을 활동 기록으로 보낸다.</summary>
        /// <param name="value">받은 글자다.</param>
        private void Collect(char value)
        {
            if (value == '\n')
            {
                FlushLine();
                return;
            }
            if (_line.Length < LineLimit)
            {
                _line.Append(value);
            }
        }
        /// <summary>모은 줄을 활동 기록으로 보낸다.</summary>
        private void FlushLine()
        {
            if (_line.Length != 0)
            {
                Note("[" + tag + "] " + _line);
                _line.Clear();
            }
        }
        /// <summary>남은 줄을 버리고 원래 출력을 닫지 않고 비운다.</summary>
        /// <param name="disposing">관리 자원 정리 여부다.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                lock (_gate)
                {
                    FlushLine();
                    inner.Flush();
                }
            }
        }
    }

    /// <summary>현재 지역 시각으로 "yyyy-MM-dd HH:mm:ss.fff" 문자열을 만든다.</summary>
    /// <returns>시각 문자열이다.</returns>
    private static string Timestamp() => DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fff", System.Globalization.CultureInfo.InvariantCulture);

    /// <summary>심각도 이름이다.</summary>
    /// <param name="level">수준이다.</param>
    /// <returns>고정 폭 이름이다.</returns>
    private static string LevelName(LogLevel level) => level switch
    {
        LogLevel.Info => "INFO ",
        LogLevel.Warning => "WARN ",
        LogLevel.Error => "ERROR",
        _ => "FATAL",
    };

    /// <summary>한 줄을 만든다. 개행은 공백 네 칸 들여쓰기로 접어 한 기록이 이어지는 줄로 남는다.</summary>
    /// <param name="level">수준이다.</param><param name="category">분류다.</param><param name="message">본문이다.</param>
    /// <returns>기록 한 줄이다.</returns>
    private static string FormatLine(LogLevel level, string category, string message)
    {
        // 다중 줄 메시지의 전체 상한이다
        string text = message.Length > LineLimit * 8 ? message[..(LineLimit * 8)] : message;
        var line = new StringBuilder();
        line.Append('[').Append(Timestamp()).Append("] [").Append(LevelName(level)).Append("] [").Append(category).Append("] ");
        // 이어지는 줄의 앞에 들여쓰기를 붙이는 반복이다
        for (int i = 0; i < text.Length; i++)
        {
            line.Append(text[i]);
            if (text[i] == '\n' && i + 1 < text.Length)
            {
                line.Append("    ");
            }
        }
        return line.ToString();
    }

    /// <summary>파일을 연다(없으면 폴더부터 만든다). 이미 열려 있으면 아무 것도 하지 않는다. 잠금은 호출자가 잡는다.</summary>
    /// <returns>열려 있으면 참이다.</returns>
    private static bool EnsureOpenLocked()
    {
        if (_file is not null)
        {
            return true;
        }
        if (!_configured || _directory.Length == 0)
        {
            return false;
        }
        UserDataRoot.EnsureDirectory(_directory);
        DateTime now = DateTime.Now;
        string path = Path.Combine(_directory, FileNameFor(now.Year, now.Month, now.Day, now.Hour, now.Minute, now.Second));
        bool existed = File.Exists(path);
        // 같은 초에 만든 같은 이름이 있어도 덮어쓰지 않고 이어 쓴다
        _file = new FileStream(path, FileMode.Append, FileAccess.Write, FileShare.ReadWrite);
        if (!existed)
        {
            // 메모장 등이 UTF-8로 인식하도록 BOM을 붙인다
            byte[] preamble = Encoding.UTF8.GetPreamble();
            _file.Write(preamble, 0, preamble.Length);
        }
        _writer = new StreamWriter(_file, new UTF8Encoding(false), 4096, leaveOpen: true);
        if (_filePath.Length == 0)
        {
            _filePath = Path.GetFullPath(path);
        }
        return true;
    }

    /// <summary>파일 머리말(처음 한 번)이다. 제품·프로세스·OS 정보를 남긴다.</summary>
    /// <returns>머리말 본문이다.</returns>
    private static string HeaderText()
    {
        var text = new StringBuilder();
        text.Append("==== NetStorm 오류 로그 ====\n");
        text.Append("제품 버전: ").Append(_version).Append('\n');
        text.Append("프로세스 ID: ").Append(Environment.ProcessId).Append('\n');
        text.Append("실행 파일: ").Append(Environment.ProcessPath ?? "").Append('\n');
        text.Append("작업 폴더: ").Append(Environment.CurrentDirectory).Append('\n');
        text.Append("명령줄: ").Append(Environment.CommandLine).Append('\n');
        text.Append("운영체제: ").Append(RuntimeInformation.OSDescription).Append(", ").Append(RuntimeInformation.OSArchitecture).Append('\n');
        text.Append("로그 시작: ").Append(Timestamp()).Append('\n');
        return text.ToString();
    }

    /// <summary>파일이 처음 만들어질 때 머리말과 이전 활동 기록을 쓴다. 잠금은 호출자가 잡는다.</summary>
    private static void WriteHeaderAndRingLocked()
    {
        _writer!.Write(HeaderText());
        _writer.Write("---- 이전 활동 기록(최근 " + Ring.Count + "줄) ----\n");
        // 메모리 활동 기록을 시간 순서대로 파일에 옮기는 반복이다
        foreach (string line in Ring)
        {
            _writer.Write(line + "\n");
        }
        _writer.Write("---- 이후 기록 ----\n");
    }

    /// <summary>메모리 활동 기록에 한 줄을 넣고 오래된 줄을 버린다. 잠금은 호출자가 잡는다.</summary>
    /// <param name="line">넣을 줄이다.</param>
    private static void PushRingLocked(string line)
    {
        Ring.Enqueue(line.Length > LineLimit ? line[..LineLimit] + "...(잘림)" : line);
        // 상한을 넘으면 가장 오래된 줄부터 버리는 반복이다
        while (Ring.Count > RingLimit)
        {
            Ring.Dequeue();
        }
    }

    /// <summary>한 줄을 기록한다. 수준이 경고 이상이면 파일에 쓰고 오류 이상이면 디스크에 반영한다.</summary>
    /// <param name="level">수준이다.</param><param name="category">분류다.</param><param name="message">본문이다.</param>
    private static void RecordLine(LogLevel level, string category, string message)
    {
        string line = FormatLine(level, category, message);
        lock (Mutex)
        {
            // 아직 파일이 없고 경고 이상이면 파일을 만들고 머리말과 이전 활동 기록을 먼저 쓴다
            if (_file is null && level >= LogLevel.Warning && EnsureOpenLocked())
            {
                WriteHeaderAndRingLocked();
            }
            if (_file is not null)
            {
                // 파일이 열려 있으면 낮은 수준도 이어서 남긴다. 관리 버퍼는 매번 비워 OS 버퍼까지만 전진한다.
                _writer!.Write(line + "\n");
                _writer.Flush();
                if (level >= LogLevel.Error)
                {
                    _file.Flush(true);
                }
            }
            else
            {
                // 파일이 없으면(경고 미만이거나 구성되지 않음) 메모리 활동 기록에만 둔다
                PushRingLocked(line);
            }
        }
    }

    /// <summary>예외 한 개와 내부 예외를 문자열로 푼다. 깊이는 무한 중첩을 막는 한계다.</summary>
    /// <param name="error">풀 예외다.</param><param name="depth">현재 깊이다.</param>
    /// <returns>설명 문자열이다.</returns>
    private static string DescribeException(Exception error, int depth = 0)
    {
        if (depth > 8)
        {
            return "(중첩 예외가 너무 깊어 생략)";
        }
        string text = (error.GetType().FullName ?? error.GetType().Name) + ": " + error.Message;
        if (error.InnerException is not null)
        {
            text += "\n  원인: " + DescribeException(error.InnerException, depth + 1);
        }
        return text;
    }

    /// <summary>로그 폴더와 제품 버전을 지정한다. 파일은 만들지 않으며 폴더도 첫 파일 기록 때 만든다.</summary>
    /// <param name="logsDirectory">로그 폴더다.</param><param name="productVersion">제품 버전이다.</param>
    /// <remarks>이미 구성돼 있으면 열린 파일을 닫고 새 폴더로 다시 구성한다(검사용 재구성 포함).</remarks>
    public static void Configure(string logsDirectory, string productVersion)
    {
        ArgumentNullException.ThrowIfNull(logsDirectory);
        ArgumentNullException.ThrowIfNull(productVersion);
        lock (Mutex)
        {
            CloseLocked();
            _filePath = "";
            _directory = Path.GetFullPath(logsDirectory.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar));
            _version = productVersion;
            _configured = _directory.Length != 0;
            Interlocked.Exchange(ref _fatalDepth, 0);
        }
    }

    /// <summary>구성 여부다. 거짓이면 기록은 메모리 기록만 한다.</summary>
    /// <returns>구성됐으면 참이다.</returns>
    public static bool Configured()
    {
        lock (Mutex)
        {
            return _configured;
        }
    }

    /// <summary>정상 활동 한 줄을 메모리 기록(최근 1024줄)에만 남긴다. 파일을 만들지 않으며 치명 오류 때 앞부분으로 함께 기록된다.</summary>
    /// <param name="text">남길 본문이다.</param>
    public static void Note(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        RecordLine(LogLevel.Info, "info", text);
    }

    /// <summary>지정한 심각도로 한 줄을 남긴다. 경고 이상이면 파일을 만들고(처음 한 번) 이전 활동 기록을 먼저 쓴다.</summary>
    /// <param name="level">수준이다.</param><param name="category">분류다.</param><param name="message">본문이다.</param>
    public static void Record(LogLevel level, string category, string message)
    {
        ArgumentNullException.ThrowIfNull(category);
        ArgumentNullException.ThrowIfNull(message);
        RecordLine(level, category, message);
    }

    /// <summary>처리된 예외나 던져진 예외를 형식 이름·메시지·내부 예외까지 풀어 기록한다. 위치는 발생·포획 위치 설명이다.</summary>
    /// <param name="where">위치 설명이다.</param><param name="error">풀 예외다. 없으면 그 취지로 기록한다.</param><param name="level">기록 수준이다.</param>
    public static void RecordException(string where, Exception? error, LogLevel level = LogLevel.Error)
    {
        ArgumentNullException.ThrowIfNull(where);
        RecordLine(level, "exception", where + ": " + (error is null ? "(예외 없음)" : DescribeException(error)));
    }

    /// <summary>원문 assertion의 표현식·파일·줄을 경고로 기록한다. 환경 변수 NETSTORM_ASSERT_WINDOW=1이면 확인 창도 띄운다.</summary>
    /// <param name="expression">표현식이다.</param><param name="file">파일이다.</param><param name="line">줄이다.</param>
    /// <remarks>무인 실행을 막지 않도록 기본은 창을 띄우지 않는다.</remarks>
    public static void RecordAssertion(string expression, string file, uint line)
    {
        ArgumentNullException.ThrowIfNull(expression);
        ArgumentNullException.ThrowIfNull(file);
        string text = file + ":" + line + " " + expression;
        RecordLine(LogLevel.Warning, "assertion", text);
        if (Environment.GetEnvironmentVariable("NETSTORM_ASSERT_WINDOW") == "1" && RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            const uint MB_ICONERROR = 0x10, MB_OKCANCEL = 0x1, MB_TOPMOST = 0x40000;
            const int IDCANCEL = 2;
            if (MessageBoxW(IntPtr.Zero, text, "NetStorm assertion", MB_ICONERROR | MB_OKCANCEL | MB_TOPMOST) == IDCANCEL)
            {
                RecordFatal("assertion 창", "사용자가 취소를 선택해 종료합니다: " + text);
                Environment.Exit(3);
            }
        }
    }

    /// <summary>확인 창이다. assert 창 표시에만 쓴다.</summary>
    /// <param name="owner">소유 창, 없으면 0이다.</param><param name="text">본문이다.</param><param name="caption">제목이다.</param><param name="style">형식 비트다.</param>
    /// <returns>누른 단추 번호다.</returns>
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int MessageBoxW(IntPtr owner, string text, string caption, uint style);

    /// <summary>종료 직전 원인을 치명으로 기록하고 디스크에 반영한다. 호출 위치의 설명도 함께 남긴다.</summary>
    /// <param name="where">위치 설명이다.</param><param name="cause">원인이다.</param>
    public static void RecordFatal(string where, string cause)
    {
        ArgumentNullException.ThrowIfNull(where);
        ArgumentNullException.ThrowIfNull(cause);
        RecordLine(LogLevel.Fatal, "fatal", where + ": " + cause);
        lock (Mutex)
        {
            _writer?.Flush();
            _file?.Flush(true);
        }
    }

    /// <summary>처리되지 않은 예외를 로그로 남기는 핸들러를 한 번만 설치한다. 기록 뒤에는 기본 종료에 맡긴다.</summary>
    public static void InstallCrashHandlers()
    {
        lock (Mutex)
        {
            if (_crashHandlersInstalled)
            {
                return;
            }
            _crashHandlersInstalled = true;
        }
        AppDomain.CurrentDomain.UnhandledException += (_, arguments) =>
        {
            // 핸들러 안에서 다시 충돌하면 재귀하지 않는다
            if (Interlocked.Increment(ref _fatalDepth) > 1)
            {
                return;
            }
            object? data = arguments.ExceptionObject;
            RecordFatal("unhandled", data is Exception error ? DescribeException(error) : "알 수 없는 형식의 종료 원인");
        };
    }

    /// <summary>표준 출력·오류의 모든 줄을 원래 출력과 함께 메모리 활동 기록으로 복제한다. 콘솔 출력은 바뀌지 않는다.</summary>
    public static void CaptureStandardStreams()
    {
        lock (Mutex)
        {
            if (_streamsCaptured)
            {
                return;
            }
            _streamsCaptured = true;
        }
        // 프로세스 끝까지 유지하는 의도적 보관이다
        Console.SetOut(new TeeWriter(Console.Out, "stdout"));
        Console.SetError(new TeeWriter(Console.Error, "stderr"));
    }

    /// <summary>지금까지 만든 로그 파일 경로다. 아직 아무것도 기록하지 않았으면 빈 문자열이다.</summary>
    /// <returns>파일 경로 또는 빈 문자열이다.</returns>
    public static string CurrentFile()
    {
        lock (Mutex)
        {
            return _filePath;
        }
    }

    /// <summary>메모리 활동 기록에 남아 있는 줄 수다(검사용).</summary>
    /// <returns>줄 수다.</returns>
    public static int BufferedLineCount()
    {
        lock (Mutex)
        {
            return Ring.Count;
        }
    }

    /// <summary>로그 파일 이름 yyyyMMddHHmmss.log를 계산한다(요구 사양의 이름 규칙, 검사용으로 공개).</summary>
    /// <param name="year">연이다.</param><param name="month">월이다.</param><param name="day">일이다.</param>
    /// <param name="hour">시다.</param><param name="minute">분이다.</param><param name="second">초다.</param>
    /// <returns>파일 이름이다.</returns>
    public static string FileNameFor(int year, int month, int day, int hour, int minute, int second) =>
        $"{year:D4}{month:D2}{day:D2}{hour:D2}{minute:D2}{second:D2}.log";

    /// <summary>구성·열린 파일·메모리 기록을 모두 초기화한다(검사용). 설치된 핸들러는 그대로 둔다.</summary>
    public static void ResetForTesting()
    {
        lock (Mutex)
        {
            CloseLocked();
            _filePath = "";
            _directory = "";
            _version = "";
            Ring.Clear();
            _configured = false;
            Interlocked.Exchange(ref _fatalDepth, 0);
        }
    }

    /// <summary>열린 파일을 닫고 라이터를 버린다. 잠금은 호출자가 잡는다.</summary>
    private static void CloseLocked()
    {
        _writer?.Dispose();
        _writer = null;
        _file?.Dispose();
        _file = null;
    }

    /// <summary>개발 검사용: 지정한 종류로 의도적으로 비정상 종료한다. throw·인자 오류 종류만 프로세스 안에서 검사할 수 있다.</summary>
    /// <param name="kind">throw·terminate·access·divzero·abort·stack·invalid·purecall 중 하나다.</param>
    /// <exception cref="ArgumentException">알 수 없는 종류다.</exception>
    /// <remarks>
    /// throw는 잡을 수 있는 예외라 단위 검사에서 확인한다. terminate·access·divzero·abort·stack은 프로세스를 끝내므로
    /// 별도 프로세스에서 손으로만 확인한다. invalid·purecall에 대응하는 .NET 고유 경로가 없어 지원하지 않는다.
    /// </remarks>
    public static void CrashForTest(string kind)
    {
        ArgumentNullException.ThrowIfNull(kind);
        switch (kind)
        {
            case "throw":
                throw new InvalidOperationException("로그 검사용 의도적 예외");
            case "terminate":
                Environment.FailFast("로그 검사용 terminate");
                break;
            case "access":
                Marshal.ReadInt32(IntPtr.Zero);
                break;
            case "divzero":
            {
                int zero = 0;
                _ = 1 / zero;
                break;
            }
            case "abort":
                Environment.Exit(3);
                break;
            case "stack":
                CauseStackOverflow(0);
                break;
            case "invalid":
            case "purecall":
                throw new NotSupportedException("CRT 고유 경로라 .NET에서 지원하지 않는다: " + kind);
            default:
                throw new ArgumentException("알 수 없는 로그 검사 종류", nameof(kind));
        }
    }

    /// <summary>스택 오버플로 시험용 의도적 무한 재귀다.</summary>
    /// <param name="depth">현재 깊이다.</param>
    /// <returns>도달하지 않는다.</returns>
    private static int CauseStackOverflow(int depth)
    {
        Span<byte> pad = stackalloc byte[512];
        pad[0] = (byte)depth;
        return CauseStackOverflow(depth + 1) + pad[0];
    }
}
