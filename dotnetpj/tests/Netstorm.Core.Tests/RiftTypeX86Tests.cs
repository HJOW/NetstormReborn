using System.Globalization;
using System.Text.RegularExpressions;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 타입 표(<see cref="RiftTypeTable"/>)를 원본 x86 실행 기대값으로 대조한다. cpppj <c>TypeHotspotTests</c>·<c>TypeDefaultCostTests</c>·<c>TypeTechBitTests</c>(최초 27입력)와
/// 두 판본의 생성자 주소 표를 직역했다. 기대값은 C# 결과로 만들지 않는다. 기대값 파일은 cpppj가 소유한 fixture를 링크로 읽는다. (F1, 2026-11-02)
/// </summary>
/// <remarks>
/// 이 파일의 검사는 작성 시점(2026-11-02)에 아직 실행하지 않았다(사용자 지시로 테스트 수행을 미뤘다). 첫 실행 결과로 L1 판정을 확정한다.
/// </remarks>
public sealed class RiftTypeX86Tests
{
    /// <summary>기준점 fixture의 판본 이름과 대응 판본이다. 10.37 행은 판본이 아니라 같은 함수의 별도 PE 관찰이다.</summary>
    private static readonly string[] HotspotEditions = ["originals", "originalCD", "original1037"];

    /// <summary>원본 판본 이름(fixture의 첫 칸)을 판본 값으로 바꾼다.</summary>
    /// <param name="name">fixture의 판본 이름.</param>
    private static OriginalEdition EditionOf(string name) => name == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;

    /// <summary>두 판본의 타입별 생성자 주소 표가 원본의 대입 구간 실행 결과와 모두 같다(패치 188개, CD 171개).</summary>
    [Fact]
    public void ConstructorAddressTablesMatchOriginalX86ForBothEditions()
    {
        foreach ((string name, int count) in new[] { ("originals", 188), ("originalCD", 171) })
        {
            string[][] rows = X86Fixture.Read("constructors", name);
            Assert.Equal(count, rows.Length);
            OriginalEdition edition = EditionOf(name);
            X86Fixture.CheckRows(rows, row => OriginalConstructors.Address(edition, X86Fixture.Int(row[1])) == X86Fixture.UInt(row[2]));
        }
        Assert.Throws<ArgumentOutOfRangeException>(() => OriginalConstructors.Address(OriginalEdition.Cd1072, 171));
        Assert.Throws<ArgumentOutOfRangeException>(() => OriginalConstructors.Address(OriginalEdition.Patch1078, 188));
        Assert.Throws<ArgumentOutOfRangeException>(() => OriginalConstructors.Address(OriginalEdition.Cd1072, -1));
    }

    /// <summary>
    /// 숫자 속성 구간의 세 실제 PE 기대값(판본별 468행, 합 1,404행)을 <see cref="RiftTypeTable.TypeHotFootOffset"/>으로 재생한다.
    /// 첫 입력 이후 다른 값을 써서 마지막 속성이 앞의 값을 덮는지도 확인한다.
    /// </summary>
    [Fact]
    public void HotFootOffsetReplaysOriginalX86()
    {
        string[][] all = X86Fixture.Read("typehotspot");
        Assert.Equal(468 * HotspotEditions.Length, all.Length);
        // 판본(과 10.37 별도 PE)마다 재생한다
        foreach (string edition in HotspotEditions)
        {
            string[][] rows = X86Fixture.Section(all, edition);
            Assert.Equal(468, rows.Length);
            X86Fixture.CheckRows(rows, row =>
            {
                int x = X86Fixture.Int(row[4]);
                int y = X86Fixture.Int(row[5]);
                // 순서 문자열의 글자마다 가로(X) 또는 세로(Y) 속성을 float32 입력으로 적용한다
                for (int index = 0; index < row[1].Length; index++)
                {
                    bool vertical = row[1][index] == 'Y';
                    float value = X86Fixture.FloatBits(row[index == 0 ? 2 : 3]);
                    int offset = RiftTypeTable.TypeHotFootOffset(value, vertical);
                    if (vertical)
                    {
                        y = offset;
                    }
                    else
                    {
                        x = offset;
                    }
                }
                return x == X86Fixture.Int(row[6]) && y == X86Fixture.Int(row[7]);
            });
        }
    }

