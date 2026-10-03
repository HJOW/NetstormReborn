using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Tests;

/// <summary>원본 Range.cpp의 타입 분류·직선 사거리·Crossbow 60도 경계·저장 방위 규칙을 검사한다.</summary>
public sealed class RangeIndicatorTests
{
    /// <summary>원본 타입의 사거리 값과 표시 모양. 썬 캐논·울타리는 방위 -1(네 방향)이다.</summary>
    [Theory]
    [InlineData("rainCannon", 30, RangeShape.Ray, 2)]
    [InlineData("thunderCannon", 42, RangeShape.Ray, 2)]
    [InlineData("sunCannon", 20, RangeShape.Ray, -1)]
    [InlineData("sunFence", 50, RangeShape.Ray, -1)]
    [InlineData("rainFence", 45, RangeShape.Ray, -1)]
    [InlineData("thunderFence", 45, RangeShape.Ray, -1)]
    [InlineData("windArcher", 16, RangeShape.Sector, 2)]
    [InlineData("sunArcher", 9, RangeShape.Circle, -1)]
    [InlineData("thunderArcher", 15, RangeShape.Circle, -1)]
    [InlineData("sunAviary", 30, RangeShape.Circle, -1)]
    [InlineData("windAviary", 30, RangeShape.Circle, -1)]
    [InlineData("rainAviary", 28, RangeShape.Circle, -1)]
    public void OriginalTypes_UseTheirRangeAndShape(string name, double distance, RangeShape shape, int direction)
    {
        RangeDisplay display = RangeIndicator.ForType(OriginalData.RequireTypes().Find(name)!, 2)!;
        Assert.Equal(distance, display.Distance);
        Assert.Equal(shape, display.Shape);
        Assert.Equal(direction, display.Direction);
        Assert.Equal('A', display.FrameSide);
    }

    /// <summary>공격하지 않는 오브젝트에는 잘못된 사거리 원을 그리지 않는다.</summary>
    [Theory]
    [InlineData("priest")]
    [InlineData("sunFactory")]
    [InlineData("windVortex")]
    [InlineData("windBattery")]
    [InlineData("rainBlocker")]
    public void NonAttackTypes_HaveNoRange(string name) => Assert.Null(RangeIndicator.ForType(OriginalData.RequireTypes().Find(name)!));

    /// <summary>원본 bomb 플래그는 분홍 B 계열 범위를 사용한다. 에너지 공급원의 표시로 오인하지 않는다.</summary>
    [Fact]
    public void Bomb_UsesPinkRange()
    {
        RangeDisplay display = RangeIndicator.ForType(OriginalData.RequireTypes().Find("bombHeal")!)!;
        Assert.Equal(9, display.Distance);
        Assert.Equal('B', display.FrameSide);
    }

    /// <summary>배치 전 우클릭으로 정한 고정 캐논 방위만 표시하며 반대쪽에는 점이 없다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    public void FixedCannon_HasOnlyChosenRay(int direction)
    {
        RangeDisplay display = RangeIndicator.ForType(OriginalData.RequireTypes().Find("rainCannon")!, direction)!;
        IReadOnlyList<RangePoint> points = RangeIndicator.Points(display, 4.1, 30, 4.1);
        Assert.Equal(6, points.Count);
        // 북·동·남·서의 실제 좌표 부호와 직선 축을 확인한다.
        foreach (RangePoint point in points)
        {
            Assert.True(Math.Abs(direction % 2 == 0 ? point.X : point.Y) < 0.00001);
            Assert.True(direction switch { 0 => point.Y < 0, 1 => point.X > 0, 2 => point.Y > 0, _ => point.X < 0 });
            Assert.InRange(Math.Sqrt(point.X * point.X + point.Y * point.Y), 0, 30);
        }
    }

    /// <summary>썬 캐논의 네 직선을 사거리 끝까지 표시하며 자율 조준 방향과 무관하다.</summary>
    [Fact]
    public void SunCannon_ShowsFourRays()
    {
        TypeInfo type = OriginalData.RequireTypes().Find("sunCannon")!;
        RangeDisplay display = RangeIndicator.ForType(type, 0)!;
        Assert.Equal(display, RangeIndicator.ForType(type, 3));
        var points = RangeIndicator.Points(display, 2.1, 20, 2.1);
        Assert.Equal(16, points.Count);
        Assert.Contains(points, point => point.X > 0);
        Assert.Contains(points, point => point.X < 0);
        Assert.Contains(points, point => point.Y > 0);
        Assert.Contains(points, point => point.Y < 0);
    }

