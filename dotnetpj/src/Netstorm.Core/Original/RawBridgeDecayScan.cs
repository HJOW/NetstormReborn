// 원문 바깥 루프의 실제 슬롯 방문과 각 칸의 예외 경계를 현재 풀에 연결한다.
using Netstorm.Core.Bridges;

namespace Netstorm.Core.Original;

// 스캔 실행기 모듈을 담는 네임스페이스다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.

// 초기화 함수와 분리하여 현재 범위·커서·주기를 생성·반납에서 다시 쓰지 않는다. 정지→편집기→권한 관문 뒤 원문의 부분 방문·기한 재설정을 실행한다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.
public sealed class RawBridgeDecayScan
{
    /// <summary>할당 상태와 TYPE·extra를 매 방문 다시 읽는 실제 풀이다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 TYPE 표다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>같은 현재 범위·커서·기한이다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    private readonly BridgeDecayScanState _state;

    /// <summary>현재 주기(초)를 읽는다. 기본값으로 고정하지 않는다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    private readonly Func<float> _period;

    /// <summary>정지·편집기·권한을 입구에서만 검사하는 현재 실행 상태다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    private readonly Func<BridgeDecayMode> _mode;

    /// <summary>실제 한 칸 처리다. 실패 뒤에도 다음 번호로 진행한다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    private readonly Action<int> _cell;

    /// <summary>구성은 현재 스캔을 쓰지 않으며 원본의 최초 BSS·이전 월드 반납 상태도 보존한다.</summary>
    /// <param name="pool">실제 풀이다.</param>
    /// <param name="types">현재 TYPE 표다. 188개여야 한다.</param>
    /// <param name="state">같은 현재 범위·커서·기한이다.</param>
    /// <param name="period">현재 주기를 읽는다.</param>
    /// <param name="mode">현재 실행 상태를 읽는다.</param>
    /// <param name="cell">실제 한 칸 처리다.</param>
    public RawBridgeDecayScan(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, BridgeDecayScanState state, Func<float> period, Func<BridgeDecayMode> mode, Action<int> cell)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(period);
        ArgumentNullException.ThrowIfNull(mode);
        ArgumentNullException.ThrowIfNull(cell);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException("실제 다리 붕괴 스캔의 PATCH 풀·TYPE 연결 오류");
        }

        _pool = pool;
        _types = types;
        _state = state;
        _period = period;
        _mode = mode;
        _cell = cell;
    }

    /// <summary>기한이 지난 프레임은 범위 개수만 방문하며 끝 슬롯을 포함한다. 재설정은 모든 칸 처리 뒤다.</summary>
    /// <param name="time">현재 프레임 시각이다.</param>
    /// <param name="paused">정지 중이면 참이다.</param>
    public void Frame(FrameTime time, bool paused)
    {
        BridgeDecayMode mode = _mode();
        if (paused || mode.Editor || !mode.Authority)
        {
            return;
        }

        int range = Add(_state.Last, unchecked((uint)-_state.First));
        int remaining = range;
        // 기한을 넘겼으면 델타·주기를 읽지 않는 원문 분기다.
        if (_state.Next - time.Game > 0.0)
        {
            remaining = Budget(time.Delta / _period() * range);
        }

        while (remaining > 0 && _state.Cursor <= _state.Last)
        {
            --remaining;
            try
            {
                if (_state.Cursor < 0 || (uint)_state.Cursor >= (uint)_pool.Capacity)
                {
                    throw new ArgumentOutOfRangeException(nameof(_state.Cursor), "다리 붕괴 스캔의 실제 물리 번호 범위 오류");
                }

                int sid = _state.Cursor;
                ReadOnlySpan<byte> raw = _pool.Slot(sid);
                if ((raw[RawSidPool.StateOffset] & 7u) == 0)
                {
                    if (raw[RawSidPool.TypeOffset] >= (uint)_types.Count)
                    {
                        throw new ArgumentOutOfRangeException(nameof(sid), "다리 붕괴 스캔의 현재 TYPE 범위 오류");
                    }

                    if ((_types[raw[RawSidPool.TypeOffset]].Flags2 & TypeFlag2.Bridge) != 0 && (raw[40] & 1u) == 0)
                    {
                        _cell(sid);
                    }
                }
            }
            catch
            {
                // 원문은 이 슬롯의 예외만 삼키고 이미 발생한 효과를 되돌리지 않는다.
            }

            _state.Cursor = Add(_state.Cursor, 1u);
        }

        if (time.Game >= _state.Next)
        {
            // 기한과 현재 주기도 칸 효과 뒤 다시 읽으며 밀린 옛 기한에 누적하지 않는다.
            double next = _period() + time.Game;
            _state.Cursor = _state.First;
            _state.Next = next;
        }
    }

    /// <summary>원문 CRT처럼 64비트로 절삭한 뒤 하위 32비트만 소비한다. 비유한·범위 밖이면 0이다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>예산 칸 수다.</returns>
    private static int Budget(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((int)(uint)(long)value);
    }

    /// <summary>원문 sub·inc는 DWORD에서 감기므로 부호 오버플로를 피하고 같은 비트를 반환한다.</summary>
    /// <param name="first">첫 값이다.</param>
    /// <param name="second">더할 DWORD다.</param>
    /// <returns>감긴 합이다.</returns>
    private static int Add(int first, uint second)
    {
        return unchecked((int)(unchecked((uint)first) + second));
    }
}
