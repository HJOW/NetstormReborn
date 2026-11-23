// BattleInfo가 사용하는 실제 BattleOptions BYTE 변경과 제외 목록을 원문 순서로 처리한다.
// cpppj/src/o/RawBattleOptionMutation.cpp 를 이식한다.
// 0.914에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

// 같은 현재87BYTE를 빌리며 별도 설정/제외 목록 사본을 만들지 않는다.
public static class BattleOptionMutations
{
    /// <summary>실제 선택한 필드 위치에서만 읽기를 검증한다. 0.914에서 추가됐다.</summary>
    private static byte Byte(ReadOnlySpan<byte> options, int at)
    {
        if (at < 0 || at >= options.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(options), "BattleOptions 변경의 실제 BYTE 읽기 범위");
        }

        return options[at];
    }

    /// <summary>현재 한 BYTE만 쓰며 이웃 슬롯을 초기화하지 않는다. 0.914에서 추가됐다.</summary>
    private static void Store(Span<byte> options, int at, byte value)
    {
        if (at < 0 || at >= options.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(options), "BattleOptions 변경의 실제 BYTE 쓰기 범위");
        }

        options[at] = value;
    }

    /// <summary>선택한 원문 재생 위치에서만 장치를 요구한다. 0.914에서 추가됐다.</summary>
    private static void Sound(Action? play)
    {
        if (play is null)
        {
            throw new InvalidOperationException("BattleOptions 변경의 실제 guy.wav 장치 미연결");
        }

        play();
    }

    /// <summary>원문의 movzx 첫 BYTE이며 다른 현재 옵션/표를 읽지 않는다. 0.914에서 추가됐다.</summary>
    public static uint BattleOptionGameType(ReadOnlySpan<byte> options) => Byte(options, 0);

    /// <summary>같은 정적22레코드의 포함 최댓값이며 선택 개수가 아니다. 0.914에서 추가됐다.</summary>
    public static int MaximumBattleOptionChoice(int index) => BattleOptionTexts.BattleOptionText(unchecked((uint)index)).Values.Count - 1;

    /// <summary>BYTE증가/최댓값순환을 계산하고 guy.wav 뒤에 최종BYTE를 저장한다. 0.914에서 추가됐다.</summary>
    public static void CycleBattleOption(Span<byte> options, int index, Action? guySound)
    {
        int at = index;
        byte next = unchecked((byte)(Byte(options, at) + 1u));
        if ((int)next > MaximumBattleOptionChoice(index))
        {
            next = 0;
        }

        Sound(guySound);
        Store(options, at, next);
    }

    /// <summary>음수는 표 조회 없이 반환하고 포함 최댓값 이내만 실제BYTE에 쓴다. 0.914에서 추가됐다.</summary>
    public static void SetBattleOptionChoice(Span<byte> options, int index, int value)
    {
        if (value < 0)
        {
            return;
        }

        if (value <= MaximumBattleOptionChoice(index))
        {
            Store(options, index, (byte)value);
        }
    }

    /// <summary>이미 같은 BYTE가 있으면 음향 없이1이다. 전체DWORD 비교 중복·초기count40관문이다. 0.914에서 추가됐다.</summary>
    public static uint AddBattleExcludedType(Span<byte> options, uint type, Action? guySound)
    {
        int initial = Byte(options, 0x16);
        int current = 0;
        if (initial != 0)
        {
            do
            {
                // 원문처럼 각 다음 반복의 현재count를 다시 읽고 최초 일치에서 즉시 반환한다.
                if ((uint)Byte(options, current + 0x17) == type)
                {
                    return 1;
                }

                current++;
            }
            while (current < Byte(options, 0x16));
        }

        if (initial >= 40)
        {
            return 0;
        }

        Store(options, current + 0x17, (byte)type);
        Store(options, 0x16, (byte)(Byte(options, 0x16) + 1u));
        Sound(guySound);
        return 1;
    }

    /// <summary>최초 일치 뒤 초기count 길이memmove→음향→현재count BYTE감소다. 끝칸을지우지않는다. 0.914에서 추가됐다.</summary>
    public static uint RemoveBattleExcludedType(Span<byte> options, uint type, Action? guySound)
    {
        int initial = Byte(options, 0x16);
        int current = 0;
        if (initial != 0)
        {
            do
            {
                // 같은 현재count의 반복 조건으로 최초 일치만 제거한다.
                if ((uint)Byte(options, current + 0x17) == type)
                {
                    int count = initial - current - 1;
                    int destination = current + 0x17;
                    int source = current + 0x18;
                    if (destination < 0 || count < 0 || count > options.Length - destination || source < 0 || count > options.Length - source)
                    {
                        throw new ArgumentOutOfRangeException(nameof(options), "BattleOptions 제외 목록의 실제 memmove 범위");
                    }

                    options.Slice(source, count).CopyTo(options.Slice(destination, count));
                    Sound(guySound);
                    Store(options, 0x16, (byte)((uint)Byte(options, 0x16) - 1u));
                    return 1;
                }

                current++;
            }
            while (current < Byte(options, 0x16));
        }

        return 0;
    }

    /// <summary>최초count만큼 실제BYTE와 전체DWORD를 비교하고 최초 일치1/없음0이다. 0.914에서 추가됐다.</summary>
    public static uint BattleTypeExcluded(ReadOnlySpan<byte> options, uint type)
    {
        int count = Byte(options, 0x16);
        for (int index = 0; index < count; index++)
        {
            if ((uint)Byte(options, index + 0x17) == type)
            {
                return 1;
            }
        }

        return 0;
    }
}
