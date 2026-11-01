using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 MemStream 진단 종류(원본 <c>o/RawMemStream.h</c>의 <c>MemStreamAssertion</c>).
/// 진단 뒤에도 원본 본문이 계속하는 두 조건을 구별한다.
/// </summary>
/// <remarks>R0-4(2026-10-28)에 Fort SP writer가 쓰는 만큼만 <c>RawFortSpWrite.cs</c>에 있었고, 2026-11-01 0.128에서 MemStream 본체와 함께 이 파일로 옮겼다.</remarks>
public enum MemStreamAssertion
{
    /// <summary>기존 자료가 있다는 진단(할당 선행 쓰기). 반환 뒤 원본 본문이 계속한다.</summary>
    ExistingData = 0,

    /// <summary>쓰기 용량 진단(현재 위치+길이가 할당을 넘음). 반환 뒤 원본 본문이 현재 필드를 다시 읽어 계속한다.</summary>
    WriteCapacity = 1,
}

/// <summary>
/// MemStream 16바이트의 할당·쓰기·압축이 쓰는 외부 장치 연결(원본 <c>MemStreamHooks</c>).
/// 실제 힙 포인터 대신 불투명 DWORD로 같은 소유 버퍼를 찾는다. 선택된 경계만 필요하며, 연결되지 않은 장치를 실제로 호출하는 쪽이 명시 오류를 낸다.
/// </summary>
/// <remarks>R0-4(2026-10-28)에 처음 만들었고, 2026-11-01 0.128에서 버퍼 조회를 <see cref="RawBytesLookup"/>(Span 반환)으로 바꾸어 DataManager 노드·버퍼에도 같은 형식을 쓴다.</remarks>
public sealed class MemStreamHooks
{
    /// <summary>토큰으로 호출자가 소유한 같은 물리 버퍼 전체를 찾는다. 조회는 소유권을 바꾸지 않으며 사본을 만들지 않는다.</summary>
    public RawBytesLookup? Buffer { get; set; }

    /// <summary>원본 operator new의 요청 크기와 반환 포인터다.</summary>
    public Func<uint, uint>? Allocate { get; set; }

    /// <summary>원본 CRT free다. 호출 때의 같은 포인터를 반납한다.</summary>
    public Action<uint>? Release { get; set; }

    /// <summary>선택된 진단만 호출한다. 반환 뒤 호출 본문은 현재 필드를 다시 읽는다.</summary>
    public Action<MemStreamAssertion>? Assertion { get; set; }
}

/// <summary>
/// 10.78 MemStream 16바이트(+0 data 포인터, +4 pos, +8 end, +c alloc)에 할당·쓰기·압축을 원본 순서로 적용한다.
/// cpppj <c>o/RawMemStream</c>을 옮겼다. 일반 memcpy로 치환하지 않고 원본의 DWORD 단위 읽기→쓰기 순서와 진단·free 뒤 필드 재조회를 보존한다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다. 이 파일을 쓰는 주요 소비자는 <see cref="RawDataManagerResize"/>·<see cref="RawDataManagerList"/>·<see cref="RawDataManagerImage"/>다.</remarks>
public static class RawMemStream
{
    /// <summary>MemStream 한 개의 실제 물리 폭(data, pos, end, alloc의 네 DWORD)이다.</summary>
    private const int StreamBytes = 16;

    /// <summary>같은 16바이트 계약을 본문의 첫 쓰기 전에 확인한다. 부족하면 어떤 필드도 쓰기 전에 거부한다.</summary>
    /// <param name="stream">검사할 MemStream.</param>
    private static void Physical(ReadOnlySpan<byte> stream)
    {
        if (stream.Length < StreamBytes)
        {
            throw new ArgumentException("MemStream 실제16BYTE 누락");
        }
    }

    /// <summary>진단 소비자를 호출한다. 진단을 무시하는 빈 소비자를 자동으로 추가하지 않는다.</summary>
    /// <param name="hooks">외부 장치 연결.</param>
    /// <param name="kind">호출할 진단 종류.</param>
    private static void Assert(MemStreamHooks hooks, MemStreamAssertion kind)
    {
        Need(hooks.Assertion, "MemStream 실제 진단 미연결")(kind);
    }

    /// <summary>원본이 실제로 읽거나 쓰는 범위만 빌린다. 길이 0은 버퍼 조회 없이 허용한다.</summary>
    /// <param name="hooks">버퍼 조회 장치.</param>
    /// <param name="pointer">버퍼 핸들.</param>
    /// <param name="position">버퍼 안 시작 위치.</param>
    /// <param name="length">필요한 길이.</param>
    private static Span<byte> BufferRange(MemStreamHooks hooks, uint pointer, uint position, uint length)
    {
        if (length == 0)
        {
            return default;
        }
        if (pointer == 0 || hooks.Buffer is null)
        {
            throw new InvalidOperationException("MemStream 실제 버퍼 조회 미연결");
        }
        Span<byte> bytes = hooks.Buffer(pointer);
        if (position > (uint)bytes.Length || length > (uint)bytes.Length - position)
        {
            throw OutOfRange("MemStream 실제 버퍼 물리 범위");
        }
        return bytes.Slice((int)position, (int)length);
    }

