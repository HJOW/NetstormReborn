using System.Buffers.Binary;
using System.Runtime.Versioning;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 실제 Game이 조립한 현재 건물 캡처의 원본 메타데이터와 전체 픽셀을 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>섬 조각·워크샾·제네레이터·타워의 기존 독립 관측을 같은 제품 장치로 소비한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>같은 제품 장면의 캡처 원천을 실제 공장으로 만들고 이미 만든 TYPE는 재사용한다.</summary>
    private static int BuildingCaptureSource(BurningContext scene, Dictionary<int, int> sources, int type)
    {
        if (sources.TryGetValue(type, out int sid)) return sid;
        sid = scene.Session.BuildPlayerWorldHooks().Create!((uint)type, 2); sources.Add(type, sid);
        Assert.True(RawPlainBuildingCapture.Matches(scene.Session.Pool.Slot(sid))); return sid;
    }
    /// <summary>현재 TYPE 이름을 원문과 같은 자산 번호로 해석한다.</summary>
    private static int BuildingCaptureType(BurningContext scene, string name) =>
        Enumerable.Range(70, scene.Types.Count - 70).Single(type => scene.Types[type].Name.Split('\0')[0] == name);
    /// <summary>실제 Game의 분배/기본 프레임/빈+a4/보호RLE를 모든 독립 원문 픽셀과 비교한다.</summary>
    [Theory, SupportedOSPlatform("windows")]
    [InlineData("plain-workshop-capture", 2115)]
    [InlineData("support-capture", 162)]
    [InlineData("generator-capture", 180)]
    [InlineData("blocker-capture", 120)]
    public void ProductPlainBuildingCaptureMatchesNativePixels(string fixture, int expectedCount)
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); HostSession session = scene.Session;
        Dictionary<int, int> sources = []; Dictionary<int, (int X, int Y)> hotspots = []; int count = 0; long pixels = 0;
        try
        {
            // 원본 입력 칸만 조립하고 결과 픽셀은 실제 캡처 후에만 읽는다.
            foreach (string[] row in X86Fixture.Read(fixture))
            {
                bool workshop = fixture == "plain-workshop-capture", support = fixture == "support-capture";
                Assert.Equal(support || workshop ? 8 : 9, row.Length);
                int type = workshop ? X86Fixture.Int(row[0]) : BuildingCaptureType(scene, row[0]);
                int sid = BuildingCaptureSource(scene, sources, type);
                if (!hotspots.ContainsKey(type)) hotspots.Add(type, (scene.Types[type].HotspotX, scene.Types[type].HotspotY));
                scene.Types[type].HotspotX = -2; scene.Types[type].HotspotY = 3;
                Span<byte> raw = session.Pool.AllocatedBytes(sid);
                BinaryPrimitives.WriteUInt32LittleEndian(raw[36..], X86Fixture.UInt(row[1]));
                BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], support || workshop ? (ushort)0 : (ushort)X86Fixture.UInt(row[2]));
                raw[40] = (byte)X86Fixture.UInt(row[support || workshop ? 2 : 3]); raw[34] = (byte)X86Fixture.UInt(row[support || workshop ? 3 : 4]);
                ConstructionVisualImage image = scene.Visuals.Capture.Capture(sid);
                int metadata = support || workshop ? 4 : 5, expectedPixels = metadata + 1;
                Assert.Equal(row[metadata], $"{image.Width},{image.Height},{image.HotspotX},{image.HotspotY}");
                Assert.True(image.Pixels.AsSpan().SequenceEqual(X86Fixture.Bytes(row[expectedPixels])), $"실제 Game 캡처 {fixture} 행{count}/TYPE{type}/frame{row[1]}/extra{raw[40]}/owner{raw[34]}");
                if (support) { Assert.Equal(row[4], row[6]); Assert.Equal(row[5], row[7]); }
                else { Assert.Equal(X86Fixture.Int(row[metadata + 2]), image.Pixels.Length); Assert.Equal("1", row[metadata + 3]); }
                pixels += image.Pixels.Length; ++count;
            }
        }
        finally
        {
            // 세션 메모리의 명시 기준점 입력도 검사 뒤 복원한다. 보호 파일은 쓰지 않는다.
            foreach (var (type, hotspot) in hotspots) { scene.Types[type].HotspotX = hotspot.X; scene.Types[type].HotspotY = hotspot.Y; }
        }
        Assert.Equal(expectedCount, count);
        if (fixture == "generator-capture") Assert.Equal(1844370L, pixels);
        if (fixture == "blocker-capture") Assert.Equal(1317660L, pixels);
    }

    /// <summary>건물과 VT를 공유하는 미지원 파생128을 새 빈+a4로 일반화하지 않는다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void ProductPlainBuildingCaptureKeepsOtherDerivedVtableBoundary()
    {
        if (!OperatingSystem.IsWindows()) return;
        using BurningContext scene = new(); HostSession session = scene.Session;
        int sid = session.BuildPlayerWorldHooks().Create!(129, 2); Assert.True(RawPlainBuildingCapture.Matches(session.Pool.Slot(sid)));
        Span<byte> raw = session.Pool.AllocatedBytes(sid); raw[10] = 128; raw[40] = 2;
        Assert.False(RawPlainBuildingCapture.Matches(raw)); Assert.Contains("주그림", Assert.Throws<InvalidOperationException>(() => scene.Visuals.Capture.Capture(sid)).Message);
        raw[10] = 129; raw[0] ^= 4; Assert.False(RawPlainBuildingCapture.Matches(raw)); Assert.False(RawPlainBuildingCapture.Matches(raw[..49]));
    }
}
