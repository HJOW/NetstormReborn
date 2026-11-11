using Netstorm.Client.Automation;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// 미션 세션의 호스트 소유자다. Kernel·난수는 빌리고 풀은 소유한다.
public sealed class HostSession
{
    /// <summary>미션 풀 슬롯 수다. L2 thewarbegins 기준선 덤프의 pool 행과 같은 값이다.</summary>
    /// <remarks>2026-11-12 0.392에서 world 1 덤프의 첫 관문으로 고정했다. 미션별 가변은 월드 조립에서 잇는다.</remarks>
    public const int MissionPoolCapacity = 32768;

    /// <summary>덤프와 세션이 함께 읽는 현재 Kernel이다. 호출자가 소유한다.</summary>
    public Kernel Kernel { get; }

    /// <summary>덤프와 세션이 함께 읽는 게임 난수다. 호출자가 소유한다.</summary>
    public GameRandom Game { get; }

    /// <summary>덤프와 세션이 함께 읽는 CRT 난수다. 호출자가 소유한다.</summary>
    public CrtRandom Crt { get; }

    /// <summary>세션이 소유한 오브젝트 풀이다. 싱글 플레이는 로컬이 서버다.</summary>
    public RawSidPool Pool { get; }

    /// <summary>같은 Kernel·난수를 빌리고 풀을 새로 만든다.</summary>
    /// <param name="kernel">현재 Kernel.</param>
    /// <param name="game">게임 난수.</param>
    /// <param name="crt">CRT 난수.</param>
    /// <param name="poolCapacity">풀 슬롯 수. 0 이하면 미션 기본값이다.</param>
    public HostSession(Kernel kernel, GameRandom game, CrtRandom crt, int poolCapacity = 0)
    {
        ArgumentNullException.ThrowIfNull(kernel);
        ArgumentNullException.ThrowIfNull(game);
        ArgumentNullException.ThrowIfNull(crt);
        Kernel = kernel;
        Game = game;
        Crt = crt;
        Pool = new RawSidPool(poolCapacity <= 0 ? MissionPoolCapacity : poolCapacity);
    }

    /// <summary>덤프용 풀 관측을 만든다. 예약 0~4번과 사용 중 슬롯만 담고 빈 칸은 제외한다.</summary>
    /// <returns>풀 행·슬롯 행에 쓰는 관측.</returns>
    /// <remarks>제외 규칙은 cpppj 덤프와 같다(id ≥ 5이면서 free 비트가 켜진 칸은 pool 해시로만 비교한다).</remarks>
    public SessionDumpPool DumpPool()
    {
        var slots = new List<SessionDumpSlot>();
        // 번호 순서대로 읽으며 빈 칸은 건너뛴다.
        for (int sid = 0; sid < Pool.Capacity; sid++)
        {
            ReadOnlySpan<byte> raw = Pool.Slot(sid);
            if (sid >= RawSidPool.FirstOrdinary && (raw[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0) continue;
            slots.Add(new SessionDumpSlot(unchecked((uint)sid), raw.ToArray()));
        }
        return new SessionDumpPool(
            Pool.Capacity,
            RawSidPool.Stride,
            Pool.FreeCount,
            unchecked((uint)Pool.FirstFree(false)),
            unchecked((uint)Pool.FirstFree(true)),
            Pool.PredictableCursor,
            Pool.Bytes.ToArray(),
            slots);
    }
}
