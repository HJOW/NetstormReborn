namespace Netstorm.Core.Original;

/// <summary>
/// 원본 MemStream 진단 종류(원본 <c>o/RawMemStream.h</c>의 <c>MemStreamAssertion</c>).
/// 진단 뒤에도 원본 본문이 계속하는 두 조건을 구별한다. Fort SP writer는 쓰기 용량만 쓴다.
/// MemStream 본체(Allocate·Copy·Write·Compact)를 옮길 때 이 자리에서 그 파일로 옮긴다.
/// 근거: cpppj/src/o/RawMemStream.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public enum MemStreamAssertion
{
    /// <summary>기존 자료가 있다는 진단. Fort SP writer는 쓰지 않는다.</summary>
    ExistingData = 0,

    /// <summary>쓰기 용량 진단. Fort SP writer가 현재 위치+폭이 용량을 넘을 때 호출자에게 알린다.</summary>
    WriteCapacity = 1,
}

/// <summary>
/// Fort SP writer가 쓰는 MemStream 장치 연결(원본 <c>MemStreamHooks</c>).
/// 실제 힙 포인터 대신 불투명 DWORD 토큰으로 호출자가 소유한 같은 물리 버퍼를 찾는다.
/// 이 writer는 버퍼 조회와 쓰기 용량 진단만 쓰며, 할당·반납은 MemStream 본체를 옮길 때 연결한다.
/// 근거: cpppj/src/o/RawMemStream.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class MemStreamHooks
{
    /// <summary>토큰으로 호출자가 소유한 같은 물리 버퍼 전체를 찾는다. 조회는 소유권을 바꾸지 않으며, 사본을 만들지 않는다.</summary>
    public Func<uint, byte[]>? Buffer { get; set; }

    /// <summary>원본 operator new의 요청 크기와 반환 포인터. Fort SP writer는 쓰지 않는다.</summary>
    public Func<uint, uint>? Allocate { get; set; }

    /// <summary>원본 CRT free. 호출 때의 같은 포인터를 반납한다. Fort SP writer는 쓰지 않는다.</summary>
    public Action<uint>? Release { get; set; }

    /// <summary>선택된 진단만 호출자에게 알린다. 반환 뒤 writer는 현재 필드를 다시 읽는다.</summary>
    public Action<MemStreamAssertion>? Assertion { get; set; }
}

/// <summary>
/// Fort SP 쓰기의 외부 장치 연결(원본 <c>FortSpWriteHooks</c>).
/// 원문의 512회 Next(32)와 인라인 stream 쓰기 장치를 빌리며, 난수·버퍼를 별도로 소유하지 않는다.
/// 근거: cpppj/src/o/RawFortSpWrite.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class FortSpWriteHooks
{
    /// <summary>같은 게임 난수를 그 시점에 호출한다. 정상 호출은 항상 상한 32다. null이면 거부한다.</summary>
    public Func<uint, uint>? NextRandom { get; set; }

    /// <summary>버퍼 조회와 쓰기 용량 진단만 쓰는 MemStream 장치.</summary>
    public MemStreamHooks Stream { get; } = new();
}

/// <summary>
/// 요새 저장용 SP writer(원본 PATCH 004be960의 267바이트 SP writer를 같은 MemStream 16바이트·공유 게임 난수에 실행).
/// 호출자의 float32를 한 번만 실제 _ftol 계약으로 읽어 하위 DWORD를 만든 뒤, 각 비트에 16회 난수 잡음을 섞어
/// 8바이트·8워드·16더블워드 총 88바이트를 쓴다. 비유한·64비트 범위 밖 입력의 masked_ftol 불확정값은 하위 DWORD 0이다.
/// stream과 buffer가 같은 실제 할당을 가리켜도 사본으로 분리하지 않으며, buffer 조회는 순수 물리 뷰여야 한다.
/// 근거: cpppj/src/o/RawFortSpWrite.cpp (R0-4 신규 이식, 2026-10-28).
/// </summary>
public static class RawFortSpWrite
{
    /// <summary>masked_ftol의 signed 64비트 변환 한계. float32는 double로 정확히 옮겨진다.</summary>
    private const double Signed64Limit = 9223372036854775808.0;

    /// <summary>한 비트가 소비하는 난수 횟수. 같은 비트의 잡음 OR에서 제외해도 소비 자체는 생략하지 않는다.</summary>
    private const uint NoisePerBit = 16;

