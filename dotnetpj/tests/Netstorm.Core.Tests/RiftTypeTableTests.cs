using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 타입 정의 파서(<see cref="RiftTypeDefinition"/>)·프레임 검색(<see cref="RiftTypeFrames"/>)·타입 표(<see cref="RiftTypeTable"/>)의 합성 입력 검사다.
/// cpppj <c>AssetTests</c>의 TypeParser·TypeLoadOrder 3개와 <c>FortTests</c>의 타입 표 3개를 직역했고, 프레임 검색·직접 경계 검사를 더했다.
/// 원본 x86 대조는 <see cref="RiftTypeX86Tests"/>, 실제 자산 대조는 <see cref="RiftTypeOriginalFilesTests"/>가 맡는다. (F1, 2026-11-02)
/// </summary>
public sealed class RiftTypeTableTests
{
    /// <summary>검사에 쓰는 타입 정의다. 여기 없는 타입은 플래그·속성이 없는 빈 정의다(cpppj <c>kDefinitions</c>).</summary>
    private static readonly Dictionary<string, string> Definitions = new()
    {
        ["bridge"] = "typeflags bridge;",
        ["island"] = "typeflags island islandThreeByThree;",
        ["sunCannon"] = "typeflags emplacement saveFrame; { group=\"cannon\"; foot_x=3; foot_y=3; maxHitPoints=40; }",
        ["priest"] = "typeflags priest walker;",
        ["bombHeal"] = "typeflags bomb saveQA;",
        ["nugget"] = "typeflags nugget saveQB;",
        ["sunFactory"] = "typeflags factory emplacement; { foot_y=6; }",
        ["windVortex"] = "typeflags vortex emplacement; { minUsage=2; }",
        ["residence"] = "typeflags residence guy;",
        ["sunBlocker"] = "{ group=\"blocker\"; }",
        ["windBattery"] = "{ group=\"battery\"; level=3; }",
        ["fakeThreeByThreeSurface"] = "typeflags surface; { description=\"fake3x3Surface\"; }",
    };

    /// <summary>판본의 로딩 순서대로 정의를 만들어 타입 표를 구성한다. 다른 곳(Fort 검사)도 같은 표를 쓴다.</summary>
    internal sealed class TestTypes
    {
        /// <summary>로딩 순서의 읽은 정의다.</summary>
        private readonly List<RiftTypeDefinition> _definitions = [];

        /// <summary>로딩 순서의 표 입력이다.</summary>
        public List<RiftTypeSource> Sources { get; } = [];

        /// <summary>판본의 로딩 순서마다 최소 문법(typename, 본문, 클러스터 하나)의 정의를 읽는다.</summary>
        /// <param name="edition">원본 판본.</param>
        public TestTypes(OriginalEdition edition)
        {
            // 타입마다 최소 문법의 글을 읽는다
            foreach (string name in RiftTypeLoadOrder.Get(edition))
            {
                string body = Definitions.TryGetValue(name, out string? found) ? found : "{}";
                if (!body.Contains('{'))
                {
                    body += " {}";
                }
                _definitions.Add(RiftTypeDefinition.Parse($"typename {name} {body} A00:default:\"a.gif\"#0;"));
            }
            // 로딩 순서의 이름과 정의를 짝짓는다
            IReadOnlyList<string> order = RiftTypeLoadOrder.Get(edition);
            for (int index = 0; index < order.Count; index++)
            {
                Sources.Add(new RiftTypeSource(order[index], _definitions[index]));
            }
        }

        /// <summary>타입 표를 만든다.</summary>
        /// <param name="edition">원본 판본.</param>
        public RiftTypeTable Table(OriginalEdition edition) => new(edition, Sources);
    }

