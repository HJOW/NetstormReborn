// 같은 실패점과 현재 진단을 사용하는 원문 보드 접근 좌표다.
namespace Netstorm.Core.Original;

/// <summary>RawBoardTarget.cpp의41d400/41d530이며 오른쪽 확장의 double 중간값을 유지한다.</summary>
public sealed class RawBoardTarget(BoardTargetState state, Action<uint>? assertion = null)
{
    /// <summary>잘못된 방향의 FLD가 flags의 signaling NaN을 quiet으로 바꾼다.</summary>
    private static float FlagFloat(uint bits)
    {
        if ((bits & 0x7f800000) == 0x7f800000 && (bits & 0x7fffff) != 0) bits |= 0x400000;
        return BitConverter.UInt32BitsToSingle(bits);
    }
    /// <summary>북/동/남/서 변 중간점을 구하며 flags4는 네 변을 한 칸 확장한다.</summary>
    public RawPathPoint Side(SquidFootprint r, uint side, uint flags)
    {
        float left = r.Left, top = r.Top, bottom = r.Bottom; double right = r.Right;
        if ((flags & 4) != 0) { left = (float)((double)left - 1); top = (float)((double)top - 1); right += 1; bottom = (float)((double)bottom + 1); }
        if (side is 0 or 4) return new() { X = (float)((right - left) * 0.5 + left), Y = side == 0 ? top : bottom };
        if (side is 2 or 6) return new() { X = side == 2 ? (float)right : left, Y = (float)(((double)bottom - top) * 0.5 + top) };
        (assertion ?? throw new InvalidOperationException("사각형 변 중간점 원본 진단 미연결"))(0x1f9);
        float value = FlagFloat(flags); return new() { X = value, Y = value };
    }
    /// <summary>float 저장 중심의 방향과 flags2 북쪽 특례를 적용하고 실패 진단 뒤 현재 실패점을 읽는다.</summary>
    public RawPathPoint Nearest(SquidFootprint r, RawPathPoint source, uint flags)
    {
        float x = (float)(((double)r.Left + r.Right) * 0.5), y = (float)(((double)r.Bottom + r.Top) * 0.5);
        uint side = unchecked((uint)RawSquidNeighbors.Direction(source.X, source.Y, x, y) - 4) & 7;
        if ((flags & 2) != 0 && side == 0) side = source.X < ((double)r.Left + r.Right) * 0.5 ? 6u : 2u;
        if ((flags & 1) != 0) return Side(r, side, flags);
        (assertion ?? throw new InvalidOperationException("일반 접근점 원본 진단 미연결"))(0x262); return state.Invalid;
    }
}
