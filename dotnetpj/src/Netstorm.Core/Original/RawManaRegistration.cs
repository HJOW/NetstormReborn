// 같은 현재 공급원 목록·카운터와 원문 중복 진단을 연결한다.
namespace Netstorm.Core.Original;

/// <summary>현재 checking DWORD와 선택된 마나 공급원 진단 장치다.</summary>
public sealed class ManaRegistrationDiagnostics
{
    /// <summary>PATCH 중복 위치에서 현재 전체 DWORD를 읽는다. CD는 읽지 않는다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>중복 단언 뒤 정상 복귀하거나 선택된 장치 실패를 전달한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>cpppj/src/o/RawManaRegistration.cpp/.h의00472f70/CD0048a360 실제 공급원 등록이다.</summary>
public static class RawManaRegistration
{
    /// <summary>최초 signed 개수로 검색하고 중복 판단을 저장한 뒤 현재 생산 카운터를 한 번 감김 증가한다.</summary>
    public static void Register(OriginalEdition edition, RawPostPopList list, ref uint productionDirty, uint value, ManaRegistrationDiagnostics diagnostics)
    {
        ArgumentNullException.ThrowIfNull(list); ArgumentNullException.ThrowIfNull(diagnostics);
        bool patch = edition == OriginalEdition.Patch1078, already = false; int count = unchecked((int)list.Count);
        if (count > 0)
        {
            uint[] storage = list.Entries; int index = 0;
            // 최초 양수 count 뒤 첫 원소는 현재 count 재조회보다 먼저 검사한다.
            do
            {
                if ((uint)index >= storage.Length) throw new ArgumentOutOfRangeException(nameof(list), "마나 공급원 검색의 실제 물리 범위 오류");
                if (storage[index] == value) { already = true; break; } index++;
            } while (index < (patch ? count : unchecked((int)list.Count)));
        }
        if (patch && diagnostics.Checking && already)
        {
            Action<string, string, uint> assertion = diagnostics.Assertion ?? throw new InvalidOperationException("마나 공급원 중복 원문 진단 장치 미연결");
            assertion("!alreadyThere", @"\Ns\O\Mana.cpp", 0x44);
        }
        if (!already)
        {
            int used = patch ? count : unchecked((int)list.Count); uint capacity = list.Capacity;
            if (used < unchecked((int)capacity))
            {
                if (used < 0) throw new ArgumentOutOfRangeException(nameof(list), "마나 공급원 음수 저장 인덱스 오류");
                uint[] storage = list.Entries;
                if ((uint)used >= storage.Length) throw new ArgumentOutOfRangeException(nameof(list), "마나 공급원 저장의 실제 물리 범위 오류");
                storage[used] = value; list.Count = unchecked(list.Count + 1);
            }
        }
        productionDirty = unchecked(productionDirty + 1);
    }
}