    /// <summary>프레임 코드와 도움말/기본 프레임의 덮어쓰기 순서는 원본/CD 로더에서 확인한 규칙이다.</summary>
    [Fact]
    public void ParserPreservesCodesAndSpecialFrameOrder()
    {
        RiftTypeDefinition type = RiftTypeDefinition.Parse("""
            typename Sample client constructor typeflags shadow dontSave;
            { description="한글; //본문\경로"; foot_x=7; foot_y=6; COST=1; cost=2; }
            A00: default Fringe: "a.gif" #01 : "shadow.gif" #00;
            AA260: HELP hard suck rim lit unlit cracked: "b.gif" #2;
            B00: default GumpFrame BaseFrame dirt: "c.gif" #3;
            """);
        Assert.Equal("Sample", type.Name);
        Assert.Equal("client constructor", type.Constructor);
        Assert.Equal(2, type.Flags.Count);
        Assert.Equal("한글; //본문\\경로", type.String("DESCRIPTION"));
        Assert.Equal(2, type.Number("cost"));
        Assert.Null(type.Number("description"));
        Assert.Null(type.String("cost"));
        Assert.Null(type.Property("missing"));
        Assert.Equal([7, 8], type.Footprint());
        Assert.Equal(3, type.Clusters.Count);
        Assert.Equal(2, type.Clusters[0].Images.Count);
        Assert.Equal(1, type.Clusters[0].Images[0].Number);
        Assert.Equal((byte)'P', type.Clusters[0].Code.Variant);
        Assert.Equal((byte)'A', type.Clusters[1].Code.Side);
        Assert.Equal((byte)'A', type.Clusters[1].Code.Variant);
        Assert.Equal(4, type.Clusters[1].Code.Number);
        Assert.Equal(126, type.Clusters[1].Code.Flags);
        Assert.Equal(0, type.Clusters[2].Code.Flags);
        Assert.Equal(2, type.SpecialFrames.DefaultFrame);
        Assert.Equal(2, type.SpecialFrames.GumpFrame);
        Assert.Equal(1, type.SpecialFrames.HelpFrame);
        Assert.Equal(2, type.SpecialFrames.BaseFrame);
        Assert.Equal(1, type.FrameTable().FindNumber((byte)'A', (byte)'A', 4));
        RiftTypeDefinition defaults = RiftTypeDefinition.Parse("typename Empty {} A00:default:\"a\"#0; B00:default:\"b\"#0;");
        Assert.Equal(1, defaults.SpecialFrames.DefaultFrame);
        Assert.Equal(1, defaults.SpecialFrames.HelpFrame);
        // gumpframe 미지정은 원본 .type 로딩의 -1이며 그림 파서는 defaultFrame으로 복귀한다
        Assert.Equal(-1, defaults.SpecialFrames.GumpFrame);
        Assert.Equal(-1, defaults.SpecialFrames.BaseFrame);
    }

    /// <summary>잘린 문자열·구분자·방향·비유한 숫자는 무효 파일로 보고한다.</summary>
    [Fact]
    public void ParserRejectsIncompleteOrInvalidDefinitions()
    {
        Assert.Throws<InvalidDataException>(() => RiftTypeDefinition.Parse("typename A {x=\"unterminated"));
        Assert.Throws<InvalidDataException>(() => RiftTypeDefinition.Parse("typename A {x=1}"));
        Assert.Throws<InvalidDataException>(() => RiftTypeDefinition.Parse("typename A {} a00::\"x\"#0;"));
        Assert.Throws<InvalidDataException>(() => RiftTypeDefinition.Parse("typename A {} A00::\"x\"#0"));
        Assert.Throws<InvalidDataException>(() => RiftTypeDefinition.Parse("typename A {x=1e999;}"));
    }

