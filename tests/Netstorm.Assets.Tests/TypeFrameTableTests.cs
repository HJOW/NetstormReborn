namespace Netstorm.Assets.Tests;

/// <summary>원본 Rifttype.cpp 규칙의 프레임 코드 표 테스트</summary>
public sealed class TypeFrameTableTests
{
    /// <summary>한 글자·두 글자 이름, 플래그 단어, 특수 프레임이 섞인 예제</summary>
    private const string SampleType = """
        typename sample
        {
        }
        A00 : default : "a.gif" #0 ;
        A01 : : "a.gif" #1 ;
        AB02 : rim FRINGE dirt : "a.gif" #2 ;
        B00 : default gumpframe : "a.gif" #3 ;
        A02 : help baseframe : "a.gif" #4 ;
        B01 : cracked hard : "a.gif" #5 ;
        """;

    /// <summary>이름 → (측면, 변형, 번호)와 원본 플래그 비트 (모르는 단어 무시)</summary>
    [Fact]
    public void Sample_CodesAndFlags()
    {
        TypeFrameTable table = TypeDefinition.Parse(SampleType).Frames;
        Assert.Equal(new FrameCode('A', 'P', 0, FrameCodeFlags.None), table.Codes[0]);
        Assert.Equal(new FrameCode('A', 'B', 2, FrameCodeFlags.Rim | FrameCodeFlags.Fringe), table.Codes[2]);
        Assert.Equal(FrameCodeFlags.Cracked | FrameCodeFlags.Hard, table.Codes[5].Flags);
    }

    /// <summary>기본은 마지막 default, 도움말은 help 우선, gump·base 는 지정 클러스터</summary>
    [Fact]
    public void Sample_SpecialFrames()
    {
        TypeFrameTable table = TypeDefinition.Parse(SampleType).Frames;
        Assert.Equal(3, table.DefaultFrame);
        Assert.Equal(4, table.HelpFrame);
        Assert.Equal(3, table.GumpFrame);
        Assert.Equal(4, table.BaseFrame);
    }

    /// <summary>help 가 없으면 도움말 프레임은 마지막 default, 아무것도 없으면 원본 초기값</summary>
    [Fact]
    public void HelpFallsBackToDefault()
    {
        TypeFrameTable withDefault = TypeDefinition.Parse("typename t\n{\n}\nA00 : : \"a\" #0 ;\nA01 : default : \"a\" #1 ;\n").Frames;
        Assert.Equal(1, withDefault.HelpFrame);
        TypeFrameTable empty = TypeDefinition.Parse("typename t\n{\n}\nA00 : : \"a\" #0 ;\n").Frames;
        Assert.Equal(0, empty.DefaultFrame);
        Assert.Equal(-1, empty.HelpFrame);
        Assert.Equal(-1, empty.BaseFrame);
    }

    /// <summary>검색 함수는 첫 일치를 돌려주고, 동작 목록은 파일 순서를 유지한다</summary>
    [Fact]
    public void Sample_Lookups()
    {
        TypeFrameTable table = TypeDefinition.Parse(SampleType).Frames;
        Assert.Equal(4, table.Find('A', 'P', 2));
        Assert.Equal(-1, table.Find('C', 'P', 0));
        Assert.Equal(2, table.Find('A', 'B', 2, FrameCodeFlags.Rim));
        Assert.Equal(-1, table.Find('A', 'B', 2, FrameCodeFlags.Lit));
        Assert.Equal(5, table.FindExactFlags('B', 'P', FrameCodeFlags.Cracked | FrameCodeFlags.Hard));
        Assert.Equal([0, 1, 4], table.Sequence('A', 'P'));
        Assert.Equal([3, 5], table.SequenceOf(5));
    }

    /// <summary>
    /// 원본 116개 타입: 모든 클러스터 이름이 원본 assert('A'~'Z', 번호는 char 범위)를 통과하고,
    /// 대표 타입의 동작 구성·특수 프레임이 .type 파일과 맞는다.
    /// </summary>
    [Fact]
    public void Original_AllTypesBuildTables()
    {
        var catalog = new TypeCatalog(GameFileSystem.Open(OriginalData.RequireDirectory()));
        // 로딩 순서의 모든 타입에 대해 코드 표를 만들고 범위를 확인한다
        foreach (TypeInfo type in catalog.Types)
        {
            TypeFrameTable table = type.Definition.Frames;
            Assert.Equal(type.Definition.Clusters.Count, table.Codes.Count);
            // 원본 로더의 assert 조건과 char 저장 범위
            foreach (FrameCode code in table.Codes)
            {
                Assert.InRange(code.Side, 'A', 'Z');
                Assert.InRange(code.Number, 0, 127);
            }
            Assert.InRange(table.DefaultFrame, 0, Math.Max(0, table.Codes.Count - 1));
        }

        // dude(typename Man): 8방향(A~H) × 걷기 8프레임
        TypeFrameTable man = catalog.Find("dude")!.Definition.Frames;
        Assert.Equal(8, man.Sequence('A', 'P').Count);
        Assert.Equal(8, man.Sequence('H', 'P').Count);
        // priest: default(18)·help(167)·gumpframe 가 각각 따로 지정된다
        TypeFrameTable priest = catalog.Find("priest")!.Definition.Frames;
        Assert.Equal(18, priest.DefaultFrame);
        Assert.Equal(167, priest.HelpFrame);
        Assert.NotEqual(0, priest.GumpFrame);
        // bridge: 금 간(cracked)·경화(hard) 프레임 비트가 원본 다리 코드(0x20·0x40)로 저장된다
        TypeFrameTable bridge = catalog.Find("bridge")!.Definition.Frames;
        Assert.Contains(bridge.Codes, c => (c.Flags & FrameCodeFlags.Cracked) != 0);
        Assert.Contains(bridge.Codes, c => (c.Flags & FrameCodeFlags.Hard) != 0);
    }
}
