namespace Netstorm.Client.Host; // 내용물 수명이 선택한 실제 커서·음향 장치다.

/// <summary>장기 내용물 수명에서 현재 미션 또는 메뉴 커서를 취소한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>미션에서는 같은 Runtime을 호출 끝까지 유지하고 메뉴에서는 장기 커서 본체를 쓴다.</summary>
    private void CancelNativeContentCursor()
    {
        if (!_gameplay.Borrowed()) { CancelNativeCursorForState(); return; }
        HostSession session = _gameplay.RequireSession();
        using var pin = _gameplay.PinCurrentRuntime(session);
        session.CancelCursor();
    }

    /// <summary>생산 패널의 취소를 현재 Runtime 수명 안에서 같은 세션 커서 관문에 전달한다.</summary>
    private void CancelNativeProductionCursor()
    {
        HostSession session = _gameplay.RequireSession();
        using var pin = _gameplay.PinCurrentRuntime(session);
        session.CancelProductionCursor();
    }

    /// <summary>생산 내용물과 원본 플래그를 현재 세션의 실제 커서 취득에 전달한다.</summary>
    /// <param name="sid">현재 생산 내용물 SID다.</param>
    /// <param name="flags">원본 생산 입력의 취득 플래그다.</param>
    private void TakeNativeProductionContent(uint sid, uint flags)
    {
        HostSession session = _gameplay.RequireSession();
        using var pin = _gameplay.PinCurrentRuntime(session);
        session.TakeProductionCursor(checked((int)sid), flags);
    }

    /// <summary>선택된 너겟 수입 위치에 같은 장기 음향 장치를 사용한다.</summary>
    private void PlayNativeContentSound(float x, float y)
    {
        (_missionAudio ?? throw new InvalidOperationException("내용물 수입의 실제 위치 음향 미연결")).PlayEffectAt(x, y, "guy.wav", 0);
    }
}
