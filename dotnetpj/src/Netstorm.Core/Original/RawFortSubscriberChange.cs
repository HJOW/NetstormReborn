using System.Text;

namespace Netstorm.Core.Original;

/// <summary>
/// 현재 Template·handle·My를 직접 빌리는 원문 공용 계층이다(cpppj <c>o/RawFortSubscriberChange.h</c>의 <c>FortSubscriberChangeHooks</c>).
/// 큰 하위가 현재 owner·handle을 바꿀 수 있으므로 진입 때 값으로 고정하지 않는다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortSubscriberChangeHooks
{
    /// <summary>00540cac의 현재 슬롯이다.</summary>
    public Func<uint>? Owner { get; set; }

    /// <summary>같은 20바이트 Template 헤더 첫 DWORD다.</summary>
    public Func<uint, uint>? Manager { get; set; }

    /// <summary>00594fb8이며 manager 비영 뒤에만 읽는다.</summary>
    public Func<uint>? FortMode { get; set; }

    /// <summary>00540ca8이 가리키는 현재 NUL 포함 물리 저장소다.</summary>
    public FortSubscriberHandle? Handle { get; set; }

    /// <summary>이름 복사 뒤 fort.mySubHandle을 쓰는 004414d0이다.</summary>
    public Action<string, string>? Configure { get; set; }

    /// <summary>설정 복귀 뒤 00460730이며 My 쓰기보다 앞이다.</summary>
    public Action? Title { get; set; }

    /// <summary>004bd400의 고정 인자 ID·현재 handle·현재 슬롯이다.</summary>
    public FortSubscriberChangeSubscriber? Subscriber { get; set; }

    /// <summary>writer 복귀 뒤 현재 슬롯의 004bd360이다.</summary>
    public Action<uint>? Notify { get; set; }
}

/// <summary>
/// 00540ca8이 가리키는 현재 NUL 포함 물리 저장소를 빌린다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public delegate Span<byte> FortSubscriberHandle();

/// <summary>
/// 004bd400의 고정 인자 ID·현재 handle·현재 슬롯을 전달한다.
/// </summary>
/// <param name="owner">현재 슬롯이다.</param>
/// <param name="id">부모 인자 ID다.</param>
/// <param name="current">현재 handle 물리 범위다. NUL 뒤 바이트도 함께 전달한다.</param>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public delegate void FortSubscriberChangeSubscriber(uint owner, uint id, ReadOnlySpan<byte> current);

/// <summary>
/// 원문 004b32f0의 현재 Fort Subscriber 변경과 큰 효과 뒤 재조회를 복원한다(cpppj <c>o/RawFortSubscriberChange</c>).
/// name 부재는 handle·설정·제목을 건드리지 않으며 앞선 부분 효과를 예외 때 되돌리지 않는다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public static class RawFortSubscriberChange
{
    /// <summary>
    /// 선택되지 않은 하위를 빈 성공으로 채우지 않는다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="effect">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort Subscriber 변경 실제 장치가 연결되지 않았다.</exception>
    private static T NeedChange<T>(T? effect) where T : class
    {
        if (effect is null)
        {
            throw new InvalidOperationException("Fort Subscriber 변경 실제 장치 누락");
        }
        return effect;
    }

    /// <summary>
    /// 실제 handle의 첫 NUL까지만 읽으며 저장소 밖의 원문 접근은 host에서 거부한다.
    /// </summary>
    /// <param name="bytes">고를 물리 저장소다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort Subscriber handle NUL 누락이다.</exception>
    private static ReadOnlySpan<byte> CString(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "Fort Subscriber handle NUL 누락");
        }
        return bytes[..(end + 1)];
    }

    /// <summary>
    /// Template 부재는 fortMode조차 읽지 않는다. 큰 설정·제목·writer 뒤에는 현재 owner·handle을 다시 읽는다.
    /// </summary>
    /// <param name="id">쓸 Subscriber ID다.</param>
    /// <param name="name">복사할 NUL 포함 이름이다. null이면 handle·설정·제목을 건드리지 않는다.</param>
    /// <param name="my">현재 My 저장소다. 호출자가 소유한다.</param>
    /// <param name="hooks">실제 공용 경계다.</param>
    public static void ChangeFortSubscriber(uint id, byte[]? name, ref uint my, FortSubscriberChangeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (NeedChange(hooks.Manager)(NeedChange(hooks.Owner)()) == 0 || NeedChange(hooks.FortMode)() == 0)
        {
            return;
        }
        if (name is not null)
        {
            Span<byte> destination = NeedChange(hooks.Handle)();
            int index = 0;
            for (; ; )
            {
                // 원문 strcpy처럼 NUL까지 순서대로 쓰며 물리 범위 실패 전의 바이트는 유지한다
                if (index >= name.Length || index >= destination.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(name), "Fort Subscriber strcpy 물리 범위 부족");
                }
                byte value = name[index];
                destination[index++] = value;
                if (value == 0)
                {
                    break;
                }
            }
            ReadOnlySpan<byte> current = CString(NeedChange(hooks.Handle)());
            NeedChange(hooks.Configure)("fort.mySubHandle", Encoding.Latin1.GetString(current[..^1]));
            NeedChange(hooks.Title)();
        }
        my = id;
        ReadOnlySpan<byte> handle = NeedChange(hooks.Handle)();
        uint owner = NeedChange(hooks.Owner)();
        NeedChange(hooks.Subscriber)(owner, id, handle);
        NeedChange(hooks.Notify)(NeedChange(hooks.Owner)());
    }
}