    /// <summary>
    /// 0044cc20: 선행 자료가 있으면 진단하고, alloc을 먼저 쓴 뒤 할당하고, 할당기가 alloc을 바꾸면 그 현재 값만큼 0으로 채운 뒤 pos·end를 0으로 쓴다.
    /// </summary>
    /// <param name="stream">MemStream 16바이트(호출자 소유).</param>
    /// <param name="size">요청 크기.</param>
    /// <param name="hooks">할당·버퍼·진단 장치.</param>
    public static void Allocate(Span<byte> stream, uint size, MemStreamHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Physical(stream);
        if (Read(stream, 0) != 0)
        {
            Assert(hooks, MemStreamAssertion.ExistingData);
        }
        Put(stream, 12, size);
        uint pointer = Need(hooks.Allocate, "MemStream 실제 할당 미연결")(size);
        Put(stream, 0, pointer);
        // 할당기가 alloc을 바꾸었어도 원본처럼 그 현재 값만큼 0으로 채운다
        uint length = Read(stream, 12);
        BufferRange(hooks, pointer, 0, length).Clear();
        Put(stream, 4, 0);
        Put(stream, 8, 0);
    }

    /// <summary>
    /// 0044cc80: 기존 할당을 반납하고 첫 16바이트를 비운 뒤 원문 end로 할당하고, 원문의 현재 data/end를 복사한다. 대입 뒤 pos는 0이다.
    /// 겹치는 복사도 같은 DWORD 전체를 읽은 뒤 쓰며 일반 memcpy로 바꾸지 않는다.
    /// </summary>
    /// <param name="stream">대상 MemStream 16바이트.</param>
    /// <param name="source">원문 MemStream 16바이트.</param>
    /// <param name="hooks">할당·반납·버퍼·진단 장치.</param>
    public static void Copy(Span<byte> stream, ReadOnlySpan<byte> source, MemStreamHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Physical(stream);
        if (source.Length < StreamBytes)
        {
            throw new ArgumentException("MemStream 대입의 실제 원문16BYTE 누락");
        }
        uint previous = Read(stream, 0);
        if (previous != 0)
        {
            Need(hooks.Release, "MemStream 대입의 실제 반납 미연결")(previous);
        }
        Put(stream, 0, 0);
        Put(stream, 12, 0);
        Put(stream, 8, 0);
        Put(stream, 4, 0);
        Allocate(stream, Read(source, 8), hooks);
        uint length = Read(source, 8);
        Span<byte> input = BufferRange(hooks, Read(source, 0), 0, length);
        Span<byte> output = BufferRange(hooks, Read(stream, 0), 0, length);
        uint position = 0;
        // 겹침도 같은 DWORD 전체를 읽은 뒤 쓴다
        for (; length - position >= 4; position += 4)
        {
            uint value = Read(input, (int)position);
            Put(output, (int)position, value);
        }
        // 남은 1~3바이트를 앞에서부터 순서대로 복사한다
        for (; position < length; position++)
        {
            output[(int)position] = input[(int)position];
        }
        Put(stream, 8, Read(source, 8));
    }

    /// <summary>
    /// 0044cce0: signed 용량이 모자라면 진단한 뒤 한 DWORD씩 앞에서부터 복사하고 나머지 바이트를 복사한다. 현재 pos와 end를 갱신한다.
    /// 겹치는 버퍼도 원본처럼 한 DWORD를 완전히 읽은 뒤 같은 네 바이트를 쓴다.
    /// </summary>
    /// <param name="stream">대상 MemStream 16바이트.</param>
    /// <param name="source">쓸 원문 바이트.</param>
    /// <param name="length">쓸 길이.</param>
    /// <param name="hooks">버퍼·진단 장치.</param>
    public static void Write(Span<byte> stream, ReadOnlySpan<byte> source, uint length, MemStreamHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Physical(stream);
        uint sum = Read(stream, 4) + length;
        if (unchecked((int)Read(stream, 12)) < unchecked((int)sum))
        {
            Assert(hooks, MemStreamAssertion.WriteCapacity);
        }
        if (length > (uint)source.Length)
        {
            throw OutOfRange("MemStream 실제 원문 물리 범위");
        }
        Span<byte> target = BufferRange(hooks, Read(stream, 0), Read(stream, 4), length);
        uint position = 0;
        // 한 DWORD를 완전히 읽은 뒤 같은 네 바이트를 쓴다
        for (; length - position >= 4; position += 4)
        {
            uint value = Read(source, (int)position);
            Put(target, (int)position, value);
        }
        // 남은 1~3바이트는 앞에서부터 한 바이트씩 읽고 쓴다
        for (; position < length; position++)
        {
            target[(int)position] = source[(int)position];
        }
        uint current = Read(stream, 4) + length;
        Put(stream, 4, current);
        if (unchecked((int)Read(stream, 8)) < unchecked((int)current))
        {
            Put(stream, 8, current);
        }
    }

    /// <summary>
    /// 0044cd40: end==alloc이면 무동작이다. end가 양수이면 end 크기로 다시 할당해 현재 내용을 복사하고 옛 버퍼를 반납한다(반납 뒤 필드를 다시 0으로 덮지 않는다).
    /// 그 밖에는 실제 소멸(<see cref="RawDataManagerLifetime.DestroyMemStream"/>)이다.
    /// </summary>
    /// <param name="stream">압축할 MemStream 16바이트.</param>
    /// <param name="hooks">할당·반납·버퍼·진단 장치.</param>
    public static void Compact(Span<byte> stream, MemStreamHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Physical(stream);
        uint end = Read(stream, 8);
        if (end == Read(stream, 12))
        {
            return;
        }
        uint previous = Read(stream, 0);
        if (unchecked((int)end) > 0)
        {
            Put(stream, 0, 0);
            Allocate(stream, end, hooks);
            uint length = Read(stream, 12);
            Span<byte> source = BufferRange(hooks, previous, 0, length);
            Write(stream, source, length, hooks);
            Need(hooks.Release, "MemStream 옛 버퍼 반납 미연결")(previous);
        }
        else
        {
            RawDataManagerLifetime.DestroyMemStream(stream, hooks.Release);
        }
    }
}
