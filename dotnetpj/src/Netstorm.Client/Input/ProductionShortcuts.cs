namespace Netstorm.Client.Input; // 같은 현재 생산 명령을 원문 키 분기로 큐에 예약한다.

/// <summary>현재 명령 DWORD 조회와 실제 FIFO 예약이다.</summary>
public sealed class ProductionShortcutHooks
{
    /// <summary>00542ac4의 현재 다리 슬롯 명령이다.</summary>
    public Func<uint>? Command { get; set; }
    /// <summary>실제 명령/slot/0 예약이며 포화 정책은 큐가 수행한다.</summary>
    public Action<uint, int, int>? Queue { get; set; }
    /// <summary>D/d가 소비하는00542b10의 별도 현재 명령이다.</summary>
    public Func<uint>? RepeatCommand { get; set; }
}

/// <summary>Source ProductionShortcuts의 일반 키 전체다. 회전·Drop은 별도 본체다.</summary>
public static class ProductionShortcuts
{
    /// <summary>현재 기술의 slot BYTE를 취득하는 원문 초기 명령이다.</summary>
    public const uint BridgeCommand = 0x020000f4;
    /// <summary>최근 TYPE와 같은 실제 항목을 다시 취득하는 초기 명령이다.</summary>
    public const uint RepeatCommand = 0x02000107;
    /// <summary>release/정지만 먼저 보고 선택한 문자마다 현재 getter 뒤 같은 큐에 예약한다.</summary>
    public static bool Queue(InputEvent input, bool paused, ProductionShortcutHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks); if ((input.Code & 0x80000000) != 0 || paused) return false;
        // E도 각 예약마다 현재 명령을 다시 읽고 getter 효과 뒤 인자값을 고정한다.
        void Enqueue(int slot, bool repeat = false) { var lookup = repeat ? hooks.RepeatCommand : hooks.Command; uint command = (lookup ?? throw new InvalidOperationException("생산 단축키의 현재 명령 조회 누락"))(); (hooks.Queue ?? throw new InvalidOperationException("생산 단축키의 실제 큐 예약 누락"))(command, slot, 0); }
        switch (input.Code & 0x03ffffff)
        {
            case 'd': case 'D': Enqueue(0, true); break;
            case 'q': case 'Q': Enqueue(0); break;
            case 'w': case 'W': Enqueue(1); break;
            case 'a': case 'A': Enqueue(2); break;
            case 's': case 'S': Enqueue(3); break;
            case 'z': case 'Z': case 'y': case 'Y': Enqueue(4); break;
            case 'x': case 'X': Enqueue(5); break;
            case 'e': case 'E':
                // 실제 슬롯 수와 무관하게 여섯 명령을 FIFO로 예약한다.
                for (int slot = 0; slot < 6; slot++) Enqueue(slot);
                break;
            default: return false;
        }
        return true;
    }
}
