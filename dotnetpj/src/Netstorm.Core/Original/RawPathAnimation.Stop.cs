// 같은 원문 글자 구간과 실제 Unpop/쓰기/Repop으로 도착 정지 프레임을 적용한다.
namespace Netstorm.Core.Original;

/// <summary>도착 정지 전환이 빌리는 실제 공간 효과 두 개다.</summary>
public sealed record PathAnimationHooks
{
    /// <summary>같은 현재 부모의 실제 Unpop이다.</summary>
    public Action<int, uint>? Unpop { get; init; }
    /// <summary>변경한 프레임을 현재 위치에 다시 반영한다.</summary>
    public Action<int, uint>? Repop { get; init; }
}

/// <summary>RawPathAnimation.cpp의 RestFrame/Stop과 프레임 표 값형 복사 부분이다.</summary>
public sealed partial class RawPathAnimation
{
    /// <summary>현재 프레임을 읽고 쓰는 같은 실제 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>원문 vector 복사와 같이 코드 배열까지 소유한 타입별 프레임 표다.</summary>
    private readonly RiftTypeFrames[] _frames;
    /// <summary>원문 값형으로 복사한 실제 가상 공간 함수다.</summary>
    private readonly PathAnimationHooks _hooks;
    /// <summary>그림 표를 독립 복사하고 실제 공간 함수 두 개를 요구한다.</summary>
    public RawPathAnimation(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, PathAnimationHooks hooks)
    {
        if (hooks.Unpop is null || hooks.Repop is null) throw new ArgumentException("보행 정지 효과 누락");
        _pool = pool; _frames = frames.Select(frame => new RiftTypeFrames(frame.Codes)).ToArray(); _hooks = hooks with { };
    }
    /// <summary>현재 글자 구간의 첫 물리번호+1이며 number=1 검색과 다르다.</summary>
    public static int RestFrame(RiftTypeFrames frames, int frame)
    {
        int side = Side(frames, frame); if (side < 'A' || side > 'P') throw new ArgumentOutOfRangeException(nameof(frame), "보행 정지 글자 표 범위");
        return frames.Run((byte)side).First + 1;
    }
    /// <summary>실제 할당/TYPE/현재 프레임 검사를 마친 뒤 Unpop0→프레임→Repop40을 실행한다.</summary>
    public void Stop(int sid)
    {
        _pool.AllocatedBytes(sid); int type = _pool.Slot(sid)[10]; if (type < 70 || type >= _frames.Length) throw new ArgumentOutOfRangeException(nameof(sid), "보행 정지 자산TYPE");
        int current = RawPathSlot.Frame(_pool, sid), target = RestFrame(_frames[type], current); _hooks.Unpop!(sid, 0); RawPathSlot.SetFrame(_pool, sid, target); _hooks.Repop!(sid, 0x40);
    }
}
