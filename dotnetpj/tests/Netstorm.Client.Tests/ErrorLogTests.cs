using System.Text;
using Netstorm.Client.Host;

namespace Netstorm.Client.Tests;

/// <summary>
/// 오류 로그의 파일 이름·지연 생성·수준·예외 풀이·스트림 복제를 검사한다
/// (cpppj <c>platform/ErrorLog</c>의 계약을 직접 작성했다).
/// </summary>
/// <remarks>2026-11-11 0.360에서 7차 로그 이식과 함께 추가했다. 실제 사용자 홈을 건드리지 않고 임시 폴더만 쓴다. 프로세스를 끝내는 종류는 검사하지 않는다.</remarks>
[Collection("ErrorLogSerial")]
public sealed class ErrorLogTests
{
    /// <summary>검사 전 상태를 비우고 검사 뒤 임시 폴더를 지운다.</summary>
    /// <returns>임시 로그 폴더다.</returns>
    private static string Begin(out string temp)
    {
        ErrorLog.ResetForTesting();
        temp = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
        return Path.Combine(temp, "logs");
    }

    /// <summary>파일을 닫고 임시 폴더를 지운다.</summary>
    /// <param name="temp">지울 폴더다.</param>
    private static void End(string temp)
    {
        ErrorLog.ResetForTesting();
        if (Directory.Exists(temp))
        {
            Directory.Delete(temp, true);
        }
    }

