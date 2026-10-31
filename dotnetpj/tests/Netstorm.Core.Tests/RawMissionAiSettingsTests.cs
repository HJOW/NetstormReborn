using System.Globalization;
using System.Text;
using Netstorm.Core.Original;

// 원본 동작과 미션 설정 연결을 검증한다.
namespace Netstorm.Tests;

/// <summary>미션 AI의 600바이트 설정과 설정 조회 순서를 원본 x86 결과에 대조한다.</summary>
public sealed class RawMissionAiSettingsTests
{
    /// <summary>하나의 독립 입력을 실제 설정 처리기에 넣고 결과 바이트 및 효과를 돌려준다.</summary>
    /// <param name="row">원본 기대값 행.</param>
    private static bool Matches(string[] row)
    {
        string[] fields = row[1].Split('|');
        long[] numbers = fields[0].Split(',').Select(long.Parse).ToArray();
        var config = new Dictionary<string, string>(StringComparer.Ordinal);
        // 설정 값은 원본 바이트 hex를 Latin1 문자열로 보존한다.
        foreach (string field in fields.Skip(1).Where(x => x.Length != 0))
        {
            int equal = field.IndexOf('=');
            config[field[..equal]] = Encoding.Latin1.GetString(Convert.FromHexString(field[(equal + 1)..]));
        }
        byte[] raw = new byte[RawMissionAiSettings.Size];
        // 원본 호출자가 준비한 초기 미정 바이트 패턴을 그대로 넣는다.
        for (int index = 0; index < raw.Length; index++) raw[index] = unchecked((byte)(index * 37 + numbers[0]));
        var trace = new List<string>();
        var state = new MissionAiSettingsState { StuffRefreshRate = unchecked((int)numbers[2]), StartingSp = unchecked((int)numbers[3]), CheckingAsserts = numbers[4] != 0 };
        var hooks = new MissionAiSettingsHooks
        {
            Get = key => { trace.Add("G," + key); return config.GetValueOrDefault(key, ""); },
            ParseFloat = value => { trace.Add("F," + Convert.ToHexString(Encoding.Latin1.GetBytes(value)).ToLowerInvariant()); return double.Parse(value, CultureInfo.InvariantCulture); },
            UnknownAbilityAssertion = _ => trace.Add("A"),
            UnknownAbilityLog = _ => trace.Add("L"),
        };
        var settings = new RawMissionAiSettings(OriginalEdition.Patch1078, state, hooks);
        switch (row[0])
        {
            case "init": settings.Initialize(raw); break;
            case "apply": settings.Apply(raw, unchecked((int)numbers[1])); break;
            case "prepare": settings.Prepare(raw, unchecked((int)numbers[1])); break;
            case "ability": settings.SetAbility(raw, config.GetValueOrDefault("name", ""), numbers[5] != 0); break;
            default: throw new InvalidOperationException("알 수 없는 AI 설정 검사 종류");
        }
        return Convert.ToHexString(raw).ToLowerInvariant() == row[2] && (trace.Count == 0 ? "-" : string.Join(';', trace)) == row[4];
    }

    /// <summary>기본값·덮어쓰기·능력·공통/소유자 누적의 독립 원본 124개를 검사한다.</summary>
    [Fact]
    public void SettingsMatchNative124()
    {
        string[][] rows = X86Fixture.Read("mission-start").Where(row => row[0] is "init" or "apply" or "ability" or "prepare").ToArray();
        Assert.Equal(124, rows.Length);
        X86Fixture.CheckRows(rows, Matches);
    }

    /// <summary>진단이나 변환이 필요한 시점에 연결이 없으면 이후 비트 또는 필드 쓰기를 하지 않는다.</summary>
    [Fact]
    public void MissingRequiredEffectsPreservePriorState()
    {
        var raw = new byte[RawMissionAiSettings.Size];
        var state = new MissionAiSettingsState { CheckingAsserts = true };
        var settings = new RawMissionAiSettings(OriginalEdition.Patch1078, state, new MissionAiSettingsHooks { Get = key => key == "aiBridgeDrawRate" ? "1.5" : "" });
        settings.Initialize(raw);
        byte[] before = (byte[])raw.Clone();
        Assert.Throws<InvalidOperationException>(() => settings.SetAbility(raw, "unknown", false));
        Assert.Equal(before, raw);
        Assert.Throws<InvalidOperationException>(() => settings.Apply(raw, -1));
        Assert.Equal(before, raw);
        Assert.Throws<ArgumentException>(() => settings.Initialize(new byte[599]));
    }
}
