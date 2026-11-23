// 실제87BYTE 전투 옵션의8개 슬롯 색/랭크/특수 상태를 원문 폭으로 읽고 쓴다.
// cpppj/src/o/RawBattleOptionSlots.cpp 를 이식한다.
// 0.913에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

// 0041c4f0~0041c6a0의 슬롯 색/랭크/특수 읽고 쓰기다. 현재 CS 전투 옵션을 빌리며 별도 사본을 만들지 않는다.
public static class BattleOptionSlots
{
    /// <summary>0041c5b0의 실제 지역24DWORD 랭크 표다. 0.913에서 추가됐다.</summary>
    private static readonly int[] RankTable = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 35, 40, 45, 50, 60, 70, 80, 90, 100];

    /// <summary>원문 signed0..7 관문이다. 옵션 길이보다 먼저 판정한다. 0.913에서 추가됐다.</summary>
    private static bool Valid(int slot) => slot >= 0 && slot < 8;

    /// <summary>유효 slot 호출자의 실제 필드 접근 위치에서만 물리 길이를 확인한다. 0.913에서 추가됐다.</summary>
    private static int Position(ReadOnlySpan<byte> options, int field, int slot)
    {
        // 원문 필드 밖 접근은 그 위치에서 거부한다.
        long at = (long)field + slot;
        if (at < 0 || at >= options.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(options), "전투 옵션 슬롯의 실제 BYTE 물리 범위");
        }

        return (int)at;
    }

    /// <summary>BYTE 증가를 먼저 저장하고7보다 클 때만0으로 쓴다. 0.913에서 추가됐다.</summary>
    public static uint IncreaseBattleSlotColor(Span<byte> options, int slot)
    {
        if (!Valid(slot))
        {
            return 0;
        }

        int at = Position(options, 0x47, slot);
        byte next = unchecked((byte)(options[at] + 1u));
        options[at] = next;
        if (next > 7)
        {
            options[at] = 0;
        }

        return 1;
    }

    /// <summary>BYTE 감소를 먼저 저장하고ff일 때만7로 쓴다. 0.913에서 추가됐다.</summary>
    public static uint DecreaseBattleSlotColor(Span<byte> options, int slot)
    {
        if (!Valid(slot))
        {
            return 0;
        }

        int at = Position(options, 0x47, slot);
        byte next = unchecked((byte)((uint)options[at] - 1u));
        options[at] = next;
        if (next == 0xffu)
        {
            options[at] = 7;
        }

        return 1;
    }

    /// <summary>zero확장한 값+1을23과 비교한 뒤 최종BYTE만 쓴다. 0.913에서 추가됐다.</summary>
    public static uint IncreaseBattleSlotRank(Span<byte> options, int slot)
    {
        if (!Valid(slot))
        {
            return 0;
        }

        int at = Position(options, 0x4f, slot);
        uint next = (uint)options[at] + 1u;
        if (next > 23)
        {
            next = 0;
        }

        options[at] = (byte)next;
        return 1;
    }

    /// <summary>zero확장한 값-1이음수일 때만23으로 바꿔 쓴다. 0.913에서 추가됐다.</summary>
    public static uint DecreaseBattleSlotRank(Span<byte> options, int slot)
    {
        if (!Valid(slot))
        {
            return 0;
        }

        int at = Position(options, 0x4f, slot);
        int next = (int)options[at] - 1;
        if (next < 0)
        {
            next = 23;
        }

        options[at] = (byte)next;
        return 1;
    }

    /// <summary>유효slot만+4f BYTE를 실제24DWORD 표로 변환한다. 0.913에서 추가됐다.</summary>
    public static int BattleSlotRank(ReadOnlySpan<byte> options, int slot)
    {
        if (!Valid(slot))
        {
            return 0;
        }

        return RankTable[options[Position(options, 0x4f, slot)]];
    }

    /// <summary>+3f BYTE가정확히1이면0,그외면1이다. 무효slot에서 읽기 없이0을 반환한다. 0.913에서 추가됐다.</summary>
    public static uint ToggleBattleSlotSpecial(Span<byte> options, int slot)
    {
        if (!Valid(slot))
        {
            return 0;
        }

        int at = Position(options, 0x3f, slot);
        options[at] = options[at] != 1u ? (byte)1 : (byte)0;
        return 1;
    }
}
