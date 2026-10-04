namespace Netstorm.Assets.Tests;

/// <summary>.type 파서와 설정 파일 테스트</summary>
public sealed class TypeAndConfigTests
{
    /// <summary>예제 .type 텍스트 (주석·대소문자 혼용·그림자 레이어 포함)</summary>
    private const string SampleType = """
        typename suncannon constructor
        typeflags createsisland emplacement shadow;
        {
            description="Sun Cannon";
            maxHitPoints = 600;
            cost = 400; // 주석
            hotFootRatioY = 0.625;
        }
        N00	:		  : "scannon.gif" #00  : "s_scannon.gif" #00 ; // S
        L01	: default : "scannon.gif" #10  : "s_scannon.gif" #00 ;
        //N02 : : "old.gif" #1 ;
        AA03 : rim fringe : "risle.gif" #17 ;
        """;

    /// <summary>예제 텍스트의 머리·플래그·속성·클러스터 해석</summary>
    [Fact]
    public void Parse_Sample()
    {
        TypeDefinition t = TypeDefinition.Parse(SampleType);
        Assert.Equal("suncannon", t.Name);
        Assert.Equal("constructor", t.Modifier);
        Assert.True(t.HasFlag("SHADOW"));
        Assert.Equal("Sun Cannon", t.GetString("description"));
        Assert.Equal(600, t.GetInt("maxhitpoints"));
        Assert.Equal(400, t.GetInt("cost"));
        Assert.Equal(0.625, t.GetDouble("hotfootratioy"));
        Assert.Equal(3, t.Clusters.Count);
        Assert.Equal("L01", t.Clusters[1].Name);
        Assert.Contains("default", t.Clusters[1].Flags);
        Assert.Equal(2, t.LayerCount);
        Assert.Equal(new ClusterImageRef("risle.gif", 17), t.Clusters[2].Layers[0]);
    }

    /// <summary>
    /// 원본: 115개 블록에서 "클러스터 수 × 레이어 수 = 셰이프 프레임 수" (manabolt 만 예외, docs/formats/shp.md)
    /// </summary>
    [Fact]
    public void Original_ClusterLayoutMatchesShapeBlocks()
    {
        TaffArchive archive = TaffArchive.Open(OriginalData.RequireFile("netstorm.tarc"));
        ShapeDatabase shapes = ShapeDatabase.Load(OriginalData.RequireFile("d/_shapes.shp"));
        var mismatches = new List<string>();
        // 로딩 순서대로 타입과 블록을 짝지어 검사
        foreach (ShapeBlock block in shapes.Blocks)
        {
            Assert.True(archive.TryFind($"d/{block.TypeName}.type", out TaffEntry entry), $"{block.TypeName}.type 없음");
            TypeDefinition type = TypeDefinition.Parse(archive.Read(entry));
            if (type.Clusters.Count * type.LayerCount != block.Frames.Count)
            {
                mismatches.Add(block.TypeName);
            }
        }
        Assert.Equal(["manabolt"], mismatches);
    }

    /// <summary>설정 파일: 복호화 → 재인코딩이 원본과 바이트 단위로 같다</summary>
    [Theory]
    [InlineData("d/setup.cfg")]
    public void Original_ConfigRoundTrip(string relativePath)
    {
        byte[] original = File.ReadAllBytes(OriginalData.RequireFile(relativePath));
        ConfigFile config = ConfigFile.Decode(original);
        Assert.Equal(original, config.Encode());
    }

    /// <summary>클론 옵션에는 원본 설치 경로·사용자 등록 번호·캠페인 완료 기록이 없어야 한다.</summary>
    [Fact]
    public void CloneOptions_ExcludeOriginalUserSettings()
    {
        ConfigText options = ConfigText.FromFileBytes(File.ReadAllBytes(OriginalData.RequireFile("d/options.cfg")));
        Assert.False(options.TryGetRaw("InstallDir", out _));
        Assert.False(options.TryGetRaw("CDDir", out _));
        Assert.False(options.TryGetRaw("registerdSubId", out _));
        Assert.False(options.TryGetRaw("DoneThewarbegins", out _));
    }

    /// <summary>설정 파일: setup.cfg 에서 게임 팔레트 지정값을 읽는다 (주석 처리된 이전 값은 무시)</summary>
    [Fact]
    public void Original_SetupPaletteSpec()
    {
        ConfigFile setup = ConfigFile.Load(OriginalData.RequireFile("d/setup.cfg"));
        Assert.Equal("gifcloud", setup.Get("fortPal"));
        Assert.Equal(@"{DataDir}\{local.1}.COL", setup.Get("GamePalSpec"));
    }

    /// <summary>설정 값 변경과 추가</summary>
    [Fact]
    public void Config_SetAndGet()
    {
        var config = new ConfigFile("intro = \"1\"\r\nstartInFullScreen = \"1\"\r\n");
        config.Set("startInFullScreen", "0");
        config.Set("newKey", "abc");
        Assert.Equal("0", config.Get("STARTINFULLSCREEN"));
        Assert.Equal("abc", config.Get("newKey"));
        Assert.Equal("1", config.Get("intro"));
    }
}
