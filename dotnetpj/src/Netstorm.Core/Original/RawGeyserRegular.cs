// 간헐천 정기 처리의 현재 프레임과 남은 결정량별 반환값을 같은 장기 풀·타입·표시에 연결한다.
namespace Netstorm.Core.Original;

// 간헐천 정기 모듈을 담는 네임스페이스다. 2026-11-19 0.634에서 R4-4 정기 처리용으로 추가했다.

/// <summary>간헐천 사건이 요구하는 실제 표시·피해 분배 경계다.</summary>
/// <remarks>2026-11-19 0.634에서 추가했다. 근거: cpppj <c>o/RawAssetRegular.h</c>의 사용처와 <c>client/RawSurfaceStorage.cpp</c> 297행의 분배다.</remarks>
public sealed class GeyserRegularHooks
{
    /// <summary>간헐천의 가상 +88 표시 갱신이다. 실제 프레임 쓰기 전후에 한 번씩 호출한다. 2026-11-19 0.634에서 추가했다.</summary>
    public Action<int, uint>? DisplayUpdate { get; set; }

    /// <summary>선택된 조기 반환 이외의 실제 Damageable 분배다. 2026-11-19 0.634에서 추가했다.</summary>
    public RegularHandler? Damageable { get; set; }
}

/// <summary>간헐천의 실제 프레임과 남은 결정량별 반환값을 처리한다. 난수를 소비하지 않는다.</summary>
/// <remarks>2026-11-19 0.634에서 추가했다. 근거: cpppj <c>o/RawAssetRegular.cpp</c>의 <c>RunNativeGeyserRegular</c>(원문 485640)다.</remarks>
public static class RawGeyserRegular
{
    /// <summary>간헐천 가상 표(10.78)다. 이 표의 슬롯만 처리한다. 2026-11-19 0.634에서 추가했다.</summary>
    private const uint GeyserVtable = 0x0050da88u;

    /// <summary>결정량 비율 상한 구간이다. 2026-11-19 0.634에서 추가했다.</summary>
    private const double HighRatio = 0.75;

    /// <summary>결정량 비율 중간 구간이다. 2026-11-19 0.634에서 추가했다.</summary>
    private const double MidRatio = 0.5;

    /// <summary>상한 구간의 프레임 상한이다. 2026-11-19 0.634에서 추가했다.</summary>
    private const int HighFrameLimit = 16;

    /// <summary>중간 구간의 프레임 상한이다. 2026-11-19 0.634에서 추가했다.</summary>
    private const int MidFrameLimit = 32;

    /// <summary>하한 구간의 프레임 상한이다. 2026-11-19 0.634에서 추가했다.</summary>
    private const int LowFrameLimit = 48;

    /// <summary>상한 구간의 첫 프레임이다. 2026-11-19 0.634에서 추가했다.</summary>
    private const int HighFirstFrame = 0;

    /// <summary>중간 구간의 첫 프레임이다. 2026-11-19 0.634에서 추가했다.</summary>
    private const int MidFirstFrame = 17;

    /// <summary>하한 구간의 첫 프레임이다. 2026-11-19 0.634에서 추가했다.</summary>
    private const int LowFirstFrame = 33;

    /// <summary>같은 풀의 간헐천 가상표인지 확인한다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="sid">확인할 슬롯 번호다.</param>
    /// <returns>간헐천 가상표이면 참이다.</returns>
    public static bool Handles(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            return false;
        }

        return Read(pool.Slot(sid), 0) == GeyserVtable;
    }

    /// <summary>현재 가상88을 먼저 실행하고 실제로 선택된 타입 필드만 읽는다.</summary>
    /// <param name="pool">슬롯 풀이다.</param>
    /// <param name="types">타입 번호 순서의 타입 표다. 비용을 읽는다.</param>
    /// <param name="sid">처리할 간헐천 부모 번호다.</param>
    /// <param name="event">사건 번호다. 0만 프레임을 진행한다.</param>
    /// <param name="count">개수다. 그대로 Damageable에 전달한다.</param>
    /// <param name="payload">전달 값이다. 사건 0에서는 결정량 구간값으로 바뀐다.</param>
    /// <param name="hooks">실제 표시·피해 분배 묶음이다.</param>
    /// <returns>피해 분배의 반환값이다.</returns>
    /// <exception cref="ArgumentException">풀·타입·가상 표 불일치다.</exception>
    /// <exception cref="InvalidOperationException">선택된 실제 장치가 미연결이다.</exception>
    public static float Run(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, int sid, uint @event, uint count, float payload, GeyserRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("GeyserRegular는 PATCH10.78 본문이다", nameof(pool));
        }

        if (!Handles(pool, sid))
        {
            throw new ArgumentException("간헐천 정기 가상 표·타입 미복원", nameof(sid));
        }

        ReadOnlySpan<byte> raw = pool.Slot(sid);
        if (raw[RawSidPool.TypeOffset] >= types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "간헐천 정기 현재 타입 물리 범위 오류");
        }

        GeyserNeed(hooks.DisplayUpdate)(sid, 0);
        if (@event == 0)
        {
            uint quantity = Read(raw, 12, 2);
            double ratio = (double)quantity / (double)types[raw[RawSidPool.TypeOffset]].Cost;
            int frame = unchecked((int)Read(pool.Slot(sid), 36));
            int next;
            if (ratio > HighRatio && ratio <= 1.0)
            {
                next = frame < HighFrameLimit ? frame + 1 : HighFirstFrame;
                payload = BitConverter.UInt32BitsToSingle(0x3cf5c28fu);
            }
            else if (ratio > MidRatio && ratio <= HighRatio)
            {
                next = frame < MidFrameLimit ? frame + 1 : MidFirstFrame;
                payload = BitConverter.UInt32BitsToSingle(0x3d75c28fu);
            }
            else
            {
                next = frame < LowFrameLimit ? frame + 1 : LowFirstFrame;
                payload = BitConverter.UInt32BitsToSingle(0x3db851ecu);
            }

            Write(pool.RawSlot(sid), 36, unchecked((uint)next));
            GeyserNeed(hooks.DisplayUpdate)(sid, 0);
        }

        return GeyserNeed(hooks.Damageable)(sid, @event, count, payload);
    }

    /// <summary>정렬되지 않은 little endian 값을 현재 슬롯에서 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <param name="width">읽는 너비(BYTE 수)다.</param>
    /// <returns>결합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // 현재 필드의 지정 바이트만 조합한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }

        return value;
    }

    /// <summary>지정 폭만 쓰고 raw 슬롯의 이웃 필드를 보존한다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Write(Span<byte> raw, int offset, uint value)
    {
        // 이웃 필드를 보존한다.
        for (int index = 0; index < 4; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>도달한 장치만 요구하며 콜백 중 원본 callable의 수명을 보존한다.</summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식)이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T GeyserNeed<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("간헐천 정기의 선택된 실제 장치 미연결");
        }

        return function;
    }
}
