using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Display;
using Netstorm.Core.Rules;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 칸별 점유 비트·공간 해시 단계(계획 4-3)와 그리기 순서 비교(계획 5-1)를 원본 x86 기대값에 대조한다.
/// 기대값은 hash-x86.tsv 의 Genus·Level 행과 renderer-x86.tsv 의 Order 행이며 열 구성은
/// cpppj/tests/HashTests.cpp·RendererTests.cpp 와 같다.
/// </summary>
public sealed class X86SpotAndOrderTests
{
    /// <summary>점유 비트 3,216개: 발자국·지붕·소수 기준점의 보정. 두 판본의 결과 열이 모두 같아야 한다.</summary>
    [Fact]
    public void EffectiveGenus_MatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("hash").Where(row => row[0] == "Genus").ToArray();
        Assert.Equal(3216, rows.Length);
        int changed = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            uint flags = X86Fixture.UInt(row[1]);
            uint value = SpotRules.EffectiveGenus(flags, X86Fixture.FloatBits(row[4]), X86Fixture.FloatBits(row[5]),
                X86Fixture.Int(row[2]), X86Fixture.Int(row[3]), X86Fixture.Int(row[6]), X86Fixture.Int(row[7]));
            changed += value != flags ? 1 : 0;
            return value == X86Fixture.UInt(row[8]) && value == X86Fixture.UInt(row[9]);
        });
        Assert.True(changed > 0);
    }

    /// <summary>해시 단계 605개: 그림 크기 2·4 경계의 바로 앞뒤와 섬·다리.</summary>
    [Fact]
    public void HashLevel_MatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("hash").Where(row => row[0] == "Level").ToArray();
        Assert.Equal(605, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            int level = SpotRules.HashLevel(X86Fixture.UInt(row[1]), X86Fixture.FloatBits(row[2]), X86Fixture.FloatBits(row[3]));
            return level == X86Fixture.Int(row[4]) && level == X86Fixture.Int(row[5]);
        });
    }

    /// <summary>그리기 순서 비교 261개: 깊이 내림차순 → y → x, 완전 동률은 1.</summary>
    [Fact]
    public void DrawOrder_MatchesOriginalX86()
    {
        string[][] rows = X86Fixture.Read("renderer").Where(row => row[0] == "Order").ToArray();
        Assert.Equal(261, rows.Length);
        // 좌표 열은 십진 소수, 깊이 열은 부호 있는 16비트다
        static float Number(string text) => float.Parse(text, CultureInfo.InvariantCulture);
        X86Fixture.CheckRows(rows, row => DrawOrder.Compare(
            new DrawOrder(Number(row[1]), Number(row[2]), (short)X86Fixture.Int(row[3])),
            new DrawOrder(Number(row[4]), Number(row[5]), (short)X86Fixture.Int(row[6]))) == X86Fixture.Int(row[7]));
    }

    /// <summary>해시의 좌표는 그냥 자르고 점유의 좌표는 0.9999 를 더해 자른다. 둘을 한 식으로 합치면 소수 좌표에서 달라진다.</summary>
    [Fact]
    public void SpotCoordinates_UseNearCeilingBias()
    {
        Assert.Equal(TypeFlagBits.Emplacement | SpotRules.InteriorBit,
            SpotRules.EffectiveGenus(TypeFlagBits.Emplacement, 20.25f, 20.25f, 3, 3, 20, 20));
        Assert.Equal(TypeFlagBits.Emplacement | SpotRules.InteriorBit,
            SpotRules.EffectiveGenus(TypeFlagBits.Emplacement, 20.0001f, 20.0001f, 3, 3, 19, 19));
        // 건물군이 아닌 타입과 발자국의 변 칸은 안쪽 비트를 받지 않는다
        Assert.Equal(TypeFlagBits.Island, SpotRules.EffectiveGenus(TypeFlagBits.Island, 20, 20, 3, 3, 19, 19));
        Assert.Equal(TypeFlagBits.Emplacement, SpotRules.EffectiveGenus(TypeFlagBits.Emplacement, 20, 20, 3, 3, 20, 19));
        Assert.Equal(0, SpotRules.HashLevel(TypeFlagBits.Bridge, 9, 9));
        Assert.Equal((1, 2, 3), (SpotRules.HashLevel(0, 2, 1), SpotRules.HashLevel(0, 2.5f, 4), SpotRules.HashLevel(0, 4.5f, 1)));
        Assert.Throws<ArgumentOutOfRangeException>(() => SpotRules.HashLevel(0, -1, 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => SpotRules.EffectiveGenus(0, 0, 0, 0, 1, 0, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => SpotRules.EffectiveGenus(0, 0, 0, 1, 1, -1, 0));
    }

    /// <summary>정렬은 깊이가 큰 것부터, 같은 깊이에서는 y·x 순서이고, 완전히 같은 키는 입력 순서를 지킨다.</summary>
    [Fact]
    public void DrawOrderSort_IsStableForEqualKeys()
    {
        (string Name, DrawOrder Key)[] items =
        [
            ("포대", new DrawOrder(5, 5, -20)), ("다리 1", new DrawOrder(3, 4, 0)), ("섬", new DrawOrder(9, 1, 0)),
            ("다리 2", new DrawOrder(3, 4, 0)), ("낙하물", new DrawOrder(0, 0, 30)), ("다리 3", new DrawOrder(2, 4, 0)),
        ];
        Assert.Equal(["낙하물", "섬", "다리 3", "다리 1", "다리 2", "포대"], DrawOrder.Sort(items, item => item.Key).Select(item => item.Name));
        Assert.Throws<ArgumentException>(() => DrawOrder.Sort(items.Append((Name: "잘못", Key: new DrawOrder(float.NaN, 0, 0))), item => item.Key));
    }
}