    /// <summary>
    /// cpppj가 검사하지 않는 파서 경계(직접): 숫자 문법(<c>from_chars</c> general 형식), '+' 한 개 건너뛰기, NUL 이후 무시, 이름 없는 입력, 글자뿐인 클러스터 이름의 다음 토큰 번호.
    /// </summary>
    [Fact]
    public void ParserNumberSyntaxAndBoundaries()
    {
        RiftTypeDefinition type = RiftTypeDefinition.Parse("typename T { a=5.; b=.5; c=-.25; d=+3; e=1E+2; g=-0; } Q 7: default : \"x\" # 7;\0 쓰레기");
        Assert.Equal(5.0, type.Number("a"));
        Assert.Equal(0.5, type.Number("b"));
        Assert.Equal(-0.25, type.Number("c"));
        Assert.Equal(3.0, type.Number("d"));
        Assert.Equal(100.0, type.Number("e"));
        // "2e"의 지수는 숫자가 없으면 소비하지 않으므로 "e"가 다음 토큰(식별자)이 되어 문법 오류다
        Assert.Throws<InvalidDataException>(() => RiftTypeDefinition.Parse("typename T { f=2e; }"));
        Assert.Equal(0.0, type.Number("g"));
        Assert.True(double.IsNegative(type.Number("g")!.Value));
        // 글자뿐인 클러스터 이름(Q)은 바로 다음 토큰의 숫자(7)가 번호다. NUL 이후는 읽지 않는다
        Assert.Single(type.Clusters);
        Assert.Equal((byte)'Q', type.Clusters[0].Code.Side);
        Assert.Equal((byte)'P', type.Clusters[0].Code.Variant);
        Assert.Equal(7, type.Clusters[0].Code.Number);
        // '+' 뒤에 또 부호가 오는 값은 from_chars가 받지 않는다
        Assert.Throws<InvalidDataException>(() => RiftTypeDefinition.Parse("typename T { a=++3; }"));
        // 음의 번호는 하위 8비트만 남는다
        Assert.Equal(255, RiftTypeDefinition.Parse("typename T {} A-1: default : \"x\" # 0;").Clusters[0].Code.Number);
        Assert.Equal(0, RiftTypeDefinition.Parse("typename T {} A256: default : \"x\" # 0;").Clusters[0].Code.Number);
        Assert.Throws<InvalidDataException>(() => RiftTypeDefinition.Parse("typename T {} A99999999999: default : \"x\" # 0;"));
        // 첫 단어가 typename이 아니거나 비었으면 거부한다
        Assert.Throws<InvalidDataException>(() => RiftTypeDefinition.Parse("typenam T {}"));
        Assert.Throws<InvalidDataException>(() => RiftTypeDefinition.Parse(""));
        Assert.Throws<ArgumentNullException>(() => RiftTypeDefinition.Parse(null!));
        // typename은 대소문자를 구분하지 않고 속성 조회는 마지막 지정값이 이긴다
        RiftTypeDefinition twice = RiftTypeDefinition.Parse("TYPENAME T { x=1; X=2; } A00: default: \"f\" # 0;");
        Assert.Equal(2, twice.Number("x"));
    }

    /// <summary>패치 전후의 로딩 순서 길이는 같지 않으며 앞부분만 공통이다.</summary>
    [Fact]
    public void LoadOrderMatchesEditionLengths()
    {
        IReadOnlyList<string> patch = RiftTypeLoadOrder.Get(OriginalEdition.Patch1078);
        IReadOnlyList<string> cd = RiftTypeLoadOrder.Get(OriginalEdition.Cd1072);
        Assert.Equal(116, patch.Count);
        Assert.Equal(101, cd.Count);
        Assert.Equal("dude", patch[0]);
        Assert.Equal("sunCannon", patch[58]);
        Assert.Equal("Monument", patch[^1]);
        Assert.Equal("fenceShield", cd[^1]);
        Assert.Equal(patch.Take(101), cd);
        // 기존 경로의 로딩 순서 표와 같다(두 표를 따로 옮겼으므로 일치를 확인한다. 기존 경로 은퇴 때 이 확인도 함께 지운다)
        Assert.Equal(Netstorm.Assets.TypeLoadOrder.Names, patch);
    }

