// 실제 프레임 표 생중계의 계약을 확인한다.
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 프레임 표 생중계 모듈을 검증하는 테스트 클래스다. 2026-11-07 F2d 연결 조회 이식에서 추가됐다.
public sealed class RawRiftFrameTablesTests
{
    /// <summary>필드가 그대로 옮겨지는지 검증한다. 2026-11-07 F2d 연결 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void ConvertsFields()
    {
        var frames = new List<RiftTypeFrames>
        {
            new([new RiftFrameCode((byte)'A', (byte)'P', 3, 0x20)]),
            new([]),
        };
        var tables = new RiftFrameCodeTables(frames);
        Assert.Equal(2, tables.Count);
        Assert.Equal([new FrameCode('A', 'P', 3, (FrameCodeFlags)0x20)], tables[0]);
        Assert.Empty(tables[1]);
    }

    /// <summary>표 교체가 바로 보이는지 검증한다. 2026-11-07 F2d 연결 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void ReflectsTableReplacement()
    {
        var frames = new List<RiftTypeFrames>
        {
            new([new RiftFrameCode((byte)'A', (byte)'P', 0, 0)]),
        };
        var tables = new RiftFrameCodeTables(frames);
        Assert.Equal('A', tables[0][0].Side);
        frames[0] = new RiftTypeFrames([new RiftFrameCode((byte)'B', (byte)'P', 1, 0)]);
        Assert.Equal('B', tables[0][0].Side);
        Assert.Equal(1, tables[0][0].Number);
    }

    /// <summary>빈 입력을 거부하는지 검증한다. 2026-11-07 F2d 연결 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void RejectsNull()
    {
        Assert.Throws<ArgumentNullException>(() => new RiftFrameCodeTables(null!));
    }
}
