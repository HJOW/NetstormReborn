namespace Netstorm.Core.Original;

/// <summary>
/// 같은 명시 stream에 기존 WriteMemStream으로 연결한다. 입력 span을 선행 복사·절단하지 않는다.
/// </summary>
/// <param name="stream">쓸 stream 토큰이다.</param>
/// <param name="bytes">쓸 바이트 구간이다.</param>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public delegate void FortSubscriberWriteWriter(uint stream, ReadOnlySpan<byte> bytes);

/// <summary>
/// 현재 manager·등록·stream 장치를 기존 저장 본체에 연결한다(cpppj <c>o/RawFortSubscriberWrite.h</c>의 <c>FortSubscriberWriteHooks</c>).
/// 선택한 외부 경계만 요구하고 실제 이름·일반 writer는 기존 본체를 재사용한다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortSubscriberWriteHooks
{
    /// <summary>고정 this 슬롯의 현재 manager를 진단 전과 등록 helper 복귀 뒤 다시 읽는다.</summary>
    public Func<uint, uint>? Manager { get; set; }

    /// <summary>Template.cpp 463(manager 0)·155(null 이름)의 실제 줄 진단이며 복귀하면 원문을 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }

    /// <summary>004b38d0의 실제 FinishFortSubscriberRegistration을 같은 Client 전역·Config에 연결한다.</summary>
    public Action? RegisterSubscriber { get; set; }

    /// <summary>현재 manager에서 CreateDataManagerSection("Subscriber")가 반환한 고정 stream 토큰이다.</summary>
    public Func<uint, string, uint>? Create { get; set; }

    /// <summary>같은 명시 stream에 기존 WriteMemStream을 연결한다. 입력 span을 선행 복사·절단하지 않는다.</summary>
    public FortSubscriberWriteWriter? Write { get; set; }
}

/// <summary>
/// 원본 004bd400의 Subscriber 생성과 캡처한 ID·현재 원문 이름 쓰기 부모다(cpppj <c>o/RawFortSubscriberWrite</c>).
/// ID는 부모의 인자값을 유지하고 이름은 캡처된 같은 물리 포인터를 ID 쓰기 뒤 문자열 writer에서 처음 읽는다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public static class RawFortSubscriberWrite
{
    /// <summary>
    /// 선택한 실제 하위만 필요한 순간에 요구한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort Subscriber 저장의 실제 하위가 연결되지 않았다.</exception>
    private static T NeedSubscriber<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort Subscriber 저장의 실제 하위 미연결");
        }
        return function;
    }

    /// <summary>
    /// helper가 global ID·이름 포인터·manager를 바꾸어도 원문이 캡처·재조회하는 경계만 보존한다.
    /// </summary>
    /// <param name="slot">고정 this 슬롯이다.</param>
    /// <param name="capturedId">등록 helper와 별개인 부모 인자 ID다.</param>
    /// <param name="capturedHandle">ID 쓰기 뒤 처음 읽는 같은 물리 이름이다. null이면 null 입력이다.</param>
    /// <param name="hooks">실제 저장 경계다.</param>
    public static void SaveFortSubscriber(uint slot, uint capturedId, byte[]? capturedHandle, FortSubscriberWriteHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (NeedSubscriber(hooks.Manager)(slot) == 0)
        {
            NeedSubscriber(hooks.Assertion)(0x1cfu);
        }
        NeedSubscriber(hooks.RegisterSubscriber)();
        uint manager = NeedSubscriber(hooks.Manager)(slot);
        uint stream = NeedSubscriber(hooks.Create)(manager, "Subscriber");
        byte[] id = new byte[4];
        // global의 현재 재등록값과 별개인 부모 인자 4바이트다
        for (int index = 0; index < id.Length; index++)
        {
            id[index] = (byte)(capturedId >> (index * 8));
        }
        NeedSubscriber(hooks.Write)(stream, id);
        FortMissionSaveHooks mission = new()
        {
            Write = (target, source) => NeedSubscriber(hooks.Write)(target, source),
            Assertion = _ => NeedSubscriber(hooks.Assertion)(0x9bu),
        };
        // 기존 004bcbf0의 null 판정과 길이 WORD·본문 일반 쓰기를 그대로 공유한다
        RawFortMissionSave.SaveFortMissionName(stream, capturedHandle, mission);
    }
}