    /// <summary>프레임 검색 세 가지는 첫 일치를 돌려주고 플래그를 부호 확장해 비교한다. 방향 글자 구간 표는 A~P만 있다.</summary>
    [Fact]
    public void FrameSearchesReturnFirstMatchAndSignExtendFlags()
    {
        RiftTypeFrames frames = new(
        [
            new RiftFrameCode((byte)'A', (byte)'P', 0, 0x00),
            new RiftFrameCode((byte)'A', (byte)'P', 1, 0x84),
            new RiftFrameCode((byte)'A', (byte)'P', 1, 0x01),
            new RiftFrameCode((byte)'C', (byte)'P', 1, 0x80),
            new RiftFrameCode((byte)'P', (byte)'X', 9, 0x20),
        ]);
        Assert.Equal(5, frames.Codes.Count);
        // 번호까지 같은 첫 프레임
        Assert.Equal(1, frames.FindNumber((byte)'A', (byte)'P', 1));
        Assert.Equal(-1, frames.FindNumber((byte)'A', (byte)'P', 2));
        Assert.Equal(-1, frames.FindNumber((byte)'A', (byte)'X', 1));
        // 마스크: 플래그 0x84는 부호 확장되어 0xffffff84이므로 높은 비트 마스크에도 걸린다
        Assert.Equal(1, frames.FindMasked((byte)'A', (byte)'P', 1, 0x80000000));
        Assert.Equal(1, frames.FindMasked((byte)'A', (byte)'P', 1, 0x04));
        Assert.Equal(2, frames.FindMasked((byte)'A', (byte)'P', 1, 0x01));
        Assert.Equal(-1, frames.FindMasked((byte)'A', (byte)'P', 1, 0x02));
        Assert.Equal(-1, frames.FindMasked((byte)'A', (byte)'P', 0, 0xffffffff));
        // 정확한 플래그: 0x84는 signed -124, 번호는 조건이 아니다
        Assert.Equal(1, frames.FindFlags((byte)'A', (byte)'P', -124));
        Assert.Equal(-1, frames.FindFlags((byte)'A', (byte)'P', 0x84));
        Assert.Equal(3, frames.FindFlags((byte)'C', (byte)'P', -128));
        Assert.Equal(0, frames.FindFlags((byte)'A', (byte)'P', 0));
        // 글자 구간
        Assert.Equal(new RiftTypeFrames.LetterRun(0, 3), frames.Run((byte)'A'));
        Assert.Equal(new RiftTypeFrames.LetterRun(-1, 0), frames.Run((byte)'B'));
        Assert.Equal(new RiftTypeFrames.LetterRun(3, 1), frames.Run((byte)'C'));
        Assert.Equal(new RiftTypeFrames.LetterRun(4, 1), frames.Run((byte)'P'));
        Assert.Equal(3, frames.LetterKinds());
        Assert.Throws<ArgumentOutOfRangeException>(() => frames.Run((byte)'@'));
        Assert.Throws<ArgumentOutOfRangeException>(() => frames.Run((byte)'Q'));
        Assert.Equal(0, new RiftTypeFrames([]).LetterKinds());
        Assert.Throws<ArgumentNullException>(() => new RiftTypeFrames(null!));
    }

