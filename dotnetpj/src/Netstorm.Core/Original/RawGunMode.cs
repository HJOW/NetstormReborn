using System.Buffers.Binary;

// 현재 raw 프레임/방향 문자와 공유 역할을 빌려 총의 공격 모드를 읽는다.
namespace Netstorm.Core.Original;

/// <summary>RawGunMode.cpp의 판본별 현재 공격 방향 getter다.</summary>
public sealed class RawGunMode
{
    /// <summary>현재 자산의 물리 저장소다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>원문 span처럼 빌리는 현재 코드 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>원문 mutable DWORD를 같은 주소에서 읽는 역할이다.</summary>
    private readonly UIntBox _windArcherType;
    /// <summary>두 판본의 전체 표 크기를 검사하고 사본 없이 빌린다.</summary>
    public RawGunMode(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, UIntBox windArcherType)
    {
        if (frames.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171)) throw new ArgumentException("총 방향 판본/프레임 표 불일치"); Pool = pool; _frames = frames; _windArcherType = windArcherType;
    }
    /// <summary>비astral 크로스보우는 graphBYTE×2, 나머지는 signed 문자×2-152의0..7이다.</summary>
    public uint Get(ushort sid)
    {
        ReadOnlySpan<byte> raw = Pool.Slot(sid); bool patch = Pool.Edition == OriginalEdition.Patch1078; int type = raw[10]; if (type < 70 || type >= _frames.Count) throw new ArgumentException("총 방향 현재 자산 TYPE 오류"); if (type == _windArcherType.Value && (raw[patch ? 40 : 35] & 8) == 0) return (uint)raw[patch ? 30 : 28] * 2;
        uint frame = patch ? BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]) : raw[34]; IReadOnlyList<RiftFrameCode> codes = _frames[type].Codes; if (frame >= codes.Count) throw new ArgumentOutOfRangeException(nameof(sid), "총 방향 현재 물리 프레임 오류"); int result = unchecked((sbyte)codes[(int)frame].Side) * 2 - 152; return result < 0 || result > 7 ? 0 : (uint)result;
    }
}
