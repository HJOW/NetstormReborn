using System.Buffers.Binary;

// 현재 방향 구간의 signed DWORD 연산과 실제 프레임 지정 효과를 연결한다.
namespace Netstorm.Core.Original;

/// <summary>원본004afc90/CD004acce0의 한 번만 감싸는 프레임 진행기다.</summary>
/// <remarks>근거: cpppj/src/o/RawFrameAdvance.h/.cpp 및 docs/exe/cpp-frame-advance-reconstruction.md.</remarks>
public sealed class RawFrameAdvance
{
    /// <summary>지정기와 공유하는 실제 슬롯 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>자산 교체를 다음 호출에 반영하는 현재 프레임 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>같은 풀의 표시·공간 효과를 실행하는 실제 지정기다.</summary>
    private readonly SquidFrame _setter;

    /// <summary>같은 풀·비어 있지 않은 현재 프레임 표를 빌린다. 표는 진행기보다 오래 유지한다.</summary>
    public RawFrameAdvance(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, SquidFrame setter)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(setter);
        if (!ReferenceEquals(pool, setter.Pool) || frames.Count == 0)
            throw new ArgumentException("프레임 진행 풀·현재 프레임 연결 오류");
        _pool = pool; _frames = frames; _setter = setter;
    }

    /// <summary>signed 증분을 DWORD로 더하고 구간 길이를 한 번 보정한 뒤 지정한다. 보정 여부를 반환한다.</summary>
    public bool Advance(int sid, int delta, uint flags = 0)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[10] >= _frames.Count) throw new ArgumentOutOfRangeException(nameof(sid), "프레임 진행 타입 오류");
        int current = _pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) : raw[34];
        RiftTypeFrames frames = _frames[raw[10]];
        int side = RawPathAnimation.Side(frames, current); RiftTypeFrames.LetterRun run = frames.Run(unchecked((byte)side));
        uint first = unchecked((uint)run.First), count = unchecked((uint)run.Count);
        uint relative = unchecked((uint)current + (uint)delta - first); bool wrapped = false;
        if (unchecked((int)relative) >= run.Count) { relative = unchecked(relative - count); wrapped = true; }
        else if (unchecked((int)relative) < 0) { relative = unchecked(relative + count); wrapped = true; }
        _setter.Set(sid, unchecked((int)(first + relative)), flags); return wrapped;
    }

    /// <summary>이벤트 어댑터가 같은 실제 풀을 사용하는지 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>직접 지정과 진행이 같은 효과 경로를 쓰는지 확인한다.</summary>
    public SquidFrame Setter => _setter;
}