    /// <summary>패치판 타입 표: 번호 체계, 내장 이름, 이름 해시. 해시 값은 실제 .fort의 <c>TypeNames</c>에서 확인한 값이다.</summary>
    [Fact]
    public void PatchTableNumberingAndNameHashesMatchOriginalFiles()
    {
        TestTypes source = new(OriginalEdition.Patch1078);
        RiftTypeTable table = source.Table(OriginalEdition.Patch1078);
        IReadOnlyList<OriginalTypeRecord> types = table.Types;
        Assert.Equal(188, types.Count);
        Assert.True(types[0].Name.Length == 0 && types[5].Name.Length == 0 && types[69].Name.Length == 0);
        Assert.True(types[1].Name == "DependForm" && types[4].Name == "PlayerSquid" && types[6].Name == "ContentForm");
        Assert.Equal("evilRaySystemProcess", types[10].Name); // 20글자에서 잘린다
        Assert.True(types[62].Name == "thunderAviaryProcess" && types[63].Name.Length == 0);
        Assert.True(types[70].Name == "dude" && types[185].Name == "Monument");
        Assert.True(types[186].Name.Length == 0 && types[187].Name.Length == 0);
        Assert.True(table.BridgeType() == 82 && table.IslandType() == 94);
        Assert.True(table.Find("BRIDGE") == 82 && table.Find("nothing") == -1);
        Assert.Equal(0, table.Find("")); // 이름 없는 타입 가운데 첫 번호
        Assert.Equal(0u, RiftTypeTable.NameHash(""));
        Assert.Equal(0xd4b73724u, RiftTypeTable.NameHash(types[1].Name));
        Assert.Equal(0x083d0100u, RiftTypeTable.NameHash(types[10].Name));
        Assert.Equal(0x6469d7c9u, RiftTypeTable.NameHash("bridge"));
        // 20글자 이상인 이름은 설명 필드와 이어져 읽힌다
        OriginalTypeRecord overflowing = types[RiftTypeTable.FirstAssetTypeNumber + 92];
        Assert.Equal("fakeThreeByThreeSurface", overflowing.AssetName);
        Assert.Equal("fakeThreeByThreeSurffake3x3Surface", overflowing.Name);
        Assert.Equal(0x0534a54bu, RiftTypeTable.NameHash(overflowing.Name));
        Assert.Equal(-1, table.Find("fakeThreeByThreeSurface"));
        // 부호 있는 글자는 부호 확장한 뒤 민다
        Assert.Equal(0xffffffffu, RiftTypeTable.NameHash("ÿ"));
        Assert.Equal(0x61u + 0xffffff00u, RiftTypeTable.NameHash("aÿ"));
        Assert.Equal(OriginalEdition.Patch1078, table.Edition);
    }

    /// <summary>CD판은 타입 171개, 프로세스 타입 52개다.</summary>
    [Fact]
    public void CdEditionHasItsOwnCounts()
    {
        TestTypes source = new(OriginalEdition.Cd1072);
        RiftTypeTable table = source.Table(OriginalEdition.Cd1072);
        Assert.Equal(171, table.Types.Count);
        Assert.True(table.Types[61].Name == "sharedRegularProcess" && table.Types[62].Name.Length == 0);
        Assert.True(table.Types[169].Name == "daisExtraFrames" && table.Types[170].Name == "fenceShield");
        Assert.Equal(52, RiftTypeTable.ProcessTypeNames(OriginalEdition.Cd1072).Count);
        Assert.Equal(53, RiftTypeTable.ProcessTypeNames(OriginalEdition.Patch1078).Count);
        // 다른 판본의 로딩 순서를 넣으면 거부한다
        TestTypes patch = new(OriginalEdition.Patch1078);
        Assert.Throws<ArgumentException>(() => patch.Table(OriginalEdition.Cd1072));
        // 정의가 없거나 순서가 틀린 입력도 거부한다
        List<RiftTypeSource> swapped = [.. patch.Sources];
        (swapped[0], swapped[1]) = (swapped[1], swapped[0]);
        Assert.Throws<ArgumentException>(() => new RiftTypeTable(OriginalEdition.Patch1078, swapped));
        List<RiftTypeSource> missing = [.. patch.Sources];
        missing[3] = missing[3] with { Definition = null };
        Assert.Throws<ArgumentException>(() => new RiftTypeTable(OriginalEdition.Patch1078, missing));
    }

