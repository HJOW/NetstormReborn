// 표시 진단 장치의 기본값·오류 상세를 검증한다.
using Netstorm.Core.Display;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 표시 진단을 검증하는 테스트 클래스다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.
public sealed class RawSquidDisplaySinkTests
{
    /// <summary>검사 전용 싱크다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    private sealed class TestSink : SquidDisplaySink
    {
        /// <summary>억제 여부를 기록한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
        public bool Muted;

        /// <summary>원본 전체 갱신·표시 억제 전역에 대응한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
        /// <returns>억제 중이면 참이다.</returns>
        public override bool Suppressed() => Muted;

        /// <summary>원본 순서대로 영역과 플래그를 기록한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
        /// <param name="rect">변경 영역이다.</param>
        /// <param name="flags">전달 플래그다.</param>
        public override void Invalidate(DisplayRect rect, uint flags)
        {
            Seen.Add((rect, flags));
        }

        /// <summary>기록한 무효화 목록이다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
        public List<(DisplayRect Rect, uint Flags)> Seen { get; } = [];
    }

    /// <summary>같은 TYPE 이름·프레임 개수와 세 인자로 상세를 만든다. 부호 변환까지 대조한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    [Fact]
    public void BuildDetailUsesCurrentNamesAndSignedValues()
    {
        var types = new List<OriginalTypeRecord> { new(), new(), new(), new(), new(), new(), new(), new(), new(), new(), new() { Name = "bridge" } };
        var shapes = new List<SquidDisplayShape>
        {
            new(), new(), new(), new(), new(), new(), new(), new(), new(), new(),
            new() { FrameCount = 4 },
        };
        Assert.Equal(
            "Frame assertion failed. Asking for type=10 (bridge) frame=-3 abs=-1\n\nType has 4 frames\n",
            SquidDisplayDetail.BuildFrameAssertionDetail(types, shapes, 10u, -3, 0xffffffffu));
    }

    /// <summary>10 미만·범위 밖 타입은 무효 이름과 0 프레임을 쓴다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    [Fact]
    public void BuildDetailFallsBackToInvalidType()
    {
        var types = new List<OriginalTypeRecord> { new() { Name = "first" } };
        var shapes = new List<SquidDisplayShape> { new() { FrameCount = 2 } };
        Assert.Equal(
            "Frame assertion failed. Asking for type=5 (invalidType) frame=0 abs=0\n\nType has 0 frames\n",
            SquidDisplayDetail.BuildFrameAssertionDetail(types, shapes, 5u, 0, 0u));
        Assert.Equal(
            "Frame assertion failed. Asking for type=70 (invalidType) frame=1 abs=0\n\nType has 0 frames\n",
            SquidDisplayDetail.BuildFrameAssertionDetail(types, shapes, 70u, 1, 0u));
        Assert.Throws<ArgumentException>(() => SquidDisplayDetail.BuildFrameAssertionDetail(types, [new(), new()], 10u, 0, 0u));
    }

    /// <summary>싱크 기본값은 검사 0·미연결 예외이며 파생 구현이 전달을 받는다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    [Fact]
    public void SinkDefaultsAndOverrides()
    {
        var sink = new TestSink();
        Assert.False(sink.Checking());
        Assert.Throws<InvalidOperationException>(() => sink.FrameError(1u, 2, 3u));
        Assert.Throws<InvalidOperationException>(() => sink.Assertion("e", "f", 4u));
        Assert.False(sink.Suppressed());
        sink.Muted = true;
        Assert.True(sink.Suppressed());
        var rect = new DisplayRect(1, 2, 3, 4);
        sink.Invalidate(rect, 8u);
        Assert.Equal([(rect, 8u)], sink.Seen);
    }

    /// <summary>뷰 기본값은 원본 카메라·Q16 배율을 둔다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    [Fact]
    public void ViewDefaultsCameraAndZoom()
    {
        var view = new SquidDisplayView();
        Assert.Equal(0, view.CameraX);
        Assert.Equal(0, view.CameraY);
        Assert.Equal(65536, view.Zoom);
        Assert.Equal(new DisplayRect(0, 0, 0, 0), view.Viewport);
    }

    /// <summary>저장소의 프레임 오류는 같은 TYPE·셰이프로 상세를 저장한다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.</summary>
    [Fact]
    public void StorageFrameErrorUsesCurrentTables()
    {
        var types = new List<OriginalTypeRecord>();
        var frames = new List<RiftTypeFrames>();
        var shapes = new List<SquidDisplayShape>();
        // 11개 타입을 이름·프레임 수와 함께 채우는 반복문이다. 2026-11-07 F2d 표시 진단 이식에서 추가됐다.
        for (int index = 0; index < 11; ++index)
        {
            types.Add(new OriginalTypeRecord { Name = "type" + index });
            frames.Add(new RiftTypeFrames([]));
            shapes.Add(new SquidDisplayShape { FrameCount = index });
        }

        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        Assert.Throws<InvalidOperationException>(() => storage.RecordDisplayFrameError(10u, 0, 0u));
        var detail = new StringBox();
        storage.BindDisplayDiagnostics(detail, (_, _, _) => { });
        storage.RecordDisplayFrameError(10u, 2, 0u);
        Assert.Equal(
            "Frame assertion failed. Asking for type=10 (type10) frame=2 abs=0\n\nType has 10 frames\n",
            detail.Value);
        storage.RecordDisplayFrameError(3u, 0, 0u);
        Assert.StartsWith("Frame assertion failed. Asking for type=3 (invalidType)", detail.Value);
    }
}
