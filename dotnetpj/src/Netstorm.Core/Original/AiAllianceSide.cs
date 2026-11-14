using System.Buffers.Binary;

// AI 생성 전에도 실제 설정의 동맹 선언을 읽어 난이도·시작 자금의 적 판정을 공유한다.
namespace Netstorm.Core.Original;

/// <summary>로컬 Player 동맹표와 AI 생성 전 설정의 동맹 선언을 함께 확인한다.</summary>
/// <remarks>cpppj/src/o/AiAllianceSide.h의 11차 보정이다. 원문 생성자00416860은 이 판정 뒤 Player 동맹표를 기록한다.</remarks>
public static class AiAllianceSide
{
    /// <summary>AI 설정의 동맹 owner 비트 마스크가 있는 비정렬 DWORD 위치다.</summary>
    public const int SettingsAllyMaskOffset = 0x23c;

    /// <summary>로컬 자신·이미 기록된 동맹·설정이 선언한 동맹을 제외한 AI만 적으로 판단한다.</summary>
    /// <param name="owner">AI 소유자0~8이다.</param>
    /// <param name="localOwner">로컬 소유자0~8이다.</param>
    /// <param name="localAllies">현재 로컬 Player+54의 동맹 비트 마스크다.</param>
    /// <param name="settings">동맹 DWORD까지 포함하는 현재 AI 설정이다. 일반 호출은600BYTE다.</param>
    public static bool IsAiEnemyOfLocal(uint owner, uint localOwner, uint localAllies, ReadOnlySpan<byte> settings)
    {
        if (settings.Length < SettingsAllyMaskOffset + sizeof(uint))
            throw new ArgumentException("AI 아군/적 판정: 설정 동맹 목록 범위 누락", nameof(settings));
        if (owner > 8 || localOwner > 8)
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 아군/적 판정: owner 범위 오류");
        if (owner == localOwner || ((localAllies >> (int)owner) & 1u) != 0) return false;
        uint declared = BinaryPrimitives.ReadUInt32LittleEndian(settings[SettingsAllyMaskOffset..]);
        return ((declared >> (int)localOwner) & 1u) == 0;
    }
}