    /// <summary>플래그 단어·속성·후처리(원본 0049c3b0, 0049b0d0).</summary>
    [Fact]
    public void FlagsFollowWordsPropertiesAndPostProcessing()
    {
        TestTypes source = new(OriginalEdition.Patch1078);
        RiftTypeTable table = source.Table(OriginalEdition.Patch1078);
        OriginalTypeRecord Type(string name) => table.Types[table.Find(name)];
        // 내장·빈 타입도 후처리를 거쳐 mayDropOnIsle이 켜진다
        Assert.True(table.Types[0].Flags1 == TypeFlag1.MayDropOnIsle && table.Types[0].Flags2 == 0);
        Assert.True(table.Types[0].Group == RiftTypeTable.NoGroup && table.Types[0].FootX == 0);
        // 3×3 emplacement + group cannon
        OriginalTypeRecord cannon = Type("sunCannon");
        Assert.Equal(TypeFlag1.SaveFrame | TypeFlag1.HasHitPoints | 6u | 0x8000u, cannon.Flags1); // 0x8000: emplacement이고 factory가 아니다
        Assert.Equal(TypeFlag2.Emplacement | TypeFlag2.DropBlocking | TypeFlag2.GroupArcherOrCannon, cannon.Flags2);
        Assert.True(cannon.Group == 1 && cannon.FootX == 3 && cannon.FootY == 3);
        // walker는 내용물을 갖고 목록 플래그는 0x20이다
        OriginalTypeRecord priest = Type("priest");
        Assert.True((priest.Flags1 & TypeFlag1.Container) != 0 && (priest.Flags1 & 0x8000) != 0);
        Assert.True(priest.ContainerListFlags == 0x20 && priest.Kind == "walker");
        // factory: 내용물 목록 플래그 0x10, foot_y 6은 8로 바뀐다
        OriginalTypeRecord factory = Type("sunFactory");
        Assert.Equal(TypeFlag1.Container | 0x4000000u, factory.Flags1 & (TypeFlag1.Container | 0x4000000u));
        Assert.True(factory.ContainerListFlags == 0x10 && factory.FootY == 8 && factory.Kind == "factory");
        // vortex: 0x84000과 0x10000000, minUsage로 전력 사용
        OriginalTypeRecord vortex = Type("windVortex");
        Assert.Equal(0x84000u | 0x10000000u, vortex.Flags1 & (0x84000u | 0x10000000u));
        Assert.Equal(2.0f, vortex.MinUsage[0]);
        // bomb: 내용물일 때의 목록 플래그 1
        Assert.True(Type("bombHeal").ContentListFlags == 1 && (Type("bombHeal").Flags1 & TypeFlag1.SaveQA) != 0);
        Assert.NotEqual(0u, Type("nugget").Flags1 & TypeFlag1.SaveQB);
        // 종류 단어는 8글자까지만 복사하고, 짧은 단어는 앞 글자만 덮어쓴다(원본 strncpy)
        Assert.Equal("guyidenc", Type("residence").Kind);
        Assert.True(Type("sunBlocker").Group == 2 && (Type("sunBlocker").Flags2 & TypeFlag2.GroupBlocker) != 0);
        Assert.True(Type("windBattery").Group == 5 && (Type("windBattery").Flags2 & TypeFlag2.GroupBattery) != 0);
        Assert.Equal(2, Type("windBattery").Level);
        // rim에만 놓는 타입이 아니면 isle 비트가 켜진다
        Assert.True((Type("bridge").Flags1 & TypeFlag1.MayDropOnIsle) != 0 && Type("bridge").Flags2 == TypeFlag2.Bridge);
    }

