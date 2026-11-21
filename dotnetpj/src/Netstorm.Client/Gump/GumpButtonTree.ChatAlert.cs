using Netstorm.Core.Original;

// 같은 UI 등록표의 원문 채팅 알림을 조회하고 실제 버튼에 점멸을 건다.
namespace Netstorm.Client.Gump;

/// <summary>RawButtonControls.cpp의00419d50 및005ca92c 저장소 연결이다.</summary>
public sealed partial class GumpButtonTree
{
    /// <summary>원문 BSS0 알림 등록 ID다. serial 검색 결과로 바꾸지 않으며 HUD 생성기가 같은 칸을 쓴다.</summary>
    public UIntBox ChatAlertId { get; } = new();

    /// <summary>현재 등록 ID의+a4/+a0/+ac 관문 뒤 같은 버튼의 SetPulse를 실행한다.</summary>
    public bool PulseRegisteredIfIdle(uint id)
    {
        if (id >= Create.Entries.Length) throw new ArgumentOutOfRangeException(nameof(id), "점멸 대상 등록 ID 범위");
        uint token = Create.Entries[id]; if (token == 0) return false;
        var button = Node(token) as ButtonGumpNode ?? throw new InvalidOperationException("점멸 대상의 실제 버튼 필드 미연결");
        if (button.Pressed != 0 || button.Value != 0 || (button.ButtonFlags & 0x10) != 0) return false;
        Buttons.SetPulse(token, true); return true;
    }
}
