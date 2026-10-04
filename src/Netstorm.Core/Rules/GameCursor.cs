namespace Netstorm.Core.Rules;

/// <summary>TEST02 원본 커서 기록과 RT_CURSOR 픽셀 대조로 확인한 다섯 가지 상태.</summary>
public enum GameCursor
{
    /// <summary>기본 흰 화살표 (그룹 113).</summary>
    Arrow,
    /// <summary>사제의 이동 불가 대상·허공 (그룹 109).</summary>
    Forbidden,
    /// <summary>선택한 사제의 지면 이동 목표 (그룹 148).</summary>
    Move,
    /// <summary>선택한 사제의 내 완공 템플 (그룹 111).</summary>
    Temple,
    /// <summary>건물을 든 상태의 굵은 십자 (그룹 110).</summary>
    Place,
}