    /// <summary>
    /// cpppj가 이 파일에서 검사하지 않는 속성 경계(직접): 깊이 문자열(이름 부분 일치·오프셋·오류), 속성 값 형식 오류, maxHitPoints 중복, 플래그 충돌, theme 범위.
    /// 비용·기준점·techBit는 원본 x86 fixture 검사가 맡는다.
    /// </summary>
    [Fact]
    public void PropertyBoundaries()
    {
        RiftTypeTable Table(string body, OriginalEdition edition = OriginalEdition.Patch1078)
        {
            RiftTypeDefinition definition = RiftTypeDefinition.Parse("typename test " + body + " A00:default:\"a.gif\"#0;");
            RiftTypeDefinition empty = RiftTypeDefinition.Parse("typename test {} A00:default:\"a.gif\"#0;");
            IReadOnlyList<string> order = RiftTypeLoadOrder.Get(edition);
            return new RiftTypeTable(edition, order.Select((name, index) => new RiftTypeSource(name, index == 0 ? definition : empty)).ToArray());
        }
        OriginalTypeRecord First(string body) => Table(body).Types[RiftTypeTable.FirstAssetTypeNumber];
        // 깊이: 이름의 부분 문자열(표 순서의 첫 일치)과 오프셋, 숫자 값
        Assert.Equal(-127, First("{ zorder=\"zoNONE\"; }").ZOrder);
        Assert.Equal(-20, First("{ zorder=\"EMPLACEMENTS\"; }").ZOrder);
        Assert.Equal(-33, First("{ zorder=\"zoFENCE+2\"; }").ZOrder);
        Assert.Equal(-37, First("{ zorder=\"zoFENCE - 2\"; }").ZOrder);
        Assert.Equal(7, First("{ zorder=7.9; }").ZOrder);
        Assert.Throws<ArgumentException>(() => Table("{ zorder=\"nothing\"; }"));
        Assert.Throws<ArgumentException>(() => Table("{ zorder=\"+\"; }"));
        Assert.Throws<ArgumentOutOfRangeException>(() => Table("{ zorder=\"zoFENCE+2147483648\"; }"));
        // 값 형식 오류
        foreach (string body in new[] { "{ maxHitPoints=\"x\"; }", "{ cost=\"x\"; }", "{ damageEffect=\"x\"; }", "{ techBit=\"x\"; }", "{ theme=1; }" })
        {
            Assert.Throws<ArgumentException>(() => Table(body));
        }
        // 최대 HP는 0이 아닌 값을 두 번 지정하면 원본 assert로 끝난다
        Assert.Equal(5, First("{ maxHitPoints=5; }").MaxHitPoints);
        Assert.Throws<InvalidOperationException>(() => Table("{ maxHitPoints=5; maxHitPoints=6; }"));
        Assert.Equal(0, First("{ maxHitPoints=0; maxHitPoints=0; }").MaxHitPoints);
        // 플래그 충돌과 거부 단어
        Assert.Throws<InvalidOperationException>(() => Table("typeflags shadow flyershadow; {}"));
        Assert.Throws<InvalidOperationException>(() => Table("typeflags dontdrawdefault; {}"));
        Assert.Throws<InvalidOperationException>(() => Table("typeflags saveQA saveQB; {}"));
        // theme: 첫 글자 조회(숫자 값은 위에서 거부), 90 미만의 표 밖 글자는 판본 기본값, 90 이상의 표 밖 글자는 0, 122 이상은 거부
        Assert.Equal(0, Table("{ theme=\"w\"; }").Types[70].Theme);
        Assert.Equal(1, Table("{ theme=\"rain\"; }").Types[70].Theme);
        Assert.Equal(2, Table("{ theme=\"t\"; }").Types[70].Theme);
        Assert.Equal(4, Table("{ theme=\"W\"; }").Types[70].Theme); // 패치는 대문자 결과도 5로 나눈 나머지
        Assert.Equal(0, Table("{ theme=\"W\"; }", OriginalEdition.Cd1072).Types[70].Theme); // CD는 4로 나눈 나머지
        Assert.Equal(5, Table("{ theme=\"A\"; }").Types[70].Theme);
        Assert.Equal(4, Table("{ theme=\"A\"; }", OriginalEdition.Cd1072).Types[70].Theme);
        Assert.Equal(0, Table("{ theme=\"a\"; }").Types[70].Theme);
        Assert.Throws<ArgumentOutOfRangeException>(() => Table("{ theme=\"z\"; }"));
        Assert.Throws<ArgumentOutOfRangeException>(() => Table("{ theme=\"한\"; }"));
    }
}
