using Netstorm.Client.Input;
using Netstorm.Client.Menu;

namespace Netstorm.Client.Tests;

/// <summary>
/// 보호 원문의 signed 255 비교·현재 모드·뗌·조회 실패 결과 전달을 대조한다
/// (cpppj <c>tests/AddRandomCommandTests.cpp</c>의 `AddRandom_Native288InputAndActualQueue`·`AddRandom_OnlyReachedDevicesAreRequired`를 직역했다).
/// </summary>
/// <remarks>2026-11-03 F2c-10에서 추가했다. 원본 x86 fixture로 진입점 전체를 관통하므로 PORT_MAP 상태는 L1이다. 대본 GameMenu 경로는 R6에서 잇는다.</remarks>
public sealed class AddRandomInputTests
{
    /// <summary>TSV의 입력·관측 순서를 보존한다.</summary>
    /// <param name="line">한 행이다.</param>
    /// <returns>탭 분리 칸이다.</returns>
    private static string[] AddFields(string line)
    {
        List<string> result = [];
        int begin = 0;
        // 마지막 빈 칸도 보존한다
        while (true)
        {
            int end = line.IndexOf('\t', begin);
            if (end < 0)
            {
                result.Add(line[begin..]);
                return [.. result];
            }
            result.Add(line[begin..end]);
            begin = end + 1;
        }
    }

    /// <summary>같은 원시 DWORD를 넘침 없이 복원한다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    /// <returns>원본 DWORD 값이다.</returns>
    private static uint AddNumber(string value)
    {
        return uint.Parse(value, System.Globalization.CultureInfo.InvariantCulture);
    }

    /// <summary>실제 경계 호출을 원문과 같은 구분 순서로 기록한다.</summary>
    /// <param name="trace">쌓을 기록이다.</param>
    /// <param name="entry">추가할 항목이다.</param>
    private static void AddTrace(System.Text.StringBuilder trace, string entry)
    {
        if (trace.Length > 0)
        {
            trace.Append(';');
        }
        trace.Append(entry);
    }

    /// <summary>보호 원문의 signed 255 비교·현재 모드·뗌·조회 실패 결과 전달을 대조한다.</summary>
    [Fact]
    public void Native288InputAndActualQueue()
    {
        string[][] rows = Netstorm.Tests.X86Fixture.Read("add-random-input");
        int count = 0;
        // 앞 4열만 입력으로 사용하고 뒤 관측은 기대값으로 읽는다
        foreach (string[] row in rows)
        {
            if (row.Length > 0 && row[0] == "event")
            {
                continue;
            }
            Assert.Equal(7, row.Length);
            uint code = AddNumber(row[0]);
            uint parameter = AddNumber(row[1]);
            uint mode = AddNumber(row[2]);
            int lookup = unchecked((int)AddNumber(row[3]));
            System.Text.StringBuilder trace = new();
            InputEvent evt = new(code, 305419896, unchecked((int)parameter));
            bool consumed = AddRandomInput.DispatchAddRandomInput(evt, (mode & 1) != 0, (mode & 2) != 0,
                token =>
                {
                    AddTrace(trace, "lookup:" + token);
                    return lookup;
                },
                (type, quantity) => AddTrace(trace, "add:" + type + ":" + quantity));
            Assert.True(consumed);
            Assert.Equal(row[4], trace.Length == 0 ? "-" : trace.ToString());
            Assert.Equal("1", row[5]);
            Assert.Equal("0", row[6]);
            count++;
        }
        Assert.Equal(288, count);
    }

    /// <summary>선택되지 않은 하위와 이미 실행된 벽시각 효과를 잘못 처리하지 않는다.</summary>
    [Fact]
    public void OnlyReachedDevicesAreRequired()
    {
        Assert.False(AddRandomInput.DispatchAddRandomInput(new InputEvent(0x02000017u, 0, 100), false, false, null, null));
        Assert.True(AddRandomInput.DispatchAddRandomInput(new InputEvent(AddRandomInput.AddRandomCommand | InputCode.Release, 0, 100), true, true, null, null));
        Assert.True(AddRandomInput.DispatchAddRandomInput(new InputEvent(AddRandomInput.AddRandomCommand, 0, 100), false, false, null, null));
        bool missing = false;
        try
        {
            AddRandomInput.DispatchAddRandomInput(new InputEvent(AddRandomInput.AddRandomCommand, 0, 100), true, false, null, null);
        }
        catch (InvalidOperationException)
        {
            missing = true;
        }
        Assert.True(missing);
    }
}
