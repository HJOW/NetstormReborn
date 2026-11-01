using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>
/// 같은 SNART 물리 이름표를 제자리 변환하고 현재 이름의 시작을 빌리는 공유 도구다(원문 004a8800/004a8850).
/// 파일 이미지 읽기와 신규 DataManager 초기화가 같은 변경 가능 표를 함께 빌린다. cpppj <c>o/RawSnart</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다. 표를 사전 문자열 목록으로 복사하지 않고 한 번 변환한 첫 바이트 0과 마지막 FF를 다음 호출에도 그대로 남긴다.</remarks>
public static class RawSnart
{
    /// <summary>이름표 첫 바이트가 가질 수 있는 구분자('/')다.</summary>
    private const byte Separator = (byte)'/';

    /// <summary>이름표의 끝 표시 바이트다. 첫 NUL을 이 값으로 바꾼다.</summary>
    private const byte EndMark = 0xff;

    /// <summary>
    /// 지정 번호 이름의 시작부터 같은 물리 끝까지의 뷰를 돌려준다. 길이를 미리 읽지 않는다.
    /// 첫 호출에서만 '/'를 NUL로 바꾸고 첫 NUL을 FF로 바꾼다(첫 바이트가 0이면 이미 변환된 표다).
    /// </summary>
    /// <param name="names">변경 가능한 이름표 물리 바이트.</param>
    /// <param name="index">찾을 이름 번호.</param>
    /// <param name="assertion">첫 구분자가 '/'가 아닐 때의 Snart.cpp 0x46 진단. 복귀하면 계속한다.</param>
    public static Span<byte> SelectName(Span<byte> names, uint index, Action? assertion)
    {
        if (names.IsEmpty)
        {
            throw OutOfRange("SNART 실제 이름표 누락");
        }
        if (names[0] != 0)
        {
            if (names[0] != Separator)
            {
                Need(assertion, "SNART separator 실제 진단 미연결")();
            }
            int end = names.IndexOf((byte)0);
            if (end < 0)
            {
                throw OutOfRange("SNART 입력 NUL 누락");
            }
            names[end] = EndMark;
            // 원문 첫 FF 전의 슬래시만 NUL로 바꾸며 나머지 원시 바이트는 보존한다
            for (int at = 0; at < names.Length && names[at] != EndMark; at++)
            {
                if (names[at] == Separator)
                {
                    names[at] = 0;
                }
            }
        }
        int cursor = 1;
        // 기존 파일 reader와 같은 현재 NUL 개수만 지나고 다음 FF를 만나면 멈춘다
        while (index != 0)
        {
            if (cursor >= names.Length)
            {
                throw OutOfRange("SNART 이름 번호 범위");
            }
            if (names[cursor] == 0)
            {
                index--;
                cursor++;
                if (cursor >= names.Length)
                {
                    throw OutOfRange("SNART 끝 누락");
                }
                if (names[cursor] == EndMark)
                {
                    break;
                }
            }
            else
            {
                cursor++;
            }
        }
        if (cursor >= names.Length)
        {
            throw OutOfRange("SNART 이름 시작 범위");
        }
        return names[cursor..];
    }

    /// <summary>기존 파일 이미지 reader처럼 선택한 이름을 현재 첫 NUL까지 읽는 편의 경계다. 신규 초기화 부모와 달리 현재 길이를 읽는 시점이 다르다.</summary>
    /// <param name="names">변경 가능한 이름표 물리 바이트.</param>
    /// <param name="index">찾을 이름 번호.</param>
    /// <param name="assertion">첫 구분자 진단.</param>
    public static string ReadName(Span<byte> names, uint index, Action? assertion)
    {
        Span<byte> tail = SelectName(names, index, assertion);
        return TerminatedText(tail, "SNART 이름 NUL 누락");
    }
}