    /// <summary>사거리 42칸의 썬더 캐논은 8개 × 5.25칸 간격이다. 고정 5칸으로 9개를 그리지 않는다.</summary>
    [Fact]
    public void ThunderCannon_DistributesEightPointsEvenly()
    {
        var display = RangeIndicator.ForType(OriginalData.RequireTypes().Find("thunderCannon")!, 1)!;
        var points = RangeIndicator.Points(display, 4.1, 42, 4.1);
        Assert.Equal(8, points.Count);
        Assert.Equal(5.25, points[1].X - points[0].X, 6);
    }

    /// <summary>표시 생성 첫 0.1초에는 1.5칸까지만 열린다.</summary>
    [Fact]
    public void Range_OpensFromCenter()
    {
        var ray = RangeIndicator.ForType(OriginalData.RequireTypes().Find("rainCannon")!)!;
        Assert.Single(RangeIndicator.Points(ray, 0.1, 1.5, 0.1));
        var circle = RangeIndicator.ForType(OriginalData.RequireTypes().Find("sunArcher")!)!;
        var points = RangeIndicator.Points(circle, 0.1, 1.5, 0.1);
        Assert.Equal(8, points.Count);
        Assert.All(points, point => Assert.Equal(1.5, Math.Sqrt(point.X * point.X + point.Y * point.Y), 6));
        Assert.Empty(RangeIndicator.Points(circle, 0, 0, 0));
    }

    /// <summary>Crossbow는 네 방위 모두 60도 안에 점이 있고, 바깥 점은 V 경계 쪽으로 접혀 짧아진다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    public void Crossbow_FoldsOutsideCircleOntoSectorEdges(int direction)
    {
        var display = RangeIndicator.ForType(OriginalData.RequireTypes().Find("windArcher")!, direction)!;
        var points = RangeIndicator.Points(display, 3, 16, 0.2);
        Assert.Equal(8, points.Count);
        // 원의 바깥쪽 점은 어느 방위에서도 Crossbow의 중심±30도 경계를 넘지 않는다.
        foreach (RangePoint point in points)
        {
            double angle = Math.Atan2(point.X, -point.Y) * 180 / Math.PI - direction * 90;
            angle = ((angle + 180) % 360 + 360) % 360 - 180;
            Assert.InRange(Math.Abs(angle), 0, 30.000001);
            Assert.InRange(Math.Sqrt(point.X * point.X + point.Y * point.Y), 0, 16.000001);
        }
        Assert.Contains(points, point => Math.Sqrt(point.X * point.X + point.Y * point.Y) < 15);
        Assert.Contains(points, point => Math.Sqrt(point.X * point.X + point.Y * point.Y) > 15.9);
    }

    /// <summary>P 조준 그림도 앞선 기본 방향과 같은 저장 방위로 읽는다.</summary>
    [Theory]
    [InlineData(0, 0)]
    [InlineData(20, 0)]
    [InlineData(25, 1)]
    [InlineData(43, 1)]
    [InlineData(50, 2)]
    [InlineData(69, 2)]
    [InlineData(75, 3)]
    [InlineData(98, 3)]
    [InlineData(100, 0)]
    public void Crossbow_SavedFrameKeepsDirection(int frame, int expected) =>
        Assert.Equal(expected, EmplacementDirection.SavedDirection(OriginalData.RequireTypes().Find("windArcher")!, frame));

    /// <summary>원형 표시의 ±30도 경계는 사격 가능하고, 1도 밖과 뒤쪽은 사격할 수 없다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    public void Crossbow_CombatSectorIncludesEdgesAndExcludesOutside(int direction)
    {
        // 각 방위에서 양쪽 경계와 경계 밖을 모두 검사해 북쪽 각도 래핑도 확인한다.
        foreach (double offset in new[] { -180.0, -31, -30, -29, 0, 29, 30, 31, 180 })
        {
            double angle = (direction * 90 + offset) * Math.PI / 180;
            Assert.Equal(Math.Abs(offset) <= 30, EmplacementDirection.InsideCrossbowSector(direction, Math.Sin(angle), -Math.Cos(angle)));
        }
    }
}
