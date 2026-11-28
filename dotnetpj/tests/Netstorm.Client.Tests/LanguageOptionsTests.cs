using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 독립 PE 관측의 감지/문자열/상태/적재 순서를 실제 선택 본체와 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>현재/공개 번호의 부분 효과와 실제 Config 구독 수명을 검증한다.</summary>
public sealed class LanguageOptionsTests
{
    /// <summary>원문 BYTE를 기대값과 같은 소문자16진수로 기록한다.</summary>
    private static string Hex(string value) => value.Length == 0 ? "-" : Convert.ToHexString(Encoding.Latin1.GetBytes(value)).ToLowerInvariant();
    /// <summary>감지·첫NUL 비교·두 적재의 당시 현재/공개 번호를4655행 전수 대조한다.</summary>
    [Fact]
    public void MatchesAll4655NativeDetectionAndSelectionRows()
    {
        string[][] rows = X86Fixture.Read("language-options"); Assert.Equal(4655, rows.Length);
        // 기대값은 새 본체로 다시 쓰지 않고 기존 원문 관측 행을 그대로 소비한다.
        foreach (string[] row in rows)
        {
            if (row[0] == "D") { Assert.Equal(X86Fixture.UInt(row[2]), LanguageOptions.Detect((ushort)X86Fixture.UInt(row[1]))); continue; }
            if (row[0] == "F") { Assert.Equal(X86Fixture.UInt(row[2]), RootLanguageNumbers.Find(X86Fixture.Text(row[1]))); continue; }
            Assert.Equal("O", row[0]); var current = new IntBox { Value = unchecked((int)X86Fixture.UInt(row[2])) }; var published = new IntBox { Value = unchecked((int)X86Fixture.UInt(row[3])) };
            var calls = new List<string>();
            bool changed = LanguageOptions.Apply(X86Fixture.UInt(row[1]), current, published, new()
            {
                Read = () => X86Fixture.Text(row[4]),
                Load = name => calls.Add($"{Hex(name)},{current.Value},{published.Value}"),
            });
            Assert.Equal(X86Fixture.UInt(row[5]) != 0, changed); Assert.Equal(unchecked((int)X86Fixture.UInt(row[6])), current.Value); Assert.Equal(unchecked((int)X86Fixture.UInt(row[7])), published.Value);
            Assert.Equal(row[8], calls.Count == 0 ? "-" : string.Join(';', calls));
        }
    }
    /// <summary>첫/두 번째 적재 실패는 앞서 쓴 현재값·콜백 변경을 보존하고 공개 확정을 생략한다.</summary>
    [Theory]
    [InlineData(1)]
    [InlineData(2)]
    public void FailedLoadKeepsEarlierCurrentAndExternalEffects(int failure)
    {
        var current = new IntBox { Value = -1 }; var published = new IntBox { Value = 77 }; int calls = 0;
        Assert.Throws<IOException>(() => LanguageOptions.Apply(0, current, published, new()
        {
            Read = () => "french",
            Load = _ => { calls++; Assert.Equal(calls == 1 ? 2 : 99, current.Value); current.Value = 99; if (calls == failure) throw new IOException("선택된 실제 적재 실패"); },
        }));
        Assert.Equal(failure, calls); Assert.Equal(99, current.Value); Assert.Equal(77, published.Value);
    }
    /// <summary>같은 번호는 적재 장치 없이 반환하고128BYTE/NUL 경계는 현재값 쓰기 전에 검사한다.</summary>
    [Fact]
    public void SameNumberAndLocalByteBoundaryOnlyRequireSelectedEffects()
    {
        var current = new IntBox { Value = 2 }; var published = new IntBox { Value = 77 };
        Assert.False(LanguageOptions.Apply(0, current, published, new() { Read = () => "french\0" + new string('x', 128) }));
        Assert.Equal(77, published.Value);
        Assert.Throws<ArgumentOutOfRangeException>(() => LanguageOptions.Apply(0, current, published, new() { Read = () => new string('é', 64) }));
        Assert.Equal(2, current.Value); Assert.Equal(77, published.Value);
        Assert.Throws<InvalidOperationException>(() => LanguageOptions.Apply(0, current, published, new() { Read = () => "german" }));
        Assert.Equal(3, current.Value); Assert.Equal(77, published.Value);
        Assert.Equal(7u, RootLanguageNumbers.Find("korean\0other"));
    }
    /// <summary>실제 LoadLanguage가 두 번째 부재 파일에서도 첫 표를 비우며 다른 Config 구독자를 보존한다.</summary>
    [Fact]
    public void RealConfigReloadsBothTablesAndOnlyReleasesOwnSubscription()
    {
        var reads = new List<string>();
        var config = new ConfigInterface(path =>
        {
            reads.Add(path);
            if (path != "L/english" && path != "L/french") return null;
            return ConfigInterface.EncodeConfigFile(ConfigTextOps.Signature + "\r\ngreeting=\"" + path + "\"\r\n");
        });
        config.Set("LanguageSpec", "L/{local.1}"); config.Set("currentLanguage", "English"); int otherChanges = 0;
        Action listener = () => otherChanges++; config.OnChanged += listener;
        var globals = new ClientLanguageGlobals(); using var options = new ClientLanguageOptions(globals, config);
        Assert.True(globals.OptionsReady); Assert.Equal(0, globals.Current.Value); Assert.Equal(0, globals.Published.Value);
        Assert.Equal(new[] { "L/english", "L/English" }, reads); Assert.Null(config.Configuration.Get("greeting"));
        reads.Clear(); config.Set("currentLanguage", "french");
        Assert.Equal(new[] { "L/french", "L/french" }, reads); Assert.Equal("L/french", config.Configuration.Get("greeting"));
        Assert.Equal(2, globals.Current.Value); Assert.Equal(2, globals.Published.Value); Assert.Equal(1, otherChanges);
        reads.Clear(); config.Set("unrelated", "17"); Assert.Empty(reads); Assert.Equal(2, otherChanges);
        options.Dispose(); config.Set("currentLanguage", "german");
        Assert.Empty(reads); Assert.Equal(2, globals.Current.Value); Assert.Equal(3, otherChanges); Assert.Equal("L/french", config.Configuration.Get("greeting"));
        Assert.Throws<ObjectDisposedException>(options.Interpret);
    }
}
