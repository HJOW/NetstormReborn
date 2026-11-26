using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 원문 초기 카메라 선택/중심 이동을 대조한다.
// 2026-11-26 1.076에서 R5-4c용으로 추가했다.
public sealed class RawCameraHomeTests
{
    private sealed class Scene
    {
        public Scene()
        {
            Pool = new RawSidPool(24000);
            Types = new List<OriginalTypeRecord>
            {
                new(),
                new() { Flags2 = 0x200000, FootX = 2, FootY = 3 },
                new() { Flags2 = 0x200000 | 0x30000 },
                new(),
                new(),
            };
        }

        public RawSidPool Pool { get; }
        public List<OriginalTypeRecord> Types { get; }
    }

    private static void Put(Scene scene, int sid, uint type, uint owner, float x, float y, byte state = 0)
    {
        Span<byte> raw = scene.Pool.RawSlot(sid);
        raw.Clear();
        raw[10] = (byte)type;
        raw[11] = state;
        raw[34] = (byte)owner;
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x);
        BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
        BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], 0);
    }

    private static CameraHomeState Battle(uint localPlayer = 1) => new() { LocalPlayer = localPlayer, Battle = true, Width = 1024, Height = 768 };

    /// <summary>생성자 범위를 먼저 검사한다.</summary>
    [Fact]
    public void ConstructorValidatesRanges()
    {
        var scene = new Scene();
        var state = Battle();
        Assert.Throws<ArgumentException>(() => new RawCameraHome(scene.Pool, [], state, (_, _) => { }));
        Assert.Throws<ArgumentNullException>(() => new RawCameraHome(scene.Pool, scene.Types, state, null!));
        Assert.Throws<ArgumentException>(() => new RawCameraHome(scene.Pool, scene.Types, new CameraHomeState { Width = 0 }, (_, _) => { }));
        Assert.Throws<ArgumentException>(() => new RawCameraHome(scene.Pool, scene.Types, state, (_, _) => { },
            new CameraHomeHooks { SuppressionDepth = () => 0 }));
    }

    /// <summary>마지막 내 사제를 골라 발자국을 뺀 좌표로 중심을 옮긴다.</summary>
    [Fact]
    public void ResetSelectsLastOwnPriest()
    {
        var scene = new Scene();
        Put(scene, 5, 1, 1, 100, 200);
        Put(scene, 7, 1, 1, 300, 400);
        Put(scene, 9, 1, 2, 900, 900);
        var points = new List<(float X, float Y)>();
        var home = new RawCameraHome(scene.Pool, scene.Types, Battle(), (x, y) => points.Add((x, y)));
        home.Reset();
        Assert.Equal([(298f, 397f)], points);
    }

    /// <summary>빈 슬롯을 건너뛰고 죽은 사제는 자연 복귀한다.</summary>
    [Fact]
    public void DeadPriestReturnsNaturally()
    {
        var scene = new Scene();
        Put(scene, 5, 1, 1, 100, 200, state: 2);
        var points = new List<(float X, float Y)>();
        var state = Battle();
        new RawCameraHome(scene.Pool, scene.Types, state, (x, y) => points.Add((x, y))).Reset();
        Assert.Empty(points);
        Assert.Equal(0u, state.SuppressionDepth);
    }

    /// <summary>내 사제가 없으면 (128,128)로 간다.</summary>
    [Fact]
    public void NoPriestsFallsBack()
    {
        var scene = new Scene();
        Put(scene, 5, 3, 1, 100, 200);
        var points = new List<(float X, float Y)>();
        new RawCameraHome(scene.Pool, scene.Types, Battle(), (x, y) => points.Add((x, y))).Reset();
        Assert.Equal([(128f, 128f)], points);
    }

    /// <summary>알 수 없는 타입은 순회 위치에서 거부된다.</summary>
    [Fact]
    public void BadTypeThrows()
    {
        var scene = new Scene();
        Put(scene, 5, 90, 1, 100, 200);
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            new RawCameraHome(scene.Pool, scene.Types, Battle(), (_, _) => { }).Reset());
    }

    /// <summary>Reset에서는 억제를 먼저 올려 Carrier를 자연 통과시키고 깊이를 복원한다.</summary>
    [Fact]
    public void ResetSuppressesCarrierPreviewGap()
    {
        var scene = new Scene();
        Put(scene, 5, 2, 1, 100, 200);
        var points = new List<(float X, float Y)>();
        var state = Battle();
        new RawCameraHome(scene.Pool, scene.Types, state, (x, y) => points.Add((x, y))).Reset();
        Assert.Equal([(99f, 199f)], points);
        Assert.Equal(0u, state.SuppressionDepth);
    }

    /// <summary>직접 중심 이동은 억제 0에서 Carrier 표시를 요구한다.</summary>
    [Fact]
    public void DirectCarrierCenterNeedsPreview()
    {
        var scene = new Scene();
        Put(scene, 5, 2, 1, 100, 200);
        var missing = Assert.Throws<InvalidOperationException>(() =>
            new RawCameraHome(scene.Pool, scene.Types, Battle(), (_, _) => { }).CenterOn(5));
        Assert.Contains("내용물 표시", missing.Message);
    }

    /// <summary>Carrier preview가 있으면 표시하고 깊이를 복원한다.</summary>
    [Fact]
    public void CarrierPreviewRuns()
    {
        var scene = new Scene();
        Put(scene, 5, 2, 1, 100, 200);
        int previewed = 0;
        var state = Battle();
        var home = new RawCameraHome(scene.Pool, scene.Types, state, (_, _) => { },
            new CameraHomeHooks { ContentPreview = _ => previewed++ });
        home.Reset();
        Assert.Equal(1, previewed);
        Assert.Equal(0u, state.SuppressionDepth);
    }

    /// <summary>감김이 걸리면 외부 표시 경로 없음을 알린다.</summary>
    [Fact]
    public void DepthOverflowThrows()
    {
        var scene = new Scene();
        Put(scene, 5, 1, 1, 100, 200);
        var state = Battle();
        state.SuppressionDepth = uint.MaxValue;
        Assert.Throws<OverflowException>(() =>
            new RawCameraHome(scene.Pool, scene.Types, state, (_, _) => { }).Reset());
    }

    /// <summary>Challenge는 내 유닛을 고르고 없으면 월드 가운데로 간다.</summary>
    [Fact]
    public void ChallengeSelectsOwnUnit()
    {
        var scene = new Scene();
        Put(scene, 5, 4, 1, 60, 70);
        var points = new List<(float X, float Y)>();
        var state = Battle();
        state.Challenge = true;
        state.ChallengeType = 4;
        new RawCameraHome(scene.Pool, scene.Types, state, (x, y) => points.Add((x, y))).Reset();
        Assert.Equal([(60f, 70f)], points);
        var empty = new Scene();
        var fallback = new List<(float X, float Y)>();
        var fallbackState = Battle();
        fallbackState.Challenge = true;
        fallbackState.ChallengeType = 4;
        fallbackState.WorldWidth = 256;
        fallbackState.WorldHeight = 256;
        new RawCameraHome(empty.Pool, empty.Types, fallbackState, (x, y) => fallback.Add((x, y))).Reset();
        Assert.Single(fallback);
    }

    /// <summary>Challenge에서 남의 것만 있으면 미초기화 경로로 거부된다.</summary>
    [Fact]
    public void ChallengeForeignOnlyThrows()
    {
        var scene = new Scene();
        Put(scene, 5, 4, 2, 60, 70);
        var state = Battle();
        state.Challenge = true;
        state.ChallengeType = 4;
        Assert.Throws<InvalidOperationException>(() =>
            new RawCameraHome(scene.Pool, scene.Types, state, (_, _) => { }).Reset());
    }

    /// <summary>대본 ViewSpot 인자를 칸 단위 float로 읽는다.</summary>
    [Fact]
    public void ParseViewSpotReadsCells()
    {
        Assert.Equal(new ViewSpotPoint(12, 34), RawCameraHome.ParseViewSpot("(12x34)"));
        Assert.Equal(new ViewSpotPoint(7, 8), RawCameraHome.ParseViewSpot("7x8"));
        Assert.Equal(new ViewSpotPoint(0, 0), RawCameraHome.ParseViewSpot("abc"));
        Assert.Equal(new ViewSpotPoint(0, 0), RawCameraHome.ParseViewSpot(""));
    }
}