    /// <summary>초기값/대소문자/중복 속성/AST float32 변환과 고정 배율을 두 판본의 전체 표로 검증하고, 거부하는 속성과 범위 오류를 확인한다.</summary>
    [Fact]
    public void HotFootLoaderOrderFloat32AndInvalidProperties()
    {
        RiftTypeTable Table(OriginalEdition edition, RiftTypeDefinition definition) =>
            new(edition, RiftTypeLoadOrder.Get(edition).Select(name => new RiftTypeSource(name, definition)).ToArray());
        // 두 판본의 전체 타입 표가 같은 속성 규칙을 쓴다
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            RiftTypeDefinition empty = RiftTypeDefinition.Parse("typename test typeflags default_hotspot; {} A00:default:\"a.gif\"#0;");
            RiftTypeTable initial = Table(edition, empty);
            Assert.True(initial.Types[70].HotspotX == 0 && initial.Types[70].HotspotY == 0);
            RiftTypeDefinition definition = RiftTypeDefinition.Parse(
                "typename test { foot_x=3; foot_y=6; hotFootRatioX=-1.9; hotFootRatioY=0.99999999; HOTFOOTRATIOX=0.49999999; } A00:default:\"a.gif\"#0;");
            RiftTypeTable table = Table(edition, definition);
            OriginalTypeRecord record = table.Types[70];
            Assert.True(record.FootX == 3 && record.FootY == 8 && record.HotspotX == 8 && record.HotspotY == 11);
            Assert.True(table.Types[0].HotspotX == 0 && table.Types[0].HotspotY == 0);
            // 원본이 assert하는 절대 기준점/문자열은 거부한다
            foreach (string body in new[] { "hotFootX=1;", "hotFootY=1;", "hotFootRatioX=\"1\";", "hotFootRatioY=\"1\";" })
            {
                RiftTypeDefinition bad = RiftTypeDefinition.Parse("typename test {" + body + "} A00:default:\"a.gif\"#0;");
                Assert.ThrowsAny<ArgumentException>(() => Table(edition, bad));
            }
        }
        Assert.Throws<ArgumentOutOfRangeException>(() => RiftTypeTable.TypeHotFootOffset(float.PositiveInfinity, false));
        Assert.Throws<ArgumentOutOfRangeException>(() => RiftTypeTable.TypeHotFootOffset(float.NaN, true));
        Assert.Throws<ArgumentOutOfRangeException>(() => RiftTypeTable.TypeHotFootOffset(float.MaxValue, false));
    }

    /// <summary>같은 로딩 순서의 한 TYPE에만 명시 원본 속성 입력을 공급한 표를 만든다(나머지는 무속성/NO_GROUP 초기화).</summary>
    /// <param name="edition">원본 판본.</param>
    /// <param name="number">정의를 줄 타입 번호.</param>
    /// <param name="definition">그 타입의 정의.</param>
    private static RiftTypeTable CostTable(OriginalEdition edition, uint number, RiftTypeDefinition definition)
    {
        RiftTypeDefinition empty = new();
        IReadOnlyList<string> order = RiftTypeLoadOrder.Get(edition);
        // 목적 TYPE 밖의 속성을 바꾸지 않는다
        return new RiftTypeTable(edition, order.Select((name, index) => new RiftTypeSource(name, index + 70 == number ? definition : empty)).ToArray());
    }

    /// <summary>실제 두 판본 전체 TYPE 후처리의 비용 비트(누락/양음 0·명시 비용·운반/일반·사제/사람 제외)를 판본별 1,500입력으로 로더와 대조한다.</summary>
    [Fact]
    public void DefaultCostReplaysOriginalX86ForBothEditions()
    {
        string[][] all = X86Fixture.Read("type-default-cost");
        Assert.Equal(3000, all.Length);
        foreach (string name in new[] { "originals", "originalCD" })
        {
            string[][] rows = X86Fixture.Section(all, name);
            Assert.Equal(1500, rows.Length);
            OriginalEdition edition = EditionOf(name);
            // 원문 기대 비트는 C# 계산으로 만들지 않는다
            X86Fixture.CheckRows(rows, row =>
            {
                uint number = X86Fixture.UInt(row[1]);
                uint group = X86Fixture.UInt(row[2]);
                uint flags = X86Fixture.UInt(row[3]);
                int level = X86Fixture.Int(row[4]);
                float before = X86Fixture.FloatBits(row[5]); // 원문 int level과 float32 cost 입력이다
                RiftTypeDefinition definition = new();
                if (group != 10)
                {
                    definition.Properties.Add(new TypeProperty("group", group == 0 ? "archer" : group == 3 ? "aviary" : group == 7 ? "walker" : "balloon"));
                }
                definition.Properties.Add(new TypeProperty("level", (double)(level + 1)));
                definition.Properties.Add(new TypeProperty("cost", (double)before));
                if ((flags & 0x10000U) != 0)
                {
                    definition.Flags.Add("walker");
                }
                if ((flags & 0x20000U) != 0)
                {
                    definition.Flags.Add("balloon");
                }
                if ((flags & 0x400U) != 0)
                {
                    definition.Flags.Add("guy");
                }
                if ((flags & 0x200000U) != 0)
                {
                    definition.Flags.Add("priest");
                }
                RiftTypeTable table = CostTable(edition, number, definition);
                return BitConverter.SingleToUInt32Bits(table.Types[(int)number].Cost) == X86Fixture.UInt(row[6]);
            });
        }
    }

    /// <summary>fixture 첫 칸의 [["이름",float32비트],…] 입력을 파서가 읽을 십진 문장으로 바꾼다.</summary>
    /// <param name="pairs">AST 입력 쌍 목록 문자열.</param>
    private static string TechBitInput(string pairs)
    {
        System.Text.StringBuilder text = new("typename sunArcher {");
        // 중복 속성도 원문 순서대로 넘긴다
        foreach (Match found in Regex.Matches(pairs, "\\[\"([^\"]+)\",([0-9]+)\\]"))
        {
            float number = BitConverter.UInt32BitsToSingle(uint.Parse(found.Groups[2].Value, CultureInfo.InvariantCulture));
            // 같은 float로 왕복 가능한 십진 표기(max_digits10)다
            text.Append(found.Groups[1].Value).Append('=').Append(number.ToString("G9", CultureInfo.InvariantCulture)).Append(';');
        }
        return text.Append("} A00:default:\"a.gif\"#0;").ToString();
    }

    /// <summary>원문 DWORD를 메모리 순서 16진수(8글자)에서 부호 있는 값으로 읽는다.</summary>
    /// <param name="hex">낮은 바이트부터의 16진수 8글자.</param>
    internal static int TechBitSigned(string hex)
    {
        Assert.Equal(8, hex.Length);
        uint value = 0;
        // 낮은 바이트 순서다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)Convert.ToByte(hex.Substring(index * 2, 2), 16) << (index * 8);
        }
        return unchecked((int)value);
    }

    /// <summary>실제 188 초기값과 27종 숫자/순서 입력의 native DWORD를 같은 Record 필드와 대조한다.</summary>
    [Fact]
    public void TechBitReplaysNativeDefaultAnd27Callbacks()
    {
        string[][] rows = X86Fixture.Read("type-tech-bit");
        Assert.Equal(27, rows.Length);
        RiftTypeDefinition empty = RiftTypeDefinition.Parse("typename empty {} A00:default:\"a.gif\"#0;"); // 다른 115 자산의 명시 빈 속성 입력
        IReadOnlyList<string> order = RiftTypeLoadOrder.Get(OriginalEdition.Patch1078);
        // 모든 최초 27입력을 재생한다
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(6, row.Length);
            RiftTypeDefinition definition = RiftTypeDefinition.Parse(TechBitInput(row[0]));
            // 원문 관측과 같은 실제 71번(sunArcher)만 바꾼다
            RiftTypeTable table = new(OriginalEdition.Patch1078, order.Select((name, index) => new RiftTypeSource(name, index == 1 ? definition : empty)).ToArray());
            Assert.Equal(188, table.Types.Count);
            Assert.Equal(188 * 8, row[1].Length);
            // 원문 188 초기값과 선택된 71번의 마지막 대입을 모두 확인한다
            for (int number = 0; number < table.Types.Count; number++)
            {
                int expected = number == 71 ? TechBitSigned(row[3]) : TechBitSigned(row[1].Substring(number * 8, 8));
                if (table.Types[number].TechBitIndex != expected)
                {
                    return false;
                }
            }
            return true;
        });
    }
}
