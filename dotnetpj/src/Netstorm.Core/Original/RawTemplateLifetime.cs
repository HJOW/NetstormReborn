using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>현재 DataManager의 실제 소멸과 할당 반납을 별개 효과로 연결한다(원본 <c>TemplateLifetimeHooks</c>).</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class TemplateLifetimeHooks
{
    /// <summary>004bd330의 실제 현재 manager 소멸이다.</summary>
    public Action<uint>? Destroy { get; set; }

    /// <summary>같은 저장 포인터의 CRT free다. 소멸 뒤 다시 헤더 포인터를 읽지 않는다.</summary>
    public Action<uint>? Release { get; set; }
}

/// <summary>
/// Template 9칸 전역 배열의 한 기록(20바이트: manager 포인터, 빌림 여부, isMine, 예약, 전송 값)의 실제 소유·빌림 분기와 초기화·반납 순서다.
/// 큰 소멸 효과 뒤 현재/뒤 칸을 다시 읽으며 원본이 저장한 manager 포인터는 유지한다. cpppj <c>o/RawTemplateLifetime</c>을 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public static class RawTemplateLifetime
{
    /// <summary>원본 Template 한 기록의 실제 폭이다.</summary>
    public const int HeaderBytes = 20;

    /// <summary>원본 전역 배열의 Template 칸 수다.</summary>
    public const int TemplateCount = 9;

    /// <summary>004bd2e0: 이전 포인터를 반납하지 않고 정확한 20바이트를 0으로 쓴다. 다른 폭은 원본 밖 쓰기 대신 명시적으로 거부한다.</summary>
    /// <param name="header">Template 기록 20바이트.</param>
    public static void InitializeHeader(Span<byte> header)
    {
        if (header.Length != HeaderBytes)
        {
            throw new ArgumentException("Template 실제20BYTE 헤더 누락");
        }
        header.Clear();
    }

    /// <summary>
    /// 004bf690: 소유한 manager(빌림 값이 0)만 소멸하고 free한 뒤, 포인터가 0이어도 20바이트를 비운다.
    /// 소멸이 헤더 포인터/빌림 값을 바꾸어도 처음 저장한 같은 manager를 free한다.
    /// </summary>
    /// <param name="header">Template 기록 20바이트.</param>
    /// <param name="hooks">소멸·반납 장치.</param>
    public static void ReleaseHeader(Span<byte> header, TemplateLifetimeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (header.Length != HeaderBytes)
        {
            throw new ArgumentException("Template 실제20BYTE 반납 헤더 누락");
        }
        uint manager = Read(header, 0);
        if (manager != 0 && Read(header, 4) == 0)
        {
            Need(hooks.Destroy, "Template 실제 소멸/할당 반납 미연결")(manager);
            Need(hooks.Release, "Template 실제 소멸/할당 반납 미연결")(manager);
        }
        InitializeHeader(header);
    }

    /// <summary>
    /// 004bfb00: 9칸 가운데 비영 포인터인 기록만 반납한다. 포인터 0인 기록의 stale 필드는 보존한다.
    /// 앞 소멸 효과가 바꾼 뒤 칸의 현재 포인터와 빌림 값을 순서대로 읽는다.
    /// </summary>
    /// <param name="headers">9칸 전체 180바이트.</param>
    /// <param name="hooks">소멸·반납 장치.</param>
    public static void ClearHeaders(Span<byte> headers, TemplateLifetimeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (headers.Length != TemplateCount * HeaderBytes)
        {
            throw new ArgumentException("Template 실제9개 헤더 누락");
        }
        // 앞 소멸 효과가 바꾼 뒤 칸의 현재 포인터를 순서대로 읽는다
        for (int slot = 0; slot < TemplateCount; slot++)
        {
            Span<byte> header = headers.Slice(slot * HeaderBytes, HeaderBytes);
            if (Read(header, 0) != 0)
            {
                ReleaseHeader(header, hooks);
            }
        }
    }
}
