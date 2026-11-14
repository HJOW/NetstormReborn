using Netstorm.Core.Original;

// 사제 월드와 경로 시작이 공유하는 현재TYPE/프로세스 전역과 원문 이동 선택을 보존한다.
namespace Netstorm.Client.Mission;

/// <summary>실제 장기 owner의 주소 또는 독립 값을 읽고 쓰는 이동 타입/프로세스 선택 자료다.</summary>
public sealed class RawMovementProcessTypes
{
    /// <summary>신전 제품TYPE133의 독립 상자 또는 같은 현재 장기 주소다.</summary>
    public UIntBox SunWalkerBox { get; }
    /// <summary>추가 고갈된TYPE187의 독립 상자 또는 같은 현재 장기 주소다.</summary>
    public UIntBox EmptyBox { get; }
    /// <summary>RainWalker156의 독립 상자 또는 같은 현재 장기 주소다.</summary>
    public UIntBox RainWalkerBox { get; }
    /// <summary>벌프TYPE135의 독립 상자 또는 같은 현재 장기 주소다.</summary>
    public UIntBox BulfBox { get; }
    /// <summary>Dude 프로세스24의 독립 상자 또는 같은 현재 장기 주소다.</summary>
    public UIntBox DudeProcessBox { get; }
    /// <summary>사제TYPE158의 독립 상자 또는 실제 같은 현재 장기 주소다.</summary>
    public UIntBox PriestBox { get; }
    /// <summary>같은 신전 제품TYPE를 읽고 쓰며 일반 월드 준비로 다시 초기화하지 않는다.</summary>
    public uint SunWalker { get => SunWalkerBox.Value; set => SunWalkerBox.Value = value; }
    /// <summary>같은 고갈된TYPE를 읽고 쓰며 최초187로 고정하지 않는다.</summary>
    public uint Empty { get => EmptyBox.Value; set => EmptyBox.Value = value; }
    /// <summary>기존 windWalker의 별도 선택TYPE137이다.</summary>
    public uint WindWalker { get; set; } = 137;
    /// <summary>같은 RainWalkerTYPE를 읽고 쓰며 balloon보다 먼저 비교한다.</summary>
    public uint RainWalker { get => RainWalkerBox.Value; set => RainWalkerBox.Value = value; }
    /// <summary>기존 텔레포트 효과의 별도TYPE134다.</summary>
    public uint Teleport { get; set; } = 134;
    /// <summary>같은 벌프TYPE를 읽고 쓰며 현재 번호를 최초135로 대체하지 않는다.</summary>
    public uint Bulf { get => BulfBox.Value; set => BulfBox.Value = value; }
    /// <summary>기존 Dude의 별도TYPE70이다.</summary>
    public uint Dude { get; set; } = 70;
    /// <summary>같은 현재 사제TYPE를 읽고 쓰며 명시 사본은 독립 주소를 쓴다.</summary>
    public uint Priest { get => PriestBox.Value; set => PriestBox.Value = value; }
    /// <summary>기존 마나볼트의 별도TYPE105다.</summary>
    public uint ManaBolt { get; set; } = 105;
    /// <summary>웜 이동의현재 프로세스 번호이며 최초40이다.</summary>
    public uint WormProcess { get; set; } = 40;
    /// <summary>일반 Walker의현재 프로세스 번호이며 최초38이다.</summary>
    public uint WalkerProcess { get; set; } = 38;
    /// <summary>Balloon의현재 프로세스 번호이며 최초42다.</summary>
    public uint BalloonProcess { get; set; } = 42;
    /// <summary>텔레포트의현재 프로세스 번호이며 최초23이다.</summary>
    public uint TeleportProcess { get; set; } = 23;
    /// <summary>벌프의현재 프로세스 번호이며 최초41이다.</summary>
    public uint BulfProcess { get; set; } = 41;
    /// <summary>같은 현재 Dude 프로세스 번호를 읽고 쓴다.</summary>
    public uint DudeProcess { get => DudeProcessBox.Value; set => DudeProcessBox.Value = value; }
    /// <summary>사제의현재 프로세스 번호이며 최초54다.</summary>
    public uint PriestProcess { get; set; } = 54;
    /// <summary>일반 비행의현재 프로세스 번호이며 최초39다.</summary>
    public uint FlyProcess { get; set; } = 39;
    /// <summary>공개 참조 전에 같은 장기 주소/독립 상자를 선택하며 연결한 owner의 값은 쓰지 않는다.</summary>
    public RawMovementProcessTypes(UIntBox? sharedPriest = null, RawGameAuxState? sharedRoles = null)
    {
        SunWalkerBox = sharedRoles?.TempleProductTypeBox ?? new() { Value = 133 }; EmptyBox = sharedRoles?.EmptyCarrierTypeBox ?? new() { Value = 187 }; RainWalkerBox = sharedRoles?.RainWalkerTypeBox ?? new() { Value = 156 }; BulfBox = sharedRoles?.BulfTypeBox ?? new() { Value = 135 }; DudeProcessBox = sharedRoles?.DudePathTypeBox ?? new() { Value = 24 }; PriestBox = sharedPriest ?? new() { Value = 158 };
    }
    /// <summary>명시 사본은 독립 주소를 먼저 만든 뒤 현재 값만 원문 대입 순서로 복사한다.</summary>
    public RawMovementProcessTypes(RawMovementProcessTypes source) : this() => Assign(source);
    /// <summary>목적지의 장기 주소 연결은 유지하고 원문 tuple 순서대로17현재 값을 읽어 쓴다.</summary>
    public void Assign(RawMovementProcessTypes source)
    {
        ArgumentNullException.ThrowIfNull(source);
        SunWalker = source.SunWalker; Empty = source.Empty; WindWalker = source.WindWalker; RainWalker = source.RainWalker; Teleport = source.Teleport; Bulf = source.Bulf; Dude = source.Dude; Priest = source.Priest; ManaBolt = source.ManaBolt; WormProcess = source.WormProcess; WalkerProcess = source.WalkerProcess; BalloonProcess = source.BalloonProcess; TeleportProcess = source.TeleportProcess; BulfProcess = source.BulfProcess; DudeProcess = source.DudeProcess; PriestProcess = source.PriestProcess; FlyProcess = source.FlyProcess;
    }
    /// <summary>원문 balloon genus 비트이며 특정 TYPE 비교 뒤 읽는다.</summary>
    private const uint Balloon = 0x20000;
    /// <summary>00488fe0/CD0047d020의 현재 선택 우선순위와 CD 선행sunWalker/미지원 진단을 보존한다.</summary>
    public static uint Select(OriginalEdition edition, IReadOnlyList<OriginalTypeRecord> types, RawMovementProcessTypes selection, uint type)
    {
        ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(selection); bool patch = edition == OriginalEdition.Patch1078;
        if (!patch && type == selection.SunWalker) return selection.WormProcess;
        if (type >= types.Count) throw new ArgumentOutOfRangeException(nameof(type), "이동 프로세스 선택 타입 범위 오류");
        if (patch && (type == selection.SunWalker || type == selection.Empty)) return selection.WormProcess;
        if (type == selection.WindWalker || type == selection.RainWalker) return selection.WalkerProcess;
        if ((types[(int)type].Flags2 & Balloon) != 0) return selection.BalloonProcess;
        if (type == selection.Teleport) return selection.TeleportProcess;
        if (type == selection.Bulf) return selection.BulfProcess;
        if (type == selection.Dude) return selection.DudeProcess;
        if (type == selection.Priest) return selection.PriestProcess;
        if (type == selection.ManaBolt) return selection.FlyProcess;
        if (patch) return selection.WormProcess;
        throw new ArgumentException("CD 이동 프로세스 미지원 타입(원본 assert)");
    }
}
