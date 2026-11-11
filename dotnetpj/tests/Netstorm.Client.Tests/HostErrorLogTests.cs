using System.Text;
using Netstorm.Client.Host;

namespace Netstorm.Client.Tests;

/// <summary>
/// 호스트 시작의 로그 구성과 최상위 실패 기록을 검사한다
/// (cpppj <c>app/main.cpp</c>의 시작·종료 처리를 직접 작성했다).
/// </summary>
/// <remarks>2026-11-11 0.361에서 시작 배선 이식과 함께 추가했다. 게임 실행 없이 검사한다.</remarks>
[Collection("ErrorLogSerial")]
public sealed class HostErrorLogTests
{
    /// <summary>환경 변수를 잠시 바꾸고 반드시 되돌린다.</summary>
    /// <param name="name">변수 이름이다.</param><param name="value">임시 값이다.</param><param name="body">실행할 검사다.</param>
    private static void WithEnvironment(string name, string? value, Action body)
    {
        string? saved = Environment.GetEnvironmentVariable(name);
        Environment.SetEnvironmentVariable(name, value);
        try
        {
            body();
        }
        finally
        {
            Environment.SetEnvironmentVariable(name, saved);
        }
    }

    /// <summary>시작 구성은 로그 폴더를 정하고 시작 기록을 남긴다.</summary>
    [Fact]
    public void SetupConfiguresLogsAndNotesStart()
    {
        string temp = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
        try
        {
            WithEnvironment(UserDataRoot.OverrideVariable, temp, HostErrorLog.Setup);
            Assert.True(ErrorLog.Configured());
            // 시작 기록만으로는 파일을 만들지 않는다
            Assert.Equal("", ErrorLog.CurrentFile());
            Assert.True(ErrorLog.BufferedLineCount() >= 1);
        }
        finally
        {
            ErrorLog.ResetForTesting();
            if (Directory.Exists(temp)) Directory.Delete(temp, true);
        }
    }

    /// <summary>최상위 실패는 치명 기록과 로그 경로 안내를 돌려준다.</summary>
    [Fact]
    public void TopLevelFailureRecordsFatalWithPath()
    {
        string temp = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
        try
        {
            WithEnvironment(UserDataRoot.OverrideVariable, temp, HostErrorLog.Setup);
            string message = HostErrorLog.RecordTopLevelFailure(new InvalidOperationException("boom"));
            Assert.Contains("boom", message);
            Assert.Contains("상세: 로그", message);
            Assert.Contains(ErrorLog.CurrentFile(), message);
            string file = ErrorLog.CurrentFile();
            Assert.NotEqual("", file);
            using var stream = new FileStream(file, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            using var reader = new StreamReader(stream, Encoding.UTF8);
            string text = reader.ReadToEnd();
            Assert.Contains("FATAL", text);
            Assert.Contains("boom", text);
            string unknown = HostErrorLog.RecordUnknownFailure();
            Assert.Contains("알 수 없는 형식", unknown);
            Assert.Contains("상세: 로그", unknown);
        }
        finally
        {
            ErrorLog.ResetForTesting();
            if (Directory.Exists(temp)) Directory.Delete(temp, true);
        }
    }
}
