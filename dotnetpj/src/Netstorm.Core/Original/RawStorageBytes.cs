using System.Text;

namespace Netstorm.Core.Original;

/// <summary>
/// 불투명 DWORD 핸들로 같은 물리 바이트 뷰를 빌리는 조회 장치다(원본 <c>std::function&lt;std::span&lt;uint8_t&gt;(uint32_t)&gt;</c>).
/// 호스트 포인터를 DWORD로 자르지 않고, 호출자가 소유한 같은 물리 배열의 일부를 사본 없이 돌려준다. 조회 자체는 소유권을 바꾸지 않는다.
/// </summary>
/// <param name="handle">원본 포인터를 대신하는 불투명 핸들.</param>
/// <remarks>2026-11-01 0.128 DataManager 계층 이식에서 추가했다. <see cref="Span{T}"/>를 돌려주므로 <see cref="Func{T,TResult}"/> 대신 전용 대리자를 쓴다.</remarks>
public delegate Span<byte> RawBytesLookup(uint handle);

/// <summary>
/// 읽기 전용 바이트 뷰를 돌려주는 조회 장치다(원본 <c>std::span&lt;const uint8_t&gt;(uint32_t)</c> 형태).
/// </summary>
/// <param name="handle">원본 포인터나 서식 번호를 대신하는 값.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate ReadOnlySpan<byte> RawReadOnlyBytesLookup(uint handle);

/// <summary>
/// DataManager·Template 계층(<c>o/RawMemStream</c>, <c>o/RawDataManager*</c>, <c>o/RawTemplate*</c>)이 공통으로 쓰는 비정렬 DWORD 읽기·쓰기와 장치 요구 도우미다.
/// cpppj에서는 각 .cpp의 익명 이름공간에 같은 함수가 반복되어 있어 한곳으로 모았다. 동작은 원본 little endian 네 바이트 합성/분해와 같다.
/// </summary>
/// <remarks>2026-11-01 0.128 DataManager 계층 이식에서 추가했다. 기존 Raw* 파일의 개별 private 도우미는 건드리지 않았다.</remarks>
internal static class RawStorageBytes
{
    /// <summary>비정렬 원본 DWORD를 지정 위치의 실제 네 바이트에서 읽는다. 인접 필드는 읽지 않는다.</summary>
    /// <param name="bytes">읽을 물리 바이트.</param>
    /// <param name="at">DWORD 시작 위치.</param>
    public static uint Read(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        // 낮은 바이트부터 네 바이트만 합친다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>지정한 DWORD의 네 바이트만 쓴다. 인접 이름·링크·예약 필드는 보존한다.</summary>
    /// <param name="bytes">쓸 물리 바이트.</param>
    /// <param name="at">DWORD 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    public static void Put(Span<byte> bytes, int at, uint value)
    {
        // 낮은 바이트부터 같은 네 바이트에만 기록한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>선택한 실제 장치가 연결됐는지 확인한다. 연결되지 않았으면 빈 성공으로 대신하지 않고 그 호출 지점에서 명시 오류로 멈춘다.</summary>
    /// <typeparam name="T">장치 대리자 형태.</typeparam>
    /// <param name="device">요구할 장치.</param>
    /// <param name="message">미연결 진단 문구.</param>
    public static T Need<T>(T? device, string message) where T : class
    {
        return device ?? throw new InvalidOperationException(message);
    }

    /// <summary>원본이 허용하지 않는 물리 범위 접근을 알리는 예외를 만든다(원본 <c>std::out_of_range</c> 대응).</summary>
    /// <param name="message">진단 문구.</param>
    public static ArgumentOutOfRangeException OutOfRange(string message)
    {
        return new ArgumentOutOfRangeException(null, message);
    }

    /// <summary>원본 바이트를 1:1로 문자로 바꾸는 부호화다. 비ASCII 바이트도 손실 없이 보존한다.</summary>
    public static Encoding Latin1 { get; } = Encoding.Latin1;

    /// <summary>C 문자열처럼 첫 NUL 이전만 남긴다. NUL이 없으면 그대로 돌려준다.</summary>
    /// <param name="text">NUL을 포함할 수 있는 문자열.</param>
    public static string CString(string text)
    {
        int end = text.IndexOf('\0');
        return end < 0 ? text : text[..end];
    }

    /// <summary>
    /// 바이트 뷰를 첫 NUL 이전까지 문자열로 읽는다. NUL이 뷰 안에 없으면 저장소 밖 읽기를 허용하지 않고 거부한다.
    /// </summary>
    /// <param name="bytes">읽을 물리 바이트.</param>
    /// <param name="message">NUL이 없을 때의 진단 문구.</param>
    public static string TerminatedText(ReadOnlySpan<byte> bytes, string message)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw OutOfRange(message);
        }
        return Latin1.GetString(bytes[..end]);
    }
}
