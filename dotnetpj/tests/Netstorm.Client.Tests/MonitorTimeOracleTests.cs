using System.Globalization;
using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Client.Tests; // 원문 CPU 관측의 입력과 기대값을 그대로 대조한다.

/// <summary>현재 시계/콜백 변경·NUL·부분 실패와 두 판본의 서식 물리를 검증한다.</summary>
public sealed class MonitorTimeOracleTests
{
    /// <summary>원문 double 비트의 부호와 NaN payload를 보존해 읽는다.</summary>
    private static double Double(string text) => BitConverter.UInt64BitsToDouble(ulong.Parse(text, CultureInfo.InvariantCulture));
    /// <summary>원문 BYTE 문자열을 TSV 밖의 고정16진수로 표현한다.</summary>
    private static string Hex(string text) => Convert.ToHexStringLower(Encoding.Latin1.GetBytes(text));
    /// <summary>동일한 명시 시계/달력/Config/제출 입력을 조립하며 기대 출력은 재계산하지 않는다.</summary>
    private static string[] Replay(string[] row)
    {
        var state = new MonitorTimeState { Initialized = X86Fixture.UInt(row[2]), Deadline = Double(row[3]), Monitor = X86Fixture.Int(row[4]), Alert = X86Fixture.Int(row[5]) };
        double wall = Double(row[6]), increment = Double(row[7]); uint calls = 0, changes = X86Fixture.UInt(row[9]);
        string trace = "", name = "Alpha";
        var hooks = new MonitorTimeHooks
        {
            WallSeconds = () =>
            {
                double now = wall; wall += increment; calls++;
                trace += "W," + BitConverter.DoubleToUInt64Bits(now).ToString(CultureInfo.InvariantCulture) + ';';
                if ((changes & 1) != 0 && calls == 1) state.Deadline = 10000; return now;
            },
            Calendar = () => { trace += "T;L;A;"; if ((changes & 2) != 0) name = "Bravo"; return "Wed Nov  4 01:00:00 2026\n"; },
            Handle = () => name,
            Line = (handle, calendar) => { trace += "P," + Hex("%s: !%s\n") + ',' + Hex(handle) + ',' + Hex(calendar) + ';'; return RawMonitorTime.FormatLine(handle, calendar); },
            Format = () => { trace += "Q," + Hex("monitorFormat") + ",0,0,0;"; if ((changes & 4) != 0) name = "Bravo"; return "~9~0"; },
            Submit = (format, line) =>
            {
                trace += "E," + Hex(format) + ',' + Hex(line) + ';';
                if ((changes & 8) != 0) { state.Monitor = 0; state.Alert = 0; }
                if ((changes & 16) != 0) state.Deadline = -4.25;
            },
        };
        // 한 원문 행 안의 모든 단계는 같은 실제 상태를 이어서 사용한다.
        for (uint step = 0; step < X86Fixture.UInt(row[8]); step++) RawMonitorTime.Step(state, hooks);
        return [state.Initialized.ToString(CultureInfo.InvariantCulture), BitConverter.DoubleToUInt64Bits(state.Deadline).ToString(CultureInfo.InvariantCulture),
            state.Monitor.ToString(CultureInfo.InvariantCulture), state.Alert.ToString(CultureInfo.InvariantCulture), calls.ToString(CultureInfo.InvariantCulture), trace.Length == 0 ? "-" : trace];
    }

    /// <summary>기존 원문1176행의 상태·정확한 시계 횟수·본문/서식·호출 순서를 모두 대조한다.</summary>
    [Fact]
    public void Native1176RowsPreserveCurrentCallbacksAndDeadlineBits()
    {
        string[][] rows = X86Fixture.Read("monitor-time-hardware"); Assert.Equal(1176, rows.Length);
        // 각 행은 독립 입력이며 C# 결과로 원본 기대값을 갱신하지 않는다.
        foreach (string[] row in rows)
        {
            Assert.Equal(17, row.Length); Assert.Equal(new[] { row[10], row[11], row[12], row[13], row[14], row[16] }, Replay(row));
            // CD의 무효과 atexit 등록 관측은 타이머 본체와 별도 판본 경계다.
            Assert.Equal(row[0] == "originalCD" && (X86Fixture.UInt(row[2]) & 1) == 0 ? 1u : 0u, X86Fixture.UInt(row[15]));
        }
    }

