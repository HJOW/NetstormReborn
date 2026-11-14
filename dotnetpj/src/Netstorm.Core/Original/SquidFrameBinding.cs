using System.Buffers.Binary;

// 실제 SHP 표시·같은 Pop/Unpop 공간으로 공통 프레임 지정을 연결한다.
namespace Netstorm.Core.Original;

/// <summary>원본 SquidFrameBinding.h/.cpp의 현재 표시·공간 효과 어댑터다.</summary>
public static class SquidFrameBinding
{
    /// <summary>같은 풀·표시·해시·spot을 먼저 확인하고 현재 프레임의 실제 효과 네 개를 빌린다.</summary>
    public static SquidFrameHooks MakeSquidFrameHooks(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types,
        SquidDisplay display, RawUnpop unpop, RawPop pop)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(display);
        ArgumentNullException.ThrowIfNull(unpop); ArgumentNullException.ThrowIfNull(pop);
        display.ValidateFrameBinding(pool.Edition, types); pop.ValidateFrameBinding(pool, display, unpop);
        return new SquidFrameHooks
        {
            FrameSize = (sid, frame) => display.FrameSize(pool.Edition, pool.Slot(sid), frame),
            Update = (sid, flags) => display.Update(pool.Slot(sid), flags),
            Unpop = (sid, flags) =>
            {
                byte number = pool.Slot(sid)[10];
                if (number >= types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "프레임 Unpop 타입 번호 범위 오류");
                unpop.Unpop(sid, types[number], flags);
            },
            Pop = (sid, x, y, flags) =>
            {
                byte number = pool.Slot(sid)[10];
                if (number >= types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "프레임 Pop 타입 번호 범위 오류");
                if (pool.Edition == OriginalEdition.Patch1078) { pop.Pop(sid, types[number], 0, 0, x, y, flags); return; }
                ReadOnlySpan<byte> raw = pool.Slot(sid);
                int frame = pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) : raw[34];
                float[] size = display.FrameSize(pool.Edition, raw, frame); pop.Pop(sid, types[number], size[0], size[1], x, y, flags);
            },
        };
    }
}
