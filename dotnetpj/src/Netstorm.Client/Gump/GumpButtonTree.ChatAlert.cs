using Netstorm.Core.Original;

// 같은 UI 등록표의 원문 채팅 알림을 조회하고 실제 버튼에 점멸을 건다.
namespace Netstorm.Client.Gump;

/// <summary>RawButtonControls.cpp의00419d50 및005ca92c 저장소 연결이다.</summary>
public sealed partial class GumpButtonTree
{
    /// <summary>원문 BSS0 알림 등록 ID다. serial 검색 결과로 바꾸지 않으며 HUD 생성기가 같은 칸을 쓴다.</summary>
    public UIntBox ChatAlertId { get; } = new();

    /// <summary>채팅 알림 버튼의 실제 등록 ID를 같은 칸에 쓴다. HUD 생성자가 버튼 생성 뒤 호출한다.</summary>
    /// <param name="id">등록 ID다. 실제 버튼 필드가 있어야 한다.</param>
    /// <remarks>2026-11-25 0.990에서 RU-3d용으로 추가했다. 근거: cpppj RawButtonControls.cpp의 ChatAlertSerial 슬롯과 00419d50 관문. 조회는 같은 칸을 읽고 serial로 다시 검색하지 않는다.</remarks>
    public void RegisterChatAlert(uint id)
    {
        if (id >= Create.Entries.Length) throw new ArgumentOutOfRangeException(nameof(id), "채팅 알림 등록 ID 범위");
        uint token = Create.Entries[id];
        if (token == 0) throw new InvalidOperationException("채팅 알림의 미등록 칸");
        if (Node(token) is not ButtonGumpNode) throw new InvalidOperationException("채팅 알림의 실제 버튼 필드 미연결");
        ChatAlertId.Value = id;
    }

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