    /// <summary>
    /// 원본 WriteFortSp: 32비트 순서대로 각 비트를 고정하고 16회 난수를 섞은 뒤 인라인 폭으로 써서 현재 위치·끝을 갱신한다.
    /// 일반 writer 호출은 없으며, 진단 뒤에도 현재 위치·자료를 다시 읽어 계속한다.
    /// </summary>
    /// <param name="stream">MemStream 16바이트(호출자 소유). 위치·끝·용량을 읽고 쓴다.</param>
    /// <param name="value">쓸 SP의 float32 값. 한 번만 정수 계약으로 읽는다.</param>
    /// <param name="hooks">난수·버퍼·진단의 외부 장치 연결.</param>
    public static void Write(Span<byte> stream, float value, FortSpWriteHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint integer = Integer(value);
        // 원문 EBP 0..31 순서이며 비트별 일반 writer 호출은 없다
        for (uint bit = 0; bit < 32; bit++)
        {
            uint word = integer & (1u << (int)bit);
            // 동일한 32 범위 난수를 매 비트 정확히 16회 소비한다
            for (uint noise = 0; noise < NoisePerBit; noise++)
            {
                uint selected = Need(hooks.NextRandom, "Fort SP writer의 실제 선택장치 누락")(32);
                if (selected != bit)
                {
                    word |= 1u << (int)(selected & 31u);
                }
            }
            uint width = bit < 8 ? 1u : bit < 16 ? 2u : 4u;
            uint capacity, sum;
            // 바이트 경로는 합을 먼저, 워드·더블워드 경로는 원문처럼 할당 용량을 위치 앞에 읽는다
            if (bit < 8)
            {
                sum = Read(stream, 4) + width;
                capacity = Read(stream, 12);
            }
            else
            {
                capacity = Read(stream, 12);
                sum = Read(stream, 4) + width;
            }
            if (unchecked((int)sum) > unchecked((int)capacity))
            {
                Need(hooks.Stream.Assertion, "Fort SP writer의 실제 선택장치 누락")(MemStreamAssertion.WriteCapacity);
            }
            // 진단 콜백 뒤 현재 위치·자료를 그 순서로 다시 읽는다
            uint position = Read(stream, 4);
            uint pointer = Read(stream, 0);
            if (pointer == 0)
            {
                throw new InvalidOperationException("Fort SP writer의 현재 버퍼 포인터 누락");
            }
            // 같은 stream 자체를 덮는 실제 버퍼도 그대로 쓴다
            byte[] bytes = Need(hooks.Stream.Buffer, "Fort SP writer의 실제 선택장치 누락")(pointer);
            Put(bytes, position, word, width);
            // 출력 쓰기 효과 뒤 위치·끝을 다시 읽고 끝은 위치 저장 앞에 보관한다
            uint next = Read(stream, 4) + width;
            uint end = Read(stream, 8);
            Put(stream, 4, next);
            if (unchecked((int)next) > unchecked((int)end))
            {
                Put(stream, 8, next);
            }
        }
    }

    /// <summary>실제 선택한 장치만 그 자리에서 요구한다. 비어 있으면 명시 경계 오류로 멈춘다.</summary>
    /// <typeparam name="T">장치 델리게이트 형태.</typeparam>
    /// <param name="function">요구할 장치. null이면 거부한다.</param>
    /// <param name="message">거부할 때의 진단 문구.</param>
    private static T Need<T>(T? function, string message) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException(message);
        }
        return function;
    }

    /// <summary>같은 실제 물리의 선택 폭만 검사한다. 범위 밖 접근은 명시 호스트 계약 오류다.</summary>
    /// <param name="bytes">검사할 물리 전체.</param>
    /// <param name="offset">읽거나 쓸 시작 위치.</param>
    /// <param name="width">읽거나 쓸 폭.</param>
    private static Span<byte> Range(Span<byte> bytes, uint offset, uint width)
    {
        if (offset > (uint)bytes.Length || width > (uint)bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "Fort SP writer의 실제 물리 범위");
        }
        return bytes.Slice((int)offset, (int)width);
    }

    /// <summary>지정한 비정렬 DWORD를 리틀 엔디언으로 읽으며 다른 필드를 복제하지 않는다.</summary>
    /// <param name="bytes">읽을 물리 전체.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint Read(Span<byte> bytes, uint offset)
    {
        Span<byte> field = Range(bytes, offset, 4);
        uint value = 0;
        // 같은 필드의 현재 네 바이트를 원문 DWORD의 낮은 자리부터 조합한다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)field[index] << (index * 8);
        }
        return value;
    }

    /// <summary>현재 필드나 출력의 같은 바이트·워드·더블워드 폭만 쓴다.</summary>
    /// <param name="bytes">쓸 물리 전체.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 DWORD 값. 폭 밖 바이트는 버린다.</param>
    /// <param name="width">쓸 폭(1·2·4). 생략하면 DWORD 4다.</param>
    private static void Put(Span<byte> bytes, uint offset, uint value, uint width = 4)
    {
        Span<byte> field = Range(bytes, offset, width);
        // 폭 밖의 인접 필드는 보존한다
        for (int index = 0; index < (int)width; index++)
        {
            field[index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>실제 004e49c0의 0 방향 signed 64비트 절삭과 EAX 하위 DWORD 계약이다. 호스트의 범위 밖 정수 변환을 실행하지 않는다.</summary>
    /// <param name="input">변환할 float32 값.</param>
    private static uint Integer(float input)
    {
        double value = input;
        if (!double.IsFinite(value) || value >= Signed64Limit || value < -Signed64Limit)
        {
            return 0;
        }
        return unchecked((uint)(ulong)(long)value);
    }
}
