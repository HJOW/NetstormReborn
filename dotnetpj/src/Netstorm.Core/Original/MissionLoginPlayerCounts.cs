using static Netstorm.Core.Original.RawMissionPacketFields;

// 로그인과 타이머가 같은 실제 Player9 기록을 집계한다.
namespace Netstorm.Core.Original;

/// <summary>RawMissionLoginTimer.cpp의 일곱 집계와 실제 기록 대여다.</summary>
public sealed class MissionLoginPlayerCounts
{
    /// <summary>동일 현재 Player9 원문이며 집계 사본을 만들지 않는다.</summary>
    private readonly ReadOnlyMemory<byte> _players;
    /// <summary>실제 PATCH9기록만 허용하며 어떤 집계도 미리 실행하지 않는다.</summary>
    public MissionLoginPlayerCounts(OriginalEdition edition, ReadOnlyMemory<byte> players)
    {
        if (edition != OriginalEdition.Patch1078 || players.Length != 1620) throw new ArgumentException("로그인 집계의 실제 PATCH9 Player 기록 누락");
        _players = players;
    }
    /// <summary>현재 기록의 비정렬 DWORD를 그대로 읽는다.</summary>
    private uint Read(uint owner, int offset) => Get(_players.Span, checked((int)owner * 180 + offset));
    /// <summary>Player0을 제외한 현재1..8에서 선택한 원문 조건만 각각 평가한다.</summary>
    private uint Count(Func<uint, bool> selected)
    {
        uint count = 0;
        // 조건식의 단락 평가마다 현재 실제 subscriber/flags를 다시 읽는다.
        for (uint owner = 1; owner < 9; owner++) if (selected(owner)) count++;
        return count;
    }
    /// <summary>00490ab0은 flags와 무관한 subscriber 비영만 센다.</summary>
    public uint Subscribers() => Count(owner => Read(owner, 0x2c) != 0);
    /// <summary>00490a00은 subscriber와 참여 flags2가 모두 있어야 한다.</summary>
    public uint Participating() => Count(owner => Read(owner, 0x2c) != 0 && (Read(owner, 0x50) & 2) != 0);
    /// <summary>00490bc0은 subscriber/flags2/flags100을 별도로 읽는다.</summary>
    public uint Disconnected() => Count(owner => Read(owner, 0x2c) != 0 && (Read(owner, 0x50) & 2) != 0 && (Read(owner, 0x50) & 0x100) != 0);
    /// <summary>00490b20은 subscriber/참여/로그인/Ready를 모두 요구한다.</summary>
    public uint Ready() => Count(owner => Read(owner, 0x2c) != 0 && (Read(owner, 0x50) & 2) != 0 && (Read(owner, 0x50) & 1) != 0 && (Read(owner, 0x50) & 0x40) != 0);
    /// <summary>00490970은 Ready와 별개인 subscriber/flags2/flags1을 센다.</summary>
    public uint LoggedIn() => Count(owner => Read(owner, 0x2c) != 0 && (Read(owner, 0x50) & 2) != 0 && (Read(owner, 0x50) & 1) != 0);
    /// <summary>00490c50은 subscriber가 있고 참여2만 켜진 미로그인/미단절 기록이다.</summary>
    public uint Pending() => Count(owner => Read(owner, 0x2c) != 0 && (Read(owner, 0x50) & 0x103) == 2);
    /// <summary>00490cf0은 subscriber가 있고 참여2가 아직 없는 기록이다.</summary>
    public uint Connecting() => Count(owner => Read(owner, 0x2c) != 0 && (Read(owner, 0x50) & 2) == 0);
    /// <summary>현재 이름/flags를 조회할 동일180바이트를 복제 없이 빌린다.</summary>
    public ReadOnlyMemory<byte> Record(uint owner)
    {
        if (owner >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "로그인 진행 Player 범위"); return _players.Slice((int)owner * 180, 180);
    }
}
