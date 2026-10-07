using Netstorm.Tests;

namespace Netstorm.Assets.Tests;

/// <summary>원본 실행 파일이 없는 CI에서도 설정·프레임 검색의 x86 결과를 검사한다.</summary>
public sealed class X86AssetTests
{
    /// <summary>원본 코드 표와 별도로, C++ 정적 로더의 116개 타입 플래그·깊이·비용·목록·해시를 대조한다.</summary>
    [Fact]
    public void TypeMetadata_MatchesCpp1078()
    {
        var catalog = new TypeCatalog(GameFileSystem.Open(OriginalData.RequireDirectory()));
        string[][] rows = File.ReadLines(Path.Combine(AppContext.BaseDirectory, "Fixtures", "type-metadata-1078.tsv"))
            .Where(line => line.Length > 0 && !line.StartsWith('#')).Select(line => line.Split('\t')).ToArray();
        Assert.Equal(116, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            TypeInfo type = catalog.Find(row[0]) ?? throw new InvalidDataException("타입 정의가 없습니다.");
            return type.Flags1 == X86Fixture.UInt(row[1]) && type.Flags2 == X86Fixture.UInt(row[2])
                && type.ZOrder == X86Fixture.Int(row[3]) && type.Cost == float.Parse(row[4], System.Globalization.CultureInfo.InvariantCulture)
                && type.Level == X86Fixture.Int(row[5]) && type.ContainerListFlags == X86Fixture.UInt(row[6])
                && type.ContentListFlags == X86Fixture.UInt(row[7]) && TypeCatalog.NameHash(type.RuntimeName) == X86Fixture.UInt(row[8]);
        });
    }

    /// <summary>원본의 플래그 부호 확장과 첫 일치 검색을 포함하는 1,536개 입력.</summary>
    [Fact]
    public void FrameFind_MatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("original").Where(row => row[0].StartsWith("FrameFind", StringComparison.Ordinal)).ToArray();
        Assert.Equal(1536, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            byte[] bytes = X86Fixture.Bytes(row[1]);
            var codes = new List<FrameCode>();
            // 원본 저장 순서대로 네 바이트를 한 코드로 읽는다.
            for (int i = 0; i < bytes.Length; i += 4)
                codes.Add(new FrameCode((char)bytes[i], (char)bytes[i + 1], bytes[i + 2], (FrameCodeFlags)bytes[i + 3]));
            var table = new TypeFrameTable(codes);
            char side = (char)X86Fixture.Int(row[2]), variant = (char)X86Fixture.Int(row[3]);
            int actual = row[0] switch
            {
                "FrameFindNumber" => table.Find(side, variant, X86Fixture.Int(row[4])),
                "FrameFindMasked" => table.Find(side, variant, X86Fixture.Int(row[4]), X86Fixture.UInt(row[5])),
                "FrameFindFlags" => table.FindExactFlags(side, variant, X86Fixture.Int(row[4])),
                _ => throw new InvalidDataException("모르는 프레임 기대값입니다."),
            };
            return actual == X86Fixture.Int(row[^1]);
        });
    }

    /// <summary>설정 객체의 층 순서·기본값·부재 표시·환경 변수 기대값을 전수 대조한다.</summary>
    [Theory]
    [InlineData("Subst", 495)]
    [InlineData("Get", 77)]
    public void Config_MatchesOriginalX86(string kind, int count)
    {
        string[][] all = X86Fixture.Read("config");
        var definitions = all.Where(row => row[0] == "Def").ToDictionary(row => row[1], row => X86Fixture.Text(row[2]));
        string[][] rows = all.Where(row => row[0] == kind).ToArray();
        Assert.Equal(count, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var store = new ConfigStore();
            int layers = X86Fixture.Int(row[2]);
            int end = 3 + layers * 3;
            // TSV의 등록 순서를 그대로 유지한다. '~'는 버퍼/이름 없음, '@n'은 Def 참조다.
            for (int i = 3; i < end; i += 3)
            {
                string text = row[i + 1];
                store.Push(text == "~" ? null : new ConfigText(text.StartsWith('@') ? definitions[text[1..]] : X86Fixture.Text(text)),
                    row[i] == "~" ? null : X86Fixture.Text(row[i]), row[i + 2] == "1");
            }
            // 실제 환경에 의존하지 않도록 생성 도구와 같은 두 값만 제공한다.
            store.EnvironmentLookup = name => name.ToLowerInvariant() switch { "nsenv" => "env{A}val", "nsempty" => "", _ => null };
            string key = X86Fixture.Text(row[end]);
            int source = X86Fixture.Int(row[1]);
            string? actual = kind == "Subst" ? store.ExpandFromLayer(source, key) : store.GetFromLayer(source, key);
            return (kind == "Subst" || (actual != null) == (row[end + 1] == "1"))
                && (actual ?? "") == X86Fixture.Text(row[^1]);
        });
    }

    /// <summary>16개 원본 저장 입력으로 서명 없음·중복 섹션·4KB XOR 경계·모든 상위 바이트를 검사한다.</summary>
    [Fact]
    public void OptionsSave_MatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("options");
        Assert.Equal(16, rows.Length);
        X86Fixture.CheckRows(rows, row => new ConfigFile(X86Fixture.Text(row[1]))
            .EncodeSections(X86Fixture.Text(row[0])).SequenceEqual(X86Fixture.Bytes(row[2])));
    }

    /// <summary>기계어에서 확인한 LF·CR·NUL·따옴표 경계 270개를 원시 설정 판독기에 대조한다.</summary>
    [Fact]
    public void RawConfig_MatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("original").Where(row => row[0] == "Config").ToArray();
        Assert.Equal(270, rows.Length);
        X86Fixture.CheckRows(rows, row => new ConfigText(X86Fixture.Text(row[1])).GetRaw(X86Fixture.Text(row[2]))
            == (row[4] == "missing" ? null : X86Fixture.Text(row[4][4..])));
    }

    /// <summary>원본 섹션 검색에서 얻은 본문 위치·길이를 408개 입력으로 확인한다.</summary>
    [Fact]
    public void ConfigSections_MatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("config").Where(row => row[0] == "Section").ToArray();
        Assert.Equal(408, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            string text = X86Fixture.Text(row[1]);
            int start = X86Fixture.Int(row[3]), length = X86Fixture.Int(row[4]);
            return new ConfigText(text).Section(X86Fixture.Text(row[2])) == (start < 0 ? null : text.Substring(start, length));
        });
    }
}
