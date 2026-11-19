// 원본4794b0의 현재 프레임 선저장·현재 TYPE+114·signed IDIV·실제 Frame.Set을 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// 스프라이트 정기 모듈을 담는 네임스페이스다. 2026-11-19 0.651에서 R4-4 정기 처리용으로 추가했다.

/// <summary>원문 4794b0 한 번의 signed 나머지를 수행한다. 사건·개수와 무관하며 지정 뒤 원래 payload 비트를 반환한다. 난수를 소비하지 않는다.</summary>
/// <remarks>2026-11-19 0.651에서 추가했다. 근거: cpppj <c>o/RawSpriteRegular.cpp</c>의 <c>RunSpriteRegular</c>다. 원본 TYPE+114 프레임 수는 현재 프레임 코드 수와 같다.</remarks>
public static class RawSpriteRegular
{
    /// <summary>스프라이트 가상표(10.78)다. 이 표의 슬롯만 처리한다. 0.651에서 추가했다.</summary>
    private const uint SpriteVtable = 0x0050cc08u;

    /// <summary>슬롯 안 프레임 DWORD 필드의 위치다. 0.651에서 추가했다.</summary>
    private const int FrameOffset = 36;

    /// <summary>같은 풀의 스프라이트 가상표인지 확인한다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="sid">확인할 슬롯 번호다.</param>
    /// <returns>스프라이트 가상표이면 참이다.</returns>
    public static bool Handles(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            return false;
        }

        return Read(pool.Slot(sid), 0) == SpriteVtable;
    }

    /// <summary>스프라이트 정기 사건을 실행한다. 현재 프레임을 선저장하고 1을 더한 signed 나머지를 지정한다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="frames">타입별 프레임 코드 표다. 원본 TYPE+114 프레임 수와 같다.</param>
    /// <param name="setter">같은 풀의 실제 프레임 지정기다.</param>
    /// <param name="sid">처리할 스프라이트 부모 번호다.</param>
    /// <param name="payload">전달 값이다. 그대로 돌려준다.</param>
    /// <returns>원래 payload 비트다.</returns>
    /// <exception cref="ArgumentException">풀 판본·프레임 표·가상 표 불일치다.</exception>
    /// <exception cref="InvalidOperationException">같은 풀의 지정기가 아니다.</exception>
    public static float Run(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, SquidFrame setter, int sid, float payload)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(setter);
        if (pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(pool, setter.Pool) || frames.Count != 188)
        {
            throw new ArgumentException("스프라이트 정기의 같은 PATCH·188TYPE·Frame 계약 오류", nameof(pool));
        }

        if (!Handles(pool, sid))
        {
            throw new ArgumentException("스프라이트 정기 가상 표·타입 미복원", nameof(sid));
        }

        // 원문은 전체 DWORD 프레임을 먼저 선저장한다.
        uint current = Read(pool.Slot(sid), FrameOffset);
        int type = pool.Slot(sid)[RawSidPool.TypeOffset];
        if (type >= frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "스프라이트 정기의 현재 TYPE 범위 오류");
        }

        int divisor = frames[type].Codes.Count;
        int next = unchecked((int)current + 1);
        if (divisor == 0 || (next == int.MinValue && divisor == -1))
        {
            throw new DivideByZeroException("스프라이트 원본 IDIV 오류");
        }

        setter.Set(sid, next % divisor, 0);
        return payload;
    }

    /// <summary>정렬되지 않은 little endian 값을 현재 슬롯에서 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4));
}
