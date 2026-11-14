// 원문 TYPE HP getter가 현재 진단·특별 타입·약화 전역을 읽는 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>전송·표시에서 선택한 같은 TYPE의 HP를 공장과 공유한다.</summary>
public sealed partial class RawSquidFactory
{
    /// <summary>선택한 TYPE를 번호로 다시 고르지 않고 현재 HP와 전역을 읽는다. 입력은 같은 전체 TYPE 표다.</summary>
    public int ReadSelectedHitPoints(OriginalTypeRecord type, IReadOnlyList<OriginalTypeRecord> selectedBase)
    {
        ArgumentNullException.ThrowIfNull(type); ArgumentNullException.ThrowIfNull(selectedBase);
        int selected = -1;
        // 현재 물리 칸의 번호를 저장하며 원문 getNum 관문까지 다른 표 여부는 진단하지 않는다.
        for (int index = 0; index < selectedBase.Count; index++)
            if (ReferenceEquals(selectedBase[index], type)) { selected = index; break; }
        // C# 레코드 교체도 같은 물리 TYPE 칸의 현재 필드를 읽도록 저장한 번호를 역참조한다.
        OriginalTypeRecord Selected() => selected >= 0 ? selectedBase[selected] : type;
        // 선택 당시의 물리 칸과 현재 표 뷰의 상대 번호를 실제 getNum 위치에서 계산한다.
        uint Number()
        {
            if (selected < 0)
                throw new InvalidOperationException("TYPE HP의 서로 다른 물리 표 주소 연결");
            return RawRiftHitPoints.CurrentRiftTypeNumber(selected, selectedBase, CurrentTypes());
        }
        return RawRiftHitPoints.ReadSelected(_pool.Edition, Selected, new()
        {
            Checking = _diagnostics?.Checking ?? new RawCheckingFlag(),
            Quarter = new RawCheckingFlag(CurrentWeakenedMana),
            Priest = CurrentPriestType, Dais = CurrentDaisType,
        }, new()
        {
            Number = Number,
            Assertion = (expression, file, line) => (_diagnostics?.Assertion
                ?? throw new InvalidOperationException("TYPE HP 실제 진단 미연결"))(expression, file, line),
        });
    }
}