    /// <summary>열린 로그 파일을 다른 리더와 공유하며 읽는다.</summary>
    /// <param name="file">읽을 파일이다.</param>
    /// <returns>전체 본문이다.</returns>
    private static string ReadLog(string file)
    {
        using var stream = new FileStream(file, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
        using var reader = new StreamReader(stream, Encoding.UTF8);
        return reader.ReadToEnd();
    }

    /// <summary>파일 이름은 yyyyMMddHHmmss.log 규칙이다.</summary>
    [Fact]
    public void FileNameMatchesRequiredRule()
    {
        Assert.Equal("20261110141502.log", ErrorLog.FileNameFor(2026, 11, 10, 14, 15, 2));
    }

    /// <summary>활동 기록만으로는 파일을 만들지 않는다.</summary>
    [Fact]
    public void NoteOnlyKeepsMemoryBuffer()
    {
        string logs = Begin(out string temp);
        try
        {
            ErrorLog.Configure(logs, "11.01");
            Assert.True(ErrorLog.Configured());
            ErrorLog.Note("start");
            ErrorLog.Note("enter");
            Assert.Equal("", ErrorLog.CurrentFile());
            Assert.False(Directory.Exists(logs));
            Assert.Equal(2, ErrorLog.BufferedLineCount());
        }
        finally
        {
            End(temp);
        }
    }

    /// <summary>경고가 처음 나오면 파일을 만들고 머리말·이전 기록·본문을 쓴다.</summary>
    [Fact]
    public void WarningCreatesFileWithHeaderAndRing()
    {
        string logs = Begin(out string temp);
        try
        {
            ErrorLog.Configure(logs, "11.01");
            ErrorLog.Note("before");
            ErrorLog.Record(LogLevel.Warning, "test", "warn body");
            string file = ErrorLog.CurrentFile();
            Assert.NotEqual("", file);
            Assert.Equal("logs", new DirectoryInfo(Path.GetDirectoryName(file)!).Name);
            Assert.Matches(@"^\d{14}\.log$", Path.GetFileName(file));
            string text = ReadLog(file);
            Assert.Contains("11.01", text);
            Assert.Contains("WARN", text);
            Assert.Contains("warn body", text);
            Assert.Contains("before", text);
            // 파일이 열린 뒤의 낮은 수준도 이어서 남는다
            ErrorLog.Note("after");
            ErrorLog.Record(LogLevel.Error, "test", "error body");
            text = ReadLog(file);
            Assert.Contains("after", text);
            Assert.Contains("ERROR", text);
            Assert.Contains("error body", text);
        }
        finally
        {
            End(temp);
        }
    }

    /// <summary>예외는 형식·메시지·내부 원인까지 풀어 기록한다.</summary>
    [Fact]
    public void ExceptionUnfoldsTypeMessageAndInnerCause()
    {
        string logs = Begin(out string temp);
        try
        {
            ErrorLog.Configure(logs, "11.01");
            var inner = new InvalidOperationException("inner cause");
            ErrorLog.RecordException("spot", new ArgumentException("outer", inner));
            string text = ReadLog(ErrorLog.CurrentFile());
            Assert.Contains("spot", text);
            Assert.Contains("outer", text);
            Assert.Contains("원인", text);
            Assert.Contains("inner cause", text);
            ErrorLog.RecordException("empty", null);
            text = ReadLog(ErrorLog.CurrentFile());
            Assert.Contains("(예외 없음)", text);
        }
        finally
        {
            End(temp);
        }
    }

    /// <summary>assertion은 경고로 기록되며 기본은 창을 띄우지 않는다.</summary>
    [Fact]
    public void AssertionRecordsWarningWithoutWindow()
    {
        string? saved = Environment.GetEnvironmentVariable("NETSTORM_ASSERT_WINDOW");
        Environment.SetEnvironmentVariable("NETSTORM_ASSERT_WINDOW", null);
        string logs = Begin(out string temp);
        try
        {
            ErrorLog.Configure(logs, "11.01");
            ErrorLog.RecordAssertion("x > 0", "a.cpp", 7);
            string text = ReadLog(ErrorLog.CurrentFile());
            Assert.Contains("WARN", text);
            Assert.Contains("a.cpp:7 x > 0", text);
        }
        finally
        {
            Environment.SetEnvironmentVariable("NETSTORM_ASSERT_WINDOW", saved);
            End(temp);
        }
    }

    /// <summary>재구성하면 열린 파일을 닫고 새 폴더로 바꾼다.</summary>
    [Fact]
    public void ReconfigureSwitchesDirectory()
    {
        string logs = Begin(out string temp);
        try
        {
            ErrorLog.Configure(logs, "11.01");
            ErrorLog.Record(LogLevel.Warning, "test", "first");
            string first = ErrorLog.CurrentFile();
            string other = Path.Combine(temp, "other");
            ErrorLog.Configure(other, "11.01");
            ErrorLog.Record(LogLevel.Warning, "test", "second");
            string second = ErrorLog.CurrentFile();
            Assert.NotEqual(first, second);
            Assert.StartsWith(Path.GetFullPath(other), second);
            Assert.DoesNotContain("second", ReadLog(first));
        }
        finally
        {
            End(temp);
        }
    }

    /// <summary>핸들러 설치는 반복해도 한 번만 된다.</summary>
    [Fact]
    public void CrashHandlersInstallOnce()
    {
        try
        {
            ErrorLog.ResetForTesting();
            ErrorLog.InstallCrashHandlers();
            ErrorLog.InstallCrashHandlers();
        }
        finally
        {
            ErrorLog.ResetForTesting();
        }
    }

    /// <summary>표준 출력 복제는 원래 출력과 메모리 기록에 함께 남긴다.</summary>
    [Fact]
    public void StandardStreamsTeeToBuffer()
    {
        TextWriter saved = Console.Out;
        var inner = new StringWriter();
        Console.SetOut(inner);
        string logs = Begin(out string temp);
        try
        {
            ErrorLog.Configure(logs, "11.01");
            int before = ErrorLog.BufferedLineCount();
            ErrorLog.CaptureStandardStreams();
            ErrorLog.CaptureStandardStreams();
            Console.WriteLine("hello-tee");
            Assert.Contains("hello-tee", inner.ToString());
            // 같은 창에 떠 있는 다른 검사의 출력이 함께 복제될 수 있어 증가분만 본다
            Assert.True(ErrorLog.BufferedLineCount() > before);
        }
        finally
        {
            Console.SetOut(saved);
            End(temp);
        }
    }

    /// <summary>throw 종류는 잡을 수 있고 모르는 종류는 거부한다.</summary>
    [Fact]
    public void CrashForTestThrowIsCatchable()
    {
        Assert.Throws<InvalidOperationException>(() => ErrorLog.CrashForTest("throw"));
        Assert.Throws<ArgumentException>(() => ErrorLog.CrashForTest("nope"));
    }
}
