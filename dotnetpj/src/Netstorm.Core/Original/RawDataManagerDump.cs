using System.Globalization;
using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>현재 manager 가상 +4가 돌려주는 SNART 이름표를 빌린다(첫 trace 뒤 한 번 호출해 같은 변경 가능 표 포인터를 고정한다).</summary>
/// <param name="manager">대상 DataManager.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate Span<byte> DataManagerNamesReader(Span<byte> manager);

/// <summary>덤프 출력 장치다. level 29의 완성 문자열을 같은 실제 공유 trace 부모로 전달한다. 필터는 장치가 판정한다.</summary>
/// <param name="level">trace 수준(29).</param>
/// <param name="text">완성된 출력 문자열.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate void DataManagerTrace(int level, string text);

/// <summary>덤프가 쓰는 가상 호출과 출력 경계다(원본 <c>DataManagerDumpHooks</c>). 목록 검색과 끝 합계는 원문 부모가 수행한다.</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class DataManagerDumpHooks
{
    /// <summary>가상 +0의 현재 NUL 파일명을 빌린다. 첫 trace 전에 호출한다.</summary>
    public DataManagerFileNameReader? FileName { get; set; }

    /// <summary>첫 trace 뒤 가상 +4를 한 번 호출해 같은 변경 가능 SNART 표 포인터를 고정한다.</summary>
    public DataManagerNamesReader? Names { get; set; }

    /// <summary>같은 실제 60바이트 노드와 CRT 비교를 <see cref="RawDataManagerSelection.FindSection"/>에 제공한다. 버퍼 내용과 pos는 읽지 않는다.</summary>
    public DataManagerSelectionHooks Selection { get; set; } = new();

    /// <summary>실제 SNART 변환의 잘못된 첫 구분자 진단이다. 선택하지 않으면 필요 없다.</summary>
    public Action? SeparatorAssertion { get; set; }

    /// <summary>level 29의 완성 문자열을 같은 실제 공유 trace 부모로 전달한다.</summary>
    public DataManagerTrace? Trace { get; set; }
}

/// <summary>
/// 같은 manager의 가상 파일명·이름표와 실제 current 검색을 보존하는 원문 0044cf20 DataManager 덤프다.
/// 가상 호출과 trace 뒤 현재 물리를 다시 읽고 SNART 이름표의 실제 종료 바이트를 쓴다. cpppj <c>o/RawDataManagerDump</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다. 근거: docs/exe/cpp-datamanager-dump-20261101.md.</remarks>
public static class RawDataManagerDump
{
    /// <summary>원문 0044cf20의 모든 출력이 쓰는 0x1d 수준이다.</summary>
    private const int TraceLevel = 29;

    /// <summary>덤프가 요구하는 전체 manager의 실제 폭(276)이다.</summary>
    private const int FullManagerBytes = 276;

    /// <summary>섹션 노드 한 개의 실제 최소 물리 폭이다.</summary>
    private const int NodeBytes = 60;

    /// <summary>실제 stream +8(end)의 네 바이트만 읽는다. data/pos/alloc과 인접 이름은 보존한다.</summary>
    /// <param name="stream">60바이트 노드.</param>
    private static uint End(ReadOnlySpan<byte> stream)
    {
        if (stream.Length < NodeBytes)
        {
            throw OutOfRange("DataManager덤프의실제60BYTE노드누락");
        }
        return Read(stream, 8);
    }

    /// <summary>
    /// 고정 서식 <c>%04d</c>의 부호와 최소 폭을 CRT처럼 변환한다. 음수는 부호 뒤를 0으로 채워 전체 폭 4를 맞춘다(예: -1 → "-001").
    /// </summary>
    /// <param name="value">signed 값의 DWORD 표현.</param>
    private static string Total(uint value)
    {
        long signed = unchecked((int)value);
        string digits = Math.Abs(signed).ToString(CultureInfo.InvariantCulture);
        return signed < 0 ? "-" + digits.PadLeft(3, '0') : digits.PadLeft(4, '0');
    }

    /// <summary>
    /// 0044cf20: 파일 로그→가상 이름표→각 이름 검색/누락 로그→signed 표시 합계 로그다.
    /// 전체 manager는 276바이트 이상이며 prefix null은 원문 NULL의 빈 문자열이다. 합계는 end의 하위 DWORD로 감기며
    /// 비교/로그 예외 앞의 current·이름표 변이를 되돌리지 않는다.
    /// </summary>
    /// <param name="manager">덤프할 DataManager 276바이트.</param>
    /// <param name="prefix">출력 접두 문자열. null이면 빈 문자열이다.</param>
    /// <param name="hooks">파일명·이름표·검색·출력 장치.</param>
    public static void Dump(Span<byte> manager, string? prefix, DataManagerDumpHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (manager.Length < FullManagerBytes)
        {
            throw new ArgumentException("DataManager덤프의실제276BYTE manager누락");
        }
        string file = Need(hooks.FileName, "DataManager덤프의선택된실제경계미연결")(manager);
        DataManagerTrace trace = Need(hooks.Trace, "DataManager덤프의선택된실제경계미연결");
        trace(TraceLevel, "File " + CString(file) + ": " + CString(prefix ?? "") + "\n");
        Span<byte> names = Need(hooks.Names, "DataManager덤프의선택된실제경계미연결")(manager);
        uint total = 0;
        // 원문은 이름표의 현재 첫 바이트가 0일 때 종료하며 정상 순회 중에는 노드 end 합계의 DWORD 감김을 유지한다
        for (uint index = 0; ; index++)
        {
            Span<byte> selected = RawSnart.SelectName(names, index, hooks.SeparatorAssertion);
            if (selected[0] == 0)
            {
                trace(TraceLevel, Total(total) + " bytes total.\n");
                return;
            }
            string name = TerminatedText(selected, "DataManager덤프의현재이름NUL누락");
            uint current = RawDataManagerSelection.FindSection(manager, name, hooks.Selection);
            if (current != 0)
            {
                total += End(Need(hooks.Selection.Stream, "DataManager덤프의선택된실제경계미연결")(current));
            }
            else
            {
                // 비교 뒤 같은 이름 포인터의 현재 바이트를 로그 인수로 다시 읽는다
                trace(TraceLevel, "ERROR: Missing section " + TerminatedText(selected, "DataManager덤프의현재이름NUL누락") + "\n");
            }
        }
    }
}