    /// <summary>같은 두 manager의 명시 초기 물리와 submit 변경 입력이다.</summary>
    private sealed class TemporaryScene
    {
        /// <summary>원문 토큰으로 시작하는 실제 현재 슬롯이다.</summary>
        public readonly UIntBox Current = new() { Value = 0x601000 };
        /// <summary>현재 토큰이 조회하는 서로 다른 두256BYTE다.</summary>
        public readonly Dictionary<uint, byte[]> Memory = new() { [0x601000] = Enumerable.Repeat((byte)0xab, 256).ToArray(), [0x602000] = Enumerable.Repeat((byte)0xcd, 256).ToArray() };
        /// <summary>원문 submit에서 관측한 임시 서식과 본문이다.</summary>
        public string Trace = "";
        /// <summary>원문 manager별 초기 front만 설정한다. 나머지 미작성 바이트는 그대로 둔다.</summary>
        public TemporaryScene(bool patch)
        {
            int offset = patch ? 0x14 : 0x10;
            Encoding.Latin1.GetBytes("saved-prefix\0").CopyTo(Memory[0x601000], offset);
            Encoding.Latin1.GetBytes("second-prefix\0").CopyTo(Memory[0x602000], offset);
        }
    }

    /// <summary>정상 제출 뒤 현재 manager 재조회·60/16BYTE 절단/패딩과 전체 물리12행을 대조한다.</summary>
    [Fact]
    public void Native12RowsPreserveTemporaryFormatAndCurrentManager()
    {
        string[][] rows = X86Fixture.Read("temporary-chat-format-hardware"); Assert.Equal(12, rows.Length);
        // 두 판본의 전체256BYTE 두 배열을 원문 출력 그대로 비교한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(8, row.Length); bool patch = row[0] == "originals"; var scene = new TemporaryScene(patch);
            int offset = patch ? 0x14 : 0x10; uint changes = X86Fixture.UInt(row[3]), pattern = X86Fixture.UInt(row[2]);
            string format = pattern == 0 ? "~9~0" : pattern == 1 ? new string('Z', 75) : "";
            RawMonitorTime.SubmitTemporary(scene.Current, format, "Alpha: !calendar\n\n", patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072, new()
            {
                Memory = token => scene.Memory[token],
                Submit = (token, line) =>
                {
                    ReadOnlySpan<byte> front = scene.Memory[token].AsSpan(offset); int end = front.IndexOf((byte)0);
                    scene.Trace += "S," + Convert.ToHexStringLower(front[..end]) + ',' + Hex(line) + ';';
                    if ((changes & 1) != 0) scene.Current.Value = 0x602000;
                },
            });
            Assert.Equal(row[4], scene.Current.Value == 0x602000 ? "1" : "0");
            Assert.Equal(row[5], Convert.ToHexStringLower(scene.Memory[0x601000]));
            Assert.Equal(row[6], Convert.ToHexStringLower(scene.Memory[0x602000])); Assert.Equal(row[7], scene.Trace);
        }
    }

    /// <summary>제출이 실패하면 원문에서 이미 쓴 임시 front가 남고 finally 복원은 실행하지 않는다.</summary>
    [Fact]
    public void FailedSubmitKeepsNativePartialFront()
    {
        var scene = new TemporaryScene(true); var failure = new IOException("명시 제출 실패");
        Assert.Same(failure, Assert.Throws<IOException>(() => RawMonitorTime.SubmitTemporary(scene.Current, new string('Z', 75), "x", OriginalEdition.Patch1078, new()
        { Memory = token => scene.Memory[token], Submit = (_, _) => throw failure })));
        string[] native = X86Fixture.Read("temporary-chat-format-hardware").Single(row => row[0] == "originals" && row[2] == "1" && row[3] == "1");
        Assert.Equal(native[5], Convert.ToHexStringLower(scene.Memory[0x601000])); Assert.Equal(0x601000u, scene.Current.Value);
    }

    /// <summary>첫NUL 경계·달력의 기존 개행·지역256BYTE 한계를 그대로 보존한다.</summary>
    [Fact]
    public void FixedFormatterKeepsCStringAndOriginalByteLimit()
    {
        Assert.Equal("Alpha: !calendar\n\n", RawMonitorTime.FormatLine("Alpha\0ignored", "calendar\n\0ignored"));
        Assert.Equal(255, RawMonitorTime.FormatLine(new string('A', 251), "").Length);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawMonitorTime.FormatLine(new string('A', 252), ""));
    }
}
